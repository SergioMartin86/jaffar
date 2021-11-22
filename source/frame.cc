#include "frame.h"
#include "train.h"

size_t _moveListStorageSize;
size_t _ruleCount;
size_t _maxSteps;
bool _storeMoveList;
size_t _maxFrameDiff;

Frame::Frame()
{
  // Two moves fit in one byte
  if (_storeMoveList) moveHistory.resize(_moveListStorageSize);

  // Setting initially with no differences wrt base frame
  frameDiffCount = 0;
}

size_t Frame::getSerializationSize()
{
  size_t size = 0;

  // Adding move history
  if (_storeMoveList == true)
   size += _moveListStorageSize * sizeof(char);

  // Adding diff state data
  size += sizeof(uint16_t);
  size += _MAX_FRAME_DIFF * sizeof(uint16_t);
  size += _MAX_FRAME_DIFF * sizeof(uint8_t);

  // Adding rule status information
  size += _ruleCount * sizeof(char);

  // Adding reward
  size += sizeof(float);

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

  // Adding frame difference data
  memcpy(&output[currentPos], &frameDiffCount, sizeof(uint16_t));
  currentPos += sizeof(uint16_t);
  memcpy(&output[currentPos], frameDiffPositions, _MAX_FRAME_DIFF * sizeof(uint16_t));
  currentPos += _MAX_FRAME_DIFF * sizeof(uint16_t);
  memcpy(&output[currentPos], frameDiffValues, _MAX_FRAME_DIFF * sizeof(uint8_t));
  currentPos += _MAX_FRAME_DIFF * sizeof(uint8_t);

  // Copying Rule status information
  memcpy(&output[currentPos], rulesStatus.data(), _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);

  // Copying reward
  memcpy(&output[currentPos], &reward, sizeof(float));
  currentPos += sizeof(float);
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

  // Adding frame difference data
  memcpy(&frameDiffCount, &input[currentPos], sizeof(uint16_t));
  currentPos += sizeof(uint16_t);
  memcpy(frameDiffPositions, &input[currentPos], _MAX_FRAME_DIFF * sizeof(uint16_t));
  currentPos += _MAX_FRAME_DIFF * sizeof(uint16_t);
  memcpy(frameDiffValues, &input[currentPos], _MAX_FRAME_DIFF * sizeof(uint8_t));
  currentPos += _MAX_FRAME_DIFF * sizeof(uint8_t);

  // Copying Rule status information
  rulesStatus.resize(_ruleCount);
  memcpy(rulesStatus.data(), &input[currentPos], _ruleCount * sizeof(char));
  currentPos += _ruleCount * sizeof(char);

  // Copying reward flag
  memcpy(&reward, &input[currentPos], sizeof(float));
  currentPos += sizeof(float);
}

Frame &Frame::operator=(Frame sourceFrame)
{
  if (_storeMoveList == true) moveHistory = sourceFrame.moveHistory;
  frameDiffCount = sourceFrame.frameDiffCount;
  memcpy(frameDiffPositions, sourceFrame.frameDiffPositions, _MAX_FRAME_DIFF * sizeof(uint16_t));
  memcpy(frameDiffValues, sourceFrame.frameDiffValues, _MAX_FRAME_DIFF * sizeof(uint8_t));
  rulesStatus = sourceFrame.rulesStatus;
  reward = sourceFrame.reward;
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

void Frame::computeFrameDifference(const std::string& baseFrameData, const std::string& newFrameData)
{
 frameDiffCount = 0;

 for (uint16_t i = 0; i < baseFrameData.size(); i++)
  if (baseFrameData[i] != newFrameData[i])
  {
   frameDiffPositions[frameDiffCount] = i;
   frameDiffValues[frameDiffCount] = newFrameData[i];
   frameDiffCount++;
  }

 if (frameDiffCount > _maxFrameDiff) _maxFrameDiff = frameDiffCount;
}

std::string Frame::getFrameDataFromDifference(const std::string& baseFrameData) const
{
  auto newFrameData = baseFrameData;
  for (uint16_t i = 0; i < frameDiffCount; i++) newFrameData[frameDiffPositions[i]] = frameDiffValues[i];
  return newFrameData;
}

