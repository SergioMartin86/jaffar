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

// Struct to hold all of the frame's magnet information
struct magnetInfo_t
{
 float positionX;
 float intensityX;
 float intensityY;
};

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

  // Craeting SDLPop and State class instances and rule vector, one per openMP thread
  std::vector<miniPoPInstance*> _miniPop;
  std::vector<State *> _state;
  std::vector<std::vector<Rule *>> _rules;

  // Storage for source frame data for differential load/save
  char _sourceFrameData[_FRAME_DATA_SIZE];

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

  // Obtains the score of a given frame
  float getFrameReward(const Frame &frame);

  // Evaluates the rule set on a given frame. Returns true if it is a fail.
  void evaluateRules(Frame &frame);

  // Marks the given rule as satisfied, executes its actions, and recursively runs on its sub-satisfied rules
  void satisfyRule(Frame &frame, const size_t ruleId);

  // Print Rule information
  void printRuleStatus(const Frame &frame);

  // Adds a new hash entry while making sure the number of hash entries don't exceed the maximum
  void addHashEntry(uint64_t hash);

  // Argument parser
  void parseArgs(int argc, char *argv[]);

  // Function to determine the current possible moves
  std::vector<uint8_t> getPossibleMoveIds(const Frame &frame);

  // Function for the show thread (saves states from time to time to display progress)
  static void *showThreadFunction(void *trainPtr);
  void showSavingLoop();

  // Functions that check special flags for a given frame
  void checkSpecialActions(const Frame &frame);
  bool checkFail(const Frame &frame);
  bool checkWin(const Frame &frame);

  // Function to get the static rewards obtained from rules
  float getRuleRewards(const Frame &frame);

  // Function to get magnet information
  magnetInfo_t getKidMagnetValues(const Frame &frame, const int room);
  magnetInfo_t getGuardMagnetValues(const Frame &frame, const int room);

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
