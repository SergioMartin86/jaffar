#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "SDLPopInstance.h"
#include "state.h"
#include "json.hpp"
#include "rule.h"
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

  Search(SDLPopInstance *sdlPop, State *state, nlohmann::json& config);
  ~Search();

  void run();
  void runFrame();

private:

 SDLPopInstance* _sdlPop;
 State* _state;
 std::string _baseStateData;

 // Rule collection
 std::vector<Rule*> _rules;

 // Current frame counter
 size_t _currentFrame;

 // Frame databases
 size_t _maxDatabaseSize;
 std::vector<Frame*>* _currentFrameDB;
 std::vector<Frame*>* _nextFrameDB;

 // Hash information
 absl::flat_hash_set<uint64_t> _hashes;
 size_t _hashCollisions;

 // Flag to indicate finalization
 bool _hasFinalized;

};
