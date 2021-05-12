#pragma once

#include "SDLPopInstance.h"
#include "nlohmann/json.hpp"
#include <cstddef>
#include <string>
#include <vector>

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

  unsigned int advanceRNGState(const unsigned int randomSeed);
  unsigned int reverseRNGState(const unsigned int randomSeed);
  State() = default;
  State(SDLPopInstance *sdlPop, const std::string& saveString);

  uint64_t computeHash() const;
  uint64_t kidHash() const;

  void loadState(const std::string &data);
  std::string saveState() const;

  private:
  SDLPopInstance *_sdlPop;
  std::vector<Item> _items;
};
