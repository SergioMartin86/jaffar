#pragma once

#include "json.hpp"
#include "utils.h"

enum op_t
{
  op_equal = 0,
  op_not_equal = 1,
  op_greater = 2,
  op_greater_or_equal = 3,
  op_less = 4,
  op_less_or_equal = 5
};

class Rule
{

public:

 Rule(const op_t opType);

protected:

 op_t _opType;
 bool _immediateAssigned;

};

template <typename T>
class iRule : public Rule
{

public:

  iRule(const op_t opType);
  void setOp1(const T* op1);
  void setOp2(const T* op2);
  void setImmediate1(const T immediate);
  void setImmediate2(const T immediate);
  bool evaluate();

private:

  static inline bool _opEqual(const T a, const T b) { return a == b; }
  static inline bool _opNotEqual(const T a, const T b) { return a != b; }
  static inline bool _opGreater(const T a, const T b) { return a > b; }
  static inline bool _opGreaterOrEqual(const T a, const T b) { return a >= b; }
  static inline bool _opLess(const T a, const T b) { return a < b; }
  static inline bool _opLessOrEqual(const T a, const T b) { return a <= b; }

  bool (*_opFcPtr)(const T, const T);

  T _immediate1;
  T _immediate2;
  T* _op1;
  T* _op2;
};

template <typename T>
iRule<T>::iRule(const op_t opType) : Rule(opType)
{
 if (_opType == op_equal) _opFcPtr = _opEqual;
 if (_opType == op_not_equal) _opFcPtr = _opEqual;
 if (_opType == op_greater) _opFcPtr = _opGreater;
 if (_opType == op_greater_or_equal) _opFcPtr = _opGreaterOrEqual;
 if (_opType == op_less) _opFcPtr = _opLess;
 if (_opType == op_less_or_equal) _opFcPtr = _opLessOrEqual;
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
