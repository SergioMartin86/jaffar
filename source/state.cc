#include "state.h"
#include "utils.h"

extern nlohmann::json _scriptJs;
size_t _currentStep;

template <class T>
void AddItem(std::vector<Item> *dest, T &val, ItemType type)
{
  dest->push_back({&val, sizeof(val), type});
}

std::vector<Item> GenerateDifferentialItemsMap(miniPoPInstance *_miniPop)
{
  std::vector<Item> dest;
  AddItem(&dest, gameState.quick_control, PER_FRAME_STATE);
  AddItem(&dest, gameState.level, HASHABLE_MANUAL);
  AddItem(&dest, gameState.checkpoint, PER_FRAME_STATE);
  AddItem(&dest, gameState.upside_down, PER_FRAME_STATE);
  AddItem(&dest, gameState.drawn_room, HASHABLE);
  AddItem(&dest, gameState.current_level, PER_FRAME_STATE);
  AddItem(&dest, gameState.next_level, PER_FRAME_STATE);
  AddItem(&dest, gameState.mobs_count, HASHABLE_MANUAL);
  AddItem(&dest, gameState.mobs, HASHABLE_MANUAL);
  AddItem(&dest, gameState.trobs_count, HASHABLE_MANUAL);
  AddItem(&dest, gameState.trobs, HASHABLE_MANUAL);
  return dest;
}

std::vector<Item> GenerateFixedItemsMap(miniPoPInstance *_miniPop)
{
  std::vector<Item> dest;
  AddItem(&dest, gameState.leveldoor_open, HASHABLE);
  AddItem(&dest, gameState.Kid, HASHABLE);
  AddItem(&dest, gameState.hitp_curr, HASHABLE_MANUAL);
  AddItem(&dest, gameState.hitp_max, PER_FRAME_STATE);
  AddItem(&dest, gameState.hitp_beg_lev, PER_FRAME_STATE);
  AddItem(&dest, gameState.grab_timer, HASHABLE);
  AddItem(&dest, gameState.holding_sword, HASHABLE);
  AddItem(&dest, gameState.united_with_shadow, HASHABLE);
  AddItem(&dest, gameState.have_sword, HASHABLE);
  AddItem(&dest, gameState.kid_sword_strike, HASHABLE);
  AddItem(&dest, gameState.pickup_obj_type, PER_FRAME_STATE);
  AddItem(&dest, gameState.offguard, HASHABLE);
  AddItem(&dest, gameState.Guard, PER_FRAME_STATE);
  AddItem(&dest, gameState.Char, PER_FRAME_STATE);
  AddItem(&dest, gameState.Opp, PER_FRAME_STATE);
  AddItem(&dest, gameState.guardhp_curr, PER_FRAME_STATE);
  AddItem(&dest, gameState.guardhp_max, PER_FRAME_STATE);
  AddItem(&dest, gameState.demo_index, PER_FRAME_STATE);
  AddItem(&dest, gameState.demo_time, PER_FRAME_STATE);
  AddItem(&dest, gameState.curr_guard_color, PER_FRAME_STATE);
  AddItem(&dest, gameState.guard_notice_timer, HASHABLE);
  AddItem(&dest, gameState.guard_skill, PER_FRAME_STATE);
  AddItem(&dest, gameState.shadow_initialized, PER_FRAME_STATE);
  AddItem(&dest, gameState.guard_refrac, HASHABLE);
  AddItem(&dest, gameState.justblocked, HASHABLE);
  AddItem(&dest, gameState.droppedout, HASHABLE);
  AddItem(&dest, gameState.curr_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, gameState.curr_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, gameState.below_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, gameState.below_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, gameState.above_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, gameState.above_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, gameState.prev_collision_row, PER_FRAME_STATE);
  AddItem(&dest, gameState.flash_color, PER_FRAME_STATE);
  AddItem(&dest, gameState.flash_time, PER_FRAME_STATE);
  AddItem(&dest, gameState.need_level1_music, HASHABLE);
  AddItem(&dest, gameState.is_screaming, HASHABLE);
  AddItem(&dest, gameState.is_feather_fall, HASHABLE);
  AddItem(&dest, gameState.last_loose_sound, PER_FRAME_STATE);
  AddItem(&dest, gameState.random_seed, PER_FRAME_STATE);
  AddItem(&dest, gameState.rem_min, PER_FRAME_STATE);
  AddItem(&dest, gameState.rem_tick, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_x, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_y, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_shift, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_forward, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_backward, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_up, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_down, PER_FRAME_STATE);
  AddItem(&dest, gameState.control_shift2, PER_FRAME_STATE);
  AddItem(&dest, gameState.ctrl1_forward, PER_FRAME_STATE);
  AddItem(&dest, gameState.ctrl1_backward, PER_FRAME_STATE);
  AddItem(&dest, gameState.ctrl1_up, PER_FRAME_STATE);
  AddItem(&dest, gameState.ctrl1_down, PER_FRAME_STATE);
  AddItem(&dest, gameState.ctrl1_shift2, PER_FRAME_STATE);
  AddItem(&dest, gameState.exit_room_timer, PER_FRAME_STATE);
  AddItem(&dest, gameState.replay_curr_tick, PER_FRAME_STATE);
  AddItem(&dest, gameState.is_guard_notice, PER_FRAME_STATE);
  AddItem(&dest, gameState.can_guard_see_kid, PER_FRAME_STATE);
  return dest;
}

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

  // Generating hash items map
  _differentialItems = GenerateDifferentialItemsMap(_miniPop);
  _fixedItems = GenerateFixedItemsMap(_miniPop);

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
