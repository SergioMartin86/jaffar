#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"
#include "SDLPopInstance.h"

class State {
 public:
  enum ItemType {
    PER_FRAME_STATE,
    HASHABLE,
    HASHABLE_MANUAL,
  };

  struct Item {
    void* ptr;
    size_t size;
    ItemType type;
  };

  State(SDLPopInstance *sdlPop, nlohmann::json stateConfig);

  uint64_t computeHash() const;
  uint64_t kidHash() const;

  void loadState(const std::string& data);
  std::string saveState() const;

 private:
  SDLPopInstance* _sdlPop;
  std::vector<Item> _items;
};
