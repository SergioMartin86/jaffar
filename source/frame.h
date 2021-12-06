#pragma once

#define _MAX_FRAME_DIFF 160
#define _MAX_RULE_COUNT 12
#define _MAX_MOVELIST_SIZE 450
#define _MAX_MOVELIST_STORAGE ((_MAX_MOVELIST_SIZE/2) + 1)
#define _FRAME_DATA_SIZE 2714

#include "nlohmann/json.hpp"
#include "rule.h"
#include <string>
#include <vector>

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA"};

enum frameType
{
  f_regular,
  f_win,
  f_fail,
  f_flush
};

extern size_t _maxFrameDiff;

struct frameDiff_t
{
 uint16_t pos;
 uint8_t val;
};

class Frame
{
  public:
  Frame();

  // The score calculated for this frame
  float reward;

  // Positions of the difference with respect to a base frame
  uint16_t frameDiffCount;

  // Positions of the difference with respect to a base frame
  frameDiff_t frameDiffs[_MAX_FRAME_DIFF];

  // Rule status vector
  char rulesStatus[_MAX_RULE_COUNT];

  // Frame type
  frameType _type;

  // Differentiation functions
  inline void computeFrameDifference(const char* __restrict__ baseFrameData, const char* __restrict__ newFrameData)
  {
   frameDiffCount = 0;
   for (uint16_t i = 0; i < _FRAME_DATA_SIZE; i++) if (baseFrameData[i] != newFrameData[i]) frameDiffs[frameDiffCount++].pos = i;
   if (frameDiffCount > _maxFrameDiff) _maxFrameDiff = frameDiffCount;
   if (frameDiffCount > _MAX_FRAME_DIFF) EXIT_WITH_ERROR("[Error] Exceeded maximum frame difference: %d > %d\n", frameDiffCount, _MAX_FRAME_DIFF);
   for (uint16_t i = 0; i < frameDiffCount; i++) frameDiffs[i].val = newFrameData[frameDiffs[i].pos];
  }

  inline void getFrameDataFromDifference(const char* __restrict__ baseFrameData, char* __restrict__ stateData) const
  {
    memcpy(stateData, baseFrameData, _FRAME_DATA_SIZE);
    for (uint16_t i = 0; i < frameDiffCount; i++) stateData[frameDiffs[i].pos] = frameDiffs[i].val;
  }

#ifndef JAFFAR_DISABLE_MOVE_HISTORY

  // Stores the entire move history of the frame
  char moveHistory[_MAX_MOVELIST_STORAGE];

  // Move r/w operations
  inline void setMove(const size_t idx, const uint8_t move)
  {
    size_t basePos = idx / 2;
    uint8_t baseVal = moveHistory[basePos];
    uint8_t newVal;

    if (idx % 2 == 0) newVal = (baseVal & 0xF0) | (move & 0x0F);
    if (idx % 2 == 1) newVal = (baseVal & 0x0F) | (move << 4);

    moveHistory[basePos] = newVal;
  }

  inline uint8_t getMove(const size_t idx)
  {
   size_t basePos = idx / 2;
   uint8_t val = moveHistory[basePos];
   if (idx % 2 == 0) val = val & 0x0F;
   if (idx % 2 == 1) val = val >> 4;
   return val;
  }

#endif

};

