#include "state.h"
#include "frame.h"
#include "metrohash64.h"
#include "utils.h"

extern nlohmann::json _scriptJs;

size_t _currentStep;
char quick_control[] = "........";
float replay_curr_tick = 0.0;

template <class T>
void AddItem(std::vector<Item> *dest, T &val, ItemType type)
{
  dest->push_back({&val, sizeof(val), type});
}

std::vector<Item> GenerateItemsMap(miniPoPInstance *_miniPop)
{
  std::vector<Item> dest;
  AddItem(&dest, quick_control, PER_FRAME_STATE);
  AddItem(&dest, level, HASHABLE_MANUAL);
  AddItem(&dest, checkpoint, PER_FRAME_STATE);
  AddItem(&dest, upside_down, PER_FRAME_STATE);
  AddItem(&dest, drawn_room, HASHABLE);
  AddItem(&dest, current_level, PER_FRAME_STATE);
  AddItem(&dest, next_level, PER_FRAME_STATE);
  AddItem(&dest, mobs_count, HASHABLE_MANUAL);
  AddItem(&dest, mobs, HASHABLE_MANUAL);
  AddItem(&dest, trobs_count, HASHABLE_MANUAL);
  AddItem(&dest, trobs, HASHABLE_MANUAL);
  AddItem(&dest, leveldoor_open, HASHABLE);
  AddItem(&dest, Kid, HASHABLE);
  AddItem(&dest, hitp_curr, HASHABLE_MANUAL);
  AddItem(&dest, hitp_max, PER_FRAME_STATE);
  AddItem(&dest, hitp_beg_lev, PER_FRAME_STATE);
  AddItem(&dest, grab_timer, HASHABLE);
  AddItem(&dest, holding_sword, HASHABLE);
  AddItem(&dest, united_with_shadow, HASHABLE);
  AddItem(&dest, have_sword, HASHABLE);
  /*AddItem(&dest, ctrl1_forward, HASHABLE);
  AddItem(&dest, ctrl1_backward, HASHABLE);
  AddItem(&dest, ctrl1_up, HASHABLE);
  AddItem(&dest, ctrl1_down, HASHABLE);
  AddItem(&dest, ctrl1_shift2, HASHABLE);*/
  AddItem(&dest, kid_sword_strike, HASHABLE);
  AddItem(&dest, pickup_obj_type, PER_FRAME_STATE);
  AddItem(&dest, offguard, HASHABLE);
  // guard
  AddItem(&dest, Guard, PER_FRAME_STATE);
  AddItem(&dest, Char, PER_FRAME_STATE);
  AddItem(&dest, Opp, PER_FRAME_STATE);
  AddItem(&dest, guardhp_curr, PER_FRAME_STATE);
  AddItem(&dest, guardhp_max, PER_FRAME_STATE);
  AddItem(&dest, demo_index, PER_FRAME_STATE);
  AddItem(&dest, demo_time, PER_FRAME_STATE);
  AddItem(&dest, curr_guard_color, PER_FRAME_STATE);
  AddItem(&dest, guard_notice_timer, HASHABLE);
  AddItem(&dest, guard_skill, PER_FRAME_STATE);
  AddItem(&dest, shadow_initialized, PER_FRAME_STATE);
  AddItem(&dest, guard_refrac, HASHABLE);
  AddItem(&dest, justblocked, HASHABLE);
  AddItem(&dest, droppedout, HASHABLE);
  // collision
  AddItem(&dest, curr_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, curr_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, below_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, below_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, above_row_coll_room, PER_FRAME_STATE);
  AddItem(&dest, above_row_coll_flags, PER_FRAME_STATE);
  AddItem(&dest, prev_collision_row, PER_FRAME_STATE);
  // flash
  AddItem(&dest, flash_color, PER_FRAME_STATE);
  AddItem(&dest, flash_time, PER_FRAME_STATE);
  // sounds
  AddItem(&dest, need_level1_music, HASHABLE);
  AddItem(&dest, is_screaming, HASHABLE);
  AddItem(&dest, is_feather_fall, PER_FRAME_STATE);
  AddItem(&dest, last_loose_sound, PER_FRAME_STATE);
  // AddItem(&dest, next_sound, HASHABLE);
  // AddItem(&dest, current_sound, HASHABLE);
  // random
  AddItem(&dest, random_seed, PER_FRAME_STATE);
  // remaining time
  AddItem(&dest, rem_min, PER_FRAME_STATE);
  AddItem(&dest, rem_tick, PER_FRAME_STATE);
  // saved controls
  AddItem(&dest, control_x, PER_FRAME_STATE);
  AddItem(&dest, control_y, PER_FRAME_STATE);
  AddItem(&dest, control_shift, PER_FRAME_STATE);
  AddItem(&dest, control_forward, PER_FRAME_STATE);
  AddItem(&dest, control_backward, PER_FRAME_STATE);
  AddItem(&dest, control_up, PER_FRAME_STATE);
  AddItem(&dest, control_down, PER_FRAME_STATE);
  AddItem(&dest, control_shift2, PER_FRAME_STATE);
  AddItem(&dest, ctrl1_forward, PER_FRAME_STATE);
  AddItem(&dest, ctrl1_backward, PER_FRAME_STATE);
  AddItem(&dest, ctrl1_up, PER_FRAME_STATE);
  AddItem(&dest, ctrl1_down, PER_FRAME_STATE);
  AddItem(&dest, ctrl1_shift2, PER_FRAME_STATE);
  // Support for overflow glitch
  AddItem(&dest, exit_room_timer, PER_FRAME_STATE);
  // replay recording state
  AddItem(&dest, replay_curr_tick, PER_FRAME_STATE);
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
    int idx = entry["Index"].get<int>();
    if (hashType == "Index Only") _hashTypeTrobs[idx] = INDEX_ONLY;
    if (hashType == "Full") _hashTypeTrobs[idx] = FULL;
   }

  }
  else EXIT_WITH_ERROR("[Error] State Configuration 'Active Objects Hash Types' was not defined\n");

  if (isDefined(stateConfig, "Static Tile Hash Types") == true)
  {
   for (const auto& idx : stateConfig["Static Tile Hash Types"])  _hashTypeStatic.push_back(idx.get<int>());
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

  if (_ruleCount > _MAX_RULE_COUNT) EXIT_WITH_ERROR("[ERROR] Configured Jaffar to run %lu rules, but the specified script contains %lu. Modify frame.h and rebuild to run this level.\n", _MAX_RULE_COUNT, _ruleCount);

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
  _items = GenerateItemsMap(_miniPop);

  // Update the SDLPop instance with the savefile contents
  memcpy(_inputStateData, saveString.data(), _FRAME_DATA_SIZE);
  pushState();

  // Starting level
  _miniPop->startLevel(next_level);

  // Backing up current RNG state
  auto rngState = random_seed;
  auto looseTileSound = last_loose_sound;

  // Processing screen objects that might affect RNG state
  play_frame();

  // Update the SDLPop instance with the savefile contents again
  memcpy(_inputStateData, saveString.data(), _FRAME_DATA_SIZE);
  pushState();

  // If we require seed to be overwitten, do it now:
  if (seed >= 0)
   _miniPop->setSeed(seed);
  else
   _miniPop->setSeed(rngState);

  last_loose_sound = looseTileSound;
}

uint64_t State::computeHash() const
{
  // Storage for hash calculation
  MetroHash64 hash;

  // For items that are automatically hashable, do that now
  for (const auto &item : _items) if (item.type == HASHABLE) hash.Update(item.ptr, item.size);

  // Manual hashing

  hash.Update(_miniPop->isExitDoorOpen);
  hash.Update(level.guards_x);
  hash.Update(level.guards_dir);
  if (Guard.alive) hash.Update(Guard);
  if (_hashKidCurrentHp == true) hash.Update(hitp_curr);

  // Mobs are moving objects (falling tiles only afaik).
  for (int i = 0; i < mobs_count; i++)
  {
   const auto &mob = mobs[i];
   const auto idx = std::make_pair(mob.room, mob.xh);
   if (_hashTypeMobs.count(idx))
   {
    const auto hashType = _hashTypeMobs.at(idx);
    if (hashType == INDEX_ONLY) { hash.Update(mob.room); hash.Update(mob.xh); }
    if (hashType == FULL)  hash.Update(mob);
   }
  }

  // Trobs are stationary animated objects. They only change in state, hence we only read BG
  for (int i = 0; i < trobs_count; ++i)
  {
    const auto &trob = trobs[i];
    const auto idx = (trob.room - 1) * 30 + trob.tilepos;

    if (_hashTypeTrobs.count(idx))
    {
     const auto hashType = _hashTypeTrobs.at(idx);
     if (hashType == INDEX_ONLY) hash.Update(idx);
     if (hashType == FULL) { hash.Update(idx); hash.Update(level.bg[idx]); }
    }
  }

  // Computing hash for static objects. They only change on tile type, hence we only read FG
  for (const auto idx : _hashTypeStatic)  hash.Update(level.fg[idx]);

  uint64_t result;
  hash.Finalize(reinterpret_cast<uint8_t *>(&result));
  return result;
}

void State::pushState()
{
  size_t pos = 0;
  for (const auto &item : _items) { memcpy(item.ptr, &_inputStateData[pos],item.size); pos += item.size; }
  _miniPop->isExitDoorOpen = _miniPop->isLevelExitDoorOpen();

  different_room = 1;
  // Show the room where the prince is, even if the player moved the view away
  // from it (with the H,J,U,N keys).
  next_room = drawn_room = Kid.room;
  load_room_links();
}

void State::popState()
{
  size_t pos = 0;
  for (const auto &item : _items) { memcpy(&_outputStateData[pos], item.ptr, item.size); pos += item.size; }
}

float State::getFrameReward(const Frame &frame)
{
  // Accumulator for total reward
  float reward = getRuleRewards(frame);

  // Getting kid room
  int kidCurrentRoom = Kid.room;

  // Getting magnet values for the kid
  auto kidMagnet = getKidMagnetValues(frame, kidCurrentRoom);

  // Getting kid's current frame
  const auto curKidFrame = Kid.frame;

  // Evaluating kidMagnet's reward on the X axis
  const float kidDiffX = std::abs(Kid.x - kidMagnet.positionX);
  reward += (float) kidMagnet.intensityX * (256.0f - kidDiffX);

  // For positive Y axis kidMagnet, rewarding climbing frames
  if ((float) kidMagnet.intensityY > 0.0f)
  {
    // Jumphang, because it preludes climbing (Score + 1-20)
    if (curKidFrame >= 67 && curKidFrame <= 80)
     reward += (float) kidMagnet.intensityY * (curKidFrame - 66);

    // Hang, because it preludes climbing (Score +21)
    if (curKidFrame == 91) reward += 21.0f * (float) kidMagnet.intensityY;

    // Climbing (Score +22-38)
    if (curKidFrame >= 135 && curKidFrame <= 149) reward += (float) kidMagnet.intensityY * (22.0f + (curKidFrame - 134));

    // Adding absolute reward for Y position
    reward += (float) kidMagnet.intensityY * (256.0f - Kid.y);
  }

  // For negative Y axis kidMagnet, rewarding falling/climbing down frames
  if ((float) kidMagnet.intensityY < 0.0f)
  {
    // Turning around, because it generally preludes climbing down
    if (curKidFrame >= 45 && curKidFrame <= 52) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hanging, because it preludes falling
    if (curKidFrame >= 87 && curKidFrame <= 99) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hang drop, because it preludes falling
    if (curKidFrame >= 81 && curKidFrame <= 85) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling start
    if (curKidFrame >= 102 && curKidFrame <= 105) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling itself
    if (curKidFrame == 106) reward += -2.0f + (float) kidMagnet.intensityY;

    // Climbing down
    if (curKidFrame == 148) reward += -2.0f + (float) kidMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * kidMagnet.intensityY * (Kid.y);
  }

  // Getting guard room
  int guardCurrentRoom = Guard.room;

  // Getting magnet values for the guard
  auto guardMagnet = getGuardMagnetValues(frame, guardCurrentRoom);

  // Getting guard's current frame
  const auto curGuardFrame = Guard.frame;

  // Evaluating guardMagnet's reward on the X axis
  const float guardDiffX = std::abs(Guard.x - guardMagnet.positionX);
  reward += (float) guardMagnet.intensityX * (256.0f - guardDiffX);

  // For positive Y axis guardMagnet
  if ((float) guardMagnet.intensityY > 0.0f)
  {
   // Adding absolute reward for Y position
   reward += (float) guardMagnet.intensityY * (256.0f - Guard.y);
  }

  // For negative Y axis guardMagnet, rewarding falling/climbing down frames
  if ((float) guardMagnet.intensityY < 0.0f)
  {
    // Falling start
    if (curGuardFrame >= 102 && curGuardFrame <= 105) reward += -1.0f * (float) guardMagnet.intensityY;

    // Falling itself
    if (curGuardFrame == 106) reward += -2.0f + (float) guardMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * guardMagnet.intensityY * (Guard.y);
  }

  // Apply bonus when kid is inside a non-visible room
  if (kidCurrentRoom == 0 || kidCurrentRoom >= 25) reward += 128.0f;

  // Apply bonus when kid is climbing exit stairs
  if (curKidFrame >= 217 || curKidFrame <= 228) reward += 128.0f;

  // Returning reward
  return reward;
}

std::vector<uint8_t> State::getPossibleMoveIds(const Frame &frame)
{
  // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13    14
  //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA" };

  // If dead, do nothing
  if (Kid.alive >= 0)
    return {0};

  // For level 1, if kid touches ground and music plays, try restarting level
  if (Kid.frame == 109 && need_level1_music == 33)
    return {0, 14};

  // If bumped, nothing to do
  if (Kid.action == actions_5_bumped)
    return {0};

  // If in mid air or free fall, hope to grab on to something
  if (Kid.action == actions_3_in_midair || Kid.action == actions_4_in_freefall)
    return {0, 1};

  // Move, sheath, attack, parry
  if (Kid.sword == sword_2_drawn)
    return {0, 1, 2, 3, 4, 5};

  // Kid is standing or finishing a turn, try all possibilities
  if (Kid.frame == frame_15_stand || (Kid.frame >= frame_50_turn && Kid.frame < 53))
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  // Turning frame, try all possibilities
  if (Kid.frame == frame_48_turn)
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  // Start running animation, all movement without shift
  if (Kid.frame < 4)
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

  // Starting jump up, check directions, jump and shift
  if (Kid.frame >= frame_67_start_jump_up_1 && Kid.frame < frame_70_jumphang)
    return {0, 1, 2, 3, 4, 5, 6, 8, 12};

  // Running, all movement without shift
  if (Kid.frame < 15)
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

  // Hanging, up and shift are only options
  if (Kid.frame >= frame_87_hanging_1 && Kid.frame < 100)
    return {0, 1, 2, 12};

  // Crouched, can only stand, drink, or hop
  if (Kid.frame == frame_109_crouch)
    return {0, 1, 3, 4, 5, 7, 9, 13};

  // Default, no nothing
  return {0};
}

magnetInfo_t State::getKidMagnetValues(const Frame &frame, const int room)
{

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
 {
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[ruleId];

    for (size_t i = 0; i < _rules[ruleId]->_kidMagnetPositionX.size(); i++)
     if (rule->_kidMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_kidMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[ruleId]->_kidMagnetIntensityX.size(); i++)
     if (rule->_kidMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_kidMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[ruleId]->_kidMagnetIntensityY.size(); i++)
     if (rule->_kidMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_kidMagnetIntensityY[i].value;
  }
 }

 return magnetInfo;
}

magnetInfo_t State::getGuardMagnetValues(const Frame &frame, const int room)
{

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[ruleId];

    for (size_t i = 0; i < _rules[ruleId]->_guardMagnetPositionX.size(); i++)
     if (rule->_guardMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_guardMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[ruleId]->_guardMagnetIntensityX.size(); i++)
     if (rule->_guardMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_guardMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[ruleId]->_guardMagnetIntensityY.size(); i++)
     if (rule->_guardMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_guardMagnetIntensityY[i].value;
  }

 return magnetInfo;
}

void State::evaluateRules(Frame &frame)
{
  for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  {
    // Evaluate rule only if it's active
    if (frame.rulesStatus[ruleId] == false)
    {
      // Checking dependencies first. If not met, continue to the next rule
      bool dependenciesMet = true;
      for (size_t i = 0; i < _rules[ruleId]->_dependenciesIndexes.size(); i++)
        if (frame.rulesStatus[_rules[ruleId]->_dependenciesIndexes[i]] == false)
          dependenciesMet = false;

      // If dependencies aren't met, then continue to next rule
      if (dependenciesMet == false) continue;

      // Checking if conditions are met
      bool isSatisfied = _rules[ruleId]->evaluate();

      // If it's achieved, update its status and run its actions
      if (isSatisfied) satisfyRule(frame, ruleId);
    }
  }
}

float State::getRuleRewards(const Frame &frame)
{
 float reward = 0.0;

 for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
    reward += _rules[ruleId]->_reward;
 return reward;
}

void State::satisfyRule(Frame &frame, const size_t ruleId)
{
 // Recursively run actions for the yet unsatisfied rules that are satisfied by this one and mark them as satisfied
 for (size_t satisfiedIdx = 0; satisfiedIdx < _rules[ruleId]->_satisfiesIndexes.size(); satisfiedIdx++)
 {
  // Obtaining index
  size_t subRuleId = _rules[ruleId]->_satisfiesIndexes[satisfiedIdx];

  // Only activate it if it hasn't been activated before
  if (frame.rulesStatus[subRuleId] == false) satisfyRule(frame, subRuleId);
 }

 // Setting status to satisfied
 frame.rulesStatus[ruleId] = true;

 if (_rules[ruleId]->_isFlushDBRule == true) frame._type = f_flush;
 if (_rules[ruleId]->_isFailRule == true) frame._type = f_fail;
 if (_rules[ruleId]->_isWinRule == true) frame._type = f_win;
}

void State::printRuleStatus(const Frame &frame)
{
 printf("[Jaffar]  + Rule Status: ");
 for (size_t i = 0; i < _ruleCount; i++)
 {
   if (i > 0 && i % 60 == 0) printf("\n                         ");
   printf("%d", frame.rulesStatus[i] ? 1 : 0);
 }
 printf("\n");
}
