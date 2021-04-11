#pragma once

#define _FRAME_DATA_SIZE 2689
#define _MAX_MOVE_SIZE 4
#define _VISIBLE_ROOM_COUNT 24
#define _VISIBLE_ROOM_OFFSET 1

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

 // Identifier for the frame
 size_t frameId;

 // Move executed by the frame
 std::string currentMove;

 // Stores the entire move history of the frame
 std::string moveHistory;

 // The score calculated for this frame
 float score;

 // Stores the unique hash for this frame
 uint64_t hash;

 // Stores the game state data
 std::string frameStateData;

 // Termination conditions
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
