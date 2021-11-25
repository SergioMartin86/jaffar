#pragma once

#include "miniPoPInstance.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include "frame.h"
#include <string>
#include <vector>

// Current train step is a global variable so every part of the code can see it
extern size_t _currentStep;

class State
{
  public:
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

  State() = default;
  State(miniPoPInstance *miniPop, const std::string& saveString, const nlohmann::json stateConfig);

  uint64_t computeHash() const;

  void pushState();
  void getState();

  char _stateData[_FRAME_DATA_SIZE];

  // Hash states
  bool _hashKidCurrentHp;
  std::map<int, hashType> _hashTypeTrobs;
  std::vector<int> _hashTypeStatic;
  std::map<std::pair<int, int>, hashType> _hashTypeMobs;

  private:
  miniPoPInstance *_miniPop;
  std::vector<Item> _items;
};
