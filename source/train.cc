#include "train.h"
#include "utils.h"
#include <unistd.h>
#include <chrono>
#include "argparse.hpp"

void Train::run()
{
 if (_workerId== 0)
 {
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Launching Jaffar Version %s...\n", JAFFAR_VERSION);
  printf("[Jaffar] Using configuration file: %s.\n", _scriptFile.c_str());
  printf("[Jaffar] Starting search with %lu workers.\n", _workerCount);
 }

 // Wait for all workers to be ready
 MPI_Barrier(MPI_COMM_WORLD);

 auto searchTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Storage for termination trigger
 bool terminate = false;

 while(terminate == false)
 {
  // If this is the root rank, plot the best frame and print information
  if (_workerId == 0)
  {
   // Plotting current best frame
   _state->loadBase(_baseStateData);
   _state->loadFrame(_bestFrame.frameStateData);
   _sdlPop->refreshEngine();
   _sdlPop->_currentMove = _possibleMoves[_bestFrame.moveHistory[_currentStep]];

   // Drawing frame
   _sdlPop->draw();

   // Profiling information
   auto searchTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _searchTotalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(searchTimeEnd - searchTimeBegin).count();    // Profiling

   // Printing search status
   printTrainStatus();
  }

  /////////////////////////////////////////////////////////////////
  /// Main frame processing cycle begin
  /////////////////////////////////////////////////////////////////

  // 1) Workers exchange new base frames uniformly
  distributeFrames();

  // 2) Workers process their own base frames
  computeFrames();

  // 3) Workers sort their databases and communicate partial results
  framePostprocessing();

  // 4) Workers exchange hash information and update hash databases
  updateHashDatabases();

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
  _state->loadBase(_baseStateData);
  _state->loadFrame(_globalWinFrame.frameStateData);
  _sdlPop->refreshEngine();
  _sdlPop->printFrameInfo();
  printRuleStatus(_globalWinFrame);

  // Print Move History
  printf("[Jaffar]  + Move List: ");
  for (size_t i = 0; i <= _currentStep; i++)
   printf("%s ", _possibleMoves[_globalWinFrame.moveHistory[i]].c_str());
  printf("\n");
 }

 // Barrier to wait for all workers
 MPI_Barrier(MPI_COMM_WORLD);
}

void Train::distributeFrames()
{
 auto frameDistributionTimeBegin = std::chrono::steady_clock::now(); // Profiling

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

  // Shuffling database to remove bias in the score distribution
  std::shuffle(_currentFrameDB.begin(), _currentFrameDB.end(), std::default_random_engine(_currentStep));

  // Exchanging frames with all other workers at a one by one basis
  // This could be done more efficiently in terms of message traffic and performance by using an MPI_AlltoAllv operation
  // However, this operation requires that all buffers are allocated simultaneously which puts additional pressure on memory
  // Given this bot needs as much memory as possible, the more memory efficient 1-to-1 communication is used
  for (size_t neighborId = 0; neighborId < _workerCount; neighborId++)
  {
   // Getting counts for the current worker
   int recvCount = allToAllRecvCounts[_workerId][neighborId];
   int sendCount = allToAllSendCounts[_workerId][neighborId];

   // Calculating buffer size as the max between the frames we send and receive
   size_t bufferSize = std::max(recvCount, sendCount);

   // Reserving exchange buffer
   char* frameExchangeBuffer = (char*) malloc(_frameSerializedSize * bufferSize);

   // Serializing always the last frame and reducing vector size to minimize memory usage
   size_t currentSendPosition = 0;
   for (int i = 0; i < sendCount; i++)
   {
    // Getting current frame count
    const size_t count = _currentFrameDB.size();

    // Getting last frame
    const auto& frame = _currentFrameDB[count-1];

    // Serializing frame
    frame->serialize(&frameExchangeBuffer[currentSendPosition]);

    // Advancing send buffer pointer
    currentSendPosition += _frameSerializedSize;

    // Removing last frame by resizing
    _currentFrameDB.resize(count-1);
   }

   // Exchanging frames with neighbor
   MPI_Sendrecv_replace(frameExchangeBuffer, bufferSize, _mpiFrameType, neighborId, 0, neighborId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

   // Deserializing contiguous buffer into the new frame database
   size_t currentRecvPosition = 0;
   for (int frameId = 0; frameId < recvCount; frameId++)
   {
    // Creating new frame
    auto newFrame = std::make_unique<Frame>();

    // Deserializing frame
    newFrame->deserialize(&frameExchangeBuffer[currentRecvPosition]);

    // Adding new frame into the data base
    _nextFrameDB.push_back(std::move(newFrame));

    // Advancing send buffer pointer
    currentRecvPosition += _frameSerializedSize;
   }

   // Freeing exchange buffer
   free(frameExchangeBuffer);
  }

  // Swapping database pointers
  _currentFrameDB = std::move(_nextFrameDB);

  auto frameDistributionTimeEnd = std::chrono::steady_clock::now(); // Profiling
  _frameDistributionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameDistributionTimeEnd - frameDistributionTimeBegin).count();    // Profiling
}

void Train::computeFrames()
{
 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Initializing counters
 _localStepFramesProcessedCounter = 0;
 _newCollisionCounter = 0;

 // Computing always the last frame while resizing the database to reduce memory footprint
 while (_currentFrameDB.empty() == false)
 {
  // Getting current frame count
  const size_t count = _currentFrameDB.size();

  // Getting base frame pointer
  const auto& baseFrame = _currentFrameDB[count-1];

  // Loading base frame information
  _state->loadBase(_baseStateData);

  // Getting possible moves for the current frame
  auto possibleMoveIds = getPossibleMoveIds(*baseFrame);

  // Running possible moves
  for (size_t idx = 0; idx < possibleMoveIds.size(); idx++)
  {
   // Getting possible move id
   auto moveId = possibleMoveIds[idx];

   // Getting possible move string
   std::string move = _possibleMoves[moveId].c_str();

   // Loading frame state
   _state->loadFrame(baseFrame->frameStateData);

   // Refreshing engine
   _sdlPop->refreshEngine();

   // Perform the selected move
   _sdlPop->performMove(move);

   // Advance a single frame
   _sdlPop->advanceFrame();

   // Increasing  frames processed counter
   _localStepFramesProcessedCounter++;

   // Compute hash value
   auto hash = _state->computeHash();

   // Inserting and checking for the existence of the hash in the hash databases
   bool collisionDetected = false;
   for (size_t i = 0; i < HASH_DATABASE_COUNT; i++)
    collisionDetected |= _hashDatabases[i].contains(hash);

   // If collision detected locally, discard this frame
   if (collisionDetected) { _newCollisionCounter++; continue; }

   // If not previously observed add new hash to the database
   addHashEntry(hash);

   // Creating new frame, mixing base frame information and the current sdlpop state
   auto newFrame = std::make_unique<Frame>();
   newFrame->isWin = false;
   newFrame->isFail = false;
   newFrame->moveHistory = baseFrame->moveHistory;
   newFrame->moveHistory[_currentStep] = moveId;
   newFrame->rulesStatus = baseFrame->rulesStatus;
   newFrame->frameStateData = _state->saveFrame();
   newFrame->magnets = baseFrame->magnets;

   // Evaluating rules on the new frame
   evaluateRules(*newFrame);

   // Calculating score
   newFrame->score = getFrameScore(*newFrame);

   // If frame has failed, then proceed to the next one
   if (newFrame->isFail == true) continue;

   // If frame has succeded, then flag it
   if (newFrame->isWin == true) { _localWinFound = true; _localWinFrame = *newFrame; }

   // Adding novel frame in the next frame database
   _nextFrameDB.push_back(std::move(newFrame));

   // Adding hash into the new hashes table
   _newHashes.insert(hash);
  }

  // Reducing base database size
  _currentFrameDB.resize(count-1);
 }

 auto frameComputationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count();    // Profiling
}

void Train::framePostprocessing()
{
 auto framePostprocessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Swapping database pointers
 _currentFrameDB = std::move(_nextFrameDB);

 // Clearing next frame database
 _nextFrameDB.clear();

 // Sorting local DB frames by score
 std::sort(_currentFrameDB.begin(), _currentFrameDB.end(), [](const auto& a, const auto& b) { return a->score > b->score; });

 // Finding local best frame score
 float localBestFrameScore = -std::numeric_limits<float>::infinity();
 if (_currentFrameDB.empty() == false) localBestFrameScore = _currentFrameDB[0]->score;

 // Clipping local database to the maximum threshold
 if (_currentFrameDB.size() > _maxLocalDatabaseSize) _currentFrameDB.resize(_maxLocalDatabaseSize);

 // Finding best global frame score
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

 // Calculating global frame count
 size_t newLocalFrameDatabaseSize = _currentFrameDB.size();
 MPI_Allreduce(&newLocalFrameDatabaseSize, &_globalFrameCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

 // Exchanging the fact that a win frame has been found
 int winRank = -1;
 int localHasWinFrame = _localWinFound == false ? 0 : 1;
 std::vector<int> globalHasWinFrameVector(_workerCount);
 MPI_Allgather(&localHasWinFrame, 1, MPI_INT, globalHasWinFrameVector.data(), 1, MPI_INT, MPI_COMM_WORLD);
 for (size_t i = 0; i < _workerCount; i++) if (globalHasWinFrameVector[i] == 1) winRank = i;

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

 auto framePostprocessingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePostprocessingTimeEnd - framePostprocessingTimeBegin).count();    // Profiling
}

void Train::updateHashDatabases()
{
 auto hashExchangeTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Gathering number of new hash entries
 int localNewHashEntryCount = (int)_newHashes.size();
 std::vector<int> globalNewHashEntryCounts(_workerCount);
 MPI_Allgather(&localNewHashEntryCount, 1, MPI_INT, globalNewHashEntryCounts.data(), 1, MPI_INT, MPI_COMM_WORLD);

 // Calculating displacements for new hash entries
 std::vector<int> globalNewHashEntryDispls(_workerCount);
 int currentDispl = 0;
 for(size_t i = 0; i < _workerCount; i++)
 {
  globalNewHashEntryDispls[i] = currentDispl;
  currentDispl += globalNewHashEntryCounts[i];
 }

 // Serializing new hash entries
 std::vector<uint64_t> localNewHashVector;
 for (const auto& key : _newHashes) localNewHashVector.push_back(key);

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

 auto hashExchangeTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _hashExchangeTime = std::chrono::duration_cast<std::chrono::nanoseconds>(hashExchangeTimeEnd - hashExchangeTimeBegin).count();    // Profiling
}

void Train::evaluateRules(Frame& frame)
{
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
 {
  // Evaluate rule only if it's active
  if (frame.rulesStatus[ruleId] == st_active)
  {
   // Checking dependencies first. If not met, continue to the next rule
   bool dependenciesMet = true;
   for (size_t i = 0; i < _rules[ruleId]->_dependencies.size(); i++)
    if (frame.rulesStatus[_rules[ruleId]->_dependencies[i]] != st_achieved)
     dependenciesMet = false;

   // If dependencies aren't met, then continue to next rule
   if (dependenciesMet == false) continue;

   // Checking if conditions are met
   bool isAchieved = _rules[ruleId]->evaluate();

   // If it's achieved, update its status and run its actions
   if (isAchieved)
   {
    // Setting status to achieved
    frame.rulesStatus[ruleId] = st_achieved;

    // Perform actions
    for (size_t actionId = 0; actionId < _rules[ruleId]->_actions.size(); actionId++)
    {
     const auto actionJs = _rules[ruleId]->_actions[actionId];

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

     if (actionType == "Set Horizontal Magnet Intensity")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensityX = actionJs["Value"].get<float>();

      frame.magnets[room].intensityX = intensityX;
      recognizedActionType = true;
     }

     if (actionType == "Set Horizontal Magnet Position")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float positionX = actionJs["Value"].get<float>();

      frame.magnets[room].positionX = positionX;
      recognizedActionType = true;
     }

     if (actionType == "Set Vertical Magnet Intensity")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensityY = actionJs["Value"].get<float>();

      frame.magnets[room].intensityY = intensityY;
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
  for (int i = HASH_DATABASE_COUNT-1; i > 0; i--)
   _hashDatabases[i] = std::move(_hashDatabases[i-1]);

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
 printf("[Jaffar] Database Size: %lu / %lu\n", _globalFrameCounter, _maxLocalDatabaseSize*_workerCount);
 printf("[Jaffar] Frames Processed: (Step/Total): %lu / %lu\n", _stepFramesProcessedCounter, _totalFramesProcessedCounter);
 printf("[Jaffar] Elapsed Time (Step/Total): %3.3fs / %3.3fs\n", (_frameDistributionTime + _frameComputationTime + _framePostprocessingTime) / 1.0e+9, _searchTotalTime / 1.0e+9);

 printf("[Jaffar] Frame Distribution Time:   %3.3fs\n", _frameDistributionTime / 1.0e+9);
 printf("[Jaffar] Frame Computation Time:    %3.3fs\n", _frameComputationTime / 1.0e+9);
 printf("[Jaffar] Hash Exchange Time:        %3.3fs\n", _hashExchangeTime / 1.0e+9);
 printf("[Jaffar] Frame Postprocessing Time: %3.3fs\n", _framePostprocessingTime / 1.0e+9);

  printf("[Jaffar] Hash DB Collisions: %lu\n", _globalHashCollisions);
  printf("[Jaffar] Hash DB Entries: %lu / %lu\n", _globalHashEntries, (size_t)HASH_DATABASE_COUNT * _hashDatabaseSizeThreshold * _workerCount);
  printf("[Jaffar] Hash DB Swaps: %lu\n", _hashDatabaseSwapCount);

  printf("[Jaffar] Best Frame Information:\n");
  _sdlPop->printFrameInfo();

  printRuleStatus(_bestFrame);

  // Printing Magnet Status
  int currentRoom = _sdlPop->Kid->room;
  const auto& magnet = _bestFrame.magnets[currentRoom];
  printf("[Jaffar]  + Horizontal Magnet Intensity / Position: %.1f / %.0f\n", magnet.intensityX, magnet.positionX);
  printf("[Jaffar]  + Vertical Magnet Intensity: %.1f\n", magnet.intensityY);

  // Print Move History
  printf("[Jaffar]  + Move List: ");
  for (size_t i = 0; i <= _currentStep; i++)
   printf("%s ", _possibleMoves[_bestFrame.moveHistory[i]].c_str());
  printf("\n");
}

void Train::printRuleStatus(const Frame& frame)
{
 printf("[Jaffar]  + Rule Status: [ %d", (int)frame.rulesStatus[0]);
 for (size_t i = 1; i < frame.rulesStatus.size(); i++)
  printf(", %d", (int) frame.rulesStatus[i]);
 printf(" ]\n");

}

float Train::getFrameScore(const Frame& frame)
{
 // Accumulator for total score
 float score = 0.0f;

 // Obtaining magnet corresponding to kid's room
 int currentRoom = _sdlPop->Kid->room;

 // Apply normal magnet if kid is inside visible rooms
 if (currentRoom >= 0 && currentRoom < _VISIBLE_ROOM_COUNT)
 {
  const auto &magnet = frame.magnets[currentRoom];
  const auto curFrame = _sdlPop->Kid->frame;

  // Evaluating magnet's score on the X axis
  const float diff = std::abs(_sdlPop->Kid->x - magnet.positionX);
  score += magnet.intensityX * (256.0f - diff);

  // For positive Y axis magnet, rewarding climbing frames
  if (magnet.intensityY > 0.0f)
  {
   // Jumphang, because it preludes climbing (Score + 1-20)
   if (curFrame >= 67 && curFrame <= 80)
   {
    float scoreAdd = magnet.intensityY * (0.0f + (curFrame - 66));
    score += scoreAdd;
   }

   // Hang, because it preludes climbing (Score +21)
   if (curFrame == 91) score += 21.0f * magnet.intensityY;

   // Climbing (Score +22-38)
   if (curFrame >= 135 && curFrame <= 149) score += magnet.intensityY * (22.0f + (curFrame - 134));
  }

  // For negative Y axis magnet, rewarding falling/climbing down frames
  if (magnet.intensityY < 0.0f)
  {
   // Turning around, because it generally preludes climbing down
   if (curFrame >= 45 && curFrame <= 52) score += -0.5f * magnet.intensityY;

   // Hanging, because it preludes falling
   if (curFrame >= 87 && curFrame <= 99) score += -0.5f * magnet.intensityY;

   // Hang drop, because it preludes falling
   if (curFrame >= 81 && curFrame <= 85) score += -1.0f * magnet.intensityY;

   // Falling start
   if (curFrame >= 102 && curFrame <= 105) score += -1.0f * magnet.intensityY;

   // Falling itself
   if (curFrame == 106) score += -2.0f + magnet.intensityY;

   // Climbing down
   if (curFrame == 148) score += -2.0f + magnet.intensityY;
  }
 }

 // Apply full magnet when kid is inside a non-visible room
 if (currentRoom == 0 || currentRoom >= _VISIBLE_ROOM_COUNT)
  score += 128.0f;

 // Now adding rule rewards
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  if (frame.rulesStatus[ruleId] == st_achieved)
   score += _rules[ruleId]->_reward;

 // Returning score
 return score;
}

std::vector<uint8_t> Train::getPossibleMoveIds(const Frame& frame)
{
 // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13
 //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

 // Loading frame state
 _state->loadFrame(frame.frameStateData);

 // Getting Kid information
 const auto& Kid = *_sdlPop->Kid;

 // If dead, do nothing
 if (Kid.alive >= 0)
  return { 0 };

 // If bumped, nothing to do
 if (Kid.action == actions_5_bumped)
  return { 0 };

 // If in mid air or free fall, hope to grab on to something
 if (Kid.action == actions_3_in_midair || Kid.action == actions_4_in_freefall)
  return { 0, 1 };

 // Move, sheath, attack, parry
 if (Kid.sword == sword_2_drawn)
  return { 0, 1, 2, 3, 4, 5 };

 // Kid is standing or finishing a turn, try all possibilities
 if ( Kid.frame == frame_15_stand ||  ( Kid.frame >= frame_50_turn && Kid.frame < 53 ) )
  return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

 // Turning frame, try all possibilities
 if (Kid.frame == frame_48_turn)
  return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

 // Start running animation, all movement without shift
 if (Kid.frame < 4)
  return { 0, 2, 3, 4, 5, 6, 7, 8, 9, };

 // Starting jump up, check directions, jump and shift
 if (Kid.frame >= frame_67_start_jump_up_1 && Kid.frame < frame_70_jumphang)
  return { 0, 1, 2, 3, 4, 5, 6, 8, 12};

 // Running, all movement without shift
 if (Kid.frame < 15)
  return { 0, 2, 3, 4, 5, 6, 7, 8, 9 };

 // Hanging, up and shift are only options
 if (Kid.frame >= frame_87_hanging_1 && Kid.frame < 100)
  return { 0, 1, 2, 12 };

 // Crouched, can only stand, drink, or hop
 if (Kid.frame == frame_109_crouch)
  return { 0, 1, 3, 4, 5, 7, 9, 13 };

 // Default, no nothing
 return { 0 };
}

Train::Train(int argc, char* argv[])
{
 // Initialize the MPI environment
 MPI_Init(&argc, &argv);

 // Get the number of processes
 int mpiSize;
 MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
 _workerCount = (size_t) mpiSize;

 // Get the rank of the process
 int mpiRank;
 MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
 _workerId = (size_t) mpiRank;

 // Parsing command line arguments
 argparse::ArgumentParser program("jaffar-train", JAFFAR_VERSION);

 program.add_argument("jaffarFile")
   .help("path to the Jaffar script (.jaffar) file to run.")
   .required();

 // Try to parse arguments
 try
 {
   program.parse_args(argc, argv);
 }
 catch (const std::runtime_error& err)
 {
   if (_workerId== 0) fprintf(stderr, "%s\n%s", err.what(), program.help().str().c_str());
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
 catch (const std::exception& err)
 {
   if (_workerId== 0) fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", _scriptFile.c_str(), err.what());
   exit(-1);
 }

 // Profiling information
 _searchTotalTime = 0.0;
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
 if(const char* hashDBMaxMBytesEnv = std::getenv("JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB"))
  hashDBMaxMBytes = std::stol(hashDBMaxMBytesEnv);
 else
  if (_workerId == 0) printf("[Jaffar] Warning: JAFFAR_MAX_WORKER_HASH_DATABASE_SIZE_MB environment variable not defined. Using conservative default...\n");

 _hashDatabaseSizeThreshold = floor(((double)hashDBMaxMBytes * 1024.0 * 1024.0) / ((double)HASH_DATABASE_COUNT * (double)sizeof(uint64_t)));
 if (_workerId == 0) printf("[Jaffar] Using %d hash databases of %lu entries each per worker.\n", HASH_DATABASE_COUNT, _hashDatabaseSizeThreshold);

 // Parsing max frame DB entries
 size_t frameDBMaxMBytes = 300;
 if(const char* frameDBMaxMBytesEnv = std::getenv("JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB"))
  frameDBMaxMBytes = std::stol(frameDBMaxMBytesEnv);
 else
  if (_workerId == 0) printf("[Jaffar] Warning: JAFFAR_MAX_WORKER_FRAME_DATABASE_SIZE_MB environment variable not defined. Using conservative default...\n");

 // Twice the size of frames to allow for communication
 _maxLocalDatabaseSize = floor(((double)frameDBMaxMBytes * 1024.0 * 1024.0) / ((double)Frame::getSerializationSize() * 2.0));
 if (_workerId == 0) printf("[Jaffar] Using a frame database of %lu entries each per worker.\n", _maxLocalDatabaseSize);

 // Initializing SDLPop
 _sdlPop = new SDLPopInstance;

 // Dont show GUI
 _sdlPop->initialize(false);

 // Initializing State Handler
 _state = new State(_sdlPop, _scriptJs);

 // Setting magnet default values. This default repels unspecified rooms.
 std::vector<Magnet> magnets(_VISIBLE_ROOM_COUNT);

 for (size_t i = 0; i < _VISIBLE_ROOM_COUNT; i++)
 {
  magnets[i].intensityY = 0.0f;
  magnets[i].intensityX = 0.0f;
  magnets[i].positionX = 0.0f;
 }

 // Processing user-specified rules
 if (isDefined(_scriptJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Train configuration file missing 'Rules' key.\n");
 for (size_t i = 0; i < _scriptJs["Rules"].size(); i++)
  _rules.push_back(new Rule(_scriptJs["Rules"][i], _sdlPop));

 // Setting global for rule count
 _ruleCount = _rules.size();

 // Setting initial status for each rule
 std::vector<status_t> rulesStatus(_ruleCount);
 for (size_t i = 0; i < _ruleCount; i++) rulesStatus[i] = st_active;

 _frameSerializedSize = Frame::getSerializationSize();
 MPI_Type_contiguous(_frameSerializedSize, MPI_BYTE, &_mpiFrameType);
 MPI_Type_commit(&_mpiFrameType);

 // Setting initial values
 _hasFinalized = false;
 _globalHashCollisions = 0;

 // Storing base frame data to be used by all frames
 _baseStateData = _state->saveBase();

 // Setting win status
 _winFrameFound = false;
 _localWinFound = false;

 // Creating initial frame on the root rank
 if (_workerId == 0)
 {
  // Computing initial hash
  const auto hash = _state->computeHash();

  auto initialFrame = std::make_unique<Frame>();
  initialFrame->moveHistory[0] = 0;
  initialFrame->frameStateData = _state->saveFrame();
  initialFrame->magnets = magnets;
  initialFrame->rulesStatus = rulesStatus;

  // Evaluating Rules on initial frame
  evaluateRules(*initialFrame);

  // Storing which rules are win rules
  for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   for (size_t actionId = 0; actionId < _rules[ruleId]->_actions.size(); actionId++)
    if (_rules[ruleId]->_actions[actionId]["Type"] == "Trigger Win")
     _winRulePositions.push_back(ruleId);

  if (_winRulePositions.size() == 0)
   EXIT_WITH_ERROR("[ERROR] No win (Trigger Win) rules were defined in the config file.\n");

  // Evaluating Score on initial frame
  initialFrame->score = getFrameScore(*initialFrame);

  // Registering hash for initial frame
  _hashDatabases[0].insert(hash);

  // Copying initial frame into the best frame
  _bestFrame = *initialFrame;

  // Adding frame to the current database
  _currentFrameDB.push_back(std::move(initialFrame));
 }

 // Starting global frame counter
 _globalFrameCounter = 1;
}

int main(int argc, char* argv[])
{
 Train train(argc, argv);

 // Running Search
 train.run();

 return MPI_Finalize();
}
