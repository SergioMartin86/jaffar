#pragma once

// Select level we are optimizing jaffar for
#define _JAFFAR_LEVEL 2

// Level 1 Configuration
#if _JAFFAR_LEVEL==1
 #define _MAX_FRAME_DIFF 150
 #define _MAX_RULE_COUNT 10
 #define _MAX_MOVELIST_SIZE 260
#endif

// Level 2 Configuration
#if _JAFFAR_LEVEL==2
 #define _MAX_FRAME_DIFF 180
 #define _MAX_RULE_COUNT 24
 #define _MAX_MOVELIST_SIZE 950
#endif

// Level 11 Configuration
#if _JAFFAR_LEVEL==11
 #define _MAX_FRAME_DIFF 250
 #define _MAX_RULE_COUNT 32
 #define _MAX_MOVELIST_SIZE 1440
#endif


#define _MAX_MOVELIST_STORAGE ((_MAX_MOVELIST_SIZE/2) + 1)
#define _FRAME_DATA_SIZE 2710

#include "nlohmann/json.hpp"
#include "rule.h"
#include <string>
#include <vector>

const std::vector<std::string> _possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA"};

extern size_t _ruleCount;
extern size_t _maxSteps;
extern size_t _maxFrameDiff;

class Frame
{
  public:
  Frame();

  // The score calculated for this frame
  float reward;

  // Positions of the difference with respect to a base frame
  uint16_t frameDiffCount;

  // Positions of the difference with respect to a base frame
  uint16_t frameDiffPositions[_MAX_FRAME_DIFF];

  // Values of the difference with respect to a base frame
  uint8_t frameDiffValues[_MAX_FRAME_DIFF];

  // Stores the entire move history of the frame
  char moveHistory[_MAX_MOVELIST_STORAGE];

  // Rule status vector
  char rulesStatus[_MAX_RULE_COUNT];

  // Differentiation functions

  inline void computeFrameDifference(const char* __restrict__ baseFrameData, const char* __restrict__ newFrameData)
  {
   frameDiffCount = 0;
   for (uint16_t i = 0; i < _FRAME_DATA_SIZE; i++) if (baseFrameData[i] != newFrameData[i]) frameDiffPositions[frameDiffCount++] = i;
   if (frameDiffCount > _maxFrameDiff) _maxFrameDiff = frameDiffCount;
   if (frameDiffCount > _MAX_FRAME_DIFF) EXIT_WITH_ERROR("[Error] Exceeded maximum frame difference: %d > %d\n", frameDiffCount, _MAX_FRAME_DIFF);
   for (uint16_t i = 0; i < frameDiffCount; i++) frameDiffValues[i] = newFrameData[frameDiffPositions[i]];
  }

  inline void getFrameDataFromDifference(const char* __restrict__ baseFrameData, char* __restrict__ stateData) const
  {
    memcpy(stateData, baseFrameData, _FRAME_DATA_SIZE);
    for (uint16_t i = 0; i < frameDiffCount; i++) stateData[frameDiffPositions[i]] = frameDiffValues[i];
  }

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

};

