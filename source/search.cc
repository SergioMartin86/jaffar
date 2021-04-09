#include "search.h"
#include "utils.h"

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

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
 _magnets[0].intensityX = 0.1f;
 _magnets[0].intensityY = 0.1f;
 _magnets[0].positionX = 128.0f;
 _magnets[0].positionY = 128.0f;

 for (size_t i = 1; i < 24; i++)
 {
  _magnets[i].intensityX = -0.1f;
  _magnets[i].intensityY = -0.1f;
  _magnets[i].positionX = 128.0f;
  _magnets[i].positionY = 128.0f;
 }

 for (size_t i = 25; i < 256; i++)
 {
   _magnets[i].intensityX = +0.1f;
   _magnets[i].intensityY = +0.1f;
   _magnets[i].positionX = 128.0f;
   _magnets[i].positionY = 128.0f;
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
   _magnets[room].intensityX = magnet["Intensity X"].get<float>();
   _magnets[room].intensityY = magnet["Intensity Y"].get<float>();
   _magnets[room].positionX = magnet["Position X"].get<float>();
   _magnets[room].positionY = magnet["Position Y"].get<float>();
 }

 // Processing user-specified rules
 if (isDefined(config, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Search configuration file missing 'Rules' key.\n");
 for (size_t i = 0; i < config["Rules"].size(); i++)
  _rules.push_back(new Rule(config["Rules"][i], _sdlPop));

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
   runFrame();

   // Plotting best frame
   auto bestFrame = (*_currentFrameDB)[0];
   _state->loadBase(_baseStateData);
   _state->loadFrame(bestFrame->frameStateData);
   _sdlPop->refreshEngine();
   _sdlPop->_currentMove = bestFrame->move;

   // Drawing frame
   _sdlPop->draw();

   // Printing search status
   printSearchStatus();
   getchar();
  }
}

void Search::printSearchStatus()
{
 printf("[Jaffar] ----------------------------------------------------------------\n");
 printf("[Jaffar] Current Frame: %lu\n", _currentFrame);
 printf("[Jaffar] Best Score: %f\n", 0.0f);
 printf("[Jaffar] Frame Information:\n");
 _sdlPop->printFrameInfo();
 printf("[Jaffar] ----------------------------------------------------------------\n");
}

void Search::runFrame()
{
 printf("Frame: %lu - DB Size: %lu\n", _currentFrame, _currentFrameDB->size());

 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
 {
   for (size_t moveId = 0; moveId < _possibleMoves.size(); moveId++)
   {
    const std::string move = _possibleMoves[moveId].c_str();

    // Loading frame state
    _state->loadBase(_baseStateData);
    _state->loadFrame((*_currentFrameDB)[frameId]->frameStateData);
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

    // Obtaining score for current frame
    const float score = getFrameScore();

    // Adding novel frame in the next frame database
    Frame* newFrame = new Frame;
    newFrame->move = move;
    newFrame->score = score;
    newFrame->frameStateData = _state->saveFrame();
    _nextFrameDB->push_back(newFrame);

    printf("New Frame - Action: %s - Hash: 0x%lX - Score: %f - NextDBSize: %lu\n", move.c_str(), hash, score, _nextFrameDB->size());
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

float Search::getFrameScore()
{
 // Accumulator for total score
 float score = 0.0f;

 // Obtaining magnet corresponding to kid's room
 int currentRoom = _sdlPop->Kid->room;
 const auto &magnet = _magnets[currentRoom];

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

 // Returning score
 return score;
}
