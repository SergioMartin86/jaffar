#include "search.h"
#include "utils.h"
#include "jaffar.h"
#include <unistd.h>
#include <chrono>

size_t _ruleCount;
const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

void Search::run()
{
 auto searchTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Storage for termination trigger
 bool terminate = false;

 while(terminate == false)
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
  }

  // All workers: running a single frame search
  runFrame();

  // Checking termination criteria
  if (_jaffarConfig.mpiRank == 0)
  {
   // Terminate if DB is depleted and no winning rule was found
   if (_globalFrameCounter == 0)
   {
    printf("[Jaffar] Frame database depleted with no winning frames, finishing...\n");
    terminate = true;
   }

   // Terminate if a winning rule was found
   if (_winFrameFound == true)
   {
    printf("[Jaffar] Winning frame reached, finishing...\n");
    terminate = true;
   }

   // Terminate if maximum number of frames was reached
   if (_currentFrame > _maxFrames)
   {
    printf("[Jaffar] Maximum frame number reached, finishing...\n");
    terminate = true;
   }
  }

  // Broadcasting whether a winning frame was found
  MPI_Bcast(&terminate, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

  // Advancing frame
  _currentFrame++;
 }

 // Print winning frame if found
 if (_winFrameFound == true)
 {
  printf("[Jaffar] Win Frame Information:\n");
  _state->loadBase(_baseStateData);
  _state->loadFrame(_winFrame->frameStateData);
  _sdlPop->refreshEngine();
  _sdlPop->printFrameInfo();
  printRuleStatus(_winFrame);
  printf("[Jaffar]  + Move List: %s\n", _winFrame->moveHistory.c_str());
 }

 // Barrier to wait for all workers
 MPI_Barrier(MPI_COMM_WORLD);
}

void Search::runFrame()
{
 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Preprocessing] Redistribute base frames all-to-all among workers
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 auto framePreprocessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Getting worker's own base frame count
 size_t localBaseFrameCount = _currentFrameDB->size();

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

 // Determining all-to-all send/recv displacements
 std::vector<std::vector<int>> allToAllSendDispls(_workerCount);
 std::vector<std::vector<int>> allToAllRecvDispls(_workerCount);
 for (size_t i = 0; i < _workerCount; i++) allToAllSendDispls[i].resize(_workerCount, 0);
 for (size_t i = 0; i < _workerCount; i++) allToAllRecvDispls[i].resize(_workerCount, 0);

 // Send displacements
 for (size_t i = 0; i < _workerCount; i++)
 {
  size_t curSendDispl = 0;
  for (size_t j = 0; j < _workerCount; j++)
  {
   allToAllSendDispls[i][j] = curSendDispl;
   curSendDispl += allToAllSendCounts[i][j];
  }
 }

 // Receive displacements
 for (size_t i = 0; i < _workerCount; i++)
 {
  size_t curRecvDispl = 0;
  for (size_t j = 0; j < _workerCount; j++)
  {
   allToAllRecvDispls[i][j] = curRecvDispl;
   curRecvDispl += allToAllRecvCounts[i][j];
  }
 }

 // Resizing next DB to receive the new frames
 size_t localNextFrameCount = localNextFrameCounts[_workerId];

 // Serializing database into a contiguous buffer
 size_t currentPosition = 0;
 char* frameSendBuffer = (char*) malloc(_frameSerializedSize * _currentFrameDB->size());
 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
 {
  // Serializing frame
  (*_currentFrameDB)[frameId]->serialize(&frameSendBuffer[currentPosition]);

  // Advancing send buffer pointer
  currentPosition += _frameSerializedSize;

  // Freeing frame
  delete (*_currentFrameDB)[frameId];
 }

 // Clearing database
 _currentFrameDB->clear();

 // Reserving receive buffer
 char* frameRecvBuffer = (char*) malloc(_frameSerializedSize * localNextFrameCount);

 // Exchanging frames
 MPI_Alltoallv(frameSendBuffer, allToAllSendCounts[_workerId].data(), allToAllSendDispls[_workerId].data(), _mpiFrameType,
               frameRecvBuffer, allToAllRecvCounts[_workerId].data(), allToAllRecvDispls[_workerId].data(), _mpiFrameType, MPI_COMM_WORLD);

 // Freeing send buffer
 free(frameSendBuffer);

 // Deserializing contiguous buffer into the new frame database
 currentPosition = 0;
 for (size_t frameId = 0; frameId < localNextFrameCount; frameId++)
 {
  // Creating new frame
  Frame* newFrame = new Frame;

  // Deserializing frame
  newFrame->deserialize(&frameRecvBuffer[currentPosition]);

    // Adding new frame into the data base
  _nextFrameDB->push_back(newFrame);

  // Advancing send buffer pointer
  currentPosition += _frameSerializedSize;
 }

 // Freeing send buffers
 free(frameRecvBuffer);

 // Swapping database pointers
 std::swap(_currentFrameDB, _nextFrameDB);

 // Clearing old base frames
 for (size_t frameId = 0; frameId < _nextFrameDB->size(); frameId++)
  delete (*_nextFrameDB)[frameId];

 // Clearing next frame DB
 _nextFrameDB->clear();

 auto framePreprocessingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _framePreprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePreprocessingTimeEnd - framePreprocessingTimeBegin).count();    // Profiling

 ////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Computation - Parallel] Each worker processes its own unique base frames
 ////////////////////////////////////////////////////////////////////////////////////////////////////

 MPI_Barrier(MPI_COMM_WORLD);
 auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Storing new hashes to broadcast to other workers
 absl::flat_hash_set<uint64_t> newHashes;
 size_t newCollisionCounter = 0;

 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  for (size_t moveId = 0; moveId < _possibleMoves.size(); moveId++)
  {
   const std::string move = _possibleMoves[moveId].c_str();

   // Getting base frame pointer
   Frame* baseFrame = (*_currentFrameDB)[frameId];

   // Loading frame state
   _state->loadBase(_baseStateData);
   _state->loadFrame(baseFrame->frameStateData);
   _sdlPop->refreshEngine();

   // Perform the selected move
   _sdlPop->performMove(move);

   // Advance a single frame
   _sdlPop->advanceFrame();

   // Compute hash value
   const auto hash = _state->computeHash();

   // Adding hash to the collection, and check whether it already existed
   bool collisionDetected = !_hashes.insert(hash).second;

   // If collision detected locally, discard this frame
   if (collisionDetected) { newCollisionCounter++; continue; }

   // Creating new frame, mixing base frame information and the current sdlpop state
   Frame* newFrame = new Frame;
   newFrame->currentMove = move;
   newFrame->rulesStatus = baseFrame->rulesStatus;
   newFrame->frameStateData = _state->saveFrame();
   newFrame->magnets = baseFrame->magnets;

   // Evaluating rules on the new frame
   bool isFail = evaluateRules(newFrame);

   // Calculating score
   newFrame->score =  getFrameScore(newFrame);

   // If frame has failed, then proceed to the next one
   if (isFail == true) continue;

   // Adding novel frame in the next frame database
   _nextFrameDB->push_back(newFrame);

   // Adding hash into the new hashes table
   newHashes.insert(hash);
 }

 // Swapping database pointers
 std::swap(_currentFrameDB, _nextFrameDB);

 // Clearing old base frames
 for (size_t frameId = 0; frameId < _nextFrameDB->size(); frameId++)
  delete (*_nextFrameDB)[frameId];

 // Clearing next frame DB
 _nextFrameDB->clear();

 MPI_Barrier(MPI_COMM_WORLD);

 auto frameComputationTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count();    // Profiling

 /////////////////////////////////////////////////////////////////////////////////////////////////////
 // [Postprocessing] Each worker processes its own unique base frames and communicates partial results
 /////////////////////////////////////////////////////////////////////////////////////////////////////

 auto framePostprocessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

 // Sorting local DB frames by score
 std::sort(_currentFrameDB->begin(), _currentFrameDB->end(), [](const auto& a, const auto& b) { return a->score > b->score; });

 // Finding local best frame score
 float localBestFrameScore = -std::numeric_limits<float>::infinity();
 if (_currentFrameDB->empty() == false) localBestFrameScore = (*_currentFrameDB)[0]->score;

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
 if (_workerId == (size_t)globalBestFrameRank) (*_currentFrameDB)[0]->serialize(frameBcastBuffer);
 MPI_Bcast(frameBcastBuffer, 1, _mpiFrameType, globalBestFrameRank, MPI_COMM_WORLD);
 _bestFrame.deserialize(frameBcastBuffer);

 // Clipping database to the maximum threshold
 if (_currentFrameDB->size() > _maxLocalDatabaseSize)
  _currentFrameDB->resize(_maxLocalDatabaseSize);

 // Calculating global frame count
 size_t newLocalFrameDatabaseSize = _currentFrameDB->size();
 MPI_Allreduce(&newLocalFrameDatabaseSize, &_globalFrameCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);

 // Gathering number of new hash entries
 int localNewHashEntryCount = (int)newHashes.size();
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
 for (const auto& key : newHashes) localNewHashVector.push_back(key);

 // Gathering new hash entries
 size_t globalNewHashEntryCount = currentDispl;
 std::vector<uint64_t> globalNewHashVector(globalNewHashEntryCount);
 MPI_Allgatherv(localNewHashVector.data(), localNewHashEntryCount, MPI_UINT64_T, globalNewHashVector.data(), globalNewHashEntryCounts.data(), globalNewHashEntryDispls.data(), MPI_UINT64_T, MPI_COMM_WORLD);

 // Adding new hash entries
 for (size_t i = 0; i < globalNewHashEntryCount; i++)
  _hashes.insert(globalNewHashVector[i]);

 // Finding global collision counter
 size_t globalNewCollisionCounter;
 MPI_Allreduce(&newCollisionCounter, &globalNewCollisionCounter, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
 _globalHashCollisions += globalNewCollisionCounter;

 auto framePostprocessingTimeEnd = std::chrono::steady_clock::now(); // Profiling
 _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePostprocessingTimeEnd - framePostprocessingTimeBegin).count();    // Profiling
}

bool Search::evaluateRules(Frame* frame)
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
      return true;
      recognizedActionType = true;
     }

     // This action is handled during startup, no need to do anything now.
     if (actionType == "Trigger Win")
     {
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

      frame->magnets[room].intensityX = intensityX;
      recognizedActionType = true;
     }

     if (actionType == "Set Horizontal Magnet Position")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float positionX = actionJs["Value"].get<float>();

      frame->magnets[room].positionX = positionX;
      recognizedActionType = true;
     }

     if (actionType == "Set Vertical Magnet Intensity")
     {
      if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", ruleId, actionId);
      if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", ruleId, actionId);
      float intensityY = actionJs["Value"].get<float>();

      frame->magnets[room].intensityY = intensityY;
      recognizedActionType = true;
     }

     if (recognizedActionType == false) EXIT_WITH_ERROR("[ERROR] Unrecognized action type %s\n", actionType.c_str());
    }
   }
  }
 }

 return false;
}

Search::Search()
{
 // Profiling information
 _searchTotalTime = 0.0;
 _framePreprocessingTime = 0.0;
 _frameComputationTime = 0.0;
 _framePostprocessingTime = 0.0;

 // Getting worker count
 _workerId = (size_t) _jaffarConfig.mpiRank;
 _workerCount = (size_t) _jaffarConfig.mpiSize;

 // Setting starting Frame id
 _currentFrame = 1;

 // Parsing profiling verbosity
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Show Profiling Information") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Show Profiling Information' key.\n");
 _showProfilingInformation = _jaffarConfig.configJs["Search Configuration"]["Show Profiling Information"].get<bool>();

 // Parsing debugging verbosity
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Show Debugging Information") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Show Debugging Information' key.\n");
 _showDebuggingInformation = _jaffarConfig.configJs["Search Configuration"]["Show Debugging Information"].get<bool>();

 // Parsing preview display
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Show SDLPop Display") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Show SDLPop Display' key.\n");
 _showSDLPopPreview = _jaffarConfig.configJs["Search Configuration"]["Show SDLPop Display"].get<bool>();

 // Parsing max frames from configuration file
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Max Frames") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Max Frames' key.\n");
 _maxFrames = _jaffarConfig.configJs["Search Configuration"]["Max Frames"].get<size_t>();

 // Parsing search width from configuration file
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Max Local Database Size") == false) EXIT_WITH_ERROR("[ERROR] Search configuration missing 'Max Local Database Size' key.\n");
 _maxLocalDatabaseSize = _jaffarConfig.configJs["Search Configuration"]["Max Local Database Size"].get<size_t>();

 // Initializing SDLPop
 _sdlPop = new SDLPopInstance;

 bool showSDLPopPreview = false;
 if (_workerId == 0) showSDLPopPreview = _showSDLPopPreview;
 _sdlPop->initialize(1, showSDLPopPreview);

 // Initializing State Handler
 _state = new State(_sdlPop);

 // Setting magnet default values. This default repels unspecified rooms.
 std::vector<Magnet> magnets(_VISIBLE_ROOM_COUNT);

 for (size_t i = 0; i < _VISIBLE_ROOM_COUNT; i++)
 {
  magnets[i].intensityY = 0.0f;
  magnets[i].intensityX = -1.0f;
  magnets[i].positionX = 128.0f;
 }

 // Processing user-specified rules
 if (isDefined(_jaffarConfig.configJs["Search Configuration"], "Rules") == false) EXIT_WITH_ERROR("[ERROR] Search configuration file missing 'Rules' key.\n");
 for (size_t i = 0; i < _jaffarConfig.configJs["Search Configuration"]["Rules"].size(); i++)
  _rules.push_back(new Rule(_jaffarConfig.configJs["Search Configuration"]["Rules"][i], _sdlPop));

 // Setting global for rule count
 _ruleCount = _rules.size();

 // Setting initial status for each rule
 std::vector<status_t> rulesStatus(_ruleCount);
 for (size_t i = 0; i < _ruleCount; i++) rulesStatus[i] = st_active;

 _frameSerializedSize = Frame::getSerializationSize();
 MPI_Type_contiguous(_frameSerializedSize, MPI_BYTE, &_mpiFrameType);
 MPI_Type_commit(&_mpiFrameType);

 // Instanting frame database
 _currentFrameDB = new std::vector<Frame*>();
 _nextFrameDB = new std::vector<Frame*>();

 // Setting initial values
 _hasFinalized = false;
 _globalHashCollisions = 0;

 // Storing base frame data to be used by all frames
 _baseStateData = _state->saveBase();

 // Setting win status
 _winFrameFound = false;
 _winFrame = NULL;

 // Creating initial frame on the root rank
 if (_jaffarConfig.mpiRank == 0)
 {
  // Computing initial hash
  const auto hash = _state->computeHash();

  Frame* initialFrame = new Frame;
  initialFrame->currentMove = ".";
  initialFrame->moveHistory = ".";
  initialFrame->frameStateData = _state->saveFrame();
  initialFrame->magnets = magnets;
  initialFrame->rulesStatus = rulesStatus;

  // Evaluating Rules on initial frame
  evaluateRules(initialFrame);

  // Storing which rules are win rules
  for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   for (size_t actionId = 0; actionId < _rules[ruleId]->_actions.size(); actionId++)
    if (_rules[ruleId]->_actions[actionId]["Type"] == "Trigger Win")
     _winRulePositions.push_back(ruleId);

  if (_winRulePositions.size() == 0)
   EXIT_WITH_ERROR("[ERROR] No win (Trigger Win) rules were defined in the config file.\n");

  // Evaluating Score on initial frame
  initialFrame->score = getFrameScore(initialFrame);

  // Adding frame to the current database
  _currentFrameDB->push_back(initialFrame);

  // Registering hash for initial frame
  _hashes.insert(hash);

  // Copying initial frame into the best frame
  _bestFrame = *initialFrame;
 }

 // Starting global frame counter
 _globalFrameCounter = 1;
}

Search::~Search()
{
}

void Search::printSearchStatus()
{
 printf("[Jaffar] ----------------------------------------------------------------\n");
 printf("[Jaffar] Current Frame: %lu/%lu\n", _currentFrame, _maxFrames);
 printf("[Jaffar] Best Score: %f\n", _bestFrame.score);
 printf("[Jaffar] Database Size: %lu/%lu\n", _globalFrameCounter, _maxLocalDatabaseSize*_workerCount);
 printf("[Jaffar] Elapsed Time (Frame/Total): %3.3fs / %3.3fs\n", (_framePreprocessingTime + _frameComputationTime + _framePostprocessingTime) / 1.0e+9, _searchTotalTime / 1.0e+9);

 if (_showProfilingInformation)
 {
  printf("[Jaffar] Frame Preprocessing Time:  %3.3fs\n", _framePreprocessingTime / 1.0e+9);
  printf("[Jaffar] Frame Computation Time:    %3.3fs\n", _frameComputationTime / 1.0e+9);
  printf("[Jaffar] Frame Postprocessing Time: %3.3fs\n", _framePostprocessingTime / 1.0e+9);
 }

 if (_showDebuggingInformation)
 {
  printf("[Jaffar] Hash Table Collisions: %lu\n", _globalHashCollisions);
  printf("[Jaffar] Hash Table Entries: %lu\n", _hashes.size());

  printf("[Jaffar] Best Frame Information:\n");
  _sdlPop->printFrameInfo();

  printRuleStatus(&_bestFrame);

  // Printing Magnet Status
  int currentRoom = _sdlPop->Kid->room;
  const auto& magnet = _bestFrame.magnets[currentRoom];
  printf("[Jaffar]  + Horizontal Magnet Intensity / Position: %.1f / %.0f\n", magnet.intensityX, magnet.positionX);
  printf("[Jaffar]  + Vertical Magnet Intensity: %.1f\n", magnet.intensityY);

  // Printing Move List
  printf("[Jaffar]  + Move List: %s\n", _bestFrame.moveHistory.c_str());
 }
}

void Search::printRuleStatus(const Frame* frame)
{
 printf("[Jaffar]  + Rule Status: [ %d", (int)frame->rulesStatus[0]);
 for (size_t i = 1; i < frame->rulesStatus.size(); i++)
  printf(", %d", (int) frame->rulesStatus[i]);
 printf(" ]\n");

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

 // Now adding rule rewards
 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  if (frame->rulesStatus[ruleId] == st_achieved)
   score += _rules[ruleId]->_reward;

 // Returning score
 return score;
}
