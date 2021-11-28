#pragma once

#include "miniPoPInstance.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include "frame.h"
#include <string>
#include <vector>
#include <set>

// Enumerations and structs
enum ItemType
{
  PER_FRAME_STATE,
  HASHABLE,
  HASHABLE_MANUAL,
};

enum hashType
{
  NONE,
  INDEX_ONLY,
  FULL,
};


struct Item
{
  void *ptr;
  size_t size;
  ItemType type;
};

// Current train step is a global variable so every part of the code can see it
extern size_t _currentStep;


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

  // This function computes the hash for the current state
  uint64_t computeHash() const;

  // Obtains the score of a given frame
  float getFrameReward(const Frame &frame);

  // Function to determine the current possible moves
  std::vector<uint8_t> getPossibleMoveIds(const Frame &frame);

  // Function to get magnet information
  magnetInfo_t getKidMagnetValues(const Frame &frame, const int room);
  magnetInfo_t getGuardMagnetValues(const Frame &frame, const int room);

  // Evaluates the rule set on a given frame. Returns true if it is a fail.
  void evaluateRules(Frame &frame);

  // Marks the given rule as satisfied, executes its actions, and recursively runs on its sub-satisfied rules
  void satisfyRule(Frame &frame, const size_t ruleId);

  // Function to get the static rewards obtained from rules
  float getRuleRewards(const Frame &frame);

  // Serialization/Deserialization Routines
  void pushState();
  void popState();

  // Print Rule information
  void printRuleStatus(const Frame &frame);

  // Storage for state data (input and output)
  char _inputStateData[_FRAME_DATA_SIZE];
  char _outputStateData[_FRAME_DATA_SIZE];

  // Hash states
  bool _hashKidCurrentHp;
  bool _hashTrobCount;

  std::map<int, hashType> _hashTypeTrobs;
  std::vector<int> _hashTypeStatic;
  std::map<std::pair<int, int>, hashType> _hashTypeMobs;
  std::vector<Rule *> _rules;
  size_t _ruleCount;
  miniPoPInstance *_miniPop;
  std::vector<Item> _items;
};
