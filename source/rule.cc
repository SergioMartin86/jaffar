#include "rule.h"

Rule::Rule(nlohmann::json ruleJs, SDLPopInstance* sdlPop)
{
 printf("Rule: %s\n", ruleJs.dump(2).c_str());

 // Adding conditions. All of them must be satisfied for the rule to count
 if (isDefined(ruleJs, "Conditions") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Conditions' key.\n");
 for (size_t i = 0; i < ruleJs["Conditions"].size(); i++)
 {
  auto conditionJs = ruleJs["Conditions"][i];

  // Parsing operation type
  if (isDefined(conditionJs, "Operation") == false) EXIT_WITH_ERROR("[ERROR] Rule condition missing 'Operation' key.\n");
  op_t operation = getOperationType(conditionJs["Operation"].get<std::string>());

  // Parsing first operand (property name)
  if (isDefined(conditionJs, "Operand 1") == false) EXIT_WITH_ERROR("[ERROR] Rule condition missing 'Operand 1' key.\n");
  if (conditionJs["Operand 1"].is_string() == false) EXIT_WITH_ERROR("[ERROR] Condition operand 1 must be a string with the name of a property.\n");
  datatype_t dtype = getPropertyType(conditionJs["Operand 1"].get<std::string>());
  auto property = getPropertyPointer(conditionJs["Operand 1"].get<std::string>(), sdlPop);

  // Parsing second operand (number)
  if (isDefined(conditionJs, "Operand 2") == false) EXIT_WITH_ERROR("[ERROR] Rule condition missing 'Operand 2' key.\n");
  if (conditionJs["Operand 2"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Condition operand 2 must be an integer number.\n");

  // Creating new condition object
  Condition* condition;
  if (dtype == dt_byte) condition = new _vCondition<byte>(operation, property, conditionJs["Operand 2"].get<byte>());
  if (dtype == dt_sbyte) condition = new _vCondition<sbyte>(operation, property, conditionJs["Operand 2"].get<sbyte>());
  if (dtype == dt_short) condition = new _vCondition<short>(operation, property, conditionJs["Operand 2"].get<short>());
  if (dtype == dt_int) condition = new _vCondition<int>(operation, property, conditionJs["Operand 2"].get<int>());
  if (dtype == dt_word) condition = new _vCondition<word>(operation, property, conditionJs["Operand 2"].get<word>());
  if (dtype == dt_dword) condition = new _vCondition<dword>(operation, property, conditionJs["Operand 2"].get<dword>());

  // Adding condition to the list
  _conditions.push_back(condition);
 }
}

op_t Rule::getOperationType(const std::string& operation)
{
 if (operation == "==") return op_equal;
 if (operation == "!=") return op_not_equal;
 if (operation == ">") return op_greater;
 if (operation == ">=") return op_greater_or_equal;
 if (operation == ">") return op_less;
 if (operation == ">=") return op_less_or_equal;

 EXIT_WITH_ERROR("[Error] Unrecognized (smooth) operator: %s\n", operation);

 return op_equal;
}

datatype_t Rule::getPropertyType(const std::string& property)
{
 if (property == "Kid Frame") return dt_byte;
 if (property == "Kid Current HP") return dt_word;
 if (property == "Kid Max HP") return dt_word;
 if (property == "Kid Position X") return dt_byte;
 if (property == "Kid Position Y") return dt_byte;
 if (property == "Kid Direction") return dt_sbyte;
 if (property == "Kid Current Column") return dt_sbyte;
 if (property == "Kid Current Row") return dt_sbyte;
 if (property == "Kid Action") return dt_byte;
 if (property == "Kid Fall Velocity X") return dt_sbyte;
 if (property == "Kid Fall Velocity Y") return dt_sbyte;
 if (property == "Kid Room") return dt_byte;
 if (property == "Kid Repeat") return dt_byte;
 if (property == "Kid Character Id") return dt_byte;
 if (property == "Kid Has Sword") return dt_byte;
 if (property == "Kid Is Alive") return dt_sbyte;
 if (property == "Kid Current Sequence") return dt_word;

 if (property == "Guard Frame") return dt_byte;
 if (property == "Guard Current HP") return dt_word;
 if (property == "Guard Max HP") return dt_word;
 if (property == "Guard Position X") return dt_byte;
 if (property == "Guard Position Y") return dt_byte;
 if (property == "Guard Direction") return dt_sbyte;
 if (property == "Guard Current Column") return dt_sbyte;
 if (property == "Guard Current Row") return dt_sbyte;
 if (property == "Guard Action") return dt_byte;
 if (property == "Guard Fall Velocity X") return dt_sbyte;
 if (property == "Guard Fall Velocity Y") return dt_sbyte;
 if (property == "Guard Room") return dt_byte;
 if (property == "Guard Repeat") return dt_byte;
 if (property == "Guard Character Id") return dt_byte;
 if (property == "Guard Has Sword") return dt_byte;
 if (property == "Guard Is Alive") return dt_sbyte;
 if (property == "Guard Current Sequence") return dt_word;

 if (property == "Current Level") return dt_word;
 if (property == "Next Level") return dt_word;
 if (property == "Drawn Room") return dt_word;
 if (property == "Exit Door Open") return dt_word;
 if (property == "Checkpoint Reached") return dt_word;
 if (property == "Is Upside Down") return dt_word;
 if (property == "Is Feather Fall") return dt_word;

 EXIT_WITH_ERROR("[Error] Unrecognized property: %s\n", property);

 return dt_byte;
}

void* Rule::getPropertyPointer(const std::string& property, SDLPopInstance* sdlPop)
{
 if (property == "Kid Frame") return &sdlPop->Kid->frame;
 if (property == "Kid Current HP") return sdlPop->hitp_curr;
 if (property == "Kid Max HP") return sdlPop->hitp_max;
 if (property == "Kid Position X") return &sdlPop->Kid->x;
 if (property == "Kid Position Y") return &sdlPop->Kid->y;
 if (property == "Kid Direction") return &sdlPop->Kid->direction;
 if (property == "Kid Current Column") return &sdlPop->Kid->curr_col;
 if (property == "Kid Current Row") return &sdlPop->Kid->curr_row;
 if (property == "Kid Action") return &sdlPop->Kid->action;
 if (property == "Kid Fall Velocity X") return &sdlPop->Kid->fall_x;
 if (property == "Kid Fall Velocity Y") return &sdlPop->Kid->fall_y;
 if (property == "Kid Room") return &sdlPop->Kid->room;
 if (property == "Kid Repeat") return &sdlPop->Kid->repeat;
 if (property == "Kid Character Id") return &sdlPop->Kid->charid;
 if (property == "Kid Has Sword") return &sdlPop->Kid->sword;
 if (property == "Kid Is Alive") return &sdlPop->Kid->alive;
 if (property == "Kid Current Sequence") return &sdlPop->Kid->curr_seq;

 if (property == "Guard Frame") return &sdlPop->Guard->frame;
 if (property == "Guard Current HP") return sdlPop->hitp_curr;
 if (property == "Guard Max HP") return sdlPop->hitp_max;
 if (property == "Guard Position X") return &sdlPop->Guard->x;
 if (property == "Guard Position Y") return &sdlPop->Guard->y;
 if (property == "Guard Direction") return &sdlPop->Guard->direction;
 if (property == "Guard Current Column") return &sdlPop->Guard->curr_col;
 if (property == "Guard Current Row") return &sdlPop->Guard->curr_row;
 if (property == "Guard Action") return &sdlPop->Guard->action;
 if (property == "Guard Fall Velocity X") return &sdlPop->Guard->fall_x;
 if (property == "Guard Fall Velocity Y") return &sdlPop->Guard->fall_y;
 if (property == "Guard Room") return &sdlPop->Guard->room;
 if (property == "Guard Repeat") return &sdlPop->Guard->repeat;
 if (property == "Guard Character Id") return &sdlPop->Guard->charid;
 if (property == "Guard Has Sword") return &sdlPop->Guard->sword;
 if (property == "Guard Is Alive") return &sdlPop->Guard->alive;
 if (property == "Guard Current Sequence") return &sdlPop->Guard->curr_seq;

 if (property == "Current Level") return sdlPop->current_level;
 if (property == "Next Level") return sdlPop->next_level;
 if (property == "Drawn Room") return sdlPop->drawn_room;
 if (property == "Exit Door Open") return sdlPop->leveldoor_open;
 if (property == "Checkpoint Reached") return sdlPop->checkpoint;
 if (property == "Is Upside Down") return sdlPop->upside_down;
 if (property == "Is Feather Fall") return sdlPop->is_feather_fall;

 EXIT_WITH_ERROR("[Error] Unrecognized property: %s\n", property);

 return NULL;
}
