#include "search.h"
#include "utils.h"

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" };

Search::Search(SDLPopInstance *sdlPop, State *state, Scorer *scorer, nlohmann::json& config)
{
 _sdlPop = sdlPop;
 _state = state;
 _scorer = scorer;

 // Parsing search width from configuration file
 if (isDefined(config, "Max Database Size") == false) EXIT_WITH_ERROR("[ERROR] Config file missing 'Max Database Size' key.\n");
 _maxDatabaseSize = config["Max Database Size"].get<size_t>();

 // Allocating databases
 _currentFrameDB = new std::vector<Frame*>();
 _nextFrameDB = new std::vector<Frame*>();

 // Reserving space for frame data
 _currentFrameDB->reserve(_maxDatabaseSize);
 _nextFrameDB->reserve(_maxDatabaseSize);

 // Setting initial values
 _currentFrame = 0;
 _hasFinalized = false;
 _hashCollisions = 0;

 // Storing base frame data to be used by all frames
 _baseStateData = _state->saveBase();

 // Setting initial frame
 Frame* initialFrame = new Frame;
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
  while(_hasFinalized == false)
  {
   runFrame();
   _currentFrame++;

   if (_currentFrame > 50) _hasFinalized = true;
  }
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
    const auto score = _scorer->calculateScore();

    // Adding novel frame in the next frame database
    Frame* newFrame = new Frame;
    newFrame->score = score; // TO-DO: Get score from scorer
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
}
