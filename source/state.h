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

  struct Item
  {
    void *ptr;
    size_t size;
    ItemType type;
  };

  State() = default;
  State(miniPoPInstance *sdlPop, const std::string& saveString);

  uint64_t computeHash() const;
  uint64_t kidHash() const;

  void pushState();
  void getState();

  char _stateData[_FRAME_DATA_SIZE];

  private:
  miniPoPInstance *_sdlPop;
  std::vector<Item> _items;
};
