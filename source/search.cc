#include "search.h"
#include "utils.h"

Search::Search(SDLPopInstance *sdlPop, State *state, Scorer *scorer, nlohmann::json& config)
{
 _sdlPop = sdlPop;
 _state = state;
 _scorer = scorer;

 // Parsing search width from configuration file
 if (isDefined(config, "Search Width") == false) EXIT_WITH_ERROR("[ERROR] Config file missing 'Search Width' key.\n");
 _searchWidth = config["Search Width"].get<size_t>();

 // Allocating databases
 _currentFrameDB = new std::vector<Frame*>();
 _nextFrameDB = new std::vector<Frame*>();

 // Reserving space for frame data
 _currentFrameDB->reserve(_searchWidth);
 _nextFrameDB->reserve(_searchWidth);

 // Filling possible actions per kid frame map
 for (size_t i = 0; i < 255; i++)
  _kidFrameActionMap.push_back({".", "U", "L", "R", "S", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD" });

 // Setting initial values
 _currentFrame = 0;
 _hasFinalized = false;

 _baseStateData = _state->saveBase();

 // Setting initial frame
 Frame* initialFrame = new Frame;
 initialFrame->score = 0.0f; // TO-DO: Get score from scorer
 initialFrame->frameStateData = _state->saveFrame();
 _currentFrameDB->push_back(initialFrame);
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

   if (_currentFrame > 2) _hasFinalized = true;
  }
}

void Search::runFrame()
{
 printf("Frame: %lu - DB Size: %lu\n", _currentFrame, _currentFrameDB->size());

 for (size_t frameId = 0; frameId < _currentFrameDB->size(); frameId++)
 {
   const auto kidFrame = _sdlPop->Kid->frame;

   for (size_t actionId = 0; actionId < _kidFrameActionMap[kidFrame].size(); actionId++)
   {
    printf("Action: %s\n", _kidFrameActionMap[kidFrame][actionId].c_str());
    _state->loadBase(_baseStateData);
    _state->loadFrame((*_currentFrameDB)[frameId]->frameStateData);
   }
 }
}
