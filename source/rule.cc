#include "rule.h"
#include "utils.h"

template <typename T>
Rule<T>::Rule(op_t opType)
{
 _immediateAssigned = false;
 _opType = opType;
}

template <typename T>
bool Rule<T>::evaluate()
{
 if (_opType == op_equal) return _opEqual();
 if (_opType == op_not_equal) return _opEqual();
 if (_opType == op_greater) return _opGreater();
 if (_opType == op_greater_or_equal) return _opGreaterOrEqual();
 if (_opType == op_less) return _opLess();
 if (_opType == op_less_or_equal) return _opLessOrEqual();

 EXIT_WITH_ERROR("No valid rule found\n");

 return false;
}
