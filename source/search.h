#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "nlohmann/json.hpp"
#include "SDLPopInstance.h"
#include "state.h"
#include "rule.h"
#include "frame.h"
#include <string>
#include <vector>
#include <memory>
#include <mpi.h>
#include <random>
#include <algorithm>

class Search
{

public:

  Search();
  ~Search();

  void run();

private:

 SDLPopInstance* _sdlPop;
 State* _state;
 std::string _baseStateData;
 bool _showSDLPopPreview;

 // Worker id and count
 size_t _workerId;
 size_t _workerCount;

 // Rule vector
 std::vector<Rule*> _rules;

 // Frame counter
 size_t _globalFrameCounter;
 size_t _stepFramesProcessedCounter;
 size_t _totalFramesProcessedCounter;

 // Step counter
 size_t _currentStep;

 // Frame databases
 size_t _maxLocalDatabaseSize;
 std::vector<std::unique_ptr<Frame>> _currentFrameDB;
 std::vector<std::unique_ptr<Frame>> _nextFrameDB;

 // Storage for the best frame
 Frame _bestFrame;

 // Hash information
 std::vector<absl::flat_hash_set<uint64_t>> _hashDatabases;
 size_t _hashDatabaseCount;
 size_t _hashDatabaseSizeThreshold;
 size_t _globalHashCollisions;
 size_t _globalHashEntries;
 size_t _hashDatabaseSwapCount;

 // Storage for the position of win rules, for win detection
 std::vector<size_t> _winRulePositions;
 bool _winFrameFound;
 Frame _globalWinFrame;

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
 float getFrameScore(const Frame& frame);

 // Evaluates the rule set on a given frame. Returns true if it is a fail.
 void evaluateRules(Frame& frame);

 // Print Rule information
 void printRuleStatus(const Frame& frame);

 // Function to determine the current possible moves
 std::vector<uint8_t> getPossibleMoveIds(const Frame& frame);

 // Profiling and Debugging
 bool _showProfilingInformation;
 bool _showDebuggingInformation;
 double _searchTotalTime;
 double _framePreprocessingTime;
 double _frameComputationTime;
 double _framePostprocessingTime;
};
