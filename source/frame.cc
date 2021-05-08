#include "frame.h"
#include "train.h"

size_t _ruleCount;
size_t _maxSteps;
bool _storeMoveList;


Frame::Frame()
{
  moveHistory.resize((_maxSteps + 1));
}

size_t Frame::getSerializationSize()
{
  size_t size = 0;

  // Adding move history
  if (_storeMoveList == true)
   size += (_maxSteps + 1) * sizeof(char);

  // Adding score size
  size += sizeof(float);

  // Adding frame state data
  size += _FRAME_DATA_SIZE * sizeof(char);

  // Adding kid magnet information
  size += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Adding guard magnet information
  size += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Adding rule status information
  size += _ruleCount * sizeof(char);

  return size;
}

void Frame::serialize(char *output)
{
  size_t currentPos = 0;

  // Adding move history
  if (_storeMoveList == true)
  {
   memcpy(&output[currentPos], moveHistory.data(), (_maxSteps + 1) * sizeof(char));
   currentPos += (_maxSteps + 1) * sizeof(char);
  }

  // Adding score
  memcpy(&output[currentPos], &score, sizeof(float));
  currentPos += sizeof(float);

  // Adding frame state data
  memcpy(&output[currentPos], frameStateData.c_str(), _FRAME_DATA_SIZE * sizeof(char));
  currentPos += _FRAME_DATA_SIZE * sizeof(char);

  // Copying kid magnets information
  memcpy(&output[currentPos], kidMagnets.data(), _VISIBLE_ROOM_COUNT * sizeof(Magnet));
  currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Copying guard magnets information
  memcpy(&output[currentPos], guardMagnets.data(), _VISIBLE_ROOM_COUNT * sizeof(Magnet));
  currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Copying Rule status information
  memcpy(&output[currentPos], rulesStatus.data(), _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);
}

void Frame::deserialize(const char *input)
{
  size_t currentPos = 0;

  // Copying move history information
  if (_storeMoveList == true)
  {
   moveHistory.resize((_maxSteps + 1));
   memcpy(moveHistory.data(), &input[currentPos], (_maxSteps + 1) * sizeof(char));
   currentPos += (_maxSteps + 1) * sizeof(char);
  }

  // Adding score
  memcpy(&score, &input[currentPos], sizeof(float));
  currentPos += sizeof(float);

  // Adding frame state data
  frameStateData.resize(_FRAME_DATA_SIZE);
  for (size_t i = 0; i < _FRAME_DATA_SIZE; i++) frameStateData[i] = input[currentPos++];

  // Copying Kid magnets information
  kidMagnets.resize(_VISIBLE_ROOM_COUNT);
  memcpy(kidMagnets.data(), &input[currentPos], _VISIBLE_ROOM_COUNT * sizeof(Magnet));
  currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Copying Guard magnets information
  guardMagnets.resize(_VISIBLE_ROOM_COUNT);
  memcpy(guardMagnets.data(), &input[currentPos], _VISIBLE_ROOM_COUNT * sizeof(Magnet));
  currentPos += _VISIBLE_ROOM_COUNT * sizeof(Magnet);

  // Copying Rule status information
  rulesStatus.resize(_ruleCount);
  memcpy(rulesStatus.data(), &input[currentPos], _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);
}

Frame &Frame::operator=(Frame sourceFrame)
{
  if (_storeMoveList == true) moveHistory = sourceFrame.moveHistory;
  score = sourceFrame.score;
  frameStateData = sourceFrame.frameStateData;
  kidMagnets = sourceFrame.kidMagnets;
  guardMagnets = sourceFrame.guardMagnets;
  rulesStatus = sourceFrame.rulesStatus;

  return *this;
}
