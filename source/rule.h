#pragma once

#include "json.hpp"

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

private:

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

