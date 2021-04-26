#pragma once

#define _FRAME_DATA_SIZE 2708
#define _MAX_MOVE_SIZE 4
#define _VISIBLE_ROOM_COUNT 25

#include "nlohmann/json.hpp"
#include "rule.h"
#include <string>
#include <vector>

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD"};

struct Magnet
{
  float intensityY;
  float intensityX;
  float positionX;
};

class Frame
{
  public:
  Frame();

  // Stores the entire move history of the frame
  std::vector<uint8_t> moveHistory;

  // The score calculated for this frame
  float score;

  // Stores the game state data
  std::string frameStateData;

  // Magnet vector
  std::vector<Magnet> magnets;

  // Rule status vector
  std::vector<status_t> rulesStatus;

  // Serialization functions
  static size_t getSerializationSize();
  void serialize(char *output);
  void deserialize(const char *input);

  // Additional local metadata
  bool isWin;
  bool isFail;

  Frame &operator=(Frame sourceFrame);
};

extern size_t _ruleCount;
extern size_t _maxSteps;
