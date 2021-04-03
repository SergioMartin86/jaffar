#pragma once

#include "SDLPopInstance.h"
#include "rule.h"
#include <vector>


class Search
{

public:

  Search(SDLPopInstance *sdlPop);

private:

 SDLPopInstance* _sdlPop;
 std::vector<Rule*> _rules;
};
