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

  State(const std::string& saveString, const nlohmann::json stateConfig, const nlohmann::json rulesConfig, const int seed = -1);

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
        // Checking dependencies first. If not met, continue to the next rule
        bool dependenciesMet = true;
        for (size_t i = 0; i < _rules[ruleId]->_dependenciesIndexes.size(); i++)
          if (rulesStatus[_rules[ruleId]->_dependenciesIndexes[i]] == false)
            dependenciesMet = false;

        // If dependencies aren't met, then continue to next rule
        if (dependenciesMet == false) continue;

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
};
