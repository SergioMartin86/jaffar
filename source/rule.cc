#include "rule.h"

extern size_t _currentStep;

Rule::Rule(nlohmann::json ruleJs, miniPoPInstance *sdlPop)
{
  // Adding identifying label for the rule
  if (isDefined(ruleJs, "Label") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Label' key.\n");
  _label = ruleJs["Label"].get<size_t>();

  // Defining default values
  _reward = 0.0;
  _isWinRule = false;
  _isFailRule = false;

  // Adding conditions. All of them must be satisfied for the rule to count
  if (isDefined(ruleJs, "Conditions") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Conditions' key.\n");
  for (size_t i = 0; i < ruleJs["Conditions"].size(); i++)
  {
    auto conditionJs = ruleJs["Conditions"][i];

    // Parsing operation type
    if (isDefined(conditionJs, "Op") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu condition missing 'Op' key.\n", _label);
    operator_t operation = getOperationType(conditionJs["Op"].get<std::string>());

    // Parsing first operand (property name)
    if (isDefined(conditionJs, "Property") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu condition missing 'Property' key.\n", _label);
    if (conditionJs["Property"].is_string() == false) EXIT_WITH_ERROR("[ERROR] Rule %lu condition operand 1 must be a string with the name of a property.\n", _label);
    datatype_t dtype = getPropertyType(conditionJs["Property"].get<std::string>());

    int index = -1;
    if (isDefined(conditionJs, "Index") == true)
    {
     if (conditionJs["Index"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Rule %lu tile index must be an integer.\n", _label);
     index = conditionJs["Index"].get<int>();
    }
    auto property = getPropertyPointer(conditionJs["Property"].get<std::string>(), sdlPop, index);

    // Parsing second operand (number)
    if (isDefined(conditionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu condition missing 'Value' key.\n", _label);

    bool valueFound = false;
    if (conditionJs["Value"].is_number())
    {
     // Creating new condition object
     Condition *condition;
     if (dtype == dt_byte) condition = new _vCondition<byte>(operation, property, NULL, conditionJs["Value"].get<byte>());
     if (dtype == dt_sbyte) condition = new _vCondition<sbyte>(operation, property, NULL, conditionJs["Value"].get<sbyte>());
     if (dtype == dt_short) condition = new _vCondition<short>(operation, property, NULL, conditionJs["Value"].get<short>());
     if (dtype == dt_int) condition = new _vCondition<int>(operation, property, NULL, conditionJs["Value"].get<int>());
     if (dtype == dt_word) condition = new _vCondition<word>(operation, property, NULL, conditionJs["Value"].get<word>());
     if (dtype == dt_dword) condition = new _vCondition<dword>(operation, property, NULL, conditionJs["Value"].get<dword>());
     if (dtype == dt_ulong) condition = new _vCondition<size_t>(operation, property, NULL, conditionJs["Value"].get<size_t>());

     // Adding condition to the list
     _conditions.push_back(condition);

     valueFound = true;
    }

    if (conditionJs["Value"].is_string())
    {
     // Creating new property
     datatype_t valueType = getPropertyType(conditionJs["Value"].get<std::string>());
     if (valueType != dtype) EXIT_WITH_ERROR("[ERROR] Rule %lu, property (%s) and value (%s) types must coincide.\n", _label, conditionJs["Property"].get<std::string>(), conditionJs["Value"].get<std::string>());

     // Getting value pointer
     auto valuePtr = getPropertyPointer(conditionJs["Value"].get<std::string>(), sdlPop, index);

     // Adding condition to the list
     Condition *condition;
     if (dtype == dt_byte) condition = new _vCondition<byte>(operation, property, valuePtr, 0);
     if (dtype == dt_sbyte) condition = new _vCondition<sbyte>(operation, property, valuePtr, 0);
     if (dtype == dt_short) condition = new _vCondition<short>(operation, property, valuePtr, 0);
     if (dtype == dt_int) condition = new _vCondition<int>(operation, property, valuePtr, 0);
     if (dtype == dt_word) condition = new _vCondition<word>(operation, property, valuePtr, 0);
     if (dtype == dt_dword) condition = new _vCondition<dword>(operation, property, valuePtr, 0);
     if (dtype == dt_ulong) condition = new _vCondition<size_t>(operation, property, valuePtr, 0);
     _conditions.push_back(condition);

     valueFound = true;
    }

    if (valueFound == false) EXIT_WITH_ERROR("[ERROR] Rule %lu contains an invalid 'Value' key.\n", _label, conditionJs["Value"].dump().c_str());
  }

  // Adding Dependencies. All of them must be achieved for the rule to count
  if (isDefined(ruleJs, "Dependencies") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Dependencies' key.\n");
  _dependenciesLabels = ruleJs["Dependencies"].get<std::vector<size_t>>();
  _dependenciesIndexes.resize(_dependenciesLabels.size());

  // Adding Rules that are satisfied by this rule activation
  if (isDefined(ruleJs, "Satisfies") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Satisfies' key.\n");
  _satisfiesLabels = ruleJs["Satisfies"].get<std::vector<size_t>>();
  _satisfiesIndexes.resize(_satisfiesLabels.size());

  // Adding actions
  if (isDefined(ruleJs, "Actions") == false) EXIT_WITH_ERROR("[ERROR] Rule missing 'Actions' key.\n");
  parseActions(ruleJs["Actions"]);
}

void Rule::parseActions(nlohmann::json actionsJs)
{
 for (size_t actionId = 0; actionId < actionsJs.size(); actionId++)
 {
   const auto actionJs = actionsJs[actionId];

   // Getting action type
   if (isDefined(actionJs, "Type") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Type' key.\n", _label, actionId);
   std::string actionType = actionJs["Type"].get<std::string>();

   // Running the action, depending on the type
   bool recognizedActionType = false;

   if (actionType == "Add Reward")
   {
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     _reward = actionJs["Value"].get<float>();
     recognizedActionType = true;
   }

   // Storing fail state
   if (actionType == "Trigger Fail")
   {
     _isFailRule = true;
     recognizedActionType = true;
   }

   // Storing win state
   if (actionType == "Trigger Win")
   {
     _isWinRule = true;
     recognizedActionType = true;
   }

   if (actionType == "Set Kid Horizontal Magnet Intensity")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _kidMagnetIntensityX.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (actionType == "Set Kid Horizontal Magnet Position")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _kidMagnetPositionX.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (actionType == "Set Kid Vertical Magnet Intensity")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _kidMagnetIntensityY.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (actionType == "Set Guard Horizontal Magnet Intensity")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _guardMagnetIntensityX.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (actionType == "Set Guard Horizontal Magnet Position")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _guardMagnetPositionX.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (actionType == "Set Guard Vertical Magnet Intensity")
   {
     magnet_t newMagnet;
     if (isDefined(actionJs, "Room") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Room' key.\n", _label, actionId);
     if (isDefined(actionJs, "Value") == false) EXIT_WITH_ERROR("[ERROR] Rule %lu Action %lu missing 'Value' key.\n", _label, actionId);
     newMagnet.value = actionJs["Value"].get<float>();
     newMagnet.room = actionJs["Room"].get<byte>();
     _guardMagnetIntensityY.push_back(newMagnet);
     recognizedActionType = true;
   }

   if (recognizedActionType == false) EXIT_WITH_ERROR("[ERROR] Unrecognized rule %lu, action %lu type: %s\n", _label, actionId, actionType.c_str());
  }
}

bool Rule::evaluate()
{
  bool isAchieved = true;

  // The rule is achieved only if all conditions are met
  for (size_t i = 0; i < _conditions.size(); i++)
    isAchieved = isAchieved && _conditions[i]->evaluate();

  return isAchieved;
}

operator_t Rule::getOperationType(const std::string &operation)
{
  if (operation == "==") return op_equal;
  if (operation == "!=") return op_not_equal;
  if (operation == ">") return op_greater;
  if (operation == ">=") return op_greater_or_equal;
  if (operation == "<") return op_less;
  if (operation == "<=") return op_less_or_equal;

  EXIT_WITH_ERROR("[Error] Rule %lu, unrecognized operator: %s\n", _label, operation.c_str());

  return op_equal;
}

datatype_t Rule::getPropertyType(const std::string &property)
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
  if (property == "Checkpoint Reached") return dt_word;
  if (property == "Is Upside Down") return dt_word;
  if (property == "Needs Level 1 Music") return dt_word;
  if (property == "United With Shadow") return dt_short;
  if (property == "Exit Door Timer") return dt_word;
  if (property == "Current Step") return dt_ulong;

  // Tile Configuration
  if (property == "Tile FG State") return dt_byte;
  if (property == "Tile BG State") return dt_byte;


  EXIT_WITH_ERROR("[Error] Rule %lu, unrecognized property: %s\n", _label, property.c_str());

  return dt_byte;
}

void *Rule::getPropertyPointer(const std::string &property, miniPoPInstance *sdlPop, const int index)
{
  if (property == "Kid Frame") return &Kid.frame;
  if (property == "Kid Current HP") return &hitp_curr;
  if (property == "Kid Max HP") return &hitp_max;
  if (property == "Kid Position X") return &Kid.x;
  if (property == "Kid Position Y") return &Kid.y;
  if (property == "Kid Direction") return &Kid.direction;
  if (property == "Kid Current Column") return &Kid.curr_col;
  if (property == "Kid Current Row") return &Kid.curr_row;
  if (property == "Kid Action") return &Kid.action;
  if (property == "Kid Fall Velocity X") return &Kid.fall_x;
  if (property == "Kid Fall Velocity Y") return &Kid.fall_y;
  if (property == "Kid Room") return &Kid.room;
  if (property == "Kid Repeat") return &Kid.repeat;
  if (property == "Kid Character Id") return &Kid.charid;
  if (property == "Kid Has Sword") return &Kid.sword;
  if (property == "Kid Is Alive") return &Kid.alive;
  if (property == "Kid Current Sequence") return &Kid.curr_seq;

  if (property == "Guard Frame") return &Guard.frame;
  if (property == "Guard Current HP") return &guardhp_curr;
  if (property == "Guard Max HP") return &guardhp_max;
  if (property == "Guard Position X") return &Guard.x;
  if (property == "Guard Position Y") return &Guard.y;
  if (property == "Guard Direction") return &Guard.direction;
  if (property == "Guard Current Column") return &Guard.curr_col;
  if (property == "Guard Current Row") return &Guard.curr_row;
  if (property == "Guard Action") return &Guard.action;
  if (property == "Guard Fall Velocity X") return &Guard.fall_x;
  if (property == "Guard Fall Velocity Y") return &Guard.fall_y;
  if (property == "Guard Room") return &Guard.room;
  if (property == "Guard Repeat") return &Guard.repeat;
  if (property == "Guard Character Id") return &Guard.charid;
  if (property == "Guard Has Sword") return &Guard.sword;
  if (property == "Guard Is Alive") return &Guard.alive;
  if (property == "Guard Current Sequence") return &Guard.curr_seq;

  if (property == "Current Level") return &current_level;
  if (property == "Next Level") return &next_level;
  if (property == "Drawn Room") return &drawn_room;
  if (property == "Checkpoint Reached") return &checkpoint;
  if (property == "Is Upside Down") return &upside_down;
  if (property == "Is Feather Fall") return &is_feather_fall;
  if (property == "Needs Level 1 Music") return &need_level1_music;
  if (property == "United With Shadow") return &united_with_shadow;
  if (property == "Exit Door Timer") return &leveldoor_open;
  if (property == "Current Step") return &_currentStep;

  if (property == "Tile FG State") { if (index == -1) EXIT_WITH_ERROR("[ERROR] Invalid or missing index for Tile FG State.\n"); return &level.fg[index]; }
  if (property == "Tile BG State") { if (index == -1) EXIT_WITH_ERROR("[ERROR] Invalid or missing index for Tile BG State.\n"); return &level.bg[index]; }

  EXIT_WITH_ERROR("[Error] Rule %lu, unrecognized property: %s\n", _label, property.c_str());

  return NULL;
}
