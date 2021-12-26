#pragma once

#include "miniPoPInstance.h"
#include "common.h"
#include "frame.h"
#include "nlohmann/json.hpp"
#include "rule.h"
#include "state.h"
#include "cbuffer.hpp"
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <random>
#include <string>
#include <vector>

// The frequency with which we clean the hash database of old entries
#define HASH_DATABASE_CLEAN_FREQUENCY 10

class Train
{
  public:
  Train(int argc, char *argv[]);
  void run();

  private:

  // Storage for current step
  size_t _currentStep;

  // Jaffar script file
  std::string _scriptFile;

  // File output config
  double _outputSaveBestSeconds;
  std::string _outputSaveBestPath;
  std::string _outputSolutionBestPath;
  bool _showSDLPopPreview;

  // Store the number of openMP threads in use
  int _threadCount;

  // Craeting State class instance, one per openMP thread
  std::vector<State *> _state;
  State* _showState;
  size_t _ruleCount;

  // Storage for source frame data for differential load/save
  char _sourceFrameData[_FRAME_DATA_SIZE];

  // Frame counter
  size_t _stepFramesProcessedCounter;
  size_t _totalFramesProcessedCounter;

  // Frame databases
  size_t _databaseSize;
  size_t _maxDatabaseSize;
  std::map<size_t, std::vector<std::unique_ptr<Frame>>> _frameDB;
  std::vector<std::unique_ptr<Frame>> _winFrameDB;

  // Frame database for showing
  std::vector<Frame> _showFrameDB;

  // Storage for the best and worst frame
  Frame _bestFrame;
  float _bestFrameReward;
  Frame _worstFrame;
  float _worstFrameReward;

  // Hash information
  size_t _hashAgeThreshold;
  absl::flat_hash_map<uint64_t, uint16_t> _pastHashDB;
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

  // Argument parser
  void parseArgs(int argc, char *argv[]);

  // Function for the show thread (saves states from time to time to display progress)
  static void *showThreadFunction(void *trainPtr);
  void showSavingLoop();

  // Profiling and Debugging
  double _searchTotalTime;
  double _currentStepTime;
  double _stepHashCalculationTime;
  double _stepHashCheckingTime;
  double _stepFrameAdvanceTime;
  double _stepFrameSerializationTime;
  double _stepFrameDeserializationTime;
  double _stepFrameEncodingTime;
  double _stepFrameDecodingTime;
  double _DBSortingTime;
  double _stepPastHashConsolidationTime;
};
