#pragma once

#include "miniPoPInstance.h"
#include "nlohmann/json.hpp"
#include "metrohash64.h"
#include <cstddef>
#include "frame.h"
#include <string>
#include <vector>
#include <set>

enum hashType
{
  NONE,
  INDEX_ONLY,
  FULL,
};

// Struct to hold all of the frame's magnet information
struct magnetInfo_t
{
 float positionX;
 float intensityX;
 float intensityY;
};

class State
{
  public:

  State(const std::string& miniPopStateData, const nlohmann::json stateConfig, const nlohmann::json rulesConfig, const int seed = -1);

  // Print Rule information
  void printRuleStatus(const bool* rulesStatus);

  // Hash states
  bool _hashKidCurrentHp;
  bool _hashGuardCurrentHp;
  bool _hashTrobCount;

  std::map<int, hashType> _hashTypeTrobs;
  std::vector<int> _hashTypeStatic;
  std::map<std::pair<int, int>, hashType> _hashTypeMobs;
  std::vector<Rule *> _rules;
  size_t _ruleCount;
  miniPoPInstance *_miniPop;

  // This function computes the hash for the current state
  inline uint64_t computeHash() const
  {
    // Storage for hash calculation
    MetroHash64 hash;

    // Adding fixed hash elements
    hash.Update(gameState.drawn_room);
    hash.Update(gameState.leveldoor_open);
    hash.Update(gameState.Kid);
    hash.Update(gameState.Guard);
    hash.Update(gameState.grab_timer);
    hash.Update(gameState.holding_sword);
    hash.Update(gameState.united_with_shadow);
    hash.Update(gameState.have_sword);
    hash.Update(gameState.kid_sword_strike);
    hash.Update(gameState.offguard);
    hash.Update(gameState.guard_notice_timer);
    hash.Update(gameState.guard_refrac);
    hash.Update(gameState.justblocked);
    hash.Update(gameState.droppedout);
    hash.Update(gameState.need_level1_music);
    hash.Update(gameState.is_screaming);
    hash.Update(gameState.is_feather_fall);

    // Manual hashing
    hash.Update(gameState.level.guards_x);
    hash.Update(gameState.level.guards_dir);
    if (_hashKidCurrentHp == true) hash.Update(gameState.hitp_curr);
    if (_hashGuardCurrentHp == true) hash.Update(gameState.guardhp_curr);
    if (_hashTrobCount == true) hash.Update(gameState.trobs_count);

    // Mobs are moving objects (falling tiles only afaik).
    for (int i = 0; i < gameState.mobs_count; i++)
    {
     const auto &mob = gameState.mobs[i];
     const auto idx = std::make_pair(mob.room, mob.xh);
     if (_hashTypeMobs.count(idx))
     {
      const auto hashType = _hashTypeMobs.at(idx);
      if (hashType == INDEX_ONLY) { hash.Update(mob.room); hash.Update(mob.xh); }
      if (hashType == FULL)  hash.Update(mob);
     }
    }

    // Trobs are stationary animated objects. They only change in state, hence we only read BG
    for (int i = 0; i < gameState.trobs_count; i++)
    {
      const auto &trob = gameState.trobs[i];
      const auto idx = (trob.room - 1) * 30 + trob.tilepos;

      if (_hashTypeTrobs.count(idx))
      {
       const auto hashType = _hashTypeTrobs.at(idx);
       if (hashType == INDEX_ONLY) hash.Update(idx * 255);
       if (hashType == FULL) { hash.Update(gameState.level.bg[idx] + idx * 255 ); }
      }
    }

    // Computing hash for static objects. They only change on tile type, hence we only read FG
    for (const auto idx : _hashTypeStatic)  hash.Update(gameState.level.fg[idx] + idx * 255);

    uint64_t result;
    hash.Finalize(reinterpret_cast<uint8_t *>(&result));
    return result;
  }

  // Function to determine the current possible moves
  inline std::vector<uint8_t> getPossibleMoveIds() const
  {
    // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13    14
    //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA" };

    // If dead, do nothing
    if (gameState.Kid.alive >= 0)
      return {0};

    // For level 1, if kid touches ground and music plays, try restarting level
    if (gameState.Kid.frame == 109 && gameState.need_level1_music == 33)
      return {0, 14};

    // If bumped, nothing to do
    if (gameState.Kid.action == actions_5_bumped)
      return {0};

    // If in mid air or free fall, hope to grab on to something
    if (gameState.Kid.action == actions_3_in_midair || gameState.Kid.action == actions_4_in_freefall)
      return {0, 1};

    // Move, sheath, attack, parry
    if (gameState.Kid.sword == sword_2_drawn)
      return {0, 1, 2, 3, 4, 5};

    // Kid is standing or finishing a turn, try all possibilities
    if (gameState.Kid.frame == frame_15_stand || (gameState.Kid.frame >= frame_50_turn && gameState.Kid.frame < 53))
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    // Turning frame, try all possibilities
    if (gameState.Kid.frame == frame_48_turn)
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    // Start running animation, all movement without shift
    if (gameState.Kid.frame < 4)
      return {0, 2, 3, 4, 5, 6, 7, 8, 9};

    // Starting jump up, check directions, jump and shift
    if (gameState.Kid.frame >= frame_67_start_jump_up_1 && gameState.Kid.frame < frame_70_jumphang)
      return {0, 1, 2, 3, 4, 5, 6, 8, 12};

    // Running, all movement without shift
    if (gameState.Kid.frame < 15)
      return {0, 2, 3, 4, 5, 6, 7, 8, 9};

    // Hanging, up and shift are only options
    if (gameState.Kid.frame >= frame_87_hanging_1 && gameState.Kid.frame < 100)
      return {0, 1, 2, 12};

    // Crouched, can only stand, drink, or hop
    if (gameState.Kid.frame == frame_109_crouch)
      return {0, 1, 3, 4, 5, 7, 9, 13};

    // Default, do nothing
    return {0};
  }

  // Function to get magnet information
  inline magnetInfo_t getKidMagnetValues(const bool* rulesStatus, const int room) const
  {
   // Storage for magnet information
   magnetInfo_t magnetInfo;
   magnetInfo.positionX = 0.0f;
   magnetInfo.intensityX = 0.0f;
   magnetInfo.intensityY = 0.0f;

   // Iterating rule vector
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   {
    if (rulesStatus[ruleId] == true)
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

  // Function to get magnet information
  inline magnetInfo_t getGuardMagnetValues(const bool* rulesStatus, const int room) const
  {

   // Storage for magnet information
   magnetInfo_t magnetInfo;
   magnetInfo.positionX = 0.0f;
   magnetInfo.intensityX = 0.0f;
   magnetInfo.intensityY = 0.0f;

   // Iterating rule vector
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
    if (rulesStatus[ruleId] == true)
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

  // Obtains the score of a given frame
  inline float getFrameReward(const bool* rulesStatus) const
  {
    // Getting rewards from rules
    float reward = 0.0;
    for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
     if (rulesStatus[ruleId] == true)
      reward += _rules[ruleId]->_reward;

    // Getting kid room
    int kidCurrentRoom = gameState.Kid.room;

    // Getting magnet values for the kid
    auto kidMagnet = getKidMagnetValues(rulesStatus, kidCurrentRoom);

    // Getting kid's current frame
    const auto curKidFrame = gameState.Kid.frame;

    // Evaluating kidMagnet's reward on the X axis
    const float kidDiffX = std::abs(gameState.Kid.x - kidMagnet.positionX);
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
      reward += (float) kidMagnet.intensityY * (256.0f - gameState.Kid.y);
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
      reward += (float) -1.0f * kidMagnet.intensityY * (gameState.Kid.y);
    }

    // Getting guard room
    int guardCurrentRoom = gameState.Guard.room;

    // Getting magnet values for the guard
    auto guardMagnet = getGuardMagnetValues(rulesStatus, guardCurrentRoom);

    // Getting guard's current frame
    const auto curGuardFrame = gameState.Guard.frame;

    // Evaluating guardMagnet's reward on the X axis
    const float guardDiffX = std::abs(gameState.Guard.x - guardMagnet.positionX);
    reward += (float) guardMagnet.intensityX * (256.0f - guardDiffX);

    // For positive Y axis guardMagnet
    if ((float) guardMagnet.intensityY > 0.0f)
    {
     // Adding absolute reward for Y position
     reward += (float) guardMagnet.intensityY * (256.0f - gameState.Guard.y);
    }

    // For negative Y axis guardMagnet, rewarding falling/climbing down frames
    if ((float) guardMagnet.intensityY < 0.0f)
    {
      // Falling start
      if (curGuardFrame >= 102 && curGuardFrame <= 105) reward += -1.0f * (float) guardMagnet.intensityY;

      // Falling itself
      if (curGuardFrame == 106) reward += -2.0f + (float) guardMagnet.intensityY;

      // Adding absolute reward for Y position
      reward += (float) -1.0f * guardMagnet.intensityY * (gameState.Guard.y);
    }

    // Apply bonus when kid is inside a non-visible room
    if (kidCurrentRoom == 0 || kidCurrentRoom >= 25) reward += 128.0f;

    // Apply bonus when kid is climbing exit stairs
    if (curKidFrame >= 217 && curKidFrame <= 228) reward += 2000.0f;

    // For some levels, keeping HP allows for later skips
    reward += gameState.hitp_curr * 100.0f;

    // Returning reward
    return reward;
  }

  // Evaluates the rule set on a given frame. Returns true if it is a fail.
  inline void evaluateRules(bool* rulesStatus) const
  {
    for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
    {
      // Evaluate rule only if it's active
      if (rulesStatus[ruleId] == false)
      {
        // Checking if conditions are met
        bool isSatisfied = _rules[ruleId]->evaluate();

        // If it's achieved, update its status and run its actions
        if (isSatisfied) satisfyRule(rulesStatus, ruleId);
      }
    }
  }

  // Marks the given rule as satisfied, executes its actions, and recursively runs on its sub-satisfied rules
  inline void satisfyRule(bool* rulesStatus, const size_t ruleId) const
  {
   // Recursively run actions for the yet unsatisfied rules that are satisfied by this one and mark them as satisfied
   for (size_t satisfiedIdx = 0; satisfiedIdx < _rules[ruleId]->_satisfiesIndexes.size(); satisfiedIdx++)
   {
    // Obtaining index
    size_t subRuleId = _rules[ruleId]->_satisfiesIndexes[satisfiedIdx];

    // Only activate it if it hasn't been activated before
    if (rulesStatus[subRuleId] == false) satisfyRule(rulesStatus, subRuleId);
   }

   // Setting status to satisfied
   rulesStatus[ruleId] = true;
  }

  // Serialization/Deserialization Routines
  inline void pushState(const char* __restrict__ inputStateData) const
  {
    memcpy(&gameState, inputStateData, _FRAME_DATA_SIZE);
    next_room = gameState.drawn_room = gameState.Kid.room;
    load_room_links();
  }

  inline void popState(char* __restrict__ outputStateData) const
  {
   memcpy(outputStateData, &gameState, _FRAME_DATA_SIZE);
  }

  // Get frame type
  inline frameType getFrameType(const bool* rulesStatus) const
  {
   frameType type = f_regular;

   for (size_t ruleId = 0; ruleId < _rules.size(); ruleId++)
    if (rulesStatus[ruleId] == true)
    {
     if (_rules[ruleId]->_isFailRule == true) type = f_fail;
     if (_rules[ruleId]->_isWinRule == true) type = f_win;
    }

   return type;
  }

static std::string loadSdlPopState(const std::string& sdlPopStateData)
{
 if (sdlPopStateData.size() != sizeof(sdlPopState_t)) EXIT_WITH_ERROR("Error reading sdlpop save state. Expected %lu bytes, read: %lu\n", sdlPopStateData.size(), sizeof(sdlPopState_t));

 sdlPopState_t sdlPopState;
 memcpy(&sdlPopState, sdlPopStateData.data(), sizeof(sdlPopState_t));

 miniPopState_t miniPopState;
 memcpy(&miniPopState.level, &sdlPopState.level, sizeof(miniPopState.level));
 miniPopState.checkpoint = sdlPopState.checkpoint;
 miniPopState.upside_down = sdlPopState.upside_down;
 miniPopState.drawn_room = sdlPopState.drawn_room;
 miniPopState.current_level = sdlPopState.current_level;
 miniPopState.next_level = sdlPopState.next_level;
 miniPopState.mobs_count = sdlPopState.mobs_count;
 memcpy(miniPopState.mobs, sdlPopState.mobs, sizeof(sdlPopState.mobs));
 miniPopState.trobs_count = sdlPopState.trobs_count;
 memcpy(miniPopState.trobs, sdlPopState.trobs, sizeof(miniPopState.trobs));
 miniPopState.leveldoor_open = sdlPopState.leveldoor_open;
 memcpy(&miniPopState.Kid, &sdlPopState.Kid, sizeof(miniPopState.Kid));
 miniPopState.hitp_curr = sdlPopState.hitp_curr;
 miniPopState.hitp_max = sdlPopState.hitp_max;
 miniPopState.hitp_beg_lev = sdlPopState.hitp_beg_lev;
 miniPopState.grab_timer = sdlPopState.grab_timer;
 miniPopState.holding_sword = sdlPopState.holding_sword;
 miniPopState.united_with_shadow = sdlPopState.united_with_shadow;
 miniPopState.have_sword = sdlPopState.have_sword;
 miniPopState.kid_sword_strike = sdlPopState.kid_sword_strike;
 miniPopState.pickup_obj_type = sdlPopState.pickup_obj_type;
 miniPopState.offguard = sdlPopState.offguard;
 memcpy(&miniPopState.Guard, &sdlPopState.Guard, sizeof(miniPopState.Guard));
 memcpy(&miniPopState.Char, &sdlPopState.Char, sizeof(miniPopState.Char));
 memcpy(&miniPopState.Opp, &sdlPopState.Opp, sizeof(miniPopState.Opp));
 miniPopState.guardhp_curr = sdlPopState.guardhp_curr;
 miniPopState.guardhp_max = sdlPopState.guardhp_max;
 miniPopState.demo_index = sdlPopState.demo_index;
 miniPopState.demo_time = sdlPopState.demo_time;
 miniPopState.curr_guard_color = sdlPopState.curr_guard_color;
 miniPopState.guard_notice_timer = sdlPopState.guard_notice_timer;
 miniPopState.guard_skill = sdlPopState.guard_skill;
 miniPopState.shadow_initialized = sdlPopState.shadow_initialized;
 miniPopState.guard_refrac = sdlPopState.guard_refrac;
 miniPopState.justblocked = sdlPopState.justblocked;
 miniPopState.droppedout = sdlPopState.droppedout;
 memcpy(&miniPopState.curr_row_coll_room, &sdlPopState.curr_row_coll_room, sizeof(miniPopState.curr_row_coll_room));
 memcpy(&miniPopState.curr_row_coll_flags, &sdlPopState.curr_row_coll_flags, sizeof(miniPopState.curr_row_coll_flags));
 memcpy(&miniPopState.below_row_coll_room, &sdlPopState.below_row_coll_room, sizeof(miniPopState.below_row_coll_room));
 memcpy(&miniPopState.below_row_coll_flags, &sdlPopState.below_row_coll_flags, sizeof(miniPopState.below_row_coll_flags));
 memcpy(&miniPopState.above_row_coll_room, &sdlPopState.above_row_coll_room, sizeof(miniPopState.above_row_coll_room));
 memcpy(&miniPopState.above_row_coll_flags, &sdlPopState.above_row_coll_flags, sizeof(miniPopState.above_row_coll_flags));
 miniPopState.prev_collision_row = sdlPopState.prev_collision_row;
 miniPopState.flash_color = sdlPopState.flash_color;
 miniPopState.flash_time = sdlPopState.flash_time;
 miniPopState.need_level1_music = sdlPopState.need_level1_music;
 miniPopState.is_screaming = sdlPopState.is_screaming;
 miniPopState.is_feather_fall = sdlPopState.is_feather_fall;
 miniPopState.last_loose_sound = sdlPopState.last_loose_sound;
 miniPopState.random_seed = sdlPopState.random_seed;
 miniPopState.ctrl1_forward = sdlPopState.ctrl1_forward;
 miniPopState.ctrl1_backward = sdlPopState.ctrl1_backward;
 miniPopState.ctrl1_up = sdlPopState.ctrl1_up;
 miniPopState.ctrl1_down = sdlPopState.ctrl1_down;
 miniPopState.ctrl1_shift2 = sdlPopState.ctrl1_shift2;
 miniPopState.exit_room_timer = sdlPopState.exit_room_timer;
 miniPopState.is_guard_notice = sdlPopState.is_guard_notice;
 miniPopState.can_guard_see_kid = sdlPopState.can_guard_see_kid;

 std::string stateData;
 stateData.resize(sizeof(miniPopState_t));
 memcpy(stateData.data(), &miniPopState, sizeof(miniPopState_t));
 return stateData;
}

static std::string saveSdlPopState(const std::string& miniPopStateData)
{
 if (miniPopStateData.size() != sizeof(miniPopState_t)) EXIT_WITH_ERROR("Error reading minipop save state. Expected %lu bytes, read: %lu\n", miniPopStateData.size(), sizeof(miniPopStateData));

 miniPopState_t miniPopState;
 memcpy(&miniPopState, miniPopStateData.data(), sizeof(miniPopState_t));

 sdlPopState_t sdlPopState;
 memcpy(&sdlPopState.quick_control, ".........", sizeof(sdlPopState.quick_control));
 memcpy(&sdlPopState.level, &miniPopState.level, sizeof(sdlPopState.level));
 sdlPopState.checkpoint = miniPopState.checkpoint;
 sdlPopState.upside_down = miniPopState.upside_down;
 sdlPopState.drawn_room = miniPopState.drawn_room;
 sdlPopState.current_level = miniPopState.current_level;
 sdlPopState.next_level = miniPopState.next_level;
 sdlPopState.mobs_count = miniPopState.mobs_count;
 memcpy(sdlPopState.mobs, miniPopState.mobs, sizeof(miniPopState.mobs));
 sdlPopState.trobs_count = miniPopState.trobs_count;
 memcpy(sdlPopState.trobs, miniPopState.trobs, sizeof(sdlPopState.trobs));
 sdlPopState.leveldoor_open = miniPopState.leveldoor_open;
 memcpy(&sdlPopState.Kid, &miniPopState.Kid, sizeof(sdlPopState.Kid));
 sdlPopState.hitp_curr = miniPopState.hitp_curr;
 sdlPopState.hitp_max = miniPopState.hitp_max;
 sdlPopState.hitp_beg_lev = miniPopState.hitp_beg_lev;
 sdlPopState.grab_timer = miniPopState.grab_timer;
 sdlPopState.holding_sword = miniPopState.holding_sword;
 sdlPopState.united_with_shadow = miniPopState.united_with_shadow;
 sdlPopState.have_sword = miniPopState.have_sword;
 sdlPopState.kid_sword_strike = miniPopState.kid_sword_strike;
 sdlPopState.pickup_obj_type = miniPopState.pickup_obj_type;
 sdlPopState.offguard = miniPopState.offguard;
 memcpy(&sdlPopState.Guard, &miniPopState.Guard, sizeof(sdlPopState.Guard));
 memcpy(&sdlPopState.Char, &miniPopState.Char, sizeof(sdlPopState.Char));
 memcpy(&sdlPopState.Opp, &miniPopState.Opp, sizeof(sdlPopState.Opp));
 sdlPopState.guardhp_curr = miniPopState.guardhp_curr;
 sdlPopState.guardhp_max = miniPopState.guardhp_max;
 sdlPopState.demo_index = miniPopState.demo_index;
 sdlPopState.demo_time = miniPopState.demo_time;
 sdlPopState.curr_guard_color = miniPopState.curr_guard_color;
 sdlPopState.guard_notice_timer = miniPopState.guard_notice_timer;
 sdlPopState.guard_skill = miniPopState.guard_skill;
 sdlPopState.shadow_initialized = miniPopState.shadow_initialized;
 sdlPopState.guard_refrac = miniPopState.guard_refrac;
 sdlPopState.justblocked = miniPopState.justblocked;
 sdlPopState.droppedout = miniPopState.droppedout;
 memcpy(&sdlPopState.curr_row_coll_room, &miniPopState.curr_row_coll_room, sizeof(sdlPopState.curr_row_coll_room));
 memcpy(&sdlPopState.curr_row_coll_flags, &miniPopState.curr_row_coll_flags, sizeof(sdlPopState.curr_row_coll_flags));
 memcpy(&sdlPopState.below_row_coll_room, &miniPopState.below_row_coll_room, sizeof(sdlPopState.below_row_coll_room));
 memcpy(&sdlPopState.below_row_coll_flags, &miniPopState.below_row_coll_flags, sizeof(sdlPopState.below_row_coll_flags));
 memcpy(&sdlPopState.above_row_coll_room, &miniPopState.above_row_coll_room, sizeof(sdlPopState.above_row_coll_room));
 memcpy(&sdlPopState.above_row_coll_flags, &miniPopState.above_row_coll_flags, sizeof(sdlPopState.above_row_coll_flags));
 sdlPopState.prev_collision_row = miniPopState.prev_collision_row;
 sdlPopState.flash_color = miniPopState.flash_color;
 sdlPopState.flash_time = miniPopState.flash_time;
 sdlPopState.need_level1_music = miniPopState.need_level1_music;
 sdlPopState.is_screaming = miniPopState.is_screaming;
 sdlPopState.is_feather_fall = miniPopState.is_feather_fall;
 sdlPopState.last_loose_sound = miniPopState.last_loose_sound;
 sdlPopState.random_seed = miniPopState.random_seed;
 sdlPopState.rem_min = 60;
 sdlPopState.rem_tick = 719;
 sdlPopState.control_x = 0;
 sdlPopState.control_y = 0;
 sdlPopState.control_shift = 0;
 sdlPopState.control_forward = 0;
 sdlPopState.control_backward = 0;
 sdlPopState.control_up = 0;
 sdlPopState.control_down = 0;
 sdlPopState.control_shift2 = 0;
 sdlPopState.ctrl1_forward = miniPopState.ctrl1_forward;
 sdlPopState.ctrl1_backward = miniPopState.ctrl1_backward;
 sdlPopState.ctrl1_up = miniPopState.ctrl1_up;
 sdlPopState.ctrl1_down = miniPopState.ctrl1_down;
 sdlPopState.ctrl1_shift2 = miniPopState.ctrl1_shift2;
 sdlPopState.exit_room_timer = miniPopState.exit_room_timer;
 sdlPopState.replay_curr_tick = 0;
 sdlPopState.is_guard_notice = miniPopState.is_guard_notice;
 sdlPopState.can_guard_see_kid = miniPopState.can_guard_see_kid;

 std::string stateData;
 stateData.resize(sizeof(sdlPopState_t));
 memcpy(stateData.data(), &sdlPopState, sizeof(sdlPopState_t));
 return stateData;
}

};
