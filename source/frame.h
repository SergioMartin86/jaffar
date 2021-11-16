#pragma once

#define _FRAME_DATA_SIZE 2710
#define _MAX_MOVE_SIZE 4
#define _MAX_FRAME_DIFF 150

#include "nlohmann/json.hpp"
#include "rule.h"
#include <string>
#include <vector>

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA"};

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
  void computeFrameDifference(const std::string& baseFrameData, const std::string& newFrameData);
  std::string getFrameDataFromDifference(const std::string& baseFrameData) const;

  // Serialization functions
  static size_t getSerializationSize();
  void serialize(char *output);
  void deserialize(const char *input);

  // Additional local metadata
  bool isRestart;

  // Move r/w operations
  void setMove(const size_t idx, const uint8_t move);
  uint8_t getMove(const size_t idx);

  Frame &operator=(Frame sourceFrame);
};

extern size_t _ruleCount;
extern size_t _maxSteps;
extern size_t _moveListStorageSize;
