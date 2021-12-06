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

// Number of frames to cache for showing purposes
#define SHOW_FRAME_COUNT 1000

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
  double _outputSaveCurrentSeconds;
  std::string _outputSaveBestPath;
  std::string _outputSaveCurrentPath;
  std::string _outputSolutionBestPath;
  std::string _outputSolutionCurrentPath;
  bool _showSDLPopPreview;

  // Store the number of openMP threads in use
  int _threadCount;

  // Communication schedule for frame exchange
  std::vector<size_t> _communicationSchedule;

  // Craeting State class instance, one per openMP thread
  std::vector<State *> _state;
  size_t _ruleCount;

  // Storage for source frame data for differential load/save
  char _sourceFrameData[_JAFFAR_FRAME_DATA_SIZE];

  // Frame counter
  size_t _stepFramesProcessedCounter;
  size_t _totalFramesProcessedCounter;

  // Frame databases
  size_t _databaseSize;
  size_t _maxDatabaseSize;
  std::vector<std::unique_ptr<Frame>> _frameDB;

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

  // Printing stats
  void printTrainStatus();

  // Each worker processes their own unique base frames to produce new frames
  void computeFrames();

  // Adds a new hash entry while making sure the number of hash entries don't exceed the maximum
  void addHashEntry(uint64_t hash);

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
};
