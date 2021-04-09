#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "SDLPopInstance.h"
#include "state.h"
#include "json.hpp"
#include "rule.h"
#include <string>
#include <vector>


struct Magnet
{
 float intensityX;
 float positionX;
 float intensityY;
 float positionY;
};

struct Frame
{
 std::string move;
 float score;
 std::string frameStateData;

 // Store termination conditions
 bool isFail;
 bool isWin;

 // Magnet vector
 std::vector<Magnet> magnets;

 // Rule status vector
 std::vector<status_t> rulesStatus;
};

class Search
{

public:

  Search(SDLPopInstance *sdlPop, State *state, nlohmann::json& config);
  ~Search();

  void run();

private:

 SDLPopInstance* _sdlPop;
 State* _state;
 std::string _baseStateData;

 // Rule vector
 std::vector<Rule*> _rules;

 // Frame counter
 size_t _currentFrame;
 size_t _maxFrames;

 // Frame databases
 size_t _maxDatabaseSize;
 std::vector<Frame*>* _currentFrameDB;
 std::vector<Frame*>* _nextFrameDB;

 // Hash information
 absl::flat_hash_set<uint64_t> _hashes;
 size_t _hashCollisions;

 // Flag to indicate finalization
 bool _hasFinalized;

 // Printing stats
 void printSearchStatus();

 // Runs the search for a single frame
 void runFrame();

 // Obtains the score of a given frame
 float getFrameScore(const Frame* frame);

 // Evaluates the rule set on a given frame
 void evaluateRules(Frame* frame);
};
