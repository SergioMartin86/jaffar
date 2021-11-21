#pragma once

#include "miniPoP.hpp"
#include <string>

class miniPoPInstance
{
  public:
  miniPoPInstance(const char* libraryFile, const bool multipleLibraries);
  ~miniPoPInstance();

  // Initializes the sdlPop instance
  void initialize(const bool useGUI);

  // Starts a given level
  void startLevel(const word level);

  // Set seed
  void setSeed(const dword randomSeed);

  // Draw a single frame
  void draw(ssize_t mins = -1, ssize_t secs = -1, ssize_t ms = -1);

  // Perform a single move
  void performMove(const std::string &move);

  // Advance a frame
  void advanceFrame();

  // Print information about the current frame
  void printFrameInfo();

  // Function to transfer cache file contents to reduce pressure on I/O
  std::string serializeFileCache();
  void deserializeFileCache(const std::string& cache);

  bool isExitDoorOpen;

  // Check if exit door is open
  bool isLevelExitDoorOpen();

  // Storing previously drawn room
  word _prevDrawnRoom;

  // Functions to advance/reverse RNG state
  unsigned int advanceRNGState(const unsigned int randomSeed);
  unsigned int reverseRNGState(const unsigned int randomSeed);

  // IGT Timing functions
  size_t getElapsedMins();
  size_t getElapsedSecs();
  size_t getElapsedMilisecs();
};
