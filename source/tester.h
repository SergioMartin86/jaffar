#pragma once

#include "miniPoPInstance.h"
#include "common.h"
#include "frame.h"
#include "nlohmann/json.hpp"
#include "rule.h"
#include "state.h"
#include <pthread.h>
#include <string>
#include <vector>

// Number of frames to cache for showing purposes
#define SHOW_FRAME_COUNT 1000

class Tester
{
  public:
  Tester(int argc, char *argv[]);
  void run();

  private:

  // Jaffar script file
  std::string _scriptFile;

  // Storage for source frame data for differential load/save
  char _sourceFrameData[_JAFFAR_FRAME_DATA_SIZE];
  std::unique_ptr<Frame> _baseFrame;

  // Creating State class instance, one per openMP thread
  std::vector<State *> _state;
  size_t _ruleCount;

  // Storage for move list to test
  std::vector<std::string> _moveList;

  // Store the number of openMP threads in use
  int _threadCount;
};
