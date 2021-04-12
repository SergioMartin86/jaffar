#include "frame.h"
#include "search.h"

size_t Frame::getSerializationSize()
{
 size_t size = 0;

 // Adding current move
 size += _MAX_MOVE_SIZE * sizeof(char);

 // Adding base frame id
 size += sizeof(size_t);

 // Adding score size
 size += sizeof(float);

 // Adding hash size
 size += sizeof(uint64_t);

 // Adding frame state data
 size += _FRAME_DATA_SIZE * sizeof(char);

 // Adding magnet information
 size += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

 // Adding rule status information
 size += _ruleCount * sizeof(status_t);

 return size;
}

void Frame::serialize(char* output)
{
   size_t currentPos = 0;

   // Check that the size does not exceed limits
   const size_t moveSize = currentMove.size();
   if (moveSize >= _MAX_MOVE_SIZE) EXIT_WITH_ERROR("Move %s exceeds size limit of %d\n", currentMove.c_str(), _MAX_MOVE_SIZE);

   // Copying move char by char
   for (size_t i = 0; i < moveSize; i++) output[currentPos++] = currentMove[i];

   // Filling with zeros until final position
   while(currentPos < _MAX_MOVE_SIZE) output[currentPos++] = '\0';

   // Adding base frame id
   memcpy(&output[currentPos], &frameId, sizeof(size_t));
   currentPos += sizeof(size_t);

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
   memcpy(&output[currentPos], magnets.data(), _VISIBLE_ROOM_COUNT * sizeof(Magnet));
   currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

   // Copying Rule status information
   memcpy(&output[currentPos], rulesStatus.data(), _ruleCount * sizeof(status_t));
   currentPos += _ruleCount * sizeof(status_t);
}

void Frame::deserialize(const char* input)
{
 size_t currentPos = 0;

 // Parsing input string
 currentMove = std::string(&input[currentPos]);
 currentPos = _MAX_MOVE_SIZE;

 // Adding base frame id
 memcpy(&frameId, &input[currentPos], sizeof(size_t));
 currentPos += sizeof(size_t);

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
 magnets.resize(_VISIBLE_ROOM_COUNT);
 memcpy(magnets.data(), &input[currentPos],  _VISIBLE_ROOM_COUNT * sizeof(Magnet));
 currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

 // Copying Rule status information
 rulesStatus.resize(_ruleCount);
 memcpy(rulesStatus.data(), &input[currentPos], _ruleCount * sizeof(status_t));
 currentPos += _ruleCount * sizeof(status_t);
}

Frame& Frame::operator=(Frame sourceFrame)
{
 frameId = sourceFrame.frameId;
 currentMove = sourceFrame.currentMove;
 moveHistory = sourceFrame.moveHistory;
 score = sourceFrame.score;
 hash = sourceFrame.hash;
 frameStateData = sourceFrame.frameStateData;
 magnets = sourceFrame.magnets;
 rulesStatus = sourceFrame.rulesStatus;

 return *this;
}


