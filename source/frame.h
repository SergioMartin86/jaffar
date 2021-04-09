#pragma once

#define _FRAME_DATA_SIZE 2689
#define _ROOM_ENTRY_COUNT 256

#include "rule.h"
#include "json.hpp"
#include <vector>
#include <string>

struct Magnet
{
 float intensityX;
 float positionX;
 float intensityY;
 float positionY;
};

class Frame
{
public:

 std::string move;
 float score;
 uint64_t hash;
 std::string frameStateData;

 // Store termination conditions
 bool isFail;
 bool isWin;

 // Magnet vector
 std::vector<Magnet> magnets;

 // Rule status vector
 std::vector<status_t> rulesStatus;

 // Serialization functions
 static size_t getSerializationSize();
 void serialize(char* output);
 void deserialize(const char* input);
};
