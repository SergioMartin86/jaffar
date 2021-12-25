#include "state.h"
#include "utils.h"

extern nlohmann::json _scriptJs;
size_t _currentStep;

State::State(const std::string& saveString, const nlohmann::json stateConfig, const nlohmann::json rulesConfig, const int seed)
{
  // Setting hash types
  _hashKidCurrentHp = false;
  if (isDefined(stateConfig, "Property Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Property Hash Types"])
   {
    if (entry == "Kid Current HP") _hashKidCurrentHp = true;
    if (entry == "Guard Current HP") _hashGuardCurrentHp = true;
    if (entry == "Trob Count") _hashTrobCount = true;
   }
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Property Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Falling Tiles Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Falling Tiles Hash Types"])
   {
    std::string hashType = entry["Type"].get<std::string>();
    int room = entry["Room"].get<int>();
    int column = entry["Column"].get<int>();
    auto idx = std::make_pair(room, column);
    if (hashType == "Index Only") _hashTypeMobs[idx] = INDEX_ONLY;
    if (hashType == "Full") _hashTypeMobs[idx] = FULL;
   }
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Falling Tiles Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Active Objects Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Active Objects Hash Types"])
   {
    std::string hashType = entry["Type"].get<std::string>();

    int room = -1;
    if (isDefined(entry, "Room") == true)
    {
     if (entry["Room"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Active Objects Hash Types room must be an integer.\n");
     room = entry["Room"].get<int>();
    }

    int tile = -1;
    if (isDefined(entry, "Tile") == true)
    {
     if (entry["Tile"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Active Objects Hash Types tile must be an integer.\n");
     tile = entry["Tile"].get<int>();
    }

    int idx = (room-1)*30 + (tile-1);
    if (hashType == "Index Only") _hashTypeTrobs[idx] = INDEX_ONLY;
    if (hashType == "Full") _hashTypeTrobs[idx] = FULL;
   }

  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Active Objects Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Static Tile Hash Types") == true)
  {
   for (const auto& entry : stateConfig["Static Tile Hash Types"])
   {
     int room = -1;
     if (isDefined(entry, "Room") == true)
     {
      if (entry["Room"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Active Objects Hash Types room must be an integer.\n");
      room = entry["Room"].get<int>();
     }

     int tile = -1;
     if (isDefined(entry, "Tile") == true)
     {
      if (entry["Tile"].is_number() == false) EXIT_WITH_ERROR("[ERROR] Active Objects Hash Types tile must be an integer.\n");
      tile = entry["Tile"].get<int>();
     }

     int idx = (room-1)*30 + (tile-1);

    _hashTypeStatic.push_back(idx);
   }
  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Static Tile Hash Types' was not defined\n");

  // Creating mini pop instance
  _miniPop = new miniPoPInstance();
  _miniPop->initialize();

  // Processing rules
  for (size_t ruleId = 0; ruleId < rulesConfig.size(); ruleId++)
   _rules.push_back(new Rule(rulesConfig[ruleId], _miniPop));

  // Setting global rule count
  _ruleCount = _rules.size();

  if (_ruleCount > _MAX_RULE_COUNT) EXIT_WITH_ERROR("[ERROR] Configured Jaffar to run %lu rules, but the specified script contains %lu. Modify frame.h and rebuild to run this script.\n", _MAX_RULE_COUNT, _ruleCount);

  // Checking for repeated rule labels
  std::set<size_t> ruleLabelSet;
  for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
  {
   size_t label = _rules[ruleId]->_label;
   ruleLabelSet.insert(label);
   if (ruleLabelSet.size() < ruleId + 1)
    EXIT_WITH_ERROR("[ERROR] Rule label %lu is repeated in the configuration file.\n", label);
  }

  // Looking for rule dependency indexes that match their labels
  for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   for (size_t depId = 0; depId < _rules[ruleId]->_dependenciesLabels.size(); depId++)
   {
    bool foundLabel = false;
    size_t label = _rules[ruleId]->_dependenciesLabels[depId];
    if (label == _rules[ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in dependencies vector.\n", label);
    for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
     if (_rules[subRuleId]->_label == label)
     {
      _rules[ruleId]->_dependenciesIndexes[depId] = subRuleId;
      foundLabel = true;
      break;
     }
    if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as dependency by rule %lu.\n", label, _rules[ruleId]->_label);
   }

  // Looking for rule satisfied sub-rules indexes that match their labels
  for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   for (size_t satisfiedId = 0; satisfiedId < _rules[ruleId]->_satisfiesLabels.size(); satisfiedId++)
   {
    bool foundLabel = false;
    size_t label = _rules[ruleId]->_satisfiesLabels[satisfiedId];
    if (label == _rules[ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in satisfied vector.\n", label);

    for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
     if (_rules[subRuleId]->_label == label)
     {
      _rules[ruleId]->_satisfiesIndexes[satisfiedId] = subRuleId;
      foundLabel = true;
      break;
     }
    if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as satisfied by rule %lu.\n", label, satisfiedId);
   }

  // Update the SDLPop instance with the savefile contents
  pushState(saveString.data());

  // Starting level
  _miniPop->startLevel(gameState.next_level);

  // Backing up current RNG state
  auto rngState = gameState.random_seed;
  auto looseTileSound = gameState.last_loose_sound;

  // Processing screen objects that might affect RNG state
  play_frame();

  // Update the SDLPop instance with the savefile contents again
  pushState(saveString.data());

  // If we require seed to be overwitten, do it now:
  if (seed >= 0)
   _miniPop->setSeed(seed);
  else
   _miniPop->setSeed(rngState);

  gameState.last_loose_sound = looseTileSound;
}

void State::printRuleStatus(const bool* rulesStatus)
{
 printf("[Jaffar]  + Rule Status: ");
 for (size_t i = 0; i < _ruleCount; i++)
 {
   if (i > 0 && i % 60 == 0) printf("\n                         ");
   printf("%d", rulesStatus[i] ? 1 : 0);
 }
 printf("\n");
}
