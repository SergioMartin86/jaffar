#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "SDLPopInstance.h"
#include "state.h"
#include "json.hpp"
#include "rule.h"
#include "frame.h"
#include <string>
#include <vector>
#include <mpi.h>

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

 // Storage for the position of win rules, for win detection
 std::vector<size_t> _winRulePositions;
 bool _winFrameFound;
 Frame* _winFrame;

 // Storage for rule serialization size
 size_t _frameSerializedSize;
 MPI_Datatype _mpiFrameType;

 // Flag to indicate finalization
 bool _hasFinalized;

 // Printing stats
 void printSearchStatus();

 // Runs the search for a single frame
 void runFrame();

 // Obtains the score of a given frame
 float getFrameScore(const Frame* frame);

 // Evaluates the rule set on a given frame. Returns true if it is a fail.
 bool evaluateRules(Frame* frame);

 // Print Rule information
 void printRuleStatus(const Frame* frame);

 // Profiling and Debugging
 bool _showProfilingInformation;
 bool _showDebuggingInformation;
 double _searchTotalTime;
 double _frameCommunicationTime;
 double _frameComputationTime;
 double _frameStateLoadTime;
 double _frameAdvanceTime;
 double _frameHashComputationTime;
 double _frameHashInsertionTime;
 double _frameCreationTime;
 double _frameRuleEvaluationTime;
 double _frameScoreEvaluationTime;
 double _frameDatabaseUpdateTime;
 double _frameDatabaseClearTime;
 double _frameDatabaseSortTime;
 double _frameDatabaseClippingTime;
 double _framePostprocessingTime;

 double _commDatabaseSerializationTime;
 double _commFrameScatterTime;
 double _commFrameGatherTime;
 double _commGatherWorkerInformationTime;
 double _commWorkerFrameSerializationTime;
 double _commDatabaseDeserializationTime;
};

extern size_t _ruleCount;
