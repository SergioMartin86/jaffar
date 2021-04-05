#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "SDLPopInstance.h"
#include "state.h"
#include "scorer.h"
#include "json.hpp"
#include <string>
#include <vector>


struct Frame
{
 float score;
 std::string frameStateData;
};

class Search
{

public:

  Search(SDLPopInstance *sdlPop, State *state, Scorer *scorer, nlohmann::json& config);
  ~Search();

  void run();
  void runFrame();

private:

 size_t _searchWidth;
 SDLPopInstance* _sdlPop;
 State* _state;
 Scorer* _scorer;
 std::string _baseStateData;

 // Current frame counter
 size_t _currentFrame;

 // Frame databases
 std::vector<Frame*>* _currentFrameDB;
 std::vector<Frame*>* _nextFrameDB;

 // Hash information
 absl::flat_hash_set<uint64_t> _hashes;
 size_t _hashCollisions;

 // Flag to indicate finalization
 bool _hasFinalized;

};
