#pragma once

#define _FRAME_DATA_SIZE 2710
#define _MAX_FRAME_DIFF 256

#include "nlohmann/json.hpp"
#include "rule.h"
#include <string>
#include <vector>

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA"};

extern size_t _ruleCount;
extern size_t _maxSteps;
extern size_t _moveListStorageSize;
extern size_t _maxFrameDiff;

class Frame
{
  public:
  Frame();

  // Stores the entire move history of the frame
  std::vector<char> moveHistory;

  // The score calculated for this frame
  float reward;

  // Rule status vector
  std::vector<char> rulesStatus;

  // Positions of the difference with respect to a base frame
  uint16_t frameDiffCount;

  // Positions of the difference with respect to a base frame
  uint16_t frameDiffPositions[_MAX_FRAME_DIFF];

  // Values of the difference with respect to a base frame
  uint8_t frameDiffValues[_MAX_FRAME_DIFF];

  // Differentiation functions

  inline void computeFrameDifference(const char* __restrict__ baseFrameData, const char* __restrict__ newFrameData)
  {
   frameDiffCount = 0;
   for (uint16_t i = 0; i < _FRAME_DATA_SIZE; i++) if (baseFrameData[i] != newFrameData[i]) frameDiffPositions[frameDiffCount++] = i;
   for (uint16_t i = 0; i < frameDiffCount; i++) frameDiffValues[i] = newFrameData[frameDiffPositions[i]];
   if (frameDiffCount > _maxFrameDiff) _maxFrameDiff = frameDiffCount;
  }

  inline void getFrameDataFromDifference(const char* __restrict__ baseFrameData, char* __restrict__ stateData) const
  {
    memcpy(stateData, baseFrameData, _FRAME_DATA_SIZE);
    for (uint16_t i = 0; i < frameDiffCount; i++) stateData[frameDiffPositions[i]] = frameDiffValues[i];
  }

  // Serialization functions
  static size_t getSerializationSize();
  void serialize(char *output);
  void deserialize(const char *input);

  // Move r/w operations
  void setMove(const size_t idx, const uint8_t move);
  uint8_t getMove(const size_t idx);

  Frame &operator=(Frame sourceFrame);
};
