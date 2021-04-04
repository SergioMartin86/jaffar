#include "rule.h"

Rule::Rule(nlohmann::json ruleJs)
{
 printf("Rule: %s\n", ruleJs.dump(2).c_str());

 auto c = new _vCondition<int>(op_equal);
 c->setImmediate1(1);
 c->setImmediate2(1);
 if (c->evaluate()) printf("Eval: True\n");
 if (!c->evaluate()) printf("Eval: False\n");
 _conditions.push_back(c);
}

