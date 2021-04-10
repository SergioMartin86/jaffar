#include "frame.h"
#include "search.h"

size_t Frame::getSerializationSize()
{
 size_t size = 0;

 // Adding move (max size of 8)
 size += 8 * sizeof(char);

 // Adding score size
 size += sizeof(float);

 // Adding hash size
 size += sizeof(uint64_t);

 // Adding frame state data
 size += _FRAME_DATA_SIZE * sizeof(char);

 // Adding magnet information
 size += _ROOM_ENTRY_COUNT * sizeof(Magnet);

 // Adding rule status information
 size += _ruleCount * sizeof(status_t);

 return size;
}

void Frame::serialize(char* output)
{
   size_t currentPos = 0;

   // Check that the size does not exceed limits
   const size_t moveSize = move.size();
   if (moveSize > 7) EXIT_WITH_ERROR("Move %s exceeds size limit of 7\n", move.c_str());

   // Copying move char by char
   for (size_t i = 0; i < moveSize; i++) output[currentPos++] = move[i];

   // Filling with zeros until 8th position
   while(currentPos < 8) output[currentPos++] = '\0';

   // Adding score
   memcpy(&output[currentPos], &score, sizeof(float));
   currentPos += sizeof(float);

   // Adding hash
   memcpy(&output[currentPos], &hash, sizeof(uint64_t));
   currentPos += sizeof(uint64_t);

   // Adding frame state data
   memcpy(&output[currentPos], frameStateData.c_str(), _FRAME_DATA_SIZE * sizeof(char));
   currentPos += _FRAME_DATA_SIZE * sizeof(char);

   // Copying magnets information
   memcpy(&output[currentPos], magnets.data(), _ROOM_ENTRY_COUNT * sizeof(Magnet));
   currentPos += _ROOM_ENTRY_COUNT * sizeof(Magnet);

   // Copying Rule status information
   memcpy(&output[currentPos], rulesStatus.data(), _ruleCount * sizeof(status_t));
   currentPos += _ruleCount * sizeof(status_t);
}

void Frame::deserialize(const char* input)
{
 size_t currentPos = 0;

 // Parsing input string
 move = std::string(&input[currentPos]);
 currentPos = 8;

 // Adding score
 memcpy(&score, &input[currentPos], sizeof(float));
 currentPos += sizeof(float);

 // Adding hash
 memcpy(&hash, &input[currentPos], sizeof(uint64_t));
 currentPos += sizeof(uint64_t);

 // Adding frame state data
 frameStateData.resize(_FRAME_DATA_SIZE);
 for (size_t i = 0; i < _FRAME_DATA_SIZE; i++) frameStateData[i] = input[currentPos++];

 // Copying magnets information
 magnets.resize(_ROOM_ENTRY_COUNT);
 memcpy(magnets.data(), &input[currentPos],  _ROOM_ENTRY_COUNT * sizeof(Magnet));
 currentPos += _ROOM_ENTRY_COUNT * sizeof(Magnet);

 // Copying Rule status information
 rulesStatus.resize(_ruleCount);
 memcpy(rulesStatus.data(), &input[currentPos], _ruleCount * sizeof(status_t));
 currentPos += _ruleCount * sizeof(status_t);
}
