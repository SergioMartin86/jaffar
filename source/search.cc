#include "search.h"
#include "utils.h"
#include "jaffar.h"
#include <chrono>

size_t _ruleCount;
const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

void Search::run()
{
 auto searchTimeBegin = std::chrono::steady_clock::now(); // Profiling

 for(_currentFrame = 1; _currentFrame <= _maxFrames; _currentFrame++)
 {
  // If this is the root rank, plot the best frame and print information
  if (_jaffarConfig.mpiRank == 0)
  {
   // Plotting current best frame
   auto bestFrame = (*_currentFrameDB)[0];
   _state->loadBase(_baseStateData);
   _state->loadFrame(bestFrame->frameStateData);
   _sdlPop->refreshEngine();
   _sdlPop->_currentMove = bestFrame->move;

   // Drawing frame
   _sdlPop->draw();

   // Profiling information
   auto searchTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _searchTotalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(searchTimeEnd - searchTimeBegin).count();    // Profiling

   // Printing search status
   printSearchStatus();

   // Check if search came to an early end
   if (_currentFrameDB->size() == 0)
   {
    printf("[Jaffar] Frame database depleted with no winning frames, finishing...\n");
    exit(0);
   }
  }

  // All workers: running a single frame search
  runFrame();
 }

 // Barrier to wait for all workers
 MPI_Barrier(MPI_COMM_WORLD);
}

void Search::runFrame()
{
 // Resetting timers for computation profiling
 _frameStateLoadTime = 0.0;
 _frameAdvanceTime = 0.0;
 _frameHashComputationTime = 0.0;
 _frameHashInsertionTime = 0.0;
 _frameRuleEvaluationTime = 0.0;
 _frameDatabaseUpdateTime = 0.0;
 _frameCreationTime = 0.0;

 // Resetting timers for communication profiling
 _commHashBroadcastNewEntryCountTime = 0.0;
 _commHashBufferingTime = 0.0;
 _commHashBroadcastTime = 0.0;

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Communication] New hashes are distributed to the workers from the engine
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 auto communicationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Broadcasting new hash entry count
 auto hashBroadcastNewEntryCountTimeBegin = std::chrono::steady_clock::now(); // Profiling

 size_t newHashEntryCount = _newHashes.size();
 MPI_Bcast(&newHashEntryCount, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
 std::vector<uint64_t> newHashVector;
 newHashVector.resize(newHashEntryCount);

 auto hashBroadcastNewEntryCountTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _commHashBroadcastNewEntryCountTime += std::chrono::duration_cast<std::chrono::nanoseconds>(hashBroadcastNewEntryCountTimeEnd - hashBroadcastNewEntryCountTimeBegin).count();    // Profiling

 // Buffering new hash table entries for broadcasting
 if (_jaffarConfig.mpiRank == 0)
 {
  auto hashBufferingTimeBegin = std::chrono::steady_clock::now(); // Profiling

  size_t currentEntry = 0;
  for (auto hashIterator = _newHashes.begin(); hashIterator != _newHashes.end(); hashIterator++)
   newHashVector[currentEntry++] = *hashIterator;

  auto hashBufferingTimeEnd = std::chrono::steady_clock::now(); // Profiling
  _commHashBufferingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(hashBufferingTimeEnd - hashBufferingTimeBegin).count();    // Profiling
 }

 // Broadcasting new hash entries
 auto hashBroadcastingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 MPI_Bcast(newHashVector.data(), newHashEntryCount, MPI_UINT64_T, 0, MPI_COMM_WORLD);

 auto hashBroadcastingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _commHashBroadcastTime += std::chrono::duration_cast<std::chrono::nanoseconds>(hashBroadcastingTimeEnd - hashBroadcastingTimeBegin).count();    // Profiling

 // Having non-root workers update their hash tables
 if (_jaffarConfig.mpiRank != 0)
  for (size_t i = 0; i < newHashVector.size(); i++)
   _hashes.insert(newHashVector[i]);

 //printf("MPI Rank %d, New Hash Entries: %lu, Hash table size: %lu\n", _jaffarConfig.mpiRank, newHashVector.size(), _hashes.size());

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Communication] Base Frames are split into equally sized chunks for the workers to process
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 // Serializing current frame database
 auto frameDatabaseSerializationBegin = std::chrono::steady_clock::now(); // Profiling

 // Getting frame count in the current database
 size_t frameCount = _currentFrameDB->size();
 MPI_Bcast(&frameCount, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

 // Serializing all database frames into a single buffer
 size_t currentPosition = 0;
 char* frameSendBuffer = NULL;

 if (_jaffarConfig.mpiRank == 0)
 {
  frameSendBuffer = (char*) malloc(_frameSerializedSize * frameCount);

  for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  {
   (*_currentFrameDB)[frameId]->serialize(&frameSendBuffer[currentPosition]);
   currentPosition += _frameSerializedSize;
  }
 }

 auto frameDatabaseSerializationEnd = std::chrono::steady_clock::now(); // Profiling
 _commDatabaseSerializationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameDatabaseSerializationEnd - frameDatabaseSerializationBegin).count();    // Profiling

 // Figuring out how many frmes to give each worker
 auto frameScatterBegin = std::chrono::steady_clock::now(); // Profiling

 size_t framesPerWorker = frameCount / _jaffarConfig.mpiSize;

 // Figuring out work distribution
 currentPosition = 0;
 std::vector<int> startPositions(_jaffarConfig.mpiSize);
 std::vector<int> frameCounts(_jaffarConfig.mpiSize);
 for (int i = 0; i < _jaffarConfig.mpiSize; i++)
 {
  startPositions[i] = currentPosition;
  frameCounts[i] = framesPerWorker;
  currentPosition += framesPerWorker;
 }

 // The last worker gets the remainder
 frameCounts[_jaffarConfig.mpiSize-1] = frameCount - startPositions[_jaffarConfig.mpiSize-1];

 // Allocating receive buffer for incoming frames
 int rankFrameCount = frameCounts[_jaffarConfig.mpiRank];
 char* frameReceiveBuffer = (char*) malloc(_frameSerializedSize * rankFrameCount);

 // Scattering frames among the workers
 MPI_Scatterv(frameSendBuffer, frameCounts.data(), startPositions.data(), _mpiFrameType, frameReceiveBuffer, rankFrameCount, _mpiFrameType, 0, MPI_COMM_WORLD);

 auto frameScatterEnd = std::chrono::steady_clock::now(); // Profiling
 _commFrameScatterTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameScatterEnd - frameScatterBegin).count();    // Profiling

 // Freeing memory for current DB frames
 auto databaseClearTimeBegin = std::chrono::steady_clock::now(); // Profiling
 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  delete (*_currentFrameDB)[frameId];

 // Clearing current frame DB
 _currentFrameDB->clear();

 auto databaseClearTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameDatabaseClearTime = std::chrono::duration_cast<std::chrono::nanoseconds>(databaseClearTimeEnd - databaseClearTimeBegin).count();    // Profiling

 // Adding each worker's frames into the frame database
 auto frameDatabaseDeserializationBegin = std::chrono::steady_clock::now(); // Profiling

 currentPosition = 0;
 for (int frameId = 0; frameId < rankFrameCount; frameId++)
 {
  Frame* newFrame = new Frame;
  newFrame->deserialize(&frameReceiveBuffer[currentPosition]);
  _currentFrameDB->push_back(newFrame);
  currentPosition += _frameSerializedSize;
 }

 // Freeing frame buffers
 if (_jaffarConfig.mpiRank == 0) free(frameSendBuffer);
 free(frameReceiveBuffer);

 auto frameDatabaseDeserializationEnd = std::chrono::steady_clock::now(); // Profiling
 _commDatabaseDeserializationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameDatabaseDeserializationEnd - frameDatabaseDeserializationBegin).count();    // Profiling

 auto communicationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameCommunicationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(communicationTimeEnd - communicationTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Computation - Parallel] Each worker processes its own unique base frames
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Hash collision counter
 size_t hashCollisions = 0;

 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  for (size_t moveId = 0; moveId < _possibleMoves.size(); moveId++)
  {
   auto stateLoadTimeBegin = std::chrono::steady_clock::now(); // Profiling
   const std::string move = _possibleMoves[moveId].c_str();

   // Getting base frame pointer
   Frame* baseFrame = (*_currentFrameDB)[frameId];

   // Loading frame state
   _state->loadBase(_baseStateData);
   _state->loadFrame(baseFrame->frameStateData);
   _sdlPop->refreshEngine();
   auto stateLoadTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameStateLoadTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stateLoadTimeEnd - stateLoadTimeBegin).count();    // Profiling

   // Perform the selected move
   auto advanceFrameTimeBegin = std::chrono::steady_clock::now(); // Profiling
   _sdlPop->performMove(move);

   // Advance a single frame
   _sdlPop->advanceFrame();
   auto advanceFrameTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameAdvanceTime += std::chrono::duration_cast<std::chrono::nanoseconds>(advanceFrameTimeEnd - advanceFrameTimeBegin).count();    // Profiling

   // Compute hash value
   auto hashComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling
   const auto hash = _state->computeHash();
   auto hashComputationTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameHashComputationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(hashComputationTimeEnd - hashComputationTimeBegin).count();    // Profiling

   // Adding hash to the collection, and check whether it already existed
   auto hashInsertionTimeBegin = std::chrono::steady_clock::now(); // Profiling
   bool collisionDetected = !_hashes.insert(hash).second;
   auto hashInsertionTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameHashInsertionTime += std::chrono::duration_cast<std::chrono::nanoseconds>(hashInsertionTimeEnd - hashInsertionTimeBegin).count();    // Profiling

   // If collision detected, discard this frame
   if (collisionDetected)
   {
    hashCollisions++;
    continue;
   }

   // Creating new frame, mixing base frame information and the current sdlpop state
   auto newFrameCreationTimeBegin = std::chrono::steady_clock::now(); // Profiling
   Frame* newFrame = new Frame;
   newFrame->isFail = false;
   newFrame->isWin = false;
   newFrame->move = move;
   newFrame->hash = hash;
   newFrame->rulesStatus = baseFrame->rulesStatus;
   newFrame->frameStateData = _state->saveFrame();
   newFrame->magnets = baseFrame->magnets;
   auto newFrameCreationTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameCreationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(newFrameCreationTimeEnd - newFrameCreationTimeBegin).count();    // Profiling

   // Evaluating rules on the new frame
   auto ruleEvaluationTimeBegin = std::chrono::steady_clock::now(); // Profiling
   evaluateRules(newFrame);
   auto ruleEvaluationEnd = std::chrono::steady_clock::now(); // Profiling
   _frameRuleEvaluationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(ruleEvaluationEnd - ruleEvaluationTimeBegin).count();    // Profiling

   // If frame has failed, then proceed to the next one
   if (newFrame->isFail == true) continue;

   // Calculating score
   auto scoreEvaluationTimeBegin = std::chrono::steady_clock::now(); // Profiling
   newFrame->score =  getFrameScore(newFrame);
   auto scoreEvaluationTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameScoreEvaluationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(scoreEvaluationTimeEnd - scoreEvaluationTimeBegin).count();    // Profiling

   // Adding novel frame in the next frame database
   auto databaseUpdateTimeBegin = std::chrono::steady_clock::now(); // Profiling
   _nextFrameDB->push_back(newFrame);
   auto databaseUpdateTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameDatabaseUpdateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(databaseUpdateTimeEnd - databaseUpdateTimeBegin).count();    // Profiling

   //printf("New Frame - Action: %s - Hash: 0x%lX - Score: %f - NextDBSize: %lu\n", move.c_str(), hash, newFrame->score, _nextFrameDB->size());

   // If frame has succeeded, then exit
   if (newFrame->isWin == true)
   {
    printf("Success Frame found!\n");
    exit(0);
   }
 }

 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Communication] Gathering new frames and information from workers
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 communicationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 auto commGatherWorkerInformationBegin = std::chrono::steady_clock::now(); // Profiling

 // Gathering total new hash collision counter
 size_t newHashCollisions;
 MPI_Reduce(&hashCollisions, &newHashCollisions, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
 _hashCollisions += newHashCollisions;

 // Gathering total new frames per worker
 std::vector<int> newFrameCounts(_jaffarConfig.mpiSize);
 int newFrameCounter = _nextFrameDB->size();

 int allNewFrameCounter;
 MPI_Reduce(&newFrameCounter, &allNewFrameCounter, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
 MPI_Gather(&newFrameCounter, 1, MPI_INT, newFrameCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

 auto commGatherWorkerInformationEnd = std::chrono::steady_clock::now(); // Profiling
 _commGatherWorkerInformationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(commGatherWorkerInformationEnd - commGatherWorkerInformationBegin).count();    // Profiling

 // Serializing new frames
 auto commWorkerFrameSerializationBegin = std::chrono::steady_clock::now(); // Profiling

 char* newFrameSendBuffer = (char*) malloc(_frameSerializedSize * newFrameCounter);
 currentPosition = 0;
 for (int frameId = 0; frameId < newFrameCounter; frameId++)
 {
  (*_nextFrameDB)[frameId]->serialize(&newFrameSendBuffer[currentPosition]);
  currentPosition += _frameSerializedSize;
 }

 auto commWorkerFrameSerializationEnd = std::chrono::steady_clock::now(); // Profiling
 _commWorkerFrameSerializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(commWorkerFrameSerializationEnd - commWorkerFrameSerializationBegin).count();    // Profiling

 // Starting gathering
 auto commGatherTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Allocating buffer to receieve all new experiences
 char* allNewFrameRootBuffer = NULL;
 if (_jaffarConfig.mpiRank == 0)
   allNewFrameRootBuffer = (char*) malloc(allNewFrameCounter * _frameSerializedSize);

 // Calculating gather displacements
 std::vector<int> newFrameDisplacements(_jaffarConfig.mpiSize);
 size_t currentDisplacement = 0;
 for (int i = 0; i < _jaffarConfig.mpiSize; i++)
 {
  newFrameDisplacements[i] = currentDisplacement;
  currentDisplacement += newFrameCounts[i];
 }

 // Gathering new frames into the root rank
 MPI_Gatherv(newFrameSendBuffer, newFrameCounter, _mpiFrameType, allNewFrameRootBuffer, newFrameCounts.data(), newFrameDisplacements.data(), _mpiFrameType, 0, MPI_COMM_WORLD);

 auto commGatherTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _commFrameGatherTime = std::chrono::duration_cast<std::chrono::nanoseconds>(commGatherTimeEnd - commGatherTimeBegin).count();    // Profiling

 communicationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameCommunicationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(communicationTimeEnd - communicationTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Computation - Sequential] Postprocessing: Hashing new frames. Then sorting and clipping database.
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 auto postProcessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Clearing new hash table
 _newHashes.clear();

 // Freeing memory for current DB frames again
 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  delete (*_currentFrameDB)[frameId];

 // Clearing current frame DB again
 _currentFrameDB->clear();

 // Adding new frames into the new current database
 if (_jaffarConfig.mpiRank == 0)
 {
  currentPosition = 0;
  for (int frameId = 0; frameId < allNewFrameCounter; frameId++)
  {
   Frame* newFrame = new Frame;
   newFrame->deserialize(&allNewFrameRootBuffer[currentPosition]);
   currentPosition += _frameSerializedSize;

   const uint64_t hash = newFrame->hash;
   bool collisionDetected = !_hashes.insert(hash).second;

   // Adding frames as long as they are new. For frames from rank 0, we know they were new and we force them in to avoid false collisions
   if (collisionDetected == false || frameId < newFrameCounts[0])
   {
    _currentFrameDB->push_back(newFrame);
    _newHashes.insert(hash);
   }
   else
   {
    _hashCollisions++;
   }
  }
 }

 // Clearing next frame database
 for (size_t frameId = 0; frameId < _nextFrameDB->size(); frameId++)
  delete (*_nextFrameDB)[frameId];

 // Clearing next frame DB
 _nextFrameDB->clear();

 // Sorting DB frames by score
 auto databaseSortTimeBegin = std::chrono::steady_clock::now(); // Profiling
 std::sort(_currentFrameDB->begin(), _currentFrameDB->end(), [](const auto& a, const auto& b) { return a->score > b->score; });
 auto databaseSortTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameDatabaseSortTime = std::chrono::duration_cast<std::chrono::nanoseconds>(databaseSortTimeEnd - databaseSortTimeBegin).count();    // Profiling

 // Clipping excessive frames out
 auto databaseClippingTimeBegin = std::chrono::steady_clock::now(); // Profiling
 for (size_t frameId = _maxDatabaseSize; frameId < _currentFrameDB->size(); frameId++) delete (*_currentFrameDB)[frameId];
 if (_currentFrameDB->size() > _maxDatabaseSize) _currentFrameDB->resize(_maxDatabaseSize);
 auto databaseClippingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameDatabaseClippingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(databaseClippingTimeEnd - databaseClippingTimeBegin).count();    // Profiling

 // Freeing buffers
 free(newFrameSendBuffer);
 if (_jaffarConfig.mpiRank == 0) free(allNewFrameRootBuffer);

 auto postProcessingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(postProcessingTimeEnd - postProcessingTimeBegin).count();    // Profiling
 _frameComputationTime += _framePostprocessingTime;
}

void Search::evaluateRules(Frame* frame)
{
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
 {
  // Evaluate rule only if it's active
  if (frame->rulesStatus[ruleId] == st_active)
  {
   bool isAchieved = _rules[ruleId]->evaluate();

   // If it's achieved, update its status and run its actions
   if (isAchieved)
   {
    // Setting status to achieved
    frame->rulesStatus[ruleId] = st_achieved;

    // Perform actions
    for (size_t actionId = 0; actionId < _rules[ruleId]->_actions.size(); actionId++)
    {
     const auto actionJs = _rules[ruleId]->_actions[actionId];

     // Getting action type
     if (isDefined(actionJs, "Type") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Type' key.\n", ruleId, actionId);
     std::string actionType = actionJs["Type"].get<std::string>();

     // Running the action, depending on the type
     bool recognizedActionType = false;

     if (actionType == "Trigger Fail")
     {
      frame->isFail = true;
      recognizedActionType = true;
     }

     if (actionType == "Trigger Win")
     {
      frame->isWin = true;
      recognizedActionType = true;
     }

     if (actionType == "Set Magnet Intensity X")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      int room = actionJs["Room"].get<int>();

      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensity = actionJs["Value"].get<float>();

      frame->magnets[room].intensityX = intensity;
      recognizedActionType = true;
     }

     if (actionType == "Set Magnet Intensity Y")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      int room = actionJs["Room"].get<int>();

      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensity = actionJs["Value"].get<float>();

      frame->magnets[room].intensityY = intensity;
      recognizedActionType = true;
     }

     if (actionType == "Set Magnet Position X")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      int room = actionJs["Room"].get<int>();

      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float position = actionJs["Value"].get<float>();

      frame->magnets[room].positionX = position;
      recognizedActionType = true;
     }

     if (actionType == "Set Magnet Position Y")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      int room = actionJs["Room"].get<int>();

      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float position = actionJs["Value"].get<float>();

      frame->magnets[room].positionY = position;
      recognizedActionType = true;
     }

     if (recognizedActionType == false) EXIT_WITH_ERROR("[ERROR] Unrecognized action type %s\n", actionType);
    }
   }
  }
 }
}

Search::Search(SDLPopInstance *sdlPop, State *state, nlohmann::json& config)
{
 _sdlPop = sdlPop;
 _state = state;

 // Profiling information
 _searchTotalTime = 0.0;
 _frameComputationTime = 0.0;
 _frameCommunicationTime = 0.0;

 // Parsing profiling verbosity
 if (isDefined(config, "Show Profiling Information") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Show Profiling Information' key.\n");
 _showProfilingInformation = config["Show Profiling Information"].get<bool>();

 // Parsing debugging verbosity
 if (isDefined(config, "Show Debugging Information") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Show Debugging Information' key.\n");
 _showDebuggingInformation = config["Show Debugging Information"].get<bool>();

 // Parsing max frames from configuration file
 if (isDefined(config, "Max Frames") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Max Frames' key.\n");
 _maxFrames = config["Max Frames"].get<size_t>();

 // Parsing search width from configuration file
 if (isDefined(config, "Max Database Size") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Max Database Size' key.\n");
 _maxDatabaseSize = config["Max Database Size"].get<size_t>();

 // Setting magnet default values. This default repels unspecified rooms, but rewards 'glitchy' rooms
 std::vector<Magnet> magnets(256);

 magnets[0].intensityX = 0.1f;
 magnets[0].intensityY = 0.1f;
 magnets[0].positionX = 128.0f;
 magnets[0].positionY = 128.0f;

 for (size_t i = 1; i < 24; i++)
 {
  magnets[i].intensityX = -0.1f;
  magnets[i].intensityY = -0.1f;
  magnets[i].positionX = 128.0f;
  magnets[i].positionY = 128.0f;
 }

 for (size_t i = 25; i < 256; i++)
 {
   magnets[i].intensityX = +0.1f;
   magnets[i].intensityY = +0.1f;
   magnets[i].positionX = 128.0f;
   magnets[i].positionY = 128.0f;
 }

 // Processing user-specified magnets
 if (isDefined(config, "Magnets") == false) EXIT_WITH_ERROR("[ERROR] Search configuration file missing 'Magnets' key.\n");
 for (size_t i = 0; i < config["Magnets"].size(); i++)
 {
   auto magnet = config["Magnets"][i];

   if (isDefined(magnet, "Room") == false) EXIT_WITH_ERROR("[ERROR] Magnet %lu missing 'Room' key.\n", i);
   if (isDefined(magnet, "Intensity X") == false) EXIT_WITH_ERROR("[ERROR] Magnet %lu missing 'Intensity X' key.\n", i);
   if (isDefined(magnet, "Position X") == false) EXIT_WITH_ERROR("[ERROR] Magnet %lu missing 'Position X' key.\n", i);
   if (isDefined(magnet, "Intensity Y") == false) EXIT_WITH_ERROR("[ERROR] Magnet %lu missing 'Intensity Y' key.\n", i);
   if (isDefined(magnet, "Position Y") == false) EXIT_WITH_ERROR("[ERROR] Magnet %lu missing 'Position Y' key.\n", i);

   int room = magnet["Room"].get<int>();
   if (room > 255 || room < 0) EXIT_WITH_ERROR("[ERROR] Invalid room value %d for magnet %lu (must be between 0 and 255).\n", room, i);

   magnets[room].intensityX = magnet["Intensity X"].get<float>();
   magnets[room].intensityY = magnet["Intensity Y"].get<float>();
   magnets[room].positionX = magnet["Position X"].get<float>();
   magnets[room].positionY = magnet["Position Y"].get<float>();
 }

 // Processing user-specified rules
 if (isDefined(config, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Search configuration file missing 'Rules' key.\n");
 for (size_t i = 0; i < config["Rules"].size(); i++)
  _rules.push_back(new Rule(config["Rules"][i], _sdlPop));

 // Getting initial status for each rule
 std::vector<status_t> rulesStatus(config["Rules"].size());
 for (size_t i = 0; i < config["Rules"].size(); i++)
 {
  if (isDefined(config["Rules"][i], "Status") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu missing 'Status' key.\n", i);

  bool statusRecognized = false;
  if (config["Rules"][i]["Status"] == "Active") { rulesStatus[i] = st_active; statusRecognized = true; }
  if (config["Rules"][i]["Status"] == "Inactive") { rulesStatus[i] = st_inactive; statusRecognized = true; }
  if (config["Rules"][i]["Status"] == "Achieved") { rulesStatus[i] = st_achieved; statusRecognized = true; }

  if (statusRecognized == false) EXIT_WITH_ERROR("[ERROR] Rule %lu status %s not recognized.\n", i, config["Rules"][i]["Status"].get<std::string>().c_str());
 }

 // Setting global for rule count
 _ruleCount = _rules.size();

 // Calculating frame size and creating MPI datatype
 _frameSerializedSize = Frame::getSerializationSize();
 MPI_Type_contiguous(_frameSerializedSize, MPI_BYTE, &_mpiFrameType);
 MPI_Type_commit(&_mpiFrameType);

 // Allocating databases
 _currentFrameDB = new std::vector<Frame*>();
 _nextFrameDB = new std::vector<Frame*>();

 // Reserving space for frame data
 _currentFrameDB->reserve(_maxDatabaseSize * _possibleMoves.size());
 _nextFrameDB->reserve(_maxDatabaseSize * _possibleMoves.size());

 // Setting initial values
 _hasFinalized = false;
 _hashCollisions = 0;

 // Storing base frame data to be used by all frames
 _baseStateData = _state->saveBase();

 // Computing initial hash
 const auto hash = _state->computeHash();

 // Setting initial frame
 Frame* initialFrame = new Frame;
 initialFrame->move = ".";
 initialFrame->hash = hash;
 initialFrame->score = 0.0f; // TO-DO: Get score from scorer
 initialFrame->frameStateData = _state->saveFrame();
 initialFrame->magnets = magnets;
 initialFrame->rulesStatus = rulesStatus;
 initialFrame->isFail = false;
 initialFrame->isWin = false;
 _currentFrameDB->push_back(initialFrame);

 // Registering hash for initial frame
 _hashes.insert(hash);
 _newHashes.insert(hash);
}

Search::~Search()
{
 delete _currentFrameDB;
 delete _nextFrameDB;
}

void Search::printSearchStatus()
{
 auto bestFrame = (*_currentFrameDB)[0];

 printf("[Jaffar] ----------------------------------------------------------------\n");
 printf("[Jaffar] Total Elapsed Time: %3.3fs\n", _searchTotalTime / 1.0e+9);
 printf("[Jaffar] Current Frame: %lu/%lu\n", _currentFrame, _maxFrames);
 printf("[Jaffar] Best Score: %f\n", bestFrame->score);
 printf("[Jaffar] Hash Table Collisions %lu\n", _hashCollisions);
 printf("[Jaffar] Hash Table Entries (New/Total): %lu/%lu\n", _newHashes.size(), _hashes.size());
 printf("[Jaffar] Hash Table Size: %lu\n", _hashes.size());
 printf("[Jaffar] Database Size: %lu/%lu\n", _currentFrameDB->size(), _maxDatabaseSize);

 if (_showProfilingInformation)
 {
  printf("[Jaffar] Frame Computation Time: %3.3fs\n", _frameComputationTime / 1.0e+9);
  printf("[Jaffar]  + State Loading Time: %3.3fs\n", _frameStateLoadTime / 1.0e+9);
  printf("[Jaffar]  + Frame Advancing Time: %3.3fs\n", _frameAdvanceTime / 1.0e+9);
  printf("[Jaffar]  + Hash Computation Time: %3.3fs\n", _frameHashComputationTime / 1.0e+9);
  printf("[Jaffar]  + Hash Insertion Time: %3.3fs\n", _frameHashInsertionTime / 1.0e+9);
  printf("[Jaffar]  + Frame Creation Time: %3.3fs\n", _frameRuleEvaluationTime / 1.0e+9);
  printf("[Jaffar]  + Rule Evaluation Time: %3.3fs\n", _frameRuleEvaluationTime / 1.0e+9);
  printf("[Jaffar]  + Score Evaluation Time: %3.3fs\n", _frameScoreEvaluationTime / 1.0e+9);
  printf("[Jaffar]  + Database Update Time: %3.3fs\n", _frameDatabaseUpdateTime / 1.0e+9);
  printf("[Jaffar]  + Database Clear Time: %3.3fs\n", _frameDatabaseClearTime / 1.0e+9);
  printf("[Jaffar]  + Database Sort Time: %3.3fs\n", _frameDatabaseSortTime / 1.0e+9);
  printf("[Jaffar]  + Database Clipping Time: %3.3fs\n", _frameDatabaseClippingTime / 1.0e+9);
  printf("[Jaffar]  + Postprocessing Time: %3.3fs\n", _framePostprocessingTime / 1.0e+9);

  printf("[Jaffar] Communication Time: %3.3fs\n", _frameCommunicationTime / 1.0e+9);
  printf("[Jaffar]  + Hash Count Broadcasting Time: %3.3fs\n", _commHashBroadcastNewEntryCountTime / 1.0e+9);
  printf("[Jaffar]  + Hash Buffering Time: %3.3fs\n", _commHashBufferingTime / 1.0e+9);
  printf("[Jaffar]  + Hash Values Broadcasting Time: %3.3fs\n", _commHashBroadcastTime / 1.0e+9);
  printf("[Jaffar]  + Database Serialization Time: %3.3fs\n", _commDatabaseSerializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Scatter Time: %3.3fs\n", _commFrameScatterTime / 1.0e+9);
  printf("[Jaffar]  + Database Deserialization Time: %3.3fs\n", _commDatabaseDeserializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Get Worker Info Time: %3.3fs\n", _commGatherWorkerInformationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Worker Frame Serialization Time: %3.3fs\n", _commWorkerFrameSerializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Gather Time: %3.3fs\n", _commFrameGatherTime / 1.0e+9);
 }

 if (_showDebuggingInformation)
 {
  printf("[Jaffar] Best Frame Information:\n");
  _sdlPop->printFrameInfo();
 }
}

float Search::getFrameScore(const Frame* frame)
{
 // Accumulator for total score
 float score = 0.0f;

 // Obtaining magnet corresponding to kid's room
 int currentRoom = _sdlPop->Kid->room;
 const auto &magnet = frame->magnets[currentRoom];

 // Evaluating magnet's score on the X axis
 const float kidPosX = _sdlPop->Kid->x;
 const float magPosX = magnet.positionX;
 const float magIntensityX = magnet.intensityX;
 const float diffX = std::abs(kidPosX - magPosX);
 score += magIntensityX * (256.0f - diffX);

 // Evaluating magnet's score on the Y axis
 const float kidPosY = _sdlPop->Kid->y;
 const float magPosY = magnet.positionY;
 const float magIntensityY = magnet.intensityY;
 const float diffY = std::abs(kidPosY - magPosY);
 score += magIntensityY * (256.0f - diffY);

 // Now adding rule rewards
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  if (frame->rulesStatus[ruleId] == st_achieved)
   score += _rules[ruleId]->_reward;

 // Returning score
 return score;
}
