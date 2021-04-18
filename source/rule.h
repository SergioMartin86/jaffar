#pragma once

#include "nlohmann/json.hpp"
#include "utils.h"
#include "SDLPopInstance.h"
#include <vector>

enum status_t
{
  st_active = 0,
  st_inactive = -1,
  st_achieved = 1
};

enum operator_t
{
  op_equal = 0,
  op_not_equal = 1,
  op_greater = 2,
  op_greater_or_equal = 3,
  op_less = 4,
  op_less_or_equal = 5
};

enum datatype_t
{
  dt_byte = 0,
  dt_sbyte = 1,
  dt_short = 2,
  dt_int = 3,
  dt_word = 4,
  dt_dword = 5
};

class Condition;

class Rule
{

public:

 Rule(nlohmann::json ruleJs, SDLPopInstance* sdlPop);
 bool evaluate();

 // Stores the reward associated with meeting this rule
 float _reward;

 // Actions are seldom executed, so this is optimized for flexibility
 // Here, the json is stored, and the parsing is handled on runtime
 std::vector<nlohmann::json> _actions;

 // Stores dependencies with other rules
 std::vector<size_t> _dependencies;

private:

 // Conditions are evaluated frequently, so this optimized for performance
 // Operands are pre-parsed as pointers/immediates and the evaluation function
 // is a template that is created at compilation time.
 std::vector<Condition*> _conditions;
 datatype_t getPropertyType(const std::string& property);
 void* getPropertyPointer(const std::string& property, SDLPopInstance* sdlPop);
 operator_t getOperationType(const std::string& operation);
};

class Condition
{

public:

 Condition(const operator_t opType)
 {
  _immediateAssigned = false;
  _opType = opType;
 }

 virtual bool evaluate() = 0;
 virtual ~Condition() = default;

protected:

 operator_t _opType;
 bool _immediateAssigned;

};

template <typename T>
class _vCondition : public Condition
{

public:

  _vCondition(const operator_t opType, void* property, T immediate);
  void setProperty(const T* property);
  void setImmediate(const T immediate);
  bool evaluate() override;

private:

  static inline bool _opEqual(const T a, const T b) { return a == b; }
  static inline bool _opNotEqual(const T a, const T b) { return a != b; }
  static inline bool _opGreater(const T a, const T b) { return a > b; }
  static inline bool _opGreaterOrEqual(const T a, const T b) { return a >= b; }
  static inline bool _opLess(const T a, const T b) { return a < b; }
  static inline bool _opLessOrEqual(const T a, const T b) { return a <= b; }

  bool (*_opFcPtr)(const T, const T);

  T* _property;
  T _immediate;
};

template <typename T>
_vCondition<T>::_vCondition(const operator_t opType, void* property, T immediate) : Condition(opType)
{
 if (_opType == op_equal) _opFcPtr = _opEqual;
 if (_opType == op_not_equal) _opFcPtr = _opEqual;
 if (_opType == op_greater) _opFcPtr = _opGreater;
 if (_opType == op_greater_or_equal) _opFcPtr = _opGreaterOrEqual;
 if (_opType == op_less) _opFcPtr = _opLess;
 if (_opType == op_less_or_equal) _opFcPtr = _opLessOrEqual;

 _property = (T*)property;
 _immediate = immediate;
}

template <typename T>
bool _vCondition<T>::evaluate()
{
 return _opFcPtr(*_property, _immediate);
}
