#include "train.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>
#include <unistd.h>
#include <algorithm>

void Train::run()
{
  if (_workerId == 0)
  {
    printf("[Jaffar] ----------------------------------------------------------------\n");
    printf("[Jaffar] Launching Jaffar Version %s...\n", JAFFAR_VERSION);
    printf("[Jaffar] Using configuration file: %s.\n", _scriptFile.c_str());
    printf("[Jaffar] Starting search with %lu workers.\n", _workerCount);
    printf("[Jaffar] Frame DB entries per worker: %lu\n", _maxLocalDatabaseSize);
    printf("[Jaffar] Hash DBs per worker: %d x %lu.\n", HASH_DATABASE_COUNT, _hashDatabaseSizeThreshold);

    if (_outputSaveBestSeconds > 0)
    {
      printf("[Jaffar] Saving best frame every: %.3f seconds.\n", _outputSaveBestSeconds);
      printf("[Jaffar]  + Path: %s\n", _outputSaveBestPath.c_str());
    }

    if (_outputSaveCurrentSeconds > 0)
    {
      printf("[Jaffar] Saving current frame every: %.3f seconds.\n", _outputSaveCurrentSeconds);
      printf("[Jaffar]  + Path: %s\n", _outputSaveCurrentPath.c_str());
    }

    // Sleep for a second to show this message
    sleep(2);
  }

  // Wait for all workers to be ready
  MPI_Barrier(MPI_COMM_WORLD);

  auto searchTimeBegin = std::chrono::steady_clock::now();      // Profiling
  auto currentStepTimeBegin = std::chrono::steady_clock::now(); // Profiling

  // Storage for termination trigger
  bool terminate = false;

  while (terminate == false)
  {
    // If this is the root rank, plot the best frame and print information
    if (_workerId == 0)
    {
      // Profiling information
      auto searchTimeEnd = std::chrono::steady_clock::now();                                                            // Profiling
      _searchTotalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(searchTimeEnd - searchTimeBegin).count(); // Profiling

      auto currentStepTimeEnd = std::chrono::steady_clock::now();                                                                 // Profiling
      _currentStepTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentStepTimeEnd - currentStepTimeBegin).count(); // Profiling
      currentStepTimeBegin = std::chrono::steady_clock::now();                                                                    // Profiling

      // Printing search status
      printTrainStatus();

      // Updating show frames
      if (_currentFrameDB.size() > 0)
        for (size_t i = 0; i < SHOW_FRAME_COUNT; i++)
          _showFrameDB[i] = *_currentFrameDB[i % _currentFrameDB.size()];
    }

    /////////////////////////////////////////////////////////////////
    /// Main frame processing cycle begin
    /////////////////////////////////////////////////////////////////

    // 1) Workers exchange new base frames uniformly
    MPI_Barrier(MPI_COMM_WORLD);                                        // Profiling
    auto frameDistributionTimeBegin = std::chrono::steady_clock::now(); // Profiling
    distributeFrames();
    MPI_Barrier(MPI_COMM_WORLD);                                                                                                                  // Profiling
    auto frameDistributionTimeEnd = std::chrono::steady_clock::now();                                                                             // Profiling
    _frameDistributionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameDistributionTimeEnd - frameDistributionTimeBegin).count(); // Profiling

    // 2) Workers process their own base frames
    MPI_Barrier(MPI_COMM_WORLD);                                       // Profiling
    auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling
    computeFrames();
    MPI_Barrier(MPI_COMM_WORLD);                                                                                                               // Profiling
    auto frameComputationTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
    _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count(); // Profiling

    // 3) Workers sort their databases and communicate partial results
    MPI_Barrier(MPI_COMM_WORLD);                                          // Profiling
    auto framePostprocessingTimeBegin = std::chrono::steady_clock::now(); // Profiling
    framePostprocessing();
    MPI_Barrier(MPI_COMM_WORLD);                                                                                                                        // Profiling
    auto framePostprocessingTimeEnd = std::chrono::steady_clock::now();                                                                                 // Profiling
    _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePostprocessingTimeEnd - framePostprocessingTimeBegin).count(); // Profiling

    // 4) Workers exchange hash information and update hash databases
    MPI_Barrier(MPI_COMM_WORLD);                                   // Profiling
    auto hashExchangeTimeBegin = std::chrono::steady_clock::now(); // Profiling
    updateHashDatabases();
    MPI_Barrier(MPI_COMM_WORLD);                                                                                                   // Profiling
    auto hashExchangeTimeEnd = std::chrono::steady_clock::now();                                                                   // Profiling
    _hashExchangeTime = std::chrono::duration_cast<std::chrono::nanoseconds>(hashExchangeTimeEnd - hashExchangeTimeBegin).count(); // Profiling

    /////////////////////////////////////////////////////////////////
    /// Main frame processing cycle end
    /////////////////////////////////////////////////////////////////

    // Terminate if DB is depleted and no winning rule was found
    if (_globalFrameCounter == 0)
    {
      if (_workerId == 0) printf("[Jaffar] Frame database depleted with no winning frames, finishing...\n");
      terminate = true;
    }

    // Terminate if a winning rule was found
    if (_winFrameFound == true)
    {
      if (_workerId == 0) printf("[Jaffar] Winning frame reached after %lu steps, finishing...\n", _currentStep);
      terminate = true;
    }

    // Terminate if maximum number of frames was reached
    if (_currentStep > _maxSteps)
    {
      if (_workerId == 0) printf("[Jaffar] Maximum frame number reached, finishing...\n");
      terminate = true;
    }

    // Broadcasting whether a winning frame was found
    MPI_Bcast(&terminate, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

    // Advancing step
    _currentStep++;
  }

  // Print winning frame if found
  if (_workerId == 0 && _winFrameFound == true)
  {
    printf("[Jaffar] Win Frame Information:\n");
    _state[0]->loadState(_globalWinFrame.frameStateData);
    _sdlPop[0]->refreshEngine();
    _sdlPop[0]->printFrameInfo();
    printRuleStatus(_globalWinFrame);

    // Print Move History
    printf("[Jaffar]  + Move List: ");
    for (size_t i = 0; i <= _currentStep; i++)
      printf("%s ", _possibleMoves[_globalWinFrame.moveHistory[i]].c_str());
    printf("\n");
  }

  // Marking the end of the run
  _hasFinalized = true;

  // Stopping show thread
  if (_workerId == 0) pthread_join(_showThreadId, NULL);

  // Barrier to wait for all workers
  MPI_Barrier(MPI_COMM_WORLD);
}

void Train::distributeFrames()
{
  // Getting worker's own base frame count
  size_t localBaseFrameCount = _currentFrameDB.size();

  // Gathering all of te worker's base frame counts
  std::vector<size_t> localBaseFrameCounts(_workerCount);
  MPI_Allgather(&localBaseFrameCount, 1, MPI_UNSIGNED_LONG, localBaseFrameCounts.data(), 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

  // Figuring out work distribution
  std::vector<size_t> localNextFrameCounts = splitVector(_globalFrameCounter, _workerCount);

  // Figuring out frame offsets
  size_t currentOffset = 0;
  std::vector<size_t> localNextFrameStart(_workerCount);
  std::vector<size_t> localNextFrameEnd(_workerCount);
  for (size_t i = 0; i < _workerCount; i++)
  {
    localNextFrameStart[i] = currentOffset;
    localNextFrameEnd[i] = currentOffset + localNextFrameCounts[i] - 1;
    currentOffset += localNextFrameCounts[i];
  }

  // Determining all-to-all send/recv counts
  std::vector<std::vector<int>> allToAllSendCounts(_workerCount);
  std::vector<std::vector<int>> allToAllRecvCounts(_workerCount);
  for (size_t i = 0; i < _workerCount; i++) allToAllSendCounts[i].resize(_workerCount, 0);
  for (size_t i = 0; i < _workerCount; i++) allToAllRecvCounts[i].resize(_workerCount, 0);

  // Iterating over sending ranks
  size_t recvWorkerId = 0;
  auto recvFramesCount = localNextFrameCounts[0];

  for (size_t sendWorkerId = 0; sendWorkerId < _workerCount; sendWorkerId++)
  {
    auto sendFramesCount = localBaseFrameCounts[sendWorkerId];

    while (sendFramesCount > 0)
    {
      if (sendFramesCount <= recvFramesCount)
      {
        allToAllSendCounts[sendWorkerId][recvWorkerId] += sendFramesCount;
        allToAllRecvCounts[recvWorkerId][sendWorkerId] += sendFramesCount;
        recvFramesCount -= sendFramesCount;
        sendFramesCount = 0;
      }
      else
      {
        allToAllSendCounts[sendWorkerId][recvWorkerId] += recvFramesCount;
        allToAllRecvCounts[recvWorkerId][sendWorkerId] += recvFramesCount;
        sendFramesCount -= recvFramesCount;
        recvFramesCount = 0;
        recvWorkerId++;
        recvFramesCount = localNextFrameCounts[recvWorkerId];
      }
    }
  }

  // Getting current frame count
  const size_t currentFrameDBSize = _currentFrameDB.size();

  // Passing own part of experiences to itself
  size_t ownExperienceCount = allToAllSendCounts[_workerId][_workerId];
  for (size_t i = 0; i < ownExperienceCount; i++)
  {
    // Getting last frames
    auto frame = std::move(_currentFrameDB[currentFrameDBSize - ownExperienceCount + i]);

    // Adding new frame into the data base
    _nextFrameDB.push_back(std::move(frame));
  }

  // Removing frame database
  _currentFrameDB.resize(currentFrameDBSize - ownExperienceCount);

  // Storage for MPI requests
  MPI_Request reqs[2];

  // Exchanging frames with all other workers at a one by one basis
  // This could be done more efficiently in terms of message traffic and performance by using an MPI_Alltoallv operation
  // However, this operation requires that all buffers are allocated simultaneously which puts additional pressure on memory
  // Given this bot needs as much memory as possible, the more memory efficient pair-wise communication is used
  size_t nSteps = _workerCount / 2;

  // Perform right-wise communication first
  size_t leftNeighborId = _workerId == 0 ? _workerCount - 1 : _workerId - 1;
  size_t rightNeighborId = _workerId == _workerCount - 1 ? 0 : _workerId + 1;
  for (size_t step = 0; step < nSteps; step++)
  {
    // Variables to keep track of current send and recv positions
    size_t currentSendPosition;
    size_t currentRecvPosition;

    // Getting counts for the current worker
    int leftRecvCount = allToAllRecvCounts[_workerId][leftNeighborId];
    int rightSendCount = allToAllSendCounts[_workerId][rightNeighborId];

    // Reserving exchange buffers
    char *leftRecvFrameExchangeBuffer = (char *)malloc(_frameSerializedSize * leftRecvCount);
    char *rightSendFrameExchangeBuffer = (char *)malloc(_frameSerializedSize * rightSendCount);

    // Serializing always the last frame and reducing vector size to minimize memory usage
    currentSendPosition = 0;
    if ((leftNeighborId == rightNeighborId && rightNeighborId < _workerId) == false)
      for (int i = 0; i < rightSendCount; i++)
      {
        // Getting current frame count
        const size_t count = _currentFrameDB.size();

        // Getting last frame
        const auto &frame = _currentFrameDB[count - 1];

        // Serializing frame
        frame->serialize(&rightSendFrameExchangeBuffer[currentSendPosition]);

        // Advancing send buffer pointer
        currentSendPosition += _frameSerializedSize;

        // Removing last frame by resizing
        _currentFrameDB.resize(count - 1);
      }

    // Posting receive requests
    MPI_Irecv(leftRecvFrameExchangeBuffer, leftRecvCount, _mpiFrameType, leftNeighborId, 0, MPI_COMM_WORLD, &reqs[0]);

    // Making sure receives are posted before sends to prevent intermediate buffering
    MPI_Barrier(MPI_COMM_WORLD);

    // Posting send requests
    MPI_Isend(rightSendFrameExchangeBuffer, rightSendCount, _mpiFrameType, rightNeighborId, 0, MPI_COMM_WORLD, &reqs[1]);

    // Waiting for requests to finish
    MPI_Waitall(2, reqs, MPI_STATUS_IGNORE);

    // Freeing send buffers
    free(rightSendFrameExchangeBuffer);

    // Waiting for everyone to have cleaned their buffers before creating new frames
    MPI_Barrier(MPI_COMM_WORLD);

    currentRecvPosition = 0;
    if ((leftNeighborId == rightNeighborId && rightNeighborId > _workerId) == false)
      for (int frameId = 0; frameId < leftRecvCount; frameId++)
      {
        // Creating new frame
        auto newFrame = std::make_unique<Frame>();

        // Deserializing frame
        newFrame->deserialize(&leftRecvFrameExchangeBuffer[currentRecvPosition]);

        // Adding new frame into the data base
        _nextFrameDB.push_back(std::move(newFrame));

        // Advancing send buffer pointer
        currentRecvPosition += _frameSerializedSize;
      }

    // Freeing left right receive buffer
    free(leftRecvFrameExchangeBuffer);

    // Waiting for everyone to have cleaned their buffers before continuing
    MPI_Barrier(MPI_COMM_WORLD);

    // Shifting neighbors by one
    if (leftNeighborId == 0)
      leftNeighborId = _workerCount - 1;
    else
      leftNeighborId = leftNeighborId - 1;

    if (rightNeighborId == _workerCount - 1)
      rightNeighborId = 0;
    else
      rightNeighborId = rightNeighborId + 1;
  }

  // Perform left-wise communication second
  leftNeighborId = _workerId == 0 ? _workerCount - 1 : _workerId - 1;
  rightNeighborId = _workerId == _workerCount - 1 ? 0 : _workerId + 1;
  for (size_t step = 0; step < nSteps; step++)
  {
    // Variables to keep track of current send and recv positions
    size_t currentSendPosition;
    size_t currentRecvPosition;

    // Getting counts for the current worker
    int leftSendCount = allToAllSendCounts[_workerId][leftNeighborId];
    int rightRecvCount = allToAllRecvCounts[_workerId][rightNeighborId];

    // Reserving exchange buffers
    char *leftSendFrameExchangeBuffer = (char *)malloc(_frameSerializedSize * leftSendCount);
    char *rightRecvFrameExchangeBuffer = (char *)malloc(_frameSerializedSize * rightRecvCount);

    // Serializing always the last frame and reducing vector size to minimize memory usage
    currentSendPosition = 0;
    if ((leftNeighborId == rightNeighborId && rightNeighborId > _workerId) == false)
      for (int i = 0; i < leftSendCount; i++)
      {
        // Getting current frame count
        const size_t count = _currentFrameDB.size();

        // Getting last frame
        const auto &frame = _currentFrameDB[count - 1];

        // Serializing frame
        frame->serialize(&leftSendFrameExchangeBuffer[currentSendPosition]);

        // Advancing send buffer pointer
        currentSendPosition += _frameSerializedSize;

        // Removing last frame by resizing
        _currentFrameDB.resize(count - 1);
      }

    // Posting receive requests
    MPI_Irecv(rightRecvFrameExchangeBuffer, rightRecvCount, _mpiFrameType, rightNeighborId, 0, MPI_COMM_WORLD, &reqs[0]);

    // Making sure receives are posted before sends to prevent intermediate buffering
    MPI_Barrier(MPI_COMM_WORLD);

    // Posting send requests
    MPI_Isend(leftSendFrameExchangeBuffer, leftSendCount, _mpiFrameType, leftNeighborId, 0, MPI_COMM_WORLD, &reqs[1]);

    // Waiting for requests to finish
    MPI_Waitall(2, reqs, MPI_STATUS_IGNORE);

    // Freeing send buffers
    free(leftSendFrameExchangeBuffer);

    // Waiting for everyone to have cleaned their buffers before creating new frames
    MPI_Barrier(MPI_COMM_WORLD);

    // Deserializing contiguous buffer into the new frame database
    currentRecvPosition = 0;
    if ((leftNeighborId == rightNeighborId && rightNeighborId < _workerId) == false)
      for (int frameId = 0; frameId < rightRecvCount; frameId++)
      {
        // Creating new frame
        auto newFrame = std::make_unique<Frame>();

        // Deserializing frame
        newFrame->deserialize(&rightRecvFrameExchangeBuffer[currentRecvPosition]);

        // Adding new frame into the data base
        _nextFrameDB.push_back(std::move(newFrame));

        // Advancing send buffer pointer
        currentRecvPosition += _frameSerializedSize;
      }

    // Freeing right receive buffer
    free(rightRecvFrameExchangeBuffer);

    // Waiting for everyone to have cleaned their buffers before continuing
    MPI_Barrier(MPI_COMM_WORLD);

    // Shifting neighbors by one
    if (leftNeighborId == 0)
      leftNeighborId = _workerCount - 1;
    else
      leftNeighborId = leftNeighborId - 1;

    if (rightNeighborId == _workerCount - 1)
      rightNeighborId = 0;
    else
      rightNeighborId = rightNeighborId + 1;
  }

  // Swapping database pointers
  _currentFrameDB = std::move(_nextFrameDB);
}

void Train::computeFrames()
{
  // Initializing counters
  _localStepFramesProcessedCounter = 0;
  _newCollisionCounter = 0;

   // Computing always the last frame while resizing the database to reduce memory footprint
   #pragma omp parallel for schedule(guided)
   for (size_t baseFrameIdx = 0; baseFrameIdx < _currentFrameDB.size(); baseFrameIdx++)
   {
     // Getting thread id
     int threadId = omp_get_thread_num();

     // Storage for the base frame
     const auto& baseFrame = _currentFrameDB[baseFrameIdx];

     // Getting possible moves for the current frame
     std::vector<uint8_t> possibleMoveIds = getPossibleMoveIds(*baseFrame);

     // Running possible moves
     for (size_t idx = 0; idx < possibleMoveIds.size(); idx++)
     {
       // Increasing  frames processed counter
       _localStepFramesProcessedCounter++;

       // Getting possible move id
       auto moveId = possibleMoveIds[idx];

       // Getting possible move string
       std::string move = _possibleMoves[moveId].c_str();

       // Loading frame state
       _state[threadId]->loadState(baseFrame->frameStateData);

       // Perform the selected move
       _sdlPop[threadId]->performMove(move);

       // Advance a single frame
       _sdlPop[threadId]->advanceFrame();

       // Compute hash value
       auto hash = _state[threadId]->computeHash();

       // Inserting and checking for the existence of the hash in the hash databases
       bool collisionDetected = false;
       #pragma omp critical(hashTable)
       {
        for (size_t i = 0; i < HASH_DATABASE_COUNT; i++)
        {
          collisionDetected |= _hashDatabases[i].contains(hash);
          if (collisionDetected) break;
        }

        // If collision detected locally, discard this frame
        if (collisionDetected)
          _newCollisionCounter++;
        else // Otherwise, add it to the hash databases
        {
          addHashEntry(hash);
         _newHashes.insert(hash);
        }
       }

       // If collision detected locally, discard this frame
       if (collisionDetected) continue;

       // Creating new frame, mixing base frame information and the current sdlpop state
       auto newFrame = std::make_unique<Frame>();
       newFrame->isWin = false;
       newFrame->isFail = false;
       newFrame->moveHistory = baseFrame->moveHistory;
       newFrame->moveHistory[_currentStep] = moveId;
       newFrame->rulesStatus = baseFrame->rulesStatus;
       newFrame->kidMagnets = baseFrame->kidMagnets;
       newFrame->guardMagnets = baseFrame->guardMagnets;

       // Storing the frame data
       newFrame->frameStateData = _state[threadId]->saveState();

       // Evaluating rules on the new frame
       evaluateRules(*newFrame);

       // Calculating score
       newFrame->score = getFrameScore(*newFrame);

       // If frame has succeded, then flag it
       if (newFrame->isWin == true)
       {
         _localWinFound = true;
          #pragma omp critical(winFrame)
         _localWinFrame = *newFrame;
       }

       // If frame has failed, then proceed to the next one
       if (newFrame->isFail == true) continue;

       // Adding novel frame in the next frame database
       #pragma omp critical(nextFrameDB)
        _nextFrameDB.push_back(std::move(newFrame));
     }
   }
}

void Train::framePostprocessing()
{
  // Swapping database pointers
  _currentFrameDB = std::move(_nextFrameDB);

  // Clearing next frame database
  _nextFrameDB.clear();

  // Sorting local DB frames by score
  std::sort(_currentFrameDB.begin(), _currentFrameDB.end(), [](const auto &a, const auto &b) { return a->score > b->score; });

  // Finding local best frame score
  float localBestFrameScore = -std::numeric_limits<float>::infinity();
  if (_currentFrameDB.empty() == false) localBestFrameScore = _currentFrameDB[0]->score;

  // Finding best global frame score
  struct mpiLoc
  {
    float val;
    int loc;
  };
  mpiLoc mpiLocInput;
  mpiLoc mpiLocResult;
  mpiLocInput.val = localBestFrameScore;
  mpiLocInput.loc = _workerId;
  MPI_Allreduce(&mpiLocInput, &mpiLocResult, 1, MPI_FLOAT_INT, MPI_MAXLOC, MPI_COMM_WORLD);
  int globalBestFrameRank = mpiLocResult.loc;

  // Serializing, broadcasting, and deserializing best frame
  if (_currentFrameDB.size() > 0)
  {
   char frameBcastBuffer[_frameSerializedSize];
   if (_workerId == (size_t)globalBestFrameRank) _currentFrameDB[0]->serialize(frameBcastBuffer);
   MPI_Bcast(frameBcastBuffer, 1, _mpiFrameType, globalBestFrameRank, MPI_COMM_WORLD);
   _bestFrame.deserialize(frameBcastBuffer);
  }

  // Clipping local database to the maximum threshold
  if (_currentFrameDB.size() > _maxLocalDatabaseSize) _currentFrameDB.resize(_maxLocalDatabaseSize);

  // Calculating global frame count
  size_t newLocalFrameDatabaseSize = _currentFrameDB.size();
  MPI_Allreduce(&newLocalFrameDatabaseSize, &_globalFrameCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

  // Exchanging the fact that a win frame has been found
  int winRank = -1;
  int localHasWinFrame = _localWinFound == false ? 0 : 1;
  std::vector<int> globalHasWinFrameVector(_workerCount);
  MPI_Allgather(&localHasWinFrame, 1, MPI_INT, globalHasWinFrameVector.data(), 1, MPI_INT, MPI_COMM_WORLD);
  for (size_t i = 0; i < _workerCount; i++)
    if (globalHasWinFrameVector[i] == 1) winRank = i;

  // If win frame found, broadcast it
  if (winRank >= 0)
  {
    char winRankBuffer[_frameSerializedSize];
    if ((size_t)winRank == _workerId) _localWinFrame.serialize(winRankBuffer);
    MPI_Bcast(winRankBuffer, 1, _mpiFrameType, winRank, MPI_COMM_WORLD);
    _globalWinFrame.deserialize(winRankBuffer);
    _winFrameFound = true;
  }

  // Summing frame processing counters
  MPI_Allreduce(&_localStepFramesProcessedCounter, &_stepFramesProcessedCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  _totalFramesProcessedCounter += _stepFramesProcessedCounter;
}

void Train::updateHashDatabases()
{
  // Gathering number of new hash entries
  int localNewHashEntryCount = (int)_newHashes.size();
  std::vector<int> globalNewHashEntryCounts(_workerCount);
  MPI_Allgather(&localNewHashEntryCount, 1, MPI_INT, globalNewHashEntryCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

  // Calculating displacements for new hash entries
  std::vector<int> globalNewHashEntryDispls(_workerCount);
  int currentDispl = 0;
  for (size_t i = 0; i < _workerCount; i++)
  {
    globalNewHashEntryDispls[i] = currentDispl;
    currentDispl += globalNewHashEntryCounts[i];
  }

  // Serializing new hash entries
  std::vector<uint64_t> localNewHashVector;
  for (const auto &key : _newHashes) localNewHashVector.push_back(key);

  // Freeing new hashes vector
  _newHashes.clear();

  // Gathering new hash entries
  size_t globalNewHashEntryCount = currentDispl;
  std::vector<uint64_t> globalNewHashVector(globalNewHashEntryCount);
  MPI_Allgatherv(localNewHashVector.data(), localNewHashEntryCount, MPI_UINT64_T, globalNewHashVector.data(), globalNewHashEntryCounts.data(), globalNewHashEntryDispls.data(), MPI_UINT64_T, MPI_COMM_WORLD);

  // Adding new hash entries
  for (size_t i = 0; i < globalNewHashEntryCount; i++)
    addHashEntry(globalNewHashVector[i]);

  // Gathering global swap counter
  size_t globalStepSwapCounter;
  MPI_Allreduce(&_localStepSwapCounter, &globalStepSwapCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  _hashDatabaseSwapCount = globalStepSwapCounter;

  // Finding global collision counter
  size_t globalNewCollisionCounter;
  MPI_Allreduce(&_newCollisionCounter, &globalNewCollisionCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  _globalHashCollisions += globalNewCollisionCounter;

  // Calculating global hash entry count
  size_t localHashDBSize = 0;
  for (size_t i = 0; i < HASH_DATABASE_COUNT; i++) localHashDBSize += _hashDatabases[i].size();
  MPI_Allreduce(&localHashDBSize, &_globalHashEntries, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
}

void Train::evaluateRules(Frame &frame)
{
  // Getting thread id
  int threadId = omp_get_thread_num();

  for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  {
    // Evaluate rule only if it's active
    if (frame.rulesStatus[ruleId] == false)
    {
      // Checking dependencies first. If not met, continue to the next rule
      bool dependenciesMet = true;
      for (size_t i = 0; i < _rules[threadId][ruleId]->_dependencies.size(); i++)
        if (frame.rulesStatus[_rules[threadId][ruleId]->_dependencies[i]] == false)
          dependenciesMet = false;

      // If dependencies aren't met, then continue to next rule
      if (dependenciesMet == false) continue;

      // Checking if conditions are met
      bool isAchieved = _rules[threadId][ruleId]->evaluate();

      // If it's achieved, update its status and run its actions
      if (isAchieved)
      {
        // Setting status to achieved
        frame.rulesStatus[ruleId] = true;

        // Perform actions
        for (size_t actionId = 0; actionId < _rules[threadId][ruleId]->_actions.size(); actionId++)
        {
          const auto actionJs = _rules[threadId][ruleId]->_actions[actionId];

          // Getting action type
          if (isDefined(actionJs, "Type") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Type' key.\n", ruleId, actionId);
          std::string actionType = actionJs["Type"].get<std::string>();

          // Running the action, depending on the type
          bool recognizedActionType = false;

          if (actionType == "Add Reward")
          {
            // This action is handled during startup, no need to do anything now.
            recognizedActionType = true;
          }

          // Storing fail state
          if (actionType == "Trigger Fail")
          {
            frame.isFail = true;
            recognizedActionType = true;
          }

          // Storing win state
          if (actionType == "Trigger Win")
          {
            frame.isWin = true;
            recognizedActionType = true;
          }

          // Parsing room here to avoid duplicating code per each rule
          int room = 0;
          if (isDefined(actionJs, "Room")) room = actionJs["Room"].get<int>();
          if (room > _VISIBLE_ROOM_COUNT) EXIT_WITH_ERROR("[ERROR] Rule %lu, Room %lu is outside visible room scope.\n", ruleId, room);

          if (actionType == "Set Kid Horizontal Magnet Intensity")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int8_t intensityX = actionJs["Value"].get<int8_t>();

            frame.kidMagnets[room].intensityX = intensityX;
            recognizedActionType = true;
          }

          if (actionType == "Set Kid Horizontal Magnet Position")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int16_t positionX = actionJs["Value"].get<int16_t>();

            frame.kidMagnets[room].positionX = positionX;
            recognizedActionType = true;
          }

          if (actionType == "Set Kid Vertical Magnet Intensity")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int8_t intensityY = actionJs["Value"].get<int8_t>();

            frame.kidMagnets[room].intensityY = intensityY;
            recognizedActionType = true;
          }

          if (actionType == "Set Guard Horizontal Magnet Intensity")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int8_t intensityX = actionJs["Value"].get<int8_t>();

            frame.guardMagnets[room].intensityX = intensityX;
            recognizedActionType = true;
          }

          if (actionType == "Set Guard Horizontal Magnet Position")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int16_t positionX = actionJs["Value"].get<int16_t>();

            frame.guardMagnets[room].positionX = positionX;
            recognizedActionType = true;
          }

          if (actionType == "Set Guard Vertical Magnet Intensity")
          {
            if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
            if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
            int8_t intensityY = actionJs["Value"].get<int8_t>();

            frame.guardMagnets[room].intensityY = intensityY;
            recognizedActionType = true;
          }

          if (recognizedActionType == false) EXIT_WITH_ERROR("[ERROR] Unrecognized action type %s\n", actionType.c_str());
        }
      }
    }
  }
}

void Train::addHashEntry(uint64_t hash)
{
  // Inserting hash
  _hashDatabases[0].insert(hash);

  // If the current initial hash database reached critical mass, cycle it around
  if (_hashDatabases[0].size() > _hashDatabaseSizeThreshold)
  {
    // Cycling databases, discarding the oldest hashes
    for (int i = HASH_DATABASE_COUNT - 1; i > 0; i--)
      _hashDatabases[i] = std::move(_hashDatabases[i - 1]);

    // Cleaning primary hash DB
    _hashDatabases[0].clear();

    // Increasing swapping counter
    _localStepSwapCounter++;
  }
}

void Train::printTrainStatus()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Current Step #: %lu / %lu\n", _currentStep, _maxSteps);
  printf("[Jaffar] Best Score: %f\n", _bestFrame.score);
  printf("[Jaffar] Database Size: %lu / %lu\n", _globalFrameCounter, _maxLocalDatabaseSize * _workerCount);
  printf("[Jaffar] Frames Processed: (Step/Total): %lu / %lu\n", _stepFramesProcessedCounter, _totalFramesProcessedCounter);
  printf("[Jaffar] Elapsed Time (Step/Total): %3.3fs / %3.3fs\n", _currentStepTime / 1.0e+9, _searchTotalTime / 1.0e+9);
  printf("[Jaffar] Performance: %.3f Frames/s\n", (double)_stepFramesProcessedCounter / (_currentStepTime / 1.0e+9));

  printf("[Jaffar] Frame Distribution Time:   %3.3fs\n", _frameDistributionTime / 1.0e+9);
  printf("[Jaffar] Frame Computation Time:    %3.3fs\n", _frameComputationTime / 1.0e+9);
  printf("[Jaffar] Hash Exchange Time:        %3.3fs\n", _hashExchangeTime / 1.0e+9);
  printf("[Jaffar] Frame Postprocessing Time: %3.3fs\n", _framePostprocessingTime / 1.0e+9);

  printf("[Jaffar] Hash DB Collisions: %lu\n", _globalHashCollisions);
  printf("[Jaffar] Hash DB Entries: %lu / %lu\n", _globalHashEntries, (size_t)HASH_DATABASE_COUNT * _hashDatabaseSizeThreshold * _workerCount);
  printf("[Jaffar] Hash DB Swaps: %lu\n", _hashDatabaseSwapCount);

  printf("[Jaffar] Best Frame Information:\n");

  _state[0]->loadState(_bestFrame.frameStateData);
  _sdlPop[0]->printFrameInfo();
  printRuleStatus(_bestFrame);

  // Printing Magnet Status
  int currentRoom = _sdlPop[0]->Kid->room;
  const auto &kidMagnet = _bestFrame.kidMagnets[currentRoom];
  const auto &guardMagnet = _bestFrame.guardMagnets[currentRoom];
  printf("[Jaffar]  + Kid Horizontal Magnet Intensity / Position: %.1f / %.0f\n", (float) kidMagnet.intensityX, (float) kidMagnet.positionX);
  printf("[Jaffar]  + Kid Vertical Magnet Intensity: %.1f\n", (float) kidMagnet.intensityY);
  printf("[Jaffar]  + Guard Horizontal Magnet Intensity / Position: %.1f / %.0f\n", (float) guardMagnet.intensityX, (float) guardMagnet.positionX);
  printf("[Jaffar]  + Guard Vertical Magnet Intensity: %.1f\n", (float) guardMagnet.intensityY);


  // Print Move History
  printf("[Jaffar]  + Move List: ");
  for (size_t i = 0; i <= _currentStep; i++)
    printf("%s ", _possibleMoves[_bestFrame.moveHistory[i]].c_str());
  printf("\n");
}

void Train::printRuleStatus(const Frame &frame)
{
  printf("[Jaffar]  + Rule Status: [ %d", frame.rulesStatus[0] ? 1 : 0);
  for (size_t i = 1; i < frame.rulesStatus.size(); i++)
    printf(", %d", frame.rulesStatus[i] ? 1 : 0);
  printf(" ]\n");
}

float Train::getFrameScore(const Frame &frame)
{
  // Getting thread id
  int threadId = omp_get_thread_num();

  // Accumulator for total score
  float score = 0.0f;

  // Obtaining magnet corresponding to kid's room
  int currentRoom = _sdlPop[threadId]->Kid->room;

  // Apply magnets if kid is inside visible rooms
  if (currentRoom >= 0 && currentRoom < _VISIBLE_ROOM_COUNT)
  {
   // Applying Kid Magnets

    const auto &kidMagnet = frame.kidMagnets[currentRoom];
    const auto curKidFrame = _sdlPop[threadId]->Kid->frame;

    // Evaluating kidMagnet's score on the X axis
    const float kidDiffX = std::abs(_sdlPop[threadId]->Kid->x - kidMagnet.positionX);
    score += (float) kidMagnet.intensityX * (256.0f - kidDiffX);

    // For positive Y axis kidMagnet, rewarding climbing frames
    if ((float) kidMagnet.intensityY > 0.0f)
    {
      // Jumphang, because it preludes climbing (Score + 1-20)
      if (curKidFrame >= 67 && curKidFrame <= 80)
      {
        float scoreAdd = (float) kidMagnet.intensityY * (0.0f + (curKidFrame - 66));
        score += scoreAdd;
      }

      // Hang, because it preludes climbing (Score +21)
      if (curKidFrame == 91) score += 21.0f * (float) kidMagnet.intensityY;

      // Climbing (Score +22-38)
      if (curKidFrame >= 135 && curKidFrame <= 149) score += (float) kidMagnet.intensityY * (22.0f + (curKidFrame - 134));

      // Adding absolute reward for Y position
      score += (float) kidMagnet.intensityY * (256.0f - _sdlPop[threadId]->Kid->y);
    }

    // For negative Y axis kidMagnet, rewarding falling/climbing down frames
    if ((float) kidMagnet.intensityY < 0.0f)
    {
      // Turning around, because it generally preludes climbing down
      if (curKidFrame >= 45 && curKidFrame <= 52) score += -0.5f * (float) kidMagnet.intensityY;

      // Hanging, because it preludes falling
      if (curKidFrame >= 87 && curKidFrame <= 99) score += -0.5f * (float) kidMagnet.intensityY;

      // Hang drop, because it preludes falling
      if (curKidFrame >= 81 && curKidFrame <= 85) score += -1.0f * (float) kidMagnet.intensityY;

      // Falling start
      if (curKidFrame >= 102 && curKidFrame <= 105) score += -1.0f * (float) kidMagnet.intensityY;

      // Falling itself
      if (curKidFrame == 106) score += -2.0f + (float) kidMagnet.intensityY;

      // Climbing down
      if (curKidFrame == 148) score += -2.0f + (float) kidMagnet.intensityY;

      // Adding absolute reward for Y position
      score += (float) kidMagnet.intensityY * (_sdlPop[threadId]->Kid->y);
    }

    // Applying Guard Magnets

    const auto &guardMagnet = frame.guardMagnets[currentRoom];
    const auto curGuardFrame = _sdlPop[threadId]->Guard->frame;

    // Evaluating guardMagnet's score on the X axis
    const float guardDiffX = std::abs(_sdlPop[threadId]->Guard->x - guardMagnet.positionX);
    score += (float) guardMagnet.intensityX * (256.0f - guardDiffX);

    // For positive Y axis guardMagnet
    if ((float) guardMagnet.intensityY > 0.0f)
    {
     // Do nothing -- guards don't go up (or do they?)
    }

    // For negative Y axis guardMagnet, rewarding falling/climbing down frames
    if ((float) guardMagnet.intensityY < 0.0f)
    {
      // Falling start
      if (curGuardFrame >= 102 && curGuardFrame <= 105) score += -1.0f * (float) guardMagnet.intensityY;

      // Falling itself
      if (curGuardFrame == 106) score += -2.0f + (float) guardMagnet.intensityY;
    }
  }

  // Apply bonus when kid is inside a non-visible room
  if (currentRoom == 0 || currentRoom >= _VISIBLE_ROOM_COUNT)
    score += 128.0f;

  // Now adding rule rewards
  for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
    if (frame.rulesStatus[ruleId] == true)
      score += _rules[threadId][ruleId]->_reward;

  // Returning score
  return score;
}

std::vector<uint8_t> Train::getPossibleMoveIds(const Frame &frame)
{
  // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13
  //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

  // Getting thread id
  int threadId = omp_get_thread_num();

  // Loading frame state
  _state[threadId]->loadState(frame.frameStateData);

  // Getting Kid information
  const auto &Kid = *_sdlPop[threadId]->Kid;

  // If dead, do nothing
  if (Kid.alive >= 0)
    return {0};

  // If bumped, nothing to do
  if (Kid.action == actions_5_bumped)
    return {0};

  // If in mid air or free fall, hope to grab on to something
  if (Kid.action == actions_3_in_midair || Kid.action == actions_4_in_freefall)
    return {0, 1};

  // Move, sheath, attack, parry
  if (Kid.sword == sword_2_drawn)
    return {0, 1, 2, 3, 4, 5};

  // Kid is standing or finishing a turn, try all possibilities
  if (Kid.frame == frame_15_stand || (Kid.frame >= frame_50_turn && Kid.frame < 53))
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

  // Turning frame, try all possibilities
  if (Kid.frame == frame_48_turn)
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

  // Start running animation, all movement without shift
  if (Kid.frame < 4)
    return {
      0,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
    };

  // Starting jump up, check directions, jump and shift
  if (Kid.frame >= frame_67_start_jump_up_1 && Kid.frame < frame_70_jumphang)
    return {0, 1, 2, 3, 4, 5, 6, 8, 12};

  // Running, all movement without shift
  if (Kid.frame < 15)
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

  // Hanging, up and shift are only options
  if (Kid.frame >= frame_87_hanging_1 && Kid.frame < 100)
    return {0, 1, 2, 12};

  // Crouched, can only stand, drink, or hop
  if (Kid.frame == frame_109_crouch)
    return {0, 1, 3, 4, 5, 7, 9, 13};

  // Default, no nothing
  return {0};
}

Train::Train(int argc, char *argv[])
{
  // Initialize the MPI environment
  const int required = MPI_THREAD_SERIALIZED;
  int provided;
  MPI_Init_thread(&argc, &argv, required, &provided);
  if (required != provided)
   EXIT_WITH_ERROR("[ERROR] Error initializing threaded MPI");

  // Get the number of processes
  int mpiSize;
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  _workerCount = (size_t)mpiSize;

  // Get the rank of the process
  int mpiRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  _workerId = (size_t)mpiRank;

  // Setting SDL env variables to use the dummy renderer
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);

  // Parsing command line arguments
  argparse::ArgumentParser program("jaffar-train", JAFFAR_VERSION);

  program.add_argument("jaffarFile")
    .help("path to the Jaffar configuration script (.config) file to run.")
    .required();

  // Try to parse arguments
  try
  {
    program.parse_args(argc, argv);
  }
  catch (const std::runtime_error &err)
  {
    if (_workerId == 0) fprintf(stderr, "%s\n%s", err.what(), program.help().str().c_str());
    exit(-1);
  }

  // Reading config file
  _scriptFile = program.get<std::string>("jaffarFile");
  std::string scriptString;
  bool status = loadStringFromFile(scriptString, _scriptFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n%s \n", _scriptFile.c_str(), program.help().str().c_str());

  // Parsing JSON from config file
  try
  {
    _scriptJs = nlohmann::json::parse(scriptString);
  }
  catch (const std::exception &err)
  {
    if (_workerId == 0) fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", _scriptFile.c_str(), err.what());
    exit(-1);
  }

  // Profiling information
  _searchTotalTime = 0.0;
  _currentStepTime = 0.0;
  _frameDistributionTime = 0.0;
  _frameComputationTime = 0.0;
  _framePostprocessingTime = 0.0;

  // Initializing counters
  _stepFramesProcessedCounter = 0;
  _totalFramesProcessedCounter = 0;
  _hashDatabaseSwapCount = 0;
  _localStepSwapCounter = 0;
  _newCollisionCounter = 0;
  _localStepFramesProcessedCounter = 0;

  // Setting starting step
  _currentStep = 0;

  // Parsing max frames from configuration file
  if (isDefined(_scriptJs, "Max Steps") == false) EXIT_WITH_ERROR("[ERROR] Train configuration missing 'Max Steps' key.\n");
  _maxSteps = _scriptJs["Max Steps"].get<size_t>();

  // Parsing max hash DB entries
  _hashDatabases.resize(HASH_DATABASE_COUNT);
  size_t hashDBMaxMBytes = 50;
  if (const char *hashDBMaxMBytesEnv = std::getenv("JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB"))
    hashDBMaxMBytes = std::stol(hashDBMaxMBytesEnv);
  else if (_workerId == 0)
    printf("[Jaffar] Warning: JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB environment variable not defined. Using conservative default...\n");

  _hashDatabaseSizeThreshold = floor(((double)hashDBMaxMBytes * 1024.0 * 1024.0) / ((double)HASH_DATABASE_COUNT * (double)sizeof(uint64_t)));

  // Parsing max frame DB entries
  size_t frameDBMaxMBytes = 300;
  if (const char *frameDBMaxMBytesEnv = std::getenv("JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB"))
    frameDBMaxMBytes = std::stol(frameDBMaxMBytesEnv);
  else if (_workerId == 0)
    printf("[Jaffar] Warning: JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB environment variable not defined. Using conservative default...\n");

  // Parsing file output frequency
  _outputSaveBestSeconds = -1.0;
  if (const char *outputSaveBestSecondsEnv = std::getenv("JAFFAR_SAVE_BEST_EVERY_SECONDS")) _outputSaveBestSeconds = std::stof(outputSaveBestSecondsEnv);
  _outputSaveCurrentSeconds = -1.0;
  if (const char *outputSaveCurrentSecondsEnv = std::getenv("JAFFAR_SAVE_CURRENT_EVERY_SECONDS")) _outputSaveCurrentSeconds = std::stof(outputSaveCurrentSecondsEnv);

  // Parsing file output path
  _outputSaveBestPath = "/tmp/jaffar.best.sav";
  if (const char *outputSaveBestPathEnv = std::getenv("JAFFAR_SAVE_BEST_PATH")) _outputSaveBestPath = std::string(outputSaveBestPathEnv);
  _outputSaveCurrentPath = "/tmp/jaffar.current.sav";
  if (const char *outputSaveCurrentPathEnv = std::getenv("JAFFAR_SAVE_CURRENT_PATH")) _outputSaveCurrentPath = std::string(outputSaveCurrentPathEnv);

  // Twice the size of frames to allow for communication
  _maxLocalDatabaseSize = floor(((double)frameDBMaxMBytes * 1024.0 * 1024.0) / ((double)Frame::getSerializationSize()));

  // Processing user-specified rules
  if (isDefined(_scriptJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Train configuration file missing 'Rules' key.\n");

  // Setting global rule count
  _ruleCount = _scriptJs["Rules"].size();

  // Getting number of omp threads and resizing containers based on that
  _threadCount = omp_get_max_threads();
  _sdlPop.resize(_threadCount);
  _state.resize(_threadCount);
  _rules.resize(_threadCount);

  // Creating Barebones SDL Pop Instance, one per openMP Thread
  for (int threadId = 0; threadId < _threadCount; threadId++)
  {
    _sdlPop[threadId] = new SDLPopInstance("libsdlPopLibBarebones.so", true);
    _sdlPop[threadId]->initialize(false);

    // Initializing State Handler
    _state[threadId] = new State(_sdlPop[threadId], _scriptJs);

   // Adding rules, pointing to the thread-specific sdlpop instances
   for (size_t i = 0; i < _ruleCount; i++)
     _rules[threadId].push_back(new Rule(_scriptJs["Rules"][i], _sdlPop[threadId]));
  }

  printf("[Jaffar] MPI Rank %lu/%lu: SDLPop initialized.\n", _workerId, _workerCount);
  fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);

  // Setting magnet default values. This default repels unspecified rooms.
  std::vector<Magnet> magnets(_VISIBLE_ROOM_COUNT);

  for (size_t i = 0; i < _VISIBLE_ROOM_COUNT; i++)
  {
    magnets[i].intensityY = 0;
    magnets[i].intensityX = 0;
    magnets[i].positionX = 0;
  }

  // Setting initial status for each rule
  std::vector<char> rulesStatus(_ruleCount);
  for (size_t i = 0; i < _ruleCount; i++) rulesStatus[i] = false;

  _frameSerializedSize = Frame::getSerializationSize();
  MPI_Type_contiguous(_frameSerializedSize, MPI_BYTE, &_mpiFrameType);
  MPI_Type_commit(&_mpiFrameType);

  // Setting initial values
  _hasFinalized = false;
  _globalHashCollisions = 0;

  // Setting win status
  _winFrameFound = false;
  _localWinFound = false;

  // Creating initial frame on the root rank
  if (_workerId == 0)
  {
    // Computing initial hash
    const auto hash = _state[0]->computeHash();

    auto initialFrame = std::make_unique<Frame>();
    initialFrame->moveHistory[0] = 0;
    initialFrame->frameStateData = _state[0]->saveState();
    initialFrame->kidMagnets = magnets;
    initialFrame->guardMagnets = magnets;
    initialFrame->rulesStatus = rulesStatus;

    // Evaluating Rules on initial frame
    evaluateRules(*initialFrame);

    // Evaluating Score on initial frame
    initialFrame->score = getFrameScore(*initialFrame);

    // Registering hash for initial frame
    _hashDatabases[0].insert(hash);

    // Copying initial frame into the best frame
    _bestFrame = *initialFrame;

    // Filling show database
    _showFrameDB.resize(SHOW_FRAME_COUNT);
    for (size_t i = 0; i < SHOW_FRAME_COUNT; i++) _showFrameDB[i] = *initialFrame;

    // Adding frame to the current database
    _currentFrameDB.push_back(std::move(initialFrame));

    // Initializing show thread
    if (pthread_create(&_showThreadId, NULL, showThreadFunction, this) != 0)
      EXIT_WITH_ERROR("[ERROR] Could not create show thread.\n");
  }

  // Close SDL after initializing
  SDL_Quit();

  // Starting global frame counter
  _globalFrameCounter = 1;
}

// Functions for the show thread
void *Train::showThreadFunction(void *trainPtr)
{
  ((Train *)trainPtr)->showSavingLoop();
  return NULL;
}
void Train::showSavingLoop()
{
  // Timer for saving frames
  auto bestFrameSaveTimer = std::chrono::steady_clock::now();
  auto currentFrameSaveTimer = std::chrono::steady_clock::now();

  // Counter for the current best frame id
  size_t currentFrameId = 0;

  while (_hasFinalized == false)
  {
    // Sleeping for one second to prevent excessive overheads
    sleep(1);

    // Checking if we need to save best frame
    if (_outputSaveBestSeconds > 0.0)
    {
      double bestFrameTimerElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - bestFrameSaveTimer).count();
      if (bestFrameTimerElapsed / 1.0e+9 > _outputSaveBestSeconds)
      {
        // Saving best frame data
        saveStringToFile(_bestFrame.frameStateData, _outputSaveBestPath.c_str());

        // Resetting timer
        bestFrameSaveTimer = std::chrono::steady_clock::now();
      }
    }

    // Checking if we need to save current frame
    if (_outputSaveCurrentSeconds > 0.0)
    {
      double currentFrameTimerElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - currentFrameSaveTimer).count();
      if (currentFrameTimerElapsed / 1.0e+9 > _outputSaveCurrentSeconds)
      {
        // Saving best frame data
        saveStringToFile(_showFrameDB[currentFrameId].frameStateData, _outputSaveCurrentPath.c_str());

        // Resetting timer
        currentFrameSaveTimer = std::chrono::steady_clock::now();

        // Increasing counter
        currentFrameId = (currentFrameId + 1) % SHOW_FRAME_COUNT;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  Train train(argc, argv);

  // Running Search
  train.run();

  return MPI_Finalize();
}
