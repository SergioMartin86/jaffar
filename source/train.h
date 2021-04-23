#pragma once

#include <absl/container/flat_hash_set.h>
#include "nlohmann/json.hpp"
#include "SDLPopInstance.h"
#include "state.h"
#include "rule.h"
#include "frame.h"
#include "common.h"
#include <string>
#include <vector>
#include <memory>
#include <mpi.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <pthread.h>

// Number of local databases for cyclic discarding of old hashes
#define HASH_DATABASE_COUNT 10

// Number of frames to cache for showing purposes
#define SHOW_FRAME_COUNT 1000

class Train
{

public:

 Train(int argc, char* argv[]);
 void run();

private:

 // Jaffar script file to load
 std::string _scriptFile;

 // Jaffar script file contents (JSON)
 nlohmann::json _scriptJs;

 // File output config
 double _outputSaveBestSeconds;
 double _outputSaveCurrentSeconds;
 std::string _outputSaveBestPath;
 std::string _outputSaveCurrentPath;

 // Worker id and count
 size_t _workerId;
 size_t _workerCount;

 SDLPopInstance* _sdlPop;
 State* _state;
 std::string _baseStateData;
 bool _showSDLPopPreview;

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

 // Frame database for showing
 std::vector<Frame> _showFrameDB;

 // Storage for the best frame
 Frame _bestFrame;

 // Hash information
 std::vector<absl::flat_hash_set<uint64_t>> _hashDatabases;
 absl::flat_hash_set<uint64_t> _newHashes;
 size_t _hashDatabaseSizeThreshold;
 size_t _globalHashCollisions;
 size_t _globalHashEntries;
 size_t _hashDatabaseSwapCount;
 size_t _localStepSwapCounter;

 // Per-step local hash collision counter
 size_t _newCollisionCounter;

  // Per-step frame processed counter
 size_t _localStepFramesProcessedCounter;

 // Storage for the position of win rules, for win detection
 std::vector<size_t> _winRulePositions;
 bool _winFrameFound;
 Frame _globalWinFrame;
 Frame _localWinFrame;
 bool _localWinFound;

 // Storage for rule serialization size
 size_t _frameSerializedSize;
 MPI_Datatype _mpiFrameType;

 // Id for the show thread
 pthread_t _showThreadId;

 // Flag to indicate finalization
 bool _hasFinalized;

 // Printing stats
 void printTrainStatus();

 // Each worker processes their own unique base frames to produce new frames
 void computeFrames();

 // Workers sorts their databases and communicates partial results
 void framePostprocessing();

 // Obtains the score of a given frame
 float getFrameScore(const Frame& frame);

 // Evaluates the rule set on a given frame. Returns true if it is a fail.
 void evaluateRules(Frame& frame);

 // Print Rule information
 void printRuleStatus(const Frame& frame);

 // Redistribute frames uniformly among workers
 void distributeFrames();

 // Sharing hash entries among workers and cut hash tables databases to size
 void updateHashDatabases();

 // Adds a new hash entry while making sure the number of hash entries don't exceed the maximum
 void addHashEntry(uint64_t hash);

 // Argument parser
 void parseArgs(int argc, char* argv[]);

 // Function to determine the current possible moves
 std::vector<uint8_t> getPossibleMoveIds(const Frame& frame);

 // Function for the show thread (saves states from time to time to display progress)
 static void* showThreadFunction(void* trainPtr);
 void showSavingLoop();

 // Profiling and Debugging
 double _searchTotalTime;
 double _currentStepTime;
 double _frameDistributionTime;
 double _frameComputationTime;
 double _framePostprocessingTime;
 double _hashExchangeTime;
};
