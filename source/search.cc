#include "search.h"
#include "utils.h"

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

void Search::runFrame()
{
 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
 {
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
    if (!_hashes.insert(hash).second)
    {
     _hashCollisions++;
      continue;
    }

    // Creating new frame, mixing base frame information and the current sdlpop state
    Frame* newFrame = new Frame;
    newFrame->isFail = false;
    newFrame->isWin = false;
    newFrame->move = move;
    newFrame->rulesStatus = baseFrame->rulesStatus;
    newFrame->frameStateData = _state->saveFrame();
    newFrame->magnets = baseFrame->magnets;

    // Evaluating rules on the new frame
    evaluateRules(newFrame);

    // If frame has failed, then proceed to the next one
    if (newFrame->isFail == true) continue;

    // Calculating score
    newFrame->score =  getFrameScore(newFrame);

    // Adding novel frame in the next frame database
    _nextFrameDB->push_back(newFrame);

    //printf("New Frame - Action: %s - Hash: 0x%lX - Score: %f - NextDBSize: %lu\n", move.c_str(), hash, newFrame->score, _nextFrameDB->size());

    // If frame has succeeded, then exit
    if (newFrame->isWin == true)
    {
     printf("Success Frame found!\n");
     exit(0);
    }
   }
 }

 // Freeing memory for current DB frames
 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
  delete (*_currentFrameDB)[frameId];

 // Clearing current frame DB
 _currentFrameDB->clear();

 // Swapping DB pointers
 std::swap(_currentFrameDB, _nextFrameDB);

 // Sorting DB frames by score
 std::sort(_currentFrameDB->begin(), _currentFrameDB->end(), [](const auto& a, const auto& b) { return a->score > b->score; });

 // Clipping excessive frames out
 for (size_t frameId = _maxDatabaseSize; frameId < _currentFrameDB->size(); frameId++) delete (*_currentFrameDB)[frameId];
 if (_currentFrameDB->size() > _maxDatabaseSize) _currentFrameDB->resize(_maxDatabaseSize);
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

 // Setting initial frame
 Frame* initialFrame = new Frame;
 initialFrame->move = ".";
 initialFrame->score = 0.0f; // TO-DO: Get score from scorer
 initialFrame->frameStateData = _state->saveFrame();
 initialFrame->magnets = magnets;
 initialFrame->rulesStatus = rulesStatus;
 initialFrame->isFail = false;
 initialFrame->isWin = false;
 _currentFrameDB->push_back(initialFrame);

 // Registering hash for initial frame
 const auto hash = _state->computeHash();
 _hashes.insert(hash);
}

Search::~Search()
{
 delete _currentFrameDB;
 delete _nextFrameDB;
}

void Search::run()
{
  for(_currentFrame = 1; _currentFrame <= _maxFrames; _currentFrame++)
  {
   // Plotting current best frame
   auto bestFrame = (*_currentFrameDB)[0];
   _state->loadBase(_baseStateData);
   _state->loadFrame(bestFrame->frameStateData);
   _sdlPop->refreshEngine();
   _sdlPop->_currentMove = bestFrame->move;

   // Drawing frame
   _sdlPop->draw();

   // Printing search status
   printSearchStatus();

   // Running a single frame search
   runFrame();

   if (_currentFrameDB->size() == 0)
   {
    printf("[Jaffar] Frame database depleted with no winning frames, finishing...\n");
    exit(0);
   }
   //getchar();
  }
}

void Search::printSearchStatus()
{
 auto bestFrame = (*_currentFrameDB)[0];

 printf("[Jaffar] ----------------------------------------------------------------\n");
 printf("[Jaffar] Current Frame: %lu/%lu\n", _currentFrame, _maxFrames);
 printf("[Jaffar] Best Score: %f\n", bestFrame->score);
 printf("[Jaffar] Database Size: %lu/%lu\n", _currentFrameDB->size(), _maxDatabaseSize);
 printf("[Jaffar] Frame Information:\n");
 _sdlPop->printFrameInfo();
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
