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

  void advanceRNGState();
  void reverseRNGState();
  State(SDLPopInstance *sdlPop, nlohmann::json stateConfig);

  uint64_t computeHash() const;
  uint64_t kidHash() const;

  void loadState(const std::string &data);
  std::string saveState() const;

  private:
  SDLPopInstance *_sdlPop;
  std::vector<Item> _items;
};
