#include "rule.h"
#include "utils.h"

template <typename T>
Rule<T>::Rule(const op_t opType)
{
 _immediateAssigned = false;

 if (_opType == op_equal) _opFcPtr = _opEqual;
 if (_opType == op_not_equal) _opFcPtr = _opEqual;
 if (_opType == op_greater) _opFcPtr = _opGreater;
 if (_opType == op_greater_or_equal) _opFcPtr = _opGreaterOrEqual;
 if (_opType == op_less) _opFcPtr = _opLess;
 if (_opType == op_less_or_equal) _opFcPtr = _opLessOrEqual;

 EXIT_WITH_ERROR("No valid rule found\n");
}

template <typename T>
bool Rule<T>::evaluate()
{
 return _opFcPtr(*_op1, *_op2);
}

