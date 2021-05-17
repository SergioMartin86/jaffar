#include "frame.h"
#include "train.h"

size_t _moveListStorageSize;
size_t _ruleCount;
size_t _maxSteps;
bool _storeMoveList;


Frame::Frame()
{
  // Two moves fit in one byte
  moveHistory.resize(_moveListStorageSize);

  // Setting i nitial value of the restart flag
  isRestart = false;
}

size_t Frame::getSerializationSize()
{
  size_t size = 0;

  // Adding move history
  if (_storeMoveList == true)
   size += _moveListStorageSize * sizeof(char);

  // Adding frame state data
  size += _FRAME_DATA_SIZE * sizeof(char);

  // Adding rule status information
  size += _ruleCount * sizeof(char);

  // Adding restart condition
  size += sizeof(bool);

  return size;
}

void Frame::serialize(char *output)
{
  size_t currentPos = 0;

  // Adding move history
  if (_storeMoveList == true)
  {
   memcpy(&output[currentPos], moveHistory.data(), _moveListStorageSize * sizeof(char));
   currentPos += _moveListStorageSize * sizeof(char);
  }

  // Adding frame state data
  memcpy(&output[currentPos], frameStateData.c_str(), _FRAME_DATA_SIZE * sizeof(char));
  currentPos += _FRAME_DATA_SIZE * sizeof(char);

  // Copying Rule status information
  memcpy(&output[currentPos], rulesStatus.data(), _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);

  // Copying restart flag
  memcpy(&output[currentPos], &isRestart, sizeof(bool));
  currentPos += sizeof(bool);
}

void Frame::deserialize(const char *input)
{
  size_t currentPos = 0;

  // Copying move history information
  if (_storeMoveList == true)
  {
   moveHistory.resize(_moveListStorageSize);
   memcpy(moveHistory.data(), &input[currentPos], _moveListStorageSize * sizeof(char));
   currentPos += _moveListStorageSize * sizeof(char);
  }

  // Adding frame state data
  frameStateData.resize(_FRAME_DATA_SIZE);
  for (size_t i = 0; i < _FRAME_DATA_SIZE; i++) frameStateData[i] = input[currentPos++];

  // Copying Rule status information
  rulesStatus.resize(_ruleCount);
  memcpy(rulesStatus.data(), &input[currentPos], _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);

  // Copying restart flag
  memcpy(&isRestart, &input[currentPos], sizeof(bool));
  currentPos += sizeof(bool);
}

Frame &Frame::operator=(Frame sourceFrame)
{
  if (_storeMoveList == true)
   moveHistory = sourceFrame.moveHistory;
  frameStateData = sourceFrame.frameStateData;
  rulesStatus = sourceFrame.rulesStatus;
  isRestart = sourceFrame.isRestart;
  return *this;
}

// Move r/w operations
void Frame::setMove(const size_t idx, const uint8_t move)
{
  size_t basePos = idx / 2;
  uint8_t baseVal = moveHistory[basePos];
  uint8_t newVal;

  if (idx % 2 == 0) newVal = (baseVal & 0xF0) | (move & 0x0F);
  if (idx % 2 == 1) newVal = (baseVal & 0x0F) | (move << 4);

  moveHistory[basePos] = newVal;
}

uint8_t Frame::getMove(const size_t idx)
{
 size_t basePos = idx / 2;
 uint8_t val = moveHistory[basePos];
 if (idx % 2 == 0) val = val & 0x0F;
 if (idx % 2 == 1) val = val >> 4;
 return val;
}

