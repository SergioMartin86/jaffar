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
   _sdlPop->_currentMove = bestFrame->currentMove;

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
 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Communication] Base Frames are split into equally sized chunks for the workers to process
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 auto communicationTimeBegin = std::chrono::steady_clock::now(); // Profiling

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
  // Allocating send buffer
  frameSendBuffer = (char*) malloc(_frameSerializedSize * frameCount);

  for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  {
   // Setting base frame id to relate new frames to their base frames
   (*_currentFrameDB)[frameId]->frameId = frameId;

   // Serializing frame into the buffer
   (*_currentFrameDB)[frameId]->serialize(&frameSendBuffer[currentPosition]);

   // Advancing the buffer position
   currentPosition += _frameSerializedSize;
  }
 }

 auto frameDatabaseSerializationEnd = std::chrono::steady_clock::now(); // Profiling
 _commDatabaseSerializationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameDatabaseSerializationEnd - frameDatabaseSerializationBegin).count();    // Profiling

 // Figuring out how many frmes to give each worker
 auto frameScatterBegin = std::chrono::steady_clock::now(); // Profiling

 // Figuring out work distribution
 std::vector<int> startPositions(_jaffarConfig.mpiSize);
 std::vector<int> frameCounts = splitVector((int)frameCount, _jaffarConfig.mpiSize);

 currentPosition = 0;
 for (int i = 0; i < _jaffarConfig.mpiSize; i++)
 {
  startPositions[i] = currentPosition;
  currentPosition += frameCounts[i];
 }

 // Allocating receive buffer for incoming frames
 int rankFrameCount = frameCounts[_jaffarConfig.mpiRank];
 char* frameReceiveBuffer = (char*) malloc(_frameSerializedSize * rankFrameCount);

 // Scattering frames among the workers
 MPI_Scatterv(frameSendBuffer, frameCounts.data(), startPositions.data(), _mpiFrameType, frameReceiveBuffer, rankFrameCount, _mpiFrameType, 0, MPI_COMM_WORLD);

 auto frameScatterEnd = std::chrono::steady_clock::now(); // Profiling
 _commFrameScatterTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameScatterEnd - frameScatterBegin).count();    // Profiling

 // Adding each worker's frames into the frame database
 auto frameDatabaseDeserializationBegin = std::chrono::steady_clock::now(); // Profiling

 // Storage for the incoming frames
 std::vector<Frame*> workerFrames;

 // Deserializing incoming frames into the storage
 currentPosition = 0;
 for (int frameId = 0; frameId < rankFrameCount; frameId++)
 {
  Frame* newFrame = new Frame;
  newFrame->deserialize(&frameReceiveBuffer[currentPosition]);
  workerFrames.push_back(newFrame);
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

 // Resetting timers for computation profiling
 _frameStateLoadTime = 0.0;
 _frameAdvanceTime = 0.0;
 _frameHashComputationTime = 0.0;
 _frameHashInsertionTime = 0.0;
 _frameRuleEvaluationTime = 0.0;
 _frameDatabaseUpdateTime = 0.0;
 _frameCreationTime = 0.0;

 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Storage for newly produced frames by workers
 std::vector<Frame*> newWorkerFrames;

 for (size_t frameId = 0; frameId < workerFrames.size(); frameId++)
  for (size_t moveId = 0; moveId < _possibleMoves.size(); moveId++)
  {
   auto stateLoadTimeBegin = std::chrono::steady_clock::now(); // Profiling
   const std::string move = _possibleMoves[moveId].c_str();

   // Getting base frame pointer
   Frame* baseFrame = workerFrames[frameId];

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

   // If collision detected locally, discard this frame
   if (collisionDetected) continue;

   // Creating new frame, mixing base frame information and the current sdlpop state
   auto newFrameCreationTimeBegin = std::chrono::steady_clock::now(); // Profiling
   Frame* newFrame = new Frame;
   newFrame->frameId = baseFrame->frameId;
   newFrame->isFail = false;
   newFrame->isWin = false;
   newFrame->currentMove = move;
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
   newWorkerFrames.push_back(newFrame);
   auto databaseUpdateTimeEnd = std::chrono::steady_clock::now(); // Profiling
   _frameDatabaseUpdateTime += std::chrono::duration_cast<std::chrono::nanoseconds>(databaseUpdateTimeEnd - databaseUpdateTimeBegin).count();    // Profiling

   // If frame has succeeded, then exit
   if (newFrame->isWin == true)
   {
    printf("Success Frame found!\n");
    exit(0);
   }
 }

 // Freeing memory for worker frames
 auto databaseClearTimeBegin = std::chrono::steady_clock::now(); // Profiling
 for (size_t frameId = 0; frameId < workerFrames.size(); frameId++)
  delete workerFrames[frameId];

 // Clearing worker frames vector
 workerFrames.clear();

 auto databaseClearTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameDatabaseClearTime = std::chrono::duration_cast<std::chrono::nanoseconds>(databaseClearTimeEnd - databaseClearTimeBegin).count();    // Profiling

 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Communication] Gathering new frames and information from workers
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 communicationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 auto commGatherWorkerInformationBegin = std::chrono::steady_clock::now(); // Profiling

 // Gathering total new frames per worker
 std::vector<int> newFrameCounts(_jaffarConfig.mpiSize);
 size_t newFrameCounter = newWorkerFrames.size();

 size_t allNewFrameCounter;
 MPI_Reduce(&newFrameCounter, &allNewFrameCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
 MPI_Gather(&newFrameCounter, 1, MPI_INT, newFrameCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

 auto commGatherWorkerInformationEnd = std::chrono::steady_clock::now(); // Profiling
 _commGatherWorkerInformationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(commGatherWorkerInformationEnd - commGatherWorkerInformationBegin).count();    // Profiling

 // Serializing new frames
 auto commWorkerFrameSerializationBegin = std::chrono::steady_clock::now(); // Profiling

 char* newFrameSendBuffer = (char*) malloc(_frameSerializedSize * newFrameCounter);
 currentPosition = 0;
 for (size_t frameId = 0; frameId < newFrameCounter; frameId++)
 {
  newWorkerFrames[frameId]->serialize(&newFrameSendBuffer[currentPosition]);
  currentPosition += _frameSerializedSize;
 }

 auto commWorkerFrameSerializationEnd = std::chrono::steady_clock::now(); // Profiling
 _commWorkerFrameSerializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(commWorkerFrameSerializationEnd - commWorkerFrameSerializationBegin).count();    // Profiling

 // Starting gathering
 auto commGatherTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Allocating buffer to receieve all new experiences
 char* allNewFrameRootBuffer = NULL;
 size_t bufferSize = allNewFrameCounter * _frameSerializedSize;
 if (_jaffarConfig.mpiRank == 0) printf("bufferSize: %lu\n", bufferSize);

 if (_jaffarConfig.mpiRank == 0)
   allNewFrameRootBuffer = (char*) malloc(bufferSize);

 // Calculating gather displacements
 std::vector<int> newFrameDisplacements(_jaffarConfig.mpiSize);
 size_t currentDisplacement = 0;
 for (int i = 0; i < _jaffarConfig.mpiSize; i++)
 {
  newFrameDisplacements[i] = currentDisplacement;
  currentDisplacement += newFrameCounts[i];
 }

 // Gathering new frames into the root rank
 if (_jaffarConfig.mpiRank == 0)
 {
  printf("Receiving Frames: %lu\n", allNewFrameCounter);
  printf("Receive Pointer: 0x%lX\n", (unsigned long int)allNewFrameRootBuffer);
 }
 MPI_Gatherv(newFrameSendBuffer, newFrameCounter, _mpiFrameType, allNewFrameRootBuffer, newFrameCounts.data(), newFrameDisplacements.data(), _mpiFrameType, 0, MPI_COMM_WORLD);

 auto commGatherTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _commFrameGatherTime = std::chrono::duration_cast<std::chrono::nanoseconds>(commGatherTimeEnd - commGatherTimeBegin).count();    // Profiling

 communicationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameCommunicationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(communicationTimeEnd - communicationTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Computation - Sequential] Postprocessing: Hashing new frames. Then sorting and clipping database.
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 auto postProcessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Adding new frames into the new current database
 if (_jaffarConfig.mpiRank == 0)
 {
  currentPosition = 0;
  for (size_t frameId = 0; frameId < allNewFrameCounter; frameId++)
  {
   Frame* newFrame = new Frame;
   newFrame->deserialize(&allNewFrameRootBuffer[currentPosition]);
   currentPosition += _frameSerializedSize;

   const uint64_t hash = newFrame->hash;
   bool collisionDetected = !_hashes.insert(hash).second;

   // Adding frames as long as they are new. For frames from rank 0, we know they were new and we force them in to avoid false collisions
   if (collisionDetected == false || frameId < (size_t)newFrameCounts[0])
   {
    // Getting base frame id from the incoming data
    const size_t baseFrameId = newFrame->frameId;

    // Getting pointer to base frame
    const Frame* baseFrame = (*_currentFrameDB)[baseFrameId];

    // Getting id for new frame from its DB position
    const size_t frameId = _currentFrameDB->size();

    // Now setting new frame Id
    newFrame->frameId = frameId;

    // Getting corresponding base frame move history
    const std::string baseMoveHistory = baseFrame->moveHistory;

    // Appending new move to the base path
    const std::string newMoveHistory = baseMoveHistory + std::string(" ") + newFrame->currentMove;

    // Putting new path into the database
    newFrame->moveHistory = newMoveHistory;

    // Adding to next database
    _nextFrameDB->push_back(newFrame);
   }
   else
   {
    _hashCollisions++;
   }
  }
 }

 // Swapping path databases
 std::swap(_nextFrameDB, _currentFrameDB);

 // Clearing previous next frame database
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

     if (actionType == "Add Reward")
     {
      // This action is handled during startup, no need to do anything now.
      recognizedActionType = true;
     }

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

     // Parsing room here to avoid duplicating code per each rule
     int room = 0;
     if (isDefined(actionJs, "Room")) room = actionJs["Room"].get<int>();
     if (room > _VISIBLE_ROOM_COUNT) EXIT_WITH_ERROR("[ERROR] Rule %lu, Room %lu is outside visible room scope.\n", ruleId, room);

     if (actionType == "Set Horizontal Magnet Intensity")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensity = actionJs["Value"].get<float>();

      frame->magnets[room].intensity = intensity;
      recognizedActionType = true;
     }

     if (actionType == "Set Horizontal Magnet Position")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float position = actionJs["Value"].get<float>();

      frame->magnets[room].position = position;
      recognizedActionType = true;
     }

     if (recognizedActionType == false) EXIT_WITH_ERROR("[ERROR] Unrecognized action type %s\n", actionType.c_str());
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

 // Setting magnet default values. This default repels unspecified rooms.
 std::vector<Magnet> magnets(_VISIBLE_ROOM_COUNT);

 for (size_t i = 0; i < _VISIBLE_ROOM_COUNT; i++)
 {
  magnets[i].intensity = -1.0f;
  magnets[i].position = 128.0f;
 }

 // Creating frame databases
 _currentFrameDB = new std::vector<Frame*>;
 _nextFrameDB = new std::vector<Frame*>;

 // Processing user-specified rules
 if (isDefined(config, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Search configuration file missing 'Rules' key.\n");
 for (size_t i = 0; i < config["Rules"].size(); i++)
  _rules.push_back(new Rule(config["Rules"][i], _sdlPop));

 // Setting global for rule count
 _ruleCount = _rules.size();

 // Getting initial status for each rule
 std::vector<status_t> rulesStatus(_ruleCount);
 for (size_t i = 0; i < _ruleCount; i++) rulesStatus[i] = st_active;

 // Calculating frame size and creating MPI datatype
 _frameSerializedSize = Frame::getSerializationSize();
 MPI_Type_contiguous(_frameSerializedSize, MPI_BYTE, &_mpiFrameType);
 MPI_Type_commit(&_mpiFrameType);

 // Setting initial values
 _hasFinalized = false;
 _hashCollisions = 0;

 // Storing base frame data to be used by all frames
 _baseStateData = _state->saveBase();

 // Computing initial hash
 const auto hash = _state->computeHash();

 // Setting initial frame
 Frame* initialFrame = new Frame;
 initialFrame->currentMove = ".";
 initialFrame->moveHistory = ".";
 initialFrame->hash = hash;
 initialFrame->frameId = 0;
 initialFrame->frameStateData = _state->saveFrame();
 initialFrame->magnets = magnets;
 initialFrame->rulesStatus = rulesStatus;
 initialFrame->isFail = false;
 initialFrame->isWin = false;

 // Evaluating Rules on initial frame
 evaluateRules(initialFrame);

 // Evaluating Score on initial frame
 initialFrame->score = getFrameScore(initialFrame);

 // Adding frame to the current database
 _currentFrameDB->push_back(initialFrame);

 // Registering hash for initial frame
 _hashes.insert(hash);
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
 printf("[Jaffar] Current Frame: %lu/%lu\n", _currentFrame, _maxFrames);
 printf("[Jaffar] Best Score: %f\n", bestFrame->score);
 printf("[Jaffar] Database Size: %lu/%lu\n", _currentFrameDB->size(), _maxDatabaseSize);
 printf("[Jaffar] Elapsed Time (Frame/Total): %3.3fs / %3.3fs\n", (_frameComputationTime + _frameCommunicationTime) / 1.0e+9, _searchTotalTime / 1.0e+9);

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
  printf("[Jaffar]  + Database Serialization Time: %3.3fs\n", _commDatabaseSerializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Scatter Time: %3.3fs\n", _commFrameScatterTime / 1.0e+9);
  printf("[Jaffar]  + Database Deserialization Time: %3.3fs\n", _commDatabaseDeserializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Get Worker Info Time: %3.3fs\n", _commGatherWorkerInformationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Worker Frame Serialization Time: %3.3fs\n", _commWorkerFrameSerializationTime / 1.0e+9);
  printf("[Jaffar]  + Frame Gather Time: %3.3fs\n", _commFrameGatherTime / 1.0e+9);
 }

 if (_showDebuggingInformation)
 {
  printf("[Jaffar] Hash Table Collisions: %lu\n", _hashCollisions);
  printf("[Jaffar] Hash Table Entries: %lu\n", _hashes.size());

  printf("[Jaffar] Best Frame Information:\n");
  _sdlPop->printFrameInfo();

  // Printing Rule Status
  printf("[Jaffar]  + Rule Status: [ %d", (int)bestFrame->rulesStatus[0]);
  for (size_t i = 1; i < bestFrame->rulesStatus.size(); i++)
   printf(", %d", (int) bestFrame->rulesStatus[i]);
  printf(" ]\n");

  // Printing Rule Status
  int currentRoom = _sdlPop->Kid->room;
  const auto& magnet = bestFrame->magnets[currentRoom];
  printf("[Jaffar]  + Horizontal Magnet Intensity / Position: %.1f / %.0f\n", magnet.intensity, magnet.position);

  // Printing Move List
  printf("[Jaffar]  + Move List: . . %s\n", bestFrame->moveHistory.c_str());
 }
}

float Search::getFrameScore(const Frame* frame)
{
 // Accumulator for total score
 float score = 0.0f;

 // Obtaining magnet corresponding to kid's room
 int currentRoom = _sdlPop->Kid->room;

 // If room is outside visible rooms, do not apply magnet reward
 if (currentRoom >= 0 && currentRoom < _VISIBLE_ROOM_COUNT)
 {
  const auto &magnet = frame->magnets[currentRoom];

  // Evaluating magnet's score on the X axis
  const float diff = std::abs(_sdlPop->Kid->x - magnet.position);
  score += magnet.intensity * (256.0f - diff);
 }

 // Now adding rule rewards
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  if (frame->rulesStatus[ruleId] == st_achieved)
   score += _rules[ruleId]->_reward;

 // Returning score
 return score;
}
