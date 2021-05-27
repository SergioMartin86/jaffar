#include "train.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>
#include <unistd.h>
#include <boost/sort/sort.hpp>
#include <set>

void Train::run()
{
  if (_workerId == 0)
  {
    printf("[Jaffar] ----------------------------------------------------------------\n");
    printf("[Jaffar] Launching Jaffar Version %s...\n", JAFFAR_VERSION);
    printf("[Jaffar] Using configuration file(s): "); for (size_t i = 0; i < _scriptFiles.size(); i++) printf("%s ", _scriptFiles[i].c_str()); printf("\n");
    printf("[Jaffar] Starting search with %lu workers.\n", _workerCount);
    printf("[Jaffar] Frame DB entries per worker: %lu\n", _maxLocalDatabaseSize);

    if (_outputSaveBestSeconds > 0)
    {
      printf("[Jaffar] Saving best frame every: %.3f seconds.\n", _outputSaveBestSeconds);
      printf("[Jaffar]  + Savefile Path: %s\n", _outputSaveBestPath.c_str());
      printf("[Jaffar]  + Solution Path: %s\n", _outputSolutionBestPath.c_str());
    }

    if (_outputSaveCurrentSeconds > 0)
    {
      printf("[Jaffar] Saving current frame every: %.3f seconds.\n", _outputSaveCurrentSeconds);
      printf("[Jaffar]  + Savefile Path: %s\n", _outputSaveCurrentPath.c_str());
      printf("[Jaffar]  + Solution Path: %s\n", _outputSolutionCurrentPath.c_str());
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
    _hashPostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(hashExchangeTimeEnd - hashExchangeTimeBegin).count(); // Profiling

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
    _sdlPop[0]->printFrameInfo();

    printRuleStatus(_globalWinFrame);

    // Print Move History
    if (_storeMoveList)
    {
     printf("[Jaffar]  + Move List: ");
     for (size_t i = 0; i <= _currentStep; i++)
       printf("%s ", _possibleMoves[_globalWinFrame.getMove(i)].c_str());
     printf("\n");
    }
  }

  // Marking the end of the run
  _hasFinalized = true;

  // Stopping show thread
  if (_workerId == 0) pthread_join(_showThreadId, NULL);

  // Barrier to wait for all workers
  MPI_Barrier(MPI_COMM_WORLD);
}

void Train::printRuleStatus(const Frame &frame)
{
 printf("[Jaffar]  + Rule Status: ");
 for (size_t i = 0; i < frame.rulesStatus.size(); i++)
 {
   if (i > 0 && i % 60 == 0) printf("\n                         ");
   printf("%d", frame.rulesStatus[i] ? 1 : 0);
 }
 printf("\n");
}

void Train::distributeFrames()
{
  // Getting worker's own base frame count
  size_t localBaseFrameCount = _currentFrameDB.size();

  // Gathering all of te worker's base frame counts
  MPI_Allgather(&localBaseFrameCount, 1, MPI_UNSIGNED_LONG, _localBaseFrameCounts.data(), 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

  // Getting maximum frame count of all
  _maxFrameCount = 0;
  _maxFrameWorkerId = 0;
  for (size_t i = 0; i < _workerCount; i++)
   if (_localBaseFrameCounts[i] > _maxFrameCount)
    { _maxFrameCount = _localBaseFrameCounts[i]; _maxFrameWorkerId = i; }

  // Getting minimum frame count of all
  _minFrameCount = _maxFrameCount;
  _minFrameWorkerId = 0;
  for (size_t i = 0; i < _workerCount; i++)
   if (_localBaseFrameCounts[i] < _minFrameCount)
    { _minFrameCount = _localBaseFrameCounts[i]; _minFrameWorkerId = i; }

  // Figuring out work distribution
  std::vector<size_t> targetLocalNextFrameCounts = splitVector(_globalFrameCounter, _workerCount);
  std::vector<size_t> remainLocalNextFrameCounts = targetLocalNextFrameCounts;

  // Determining all-to-all send/recv counts
  std::vector<std::vector<int>> allToAllSendCounts(_workerCount);
  std::vector<std::vector<int>> allToAllRecvCounts(_workerCount);
  for (size_t i = 0; i < _workerCount; i++) allToAllSendCounts[i].resize(_workerCount, 0);
  for (size_t i = 0; i < _workerCount; i++) allToAllRecvCounts[i].resize(_workerCount, 0);

  // Iterating over sending ranks
  for (size_t sendWorkerId = 0; sendWorkerId < _workerCount; sendWorkerId++)
  {
    auto sendFramesCount = _localBaseFrameCounts[sendWorkerId];
    size_t recvWorkerId = 0;

    while (sendFramesCount > 0)
    {
     size_t maxRecv = std::min(remainLocalNextFrameCounts[recvWorkerId], 4096ul);
     size_t maxSend = std::min(sendFramesCount, 4096ul);
     size_t exchangeCount = std::min(maxRecv, maxSend);

     sendFramesCount -= exchangeCount;
     remainLocalNextFrameCounts[recvWorkerId] -= exchangeCount;
     allToAllSendCounts[sendWorkerId][recvWorkerId] += exchangeCount;
     allToAllRecvCounts[recvWorkerId][sendWorkerId] += exchangeCount;

     recvWorkerId++;
     if (recvWorkerId == _workerCount) recvWorkerId = 0;
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

  // Processing frame database in parallel
  #pragma omp parallel
  {
    // Getting thread id
    int threadId = omp_get_thread_num();

    // Creating thread-local storage for new frames
    std::vector<std::unique_ptr<Frame>> newThreadFrames;

    // Computing always the last frame while resizing the database to reduce memory footprint
    #pragma omp for schedule(guided)
    for (size_t baseFrameIdx = 0; baseFrameIdx < _currentFrameDB.size(); baseFrameIdx++)
    {
      // Storage for the base frame
      const auto& baseFrame = _currentFrameDB[baseFrameIdx];

      // Getting possible moves for the current frame
      std::vector<uint8_t> possibleMoveIds = getPossibleMoveIds(*baseFrame);

      // If the restart flag is activated, then also try hitting Ctrl+A
      if (baseFrame->isRestart == true)
      {
       possibleMoveIds.push_back(14);
       baseFrame->isRestart = false;
      }

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

        // Checking for the existence of the hash in the hash databases
        bool collisionDetected = _hashDatabase.contains(hash);

        // If no collision detected with the normal databases, check the new hash DB
        if (collisionDetected == false)
         #pragma omp critical
         {
           collisionDetected |= _newHashes.contains(hash);

           // If now there, add it now
           if (collisionDetected == false) _newHashes.insert(hash);
         }

        // If collision detected, increase collision counter
        if (collisionDetected)
         #pragma omp atomic
          _newCollisionCounter++;

        // If collision detected, discard this frame
        if (collisionDetected) continue;

        // Creating new frame, mixing base frame information and the current sdlpop state
        auto newFrame = std::make_unique<Frame>();
        newFrame->rulesStatus = baseFrame->rulesStatus;

        // If required, store move history
        if (_storeMoveList == true)
        {
         newFrame->moveHistory = baseFrame->moveHistory;
         newFrame->setMove(_currentStep, moveId);
        }

        // Evaluating rules on the new frame
        evaluateRules(*newFrame);

        // Checks whether any fail rules were activated
        bool isFailFrame = checkFail(*newFrame);

        // If frame has failed, discard it and proceed to the next one
        if (isFailFrame) continue;

        // Check special actions for this state
        checkSpecialActions(*newFrame);

        // Storing the frame data
        newFrame->frameStateData = _state[threadId]->saveState();

        // Calculating current reward
        newFrame->reward = getFrameReward(*newFrame);

        // Check if the frame triggers a win condition
        bool isWinFrame = checkWin(*newFrame);

        // If frame has succeded, then flag it
        if (isWinFrame)
        {
          _localWinFound = true;
           #pragma omp critical(winFrame)
           _localWinFrame = *newFrame;
        }

        // Adding novel frame in the next frame database
        newThreadFrames.push_back(std::move(newFrame));
      }

      // Freeing memory for the used base frame
      _currentFrameDB[baseFrameIdx].reset();
    }

    // Sequentially passing thread-local new frames to the common database
    #pragma omp critical
    for (size_t i = 0; i < newThreadFrames.size(); i++)
     _nextFrameDB.push_back(std::move(newThreadFrames[i]));
  }
}

void Train::framePostprocessing()
{
  // Clearing current frame DB
  _currentFrameDB.clear();

  // Sorting local DB frames by reward
  boost::sort::block_indirect_sort(_nextFrameDB.begin(), _nextFrameDB.end(), [](const auto &a, const auto &b) { return a->reward > b->reward; });

  // Calculating global frame count
  size_t localFrameDatabaseSize = _nextFrameDB.size();
  MPI_Allreduce(&localFrameDatabaseSize, &_globalFrameCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

  // Calculating how many frames need to be cut
  size_t framesToCut = _globalFrameCounter >_maxGlobalDatabaseSize ? _globalFrameCounter - _maxGlobalDatabaseSize : 0;

  // Finding local best frame reward
  float localBestFrameScore = -std::numeric_limits<float>::infinity();
  if (_nextFrameDB.empty() == false) localBestFrameScore = _nextFrameDB[0]->reward;

  // Finding global best frame reward
  float globalBestFrameScore;
  MPI_Allreduce(&localBestFrameScore, &globalBestFrameScore, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

  // Approximating to the cutoff score logarithmically
  size_t globalCurrentFramesCut = _globalFrameCounter;
  float currentCutoffScore = framesToCut > 0 ? globalBestFrameScore : -1.0f;
  size_t passingFramesIdx = 0;
  while(globalCurrentFramesCut > framesToCut && framesToCut > 0 && currentCutoffScore > 5.0f)
  {
   // Calculating the number of frames cut with current cutoff
   while (passingFramesIdx < localFrameDatabaseSize && _nextFrameDB[passingFramesIdx]->reward >= currentCutoffScore) passingFramesIdx++;
   size_t localCurrentFramesCut = localFrameDatabaseSize - passingFramesIdx;

   // Getting global frame cutoff
   MPI_Allreduce(&localCurrentFramesCut, &globalCurrentFramesCut, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

   //if (_workerId == 0) printf("Cutoff Score: %f - Frames Cut: %lu/%lu\n", currentCutoffScore, globalCurrentFramesCut, framesToCut);
   if (globalCurrentFramesCut > framesToCut) currentCutoffScore = currentCutoffScore * 0.9999f;
  }

  // Copying frames which pass the cutoff into the database
  _currentFrameDB.clear();
  for (size_t i = 0; i < localFrameDatabaseSize; i++)
   if (_nextFrameDB[i]->reward >= currentCutoffScore)
   _currentFrameDB.push_back(std::move(_nextFrameDB[i]));

  // Clearing next frame DB
  _nextFrameDB.clear();

  // If there are remaining frames, find best global frame reward/win
  if (_globalFrameCounter > 0)
  {
   struct mpiLoc { float val; int loc; };
   mpiLoc mpiLocInput;
   mpiLoc mpiLocResult;
   mpiLocInput.val = localBestFrameScore;
   mpiLocInput.loc = _workerId;
   MPI_Allreduce(&mpiLocInput, &mpiLocResult, 1, MPI_FLOAT_INT, MPI_MAXLOC, MPI_COMM_WORLD);
   int globalBestFrameRank = mpiLocResult.loc;

   // Serializing, broadcasting, and deserializing best frame
   char frameBcastBuffer[_frameSerializedSize];
   if (_workerId == (size_t)globalBestFrameRank) _currentFrameDB[0]->serialize(frameBcastBuffer);
   MPI_Bcast(frameBcastBuffer, 1, _mpiFrameType, globalBestFrameRank, MPI_COMM_WORLD);
   _bestFrame.deserialize(frameBcastBuffer);
   _bestFrame.reward = getFrameReward(_bestFrame);

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
  }

  // Summing frame processing counters
  MPI_Allreduce(&_localStepFramesProcessedCounter, &_stepFramesProcessedCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
  _totalFramesProcessedCounter += _stepFramesProcessedCounter;
}

void Train::updateHashDatabases()
{
 // Discarding older hashes
 for (auto hashItr = _hashDatabase.cbegin(); hashItr != _hashDatabase.cend();)
  if (_currentStep - hashItr->second > _hashAgeThreshold) _hashDatabase.erase(hashItr++);
  else hashItr++;

 // Gathering number of new hash entries
 int localNewHashEntryCount = (int)_newHashes.size();
 std::vector<int> globalNewHashEntryCounts(_workerCount);
 MPI_Allgather(&localNewHashEntryCount, 1, MPI_INT, globalNewHashEntryCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

 // Serializing new hash entries
 std::vector<uint64_t> localNewHashVector;
 localNewHashVector.resize(localNewHashEntryCount);
 size_t curPos = 0;
 for (const auto &hash : _newHashes) localNewHashVector[curPos++] = hash;

 // Freeing new hashes vector
 _newHashes.clear();

 // We use one broadcast operation per worker (as opposed to allgatherv) to reduce the amount of memory used at any given time
 for (size_t curWorkerId = 0; curWorkerId < _workerCount; curWorkerId++)
 {
  size_t workerNewHashCount = globalNewHashEntryCounts[curWorkerId];

  // Allocating memory
  std::vector<uint64_t> workerNewHashVector(workerNewHashCount);

  // Depending on the caller rank, we use a fresh buffer or the new hash vector
  void* hashBuffer = _workerId == curWorkerId ? localNewHashVector.data() : workerNewHashVector.data();

  // Broadcasting worker's new hashes
  MPI_Bcast(hashBuffer, workerNewHashCount, MPI_UNSIGNED_LONG, curWorkerId, MPI_COMM_WORLD);

  // Adding new hashes into the database
  for (size_t i = 0; i < workerNewHashCount; i++)
   _hashDatabase.insert({ ((uint64_t*)hashBuffer)[i], _currentStep});
 }

 // Finding global collision counter
 size_t globalNewCollisionCounter;
 MPI_Allreduce(&_newCollisionCounter, &globalNewCollisionCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
 _globalHashCollisions += globalNewCollisionCounter;
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
      for (size_t i = 0; i < _rules[threadId][ruleId]->_dependenciesIndexes.size(); i++)
        if (frame.rulesStatus[_rules[threadId][ruleId]->_dependenciesIndexes[i]] == false)
          dependenciesMet = false;

      // If dependencies aren't met, then continue to next rule
      if (dependenciesMet == false) continue;

      // Checking if conditions are met
      bool isSatisfied = _rules[threadId][ruleId]->evaluate();

      // If it's achieved, update its status and run its actions
      if (isSatisfied) satisfyRule(frame, ruleId);
    }
  }
}

void Train::checkSpecialActions(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
  {
   // Checking if this rule makes guard disappear
   if (_rules[threadId][ruleId]->_isRemoveGuard == true)
   {
    _sdlPop[threadId]->Guard->y = 250;
    _sdlPop[threadId]->Guard->x = 250;
    _sdlPop[threadId]->Guard->alive = 0;
   }
  }
}

bool Train::checkFail(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
   if (_rules[threadId][ruleId]->_isFailRule == true)
    return true;

 return false;
}

bool Train::checkWin(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
   if (_rules[threadId][ruleId]->_isWinRule == true)
    return true;

 return false;
}

float Train::getRuleRewards(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 float reward = 0.0;

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
    reward += _rules[threadId][ruleId]->_reward;
 return reward;
}

void Train::satisfyRule(Frame &frame, const size_t ruleId)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Recursively run actions for the yet unsatisfied rules that are satisfied by this one and mark them as satisfied
 for (size_t satisfiedIdx = 0; satisfiedIdx < _rules[threadId][ruleId]->_satisfiesIndexes.size(); satisfiedIdx++)
 {
  // Obtaining index
  size_t subRuleId = _rules[threadId][ruleId]->_satisfiesIndexes[satisfiedIdx];

  // Only activate it if it hasn't been activated before
  if(frame.rulesStatus[subRuleId] == false)
   satisfyRule(frame, subRuleId);
 }

 // Setting status to satisfied
 frame.rulesStatus[ruleId] = true;

 // It it contained a restart action, set it now (so it doesn't repeat later)
 if (_rules[threadId][ruleId]->_isRestartRule) frame.isRestart = true;
}

void Train::printTrainStatus()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Current Step #: %lu / %lu\n", _currentStep, _maxSteps);
  printf("[Jaffar] Best Reward: %f\n", _bestFrame.reward);
  printf("[Jaffar] Database Size: %lu / ~%lu\n", _globalFrameCounter, _maxLocalDatabaseSize * _workerCount);
  printf("[Jaffar] Frames Processed: (Step/Total): %lu / %lu\n", _stepFramesProcessedCounter, _totalFramesProcessedCounter);
  printf("[Jaffar] Elapsed Time (Step/Total): %3.3fs / %3.3fs\n", _currentStepTime / 1.0e+9, _searchTotalTime / 1.0e+9);
  printf("[Jaffar] Performance: %.3f Frames/s\n", (double)_stepFramesProcessedCounter / (_currentStepTime / 1.0e+9));

  printf("[Jaffar] Frame Distribution Time:   %3.3fs\n", _frameDistributionTime / 1.0e+9);
  printf("[Jaffar] Frame Computation Time:    %3.3fs\n", _frameComputationTime / 1.0e+9);
  printf("[Jaffar] Hash Postprocessing Time:  %3.3fs\n", _hashPostprocessingTime / 1.0e+9);
  printf("[Jaffar] Frame Postprocessing Time: %3.3fs\n", _framePostprocessingTime / 1.0e+9);

  printf("[Jaffar] Frame DB Entries: Min %lu (Worker: %lu) / Max %lu (Worker: %lu)\n", _minFrameCount, _minFrameWorkerId, _maxFrameCount, _maxFrameWorkerId);
  printf("[Jaffar] Hash DB Collisions: %lu\n", _globalHashCollisions);
  printf("[Jaffar] Hash DB Entries: %lu\n", _hashDatabase.size());

  printf("[Jaffar] Frame DB Size: Min %.3fmb  / Max: %.3fmb\n", (double)(_minFrameCount * Frame::getSerializationSize()) / (1024.0 * 1024.0), (double)(_maxFrameCount * Frame::getSerializationSize()) / (1024.0 * 1024.0));
  printf("[Jaffar] Hash DB Size: %.3fmb\n", (double)(_hashDatabase.size() * sizeof(std::pair<uint64_t, uint32_t>)) / (1024.0 * 1024.0));

  printf("[Jaffar] Best Frame Information:\n");

  _state[0]->loadState(_bestFrame.frameStateData);
  _sdlPop[0]->printFrameInfo();
  printRuleStatus(_bestFrame);

  // Getting kid room
  int kidCurrentRoom = _sdlPop[0]->Kid->room;

  // Getting magnet values for the kid
  auto kidMagnet = getKidMagnetValues(_bestFrame, kidCurrentRoom);

  printf("[Jaffar]  + Kid Horizontal Magnet Intensity / Position: %.1f / %.0f\n", kidMagnet.intensityX, kidMagnet.positionX);
  printf("[Jaffar]  + Kid Vertical Magnet Intensity: %.1f\n", kidMagnet.intensityY);

  // Getting guard room
  int guardCurrentRoom = _sdlPop[0]->Guard->room;

  // Getting magnet values for the guard
  auto guardMagnet = getGuardMagnetValues(_bestFrame, guardCurrentRoom);

  printf("[Jaffar]  + Guard Horizontal Magnet Intensity / Position: %.1f / %.0f\n", guardMagnet.intensityX, guardMagnet.positionX);
  printf("[Jaffar]  + Guard Vertical Magnet Intensity: %.1f\n", guardMagnet.intensityY);

  // Print Move History
  if (_storeMoveList)
  {
   printf("[Jaffar]  + Last 30 Moves: ");
   size_t startMove = (size_t)std::max((int)0, (int)_currentStep-30);
   for (size_t i = startMove; i <= _currentStep; i++)
     printf("%s ", _possibleMoves[_bestFrame.getMove(i)].c_str());
   printf("\n");
  }
}

magnetInfo_t Train::getKidMagnetValues(const Frame &frame, const int room)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < frame.rulesStatus.size(); ruleId++)
 {
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[threadId][ruleId];

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetPositionX.size(); i++)
     if (rule->_kidMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_kidMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetIntensityX.size(); i++)
     if (rule->_kidMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_kidMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetIntensityY.size(); i++)
     if (rule->_kidMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_kidMagnetIntensityY[i].value;
  }
 }

 return magnetInfo;
}

magnetInfo_t Train::getGuardMagnetValues(const Frame &frame, const int room)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < frame.rulesStatus.size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[threadId][ruleId];

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetPositionX.size(); i++)
     if (rule->_guardMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_guardMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetIntensityX.size(); i++)
     if (rule->_guardMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_guardMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetIntensityY.size(); i++)
     if (rule->_guardMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_guardMagnetIntensityY[i].value;
  }

 return magnetInfo;
}

float Train::getFrameReward(const Frame &frame)
{
  // Getting thread id
  int threadId = omp_get_thread_num();

  // Accumulator for total reward
  float reward = getRuleRewards(frame);

  // Getting kid room
  int kidCurrentRoom = _sdlPop[threadId]->Kid->room;

  // Getting magnet values for the kid
  auto kidMagnet = getKidMagnetValues(frame, kidCurrentRoom);

  // Getting kid's current frame
  const auto curKidFrame = _sdlPop[threadId]->Kid->frame;

  // Evaluating kidMagnet's reward on the X axis
  const float kidDiffX = std::abs(_sdlPop[threadId]->Kid->x - kidMagnet.positionX);
  reward += (float) kidMagnet.intensityX * (256.0f - kidDiffX);

  // For positive Y axis kidMagnet, rewarding climbing frames
  if ((float) kidMagnet.intensityY > 0.0f)
  {
    // Jumphang, because it preludes climbing (Score + 1-20)
    if (curKidFrame >= 67 && curKidFrame <= 80)
    {
      float rewardAdd = (float) kidMagnet.intensityY * (0.0f + (curKidFrame - 66));
      reward += rewardAdd;
    }

    // Hang, because it preludes climbing (Score +21)
    if (curKidFrame == 91) reward += 21.0f * (float) kidMagnet.intensityY;

    // Climbing (Score +22-38)
    if (curKidFrame >= 135 && curKidFrame <= 149) reward += (float) kidMagnet.intensityY * (22.0f + (curKidFrame - 134));

    // Adding absolute reward for Y position
    reward += (float) kidMagnet.intensityY * (256.0f - _sdlPop[threadId]->Kid->y);
  }

  // For negative Y axis kidMagnet, rewarding falling/climbing down frames
  if ((float) kidMagnet.intensityY < 0.0f)
  {
    // Turning around, because it generally preludes climbing down
    if (curKidFrame >= 45 && curKidFrame <= 52) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hanging, because it preludes falling
    if (curKidFrame >= 87 && curKidFrame <= 99) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hang drop, because it preludes falling
    if (curKidFrame >= 81 && curKidFrame <= 85) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling start
    if (curKidFrame >= 102 && curKidFrame <= 105) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling itself
    if (curKidFrame == 106) reward += -2.0f + (float) kidMagnet.intensityY;

    // Climbing down
    if (curKidFrame == 148) reward += -2.0f + (float) kidMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * kidMagnet.intensityY * (_sdlPop[threadId]->Kid->y);
  }

  // Getting guard room
  int guardCurrentRoom = _sdlPop[threadId]->Guard->room;

  // Getting magnet values for the guard
  auto guardMagnet = getGuardMagnetValues(frame, guardCurrentRoom);

  // Getting guard's current frame
  const auto curGuardFrame = _sdlPop[threadId]->Guard->frame;

  // Evaluating guardMagnet's reward on the X axis
  const float guardDiffX = std::abs(_sdlPop[threadId]->Guard->x - guardMagnet.positionX);
  reward += (float) guardMagnet.intensityX * (256.0f - guardDiffX);

  // For positive Y axis guardMagnet
  if ((float) guardMagnet.intensityY > 0.0f)
  {
   // Adding absolute reward for Y position
   reward += (float) guardMagnet.intensityY * (256.0f - _sdlPop[threadId]->Guard->y);
  }

  // For negative Y axis guardMagnet, rewarding falling/climbing down frames
  if ((float) guardMagnet.intensityY < 0.0f)
  {
    // Falling start
    if (curGuardFrame >= 102 && curGuardFrame <= 105) reward += -1.0f * (float) guardMagnet.intensityY;

    // Falling itself
    if (curGuardFrame == 106) reward += -2.0f + (float) guardMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * guardMagnet.intensityY * (_sdlPop[threadId]->Guard->y);
  }

  // Apply bonus when kid is inside a non-visible room
  if (kidCurrentRoom == 0 || kidCurrentRoom >= 25) reward += 128.0f;

  // Returning reward
  return reward;
}

std::vector<uint8_t> Train::getPossibleMoveIds(const Frame &frame)
{
  // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13    14
  //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA" };

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
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

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
  _threadCount = omp_get_max_threads();

  // Setting SDL env variables to use the dummy renderer
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);

  // Profiling information
  _searchTotalTime = 0.0;
  _currentStepTime = 0.0;
  _frameDistributionTime = 0.0;
  _frameComputationTime = 0.0;
  _framePostprocessingTime = 0.0;

  // Initializing counters
  _stepFramesProcessedCounter = 0;
  _totalFramesProcessedCounter = 0;
  _newCollisionCounter = 0;
  _localStepFramesProcessedCounter = 0;

  // Setting starting step
  _currentStep = 0;

  // Parsing max hash DB entries
  if (const char *hashAgeThresholdString = std::getenv("JAFFAR_HASH_AGE_THRESHOLD"))
   _hashAgeThreshold = std::stol(hashAgeThresholdString);
  else if (_workerId == 0)
   EXIT_WITH_ERROR("[Jaffar] JAFFAR_HASH_AGE_THRESHOLD environment variable not defined.\n");

  // Parsing max frame DB entries
  size_t frameDBMaxMBytes = 0;
  if (const char *frameDBMaxMBytesEnv = std::getenv("JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB"))
    frameDBMaxMBytes = std::stol(frameDBMaxMBytesEnv);
  else if (_workerId == 0)
   EXIT_WITH_ERROR("[Jaffar] JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB environment variable not defined. Using conservative default...\n");

  // Parsing file output frequency
  _outputSaveBestSeconds = -1.0;
  if (const char *outputSaveBestSecondsEnv = std::getenv("JAFFAR_SAVE_BEST_EVERY_SECONDS")) _outputSaveBestSeconds = std::stof(outputSaveBestSecondsEnv);
  _outputSaveCurrentSeconds = -1.0;
  if (const char *outputSaveCurrentSecondsEnv = std::getenv("JAFFAR_SAVE_CURRENT_EVERY_SECONDS")) _outputSaveCurrentSeconds = std::stof(outputSaveCurrentSecondsEnv);

  // Parsing savegame files output path
  _outputSaveBestPath = "/tmp/jaffar.best.sav";
  if (const char *outputSaveBestPathEnv = std::getenv("JAFFAR_SAVE_BEST_PATH")) _outputSaveBestPath = std::string(outputSaveBestPathEnv);
  _outputSaveCurrentPath = "/tmp/jaffar.current.sav";
  if (const char *outputSaveCurrentPathEnv = std::getenv("JAFFAR_SAVE_CURRENT_PATH")) _outputSaveCurrentPath = std::string(outputSaveCurrentPathEnv);

  // Parsing solution files output path
  _outputSolutionBestPath = "/tmp/jaffar.best.sol";
  if (const char *outputSolutionBestPathEnv = std::getenv("JAFFAR_SOLUTION_BEST_PATH")) _outputSolutionBestPath = std::string(outputSolutionBestPathEnv);
  _outputSolutionCurrentPath = "/tmp/jaffar.current.sol";
  if (const char *outputSolutionCurrentPathEnv = std::getenv("JAFFAR_SOLUTION_CURRENT_PATH")) _outputSolutionCurrentPath = std::string(outputSolutionCurrentPathEnv);

  // Parsing command line arguments
  argparse::ArgumentParser program("jaffar-train", JAFFAR_VERSION);

  program.add_argument("--savFile")
    .help("Specifies the path to the SDLPop savefile (.sav) from which to start.")
    .default_value(std::string("quicksave.sav"))
    .required();

  program.add_argument("--maxSteps")
    .help("Specifies the maximum number of steps to run jaffar for.")
    .action([](const std::string& value) { return std::stoul(value); })
    .required();

  program.add_argument("--disableHistory")
    .help("Do not store the move history during training to save memory.")
    .default_value(false)
    .implicit_value(true);

  program.add_argument("jaffarFiles")
    .help("path to the Jaffar configuration script (.jaffar) file(s) to run.")
    .remaining()
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

  // Establishing whether to store move history
  _storeMoveList = program.get<bool>("--disableHistory") == false;

  // Getting savefile path
  auto saveFilePath = program.get<std::string>("--savFile");

  // Loading save file contents
  std::string saveString;
  bool status = loadStringFromFile(saveString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());

  // Parsing max steps
  _maxSteps = program.get<size_t>("--maxSteps");

  // The move list contains two moves per byte
  _moveListStorageSize = (_maxSteps >> 1) + 1;

  // Calculating DB sizes
  _maxLocalDatabaseSize = floor(((double)frameDBMaxMBytes * 1024.0 * 1024.0) / ((double)Frame::getSerializationSize()));
  _maxGlobalDatabaseSize = _maxLocalDatabaseSize * _workerCount;

  // Parsing config files
  _scriptFiles = program.get<std::vector<std::string>>("jaffarFiles");
  std::vector<nlohmann::json> scriptFilesJs;
  for (size_t i = 0; i < _scriptFiles.size(); i++)
  {
   std::string scriptString;
   status = loadStringFromFile(scriptString, _scriptFiles[i].c_str());
   if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n%s \n", _scriptFiles[i].c_str(), program.help().str().c_str());

   nlohmann::json scriptJs;
   try
   {
    scriptJs = nlohmann::json::parse(scriptString);
   }
   catch (const std::exception &err)
   {
     if (_workerId == 0) fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", _scriptFiles[i].c_str(), err.what());
     exit(-1);
   }

   // Checking whether it contains the rules field
   if (isDefined(scriptJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Train configuration file '%s' missing 'Rules' key.\n", _scriptFiles[i].c_str());

   // Adding it to the collection
   scriptFilesJs.push_back(scriptJs);
  }

  // Getting number of omp threads and resizing containers based on that
  _sdlPop.resize(_threadCount);
  _state.resize(_threadCount);
  _rules.resize(_threadCount);

  // Creating a first instance of SDLPop that will serve as cache for all the others
  std::string fileCache;

  // Serializing the file cache to reduce pressure on I/O
  if (_workerId == 0)
  {
   _sdlPop[0] = new SDLPopInstance("libsdlPopLib.so", true);
   fileCache = _sdlPop[0]->serializeFileCache();
  }

  // Broadcasting cache
  size_t fileCacheSize = fileCache.size();
  MPI_Bcast(&fileCacheSize, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
  fileCache.resize(fileCacheSize);
  MPI_Bcast(&fileCache[0], fileCacheSize, MPI_CHAR, 0, MPI_COMM_WORLD);

  // Creating SDL Pop Instance, one per openMP Thread
  for (int threadId = 0; threadId < _threadCount; threadId++)
  {
    if ( (_workerId == 0 && threadId == 0) == false) 
     _sdlPop[threadId] = new SDLPopInstance("libsdlPopLib.so", true);
    _sdlPop[threadId]->deserializeFileCache(fileCache);
    _sdlPop[threadId]->initialize(false);

    // Exiting SDL for thread safety reasons
    SDL_Quit();

    // Initializing State Handler
    _state[threadId] = new State(_sdlPop[threadId], saveString);

   // Adding rules, pointing to the thread-specific sdlpop instances
   for (size_t scriptId = 0; scriptId < scriptFilesJs.size(); scriptId++)
    for (size_t ruleId = 0; ruleId < scriptFilesJs[scriptId]["Rules"].size(); ruleId++)
     _rules[threadId].push_back(new Rule(scriptFilesJs[scriptId]["Rules"][ruleId], _sdlPop[threadId]));

   // Setting global rule count
   _ruleCount = _rules[threadId].size();

   // Checking for repeated rule labels
   std::set<size_t> ruleLabelSet;
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   {
    size_t label = _rules[threadId][ruleId]->_label;
    ruleLabelSet.insert(label);
    if (ruleLabelSet.size() < ruleId + 1)
     EXIT_WITH_ERROR("[ERROR] Rule label %lu is repeated in the configuration file.\n", label);
   }

   // Looking for rule dependency indexes that match their labels
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
    for (size_t depId = 0; depId < _rules[threadId][ruleId]->_dependenciesLabels.size(); depId++)
    {
     bool foundLabel = false;
     size_t label = _rules[threadId][ruleId]->_dependenciesLabels[depId];
     if (label == _rules[threadId][ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in dependencies vector.\n", label);
     for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
      if (_rules[threadId][subRuleId]->_label == label)
      {
       _rules[threadId][ruleId]->_dependenciesIndexes[depId] = subRuleId;
       foundLabel = true;
       break;
      }
     if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as dependency by rule %lu.\n", label, _rules[threadId][ruleId]->_label);
    }

   // Looking for rule satisfied sub-rules indexes that match their labels
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
    for (size_t satisfiedId = 0; satisfiedId < _rules[threadId][ruleId]->_satisfiesLabels.size(); satisfiedId++)
    {
     bool foundLabel = false;
     size_t label = _rules[threadId][ruleId]->_satisfiesLabels[satisfiedId];
     if (label == _rules[threadId][ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in satisfied vector.\n", label);

     for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
      if (_rules[threadId][subRuleId]->_label == label)
      {
       _rules[threadId][ruleId]->_satisfiesIndexes[satisfiedId] = subRuleId;
       foundLabel = true;
       break;
      }
     if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as satisfied by rule %lu.\n", label, satisfiedId);
    }
  }

  printf("[Jaffar] MPI Rank %lu/%lu: SDLPop initialized.\n", _workerId, _workerCount);
  fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);

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

  // Initializing frame counts per worker
  _localBaseFrameCounts.resize(_workerCount);

  // Creating initial frame on the root rank
  if (_workerId == 0)
  {
    // Computing initial hash
    const auto hash = _state[0]->computeHash();

    auto initialFrame = std::make_unique<Frame>();
    initialFrame->frameStateData = _state[0]->saveState();
    initialFrame->rulesStatus = rulesStatus;

    // Evaluating Rules on initial frame
    evaluateRules(*initialFrame);

    // Evaluating Score on initial frame
    initialFrame->reward = getFrameReward(*initialFrame);

    // Registering hash for initial frame
    _hashDatabase.insert({ hash, 0 });

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

        // Storing the solution sequence
        if (_storeMoveList)
        {
         std::string solutionString;
         solutionString += _possibleMoves[_bestFrame.getMove(0)];
         for (size_t i = 1; i <= _currentStep; i++)
          solutionString += std::string(" ") + _possibleMoves[_bestFrame.getMove(i)];
         saveStringToFile(solutionString, _outputSolutionBestPath.c_str());
        }

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

        // Storing the solution sequence
        if (_storeMoveList)
        {
         std::string solutionString;
         solutionString += _possibleMoves[_showFrameDB[currentFrameId].getMove(0)];
         for (size_t i = 1; i <= _currentStep; i++)
          solutionString += std::string(" ") + _possibleMoves[_showFrameDB[currentFrameId].getMove(i)];
         saveStringToFile(solutionString, _outputSolutionCurrentPath.c_str());
        }

        // Resetting timer
        currentFrameSaveTimer = std::chrono::steady_clock::now();

        // Increasing counter
        currentFrameId = (currentFrameId + 1) % SHOW_FRAME_COUNT;
      }
    }
  }

  // If it has finalized with a win, save the winning frame
  if (_outputSaveBestSeconds > 0.0)
  {
   auto lastFrame = _winFrameFound ? _globalWinFrame : _bestFrame;

   saveStringToFile(lastFrame.frameStateData, _outputSaveBestPath.c_str());

   // Storing the solution sequence
   if (_storeMoveList)
   {
    std::string solutionString;
    solutionString += _possibleMoves[lastFrame.getMove(0)];
    for (size_t i = 1; i <= _currentStep; i++)
     solutionString += std::string(" ") + _possibleMoves[lastFrame.getMove(i)];
    saveStringToFile(solutionString, _outputSolutionBestPath.c_str());
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
