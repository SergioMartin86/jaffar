#include "rule.h"
#include "utils.h"

Rule::Rule(const op_t opType)
{
 _immediateAssigned = false;
 _opType = opType;
}

template <typename T>
iRule<T>::iRule(const op_t opType) : Rule(opType)
{
 if (_opType == op_equal) _opFcPtr = _opEqual;
 if (_opType == op_not_equal) _opFcPtr = _opEqual;
 if (_opType == op_greater) _opFcPtr = _opGreater;
 if (_opType == op_greater_or_equal) _opFcPtr = _opGreaterOrEqual;
 if (_opType == op_less) _opFcPtr = _opLess;
 if (_opType == op_less_or_equal) _opFcPtr = _opLessOrEqual;

 EXIT_WITH_ERROR("No valid rule found\n");
}

template <typename T>
bool iRule<T>::evaluate()
{
 return _opFcPtr(*_op1, *_op2);
}

template <typename T>
void iRule<T>::setOp1(const T* op1)
{
 _op1 = op1;
}

template <typename T>
void iRule<T>::setOp2(const T* op2)
{
 _op2 = op2;
}

template <typename T>
void iRule<T>::setImmediate1(const T immediate)
{
 _immediate1 = immediate;
 _op1 = &_immediate1;
}

template <typename T>
void iRule<T>::setImmediate2(const T immediate)
{
 _immediate2 = immediate;
 _op2 = &_immediate2;
}
