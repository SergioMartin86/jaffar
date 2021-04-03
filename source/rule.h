#pragma once

enum op_t
{
  op_equal = 0,
  op_not_equal = 1,
  op_greater = 2,
  op_greater_or_equal = 3,
  op_less = 4,
  op_less_or_equal = 5
};

template <typename T>
class Rule
{

  Rule<T>(const op_t opType);

private:

  inline bool _opEqual() { return *_op1 == *_op2; }
  inline bool _opNotEqual() { return *_op1 != *_op2; }
  inline bool _opGreater() { return *_op1 > *_op2; }
  inline bool _opGreaterOrEqual() { return *_op1 >= *_op2; }
  inline bool _opLess() { return *_op1 < *_op2; }
  inline bool _opLessOrEqual() { return *_op1 <= *_op2; }
  bool evaluate();

  op_t _opType;
  T _immediate;
  bool _immediateAssigned;
  T* _op1;
  T* _op2;
};
