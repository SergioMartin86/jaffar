#include "search.h"
#include "utils.h"

Search::Search(SDLPopInstance *sdlPop)
{
 _sdlPop = sdlPop;

 iRule<int> r(op_equal);
 r.setImmediate1(1);
 r.setImmediate2(1);
 if (r.evaluate()) printf("Eval: True\n");
 if (!r.evaluate()) printf("Eval: False\n");
 _rules.push_back(&r);
}
