#pragma once

#include "SDLPopInstance.h"
#include "state.h"
#include "scorer.h"
#include "json.hpp"
#include <string>
#include <vector>


struct Frame
{
 float score;
 std::string frameStateData;
};

class Search
{

public:

  Search(SDLPopInstance *sdlPop, State *state, Scorer *scorer, nlohmann::json& config);
  ~Search();

  void run();
  void runFrame();

private:

 size_t _searchWidth;
 SDLPopInstance* _sdlPop;
 State* _state;
 Scorer* _scorer;
 std::string _baseStateData;

 std::vector<Frame*>* _currentFrameDB;
 std::vector<Frame*>* _nextFrameDB;
 std::vector<std::vector<std::string>> _kidFrameActionMap;

 size_t _currentFrame;
 bool _hasFinalized;

};
