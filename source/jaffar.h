#pragma once

#include <string>
#include <vector>
#include "json.hpp"

// Operation Mode
enum opMode_t
{
  m_train = 0,
  m_play = 1
};

// Jaffar Configuration Struct

struct jaffarConfigStruct
{
  // Operation Mode
  opMode_t opMode;

  // Input Files (from command line)
  std::string inputConfigFile;
  std::string inputSaveFile;
  std::string inputSequenceFile;

  // Sequence to play
  std::string moveSequence;

  // Raw JSON of the config file
  nlohmann::json configJs;

  // Seed override settings
  bool overrideSeedEnabled;
  dword overrideSeedValue;
};

// Argument parser function
void parseArgs(int argc, char* argv[]);

// Configuration Singleton
extern jaffarConfigStruct _jaffarConfig;
