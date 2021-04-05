#pragma once

#include "SDLPopInstance.h"
#include "rule.h"
#include "state.h"
#include "json.hpp"
#include <vector>


class Scorer
{

public:

  Scorer(SDLPopInstance *sdlPop, State *state, nlohmann::json& config);
  float calculateScore();

private:

 SDLPopInstance* _sdlPop;
 State* _state;

 std::vector<Rule*> _rules;
};
