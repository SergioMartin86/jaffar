#pragma once

#include "miniPoPInstance.h"
#include "common.h"
#include "frame.h"
#include "nlohmann/json.hpp"
#include "rule.h"
#include "state.h"
#include "cbuffer.hpp"
#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <random>
#include <string>
#include <vector>
#include <mpi.h>

// Minimum difference between global frame counter and the prescribed maximum
#define DATABASE_LIMITER_THRESHOLD 1024

// Minimum reward difference between current lower/upper bounds for clipping database
#define DATABASE_REWARD_THRESHOLD 10.0

class Train
{
  public:
  Train(int argc, char *argv[]);
  void run();

  private:

  // Jaffar script file
  std::string _scriptFile;

  // File output config
  double _outputSaveBestSeconds;
  std::string _outputSaveBestPath;
  std::string _outputSolutionBestPath;
  bool _showSDLPopPreview;

  // Store the number of MPI workers and openMP threads in use
  size_t _workerId;
  size_t _workerCount;
  int _threadCount;

  // Communication schedule for frame exchange
  std::vector<size_t> _communicationSchedule;

  // Craeting State class instance, one per openMP thread
  std::vector<State *> _state;
  State* _showState;
  size_t _ruleCount;

  // Storage for source frame data for differential load/save
  char _sourceFrameData[_FRAME_DATA_SIZE];

  // Frame counter
  size_t _globalNextStepFrameCount;
  size_t _stepFramesProcessedCounter;
  size_t _totalFramesProcessedCounter;

  // Frame counters per worker
  std::vector<ssize_t> _localNextStepFrameCounts;
  ssize_t _maxFrameCount;
  ssize_t _maxFrameWorkerId;
  ssize_t _minFrameCount;
  ssize_t _minFrameWorkerId;

  // Frame databases
  size_t _localStoredFrameCount;
  size_t _maxDatabaseSize;
  std::map<size_t, std::vector<std::unique_ptr<Frame>>> _frameDB;
  std::map<size_t, std::vector<std::unique_ptr<Frame>>> _winFrameDB;

  // Frame database for showing
  std::vector<Frame> _showFrameDB;

  // Storage for the best frame
  Frame _bestFrame;
  float _bestFrameReward;

  // Hash information
  cBuffer<absl::flat_hash_set<uint64_t>*> _hashDatabases;
  size_t _hashAgeThreshold;
  size_t _hashCollisions;

  // Per-step local hash collision counter
  size_t _newCollisionCounter;

  // Storage for the position of win rules, for win detection
  bool _winFrameFound;
  Frame _winFrame;

  // SDLPop instance and Id for the show thread
  pthread_t _showThreadId;

  // Flag to indicate finalization
  bool _hasFinalized;

  // MPI data type for frame serialization
  MPI_Datatype _mpiFrameType;

  // Printing stats
  void printTrainStatus();

  // Each worker processes their own unique base frames to produce new frames
  void computeFrames();

  // Check for hash collisions
  bool checkForHashCollision(const uint64_t hash);

  // Argument parser
  void parseArgs(int argc, char *argv[]);

  // Function for the show thread (saves states from time to time to display progress)
  static void *showThreadFunction(void *trainPtr);
  void showSavingLoop();

  // Profiling and Debugging
  double _searchTotalTime;
  double _currentStepTime;
  double _frameComputationTime;
  double _framePostprocessingTime;
  double _stepHashCalculationTime;
  double _stepHashCheckingTime;
  double _stepFrameAdvanceTime;
  double _stepFrameSerializationTime;
  double _stepFrameDeserializationTime;
  double _stepFrameEncodingTime;
  double _stepFrameDecodingTime;
  double _DBSortingTime;
  double _DBExchangeTime;
};
