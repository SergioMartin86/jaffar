#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "SDLPopInstance.h"

class State {
 public:
  enum ItemType {
    ONLY_QUICKSAVE,
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

  void Quickload(const std::string& filename);
  void Quicksave(const std::string& filename);
  uint64_t ComputeHash() const;
  uint64_t KidHash() const;

  void LoadBase(const std::string& data);
  std::string SaveBase() const;

  void LoadFrame(const std::string& data);
  std::string SaveFrame() const;

 private:
  SDLPopInstance* sdlPop_;
  std::vector<Item> items_;
};
