#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"

#include "SDLPopInstance.h"

class State {
 public:
  enum ItemType {
    ONLY_quickSave,
    BASE_LAYER,
    PER_FRAME_STATE,
    ONLY_STATE,
    HASHABLE,
    HASHABLE_MANUAL,
  };

  struct Item {
    void* ptr;
    size_t size;
    ItemType type;
  };

  State(SDLPopInstance *sdlPop);

  void quickLoad(const std::string& filename);
  void quickSave(const std::string& filename);
  uint64_t computeHash() const;
  uint64_t kidHash() const;

  void loadBase(const std::string& data);
  std::string saveBase() const;

  void loadFrame(const std::string& data);
  std::string saveFrame() const;

 private:
  SDLPopInstance* _sdlPop;
  std::vector<Item> _items;
};
