#pragma once

#include "miniPoP.hpp"
#include <string>

extern const char* seqNames[];
extern const word seqOffsets[];

class miniPoPInstance
{
  public:
  miniPoPInstance();
  ~miniPoPInstance();

  // Initializes the miniPop instance
  void initialize();

  // Starts a given level
  void startLevel(const word level);

  // Set seed
  void setSeed(const dword randomSeed);

  // Draw a single frame
  void draw(ssize_t mins = -1, ssize_t secs = -1, ssize_t ms = -1);

  // Advance a frame
  void advanceFrame(const uint8_t &move);

  // Print information about the current frame
  void printFrameInfo();

  // Function to transfer cache file contents to reduce pressure on I/O
  std::string serializeFileCache();
  void deserializeFileCache(const std::string& cache);

  // Functions to advance/reverse RNG state
  unsigned int advanceRNGState(const unsigned int randomSeed);
  unsigned int reverseRNGState(const unsigned int randomSeed);

  // IGT Timing functions
  size_t getElapsedMins();
  size_t getElapsedSecs();
  size_t getElapsedMilisecs();

  int getKidSequenceId();
  int getGuardSequenceId();
};
