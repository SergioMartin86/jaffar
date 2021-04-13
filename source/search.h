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
 size_t _maxSteps;

 // Frame databases
 size_t _maxLocalDatabaseSize;
 std::vector<Frame*>* _currentFrameDB;
 std::vector<Frame*>* _nextFrameDB;

 // Storage for the best frame
 Frame _bestFrame;

 // Hash information
 absl::flat_hash_set<uint64_t> _hashes;
 size_t _globalHashCollisions;

 // Storage for the position of win rules, for win detection
 std::vector<size_t> _winRulePositions;
 bool _winFrameFound;
 Frame* _globalWinFrame;

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
 void evaluateRules(Frame* frame);

 // Print Rule information
 void printRuleStatus(const Frame* frame);

 // Profiling and Debugging
 bool _showProfilingInformation;
 bool _showDebuggingInformation;
 double _searchTotalTime;
 double _framePreprocessingTime;
 double _frameComputationTime;
 double _framePostprocessingTime;
};

extern size_t _ruleCount;
