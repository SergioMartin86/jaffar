#include "scorer.h"
#include "utils.h"

Scorer::Scorer(SDLPopInstance *sdlPop, State *state, nlohmann::json& config)
{
 _sdlPop = sdlPop;
 _state = state;
}
