#include "frame.h"
#include "train.h"

size_t _moveListStorageSize;
size_t _ruleCount;
size_t _maxSteps;
bool _storeMoveList;
size_t _maxFrameDiff;

Frame::Frame()
{
  // Setting initially with no differences wrt base frame
  frameDiffCount = 0;
}

