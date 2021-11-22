#include "train.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>
#include <unistd.h>
#include <algorithm>
#include <set>
#include <boost/sort/sort.hpp>

void Train::run()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Launching Jaffar Version %s...\n", JAFFAR_VERSION);
  printf("[Jaffar] Using configuration file: "); printf("%s ", _scriptFile.c_str()); printf("\n");
  printf("[Jaffar] Max Frame DB entries: %lu\n", _maxDatabaseSize);

  if (_outputSaveBestSeconds > 0)
  {
    printf("[Jaffar] Saving best frame every: %.3f seconds.\n", _outputSaveBestSeconds);
    printf("[Jaffar]  + Savefile Path: %s\n", _outputSaveBestPath.c_str());
    printf("[Jaffar]  + Solution Path: %s\n", _outputSolutionBestPath.c_str());
  }

  if (_outputSaveCurrentSeconds > 0)
  {
    printf("[Jaffar] Saving current frame every: %.3f seconds.\n", _outputSaveCurrentSeconds);
    printf("[Jaffar]  + Savefile Path: %s\n", _outputSaveCurrentPath.c_str());
    printf("[Jaffar]  + Solution Path: %s\n", _outputSolutionCurrentPath.c_str());
  }

  // Sleep for a second to show this message
  sleep(2);

  auto searchTimeBegin = std::chrono::steady_clock::now();      // Profiling
  auto currentStepTimeBegin = std::chrono::steady_clock::now(); // Profiling

  // Storage for termination trigger
  bool terminate = false;

  while (terminate == false)
  {
    // Profiling information
    auto searchTimeEnd = std::chrono::steady_clock::now();                                                            // Profiling
    _searchTotalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(searchTimeEnd - searchTimeBegin).count(); // Profiling

    auto currentStepTimeEnd = std::chrono::steady_clock::now();                                                                 // Profiling
    _currentStepTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentStepTimeEnd - currentStepTimeBegin).count(); // Profiling
    currentStepTimeBegin = std::chrono::steady_clock::now();                                                                    // Profiling

    // Printing search status
    printTrainStatus();

    // Updating show frames
    if (_frameDB.size() > 0)
      for (size_t i = 0; i < SHOW_FRAME_COUNT; i++)
        _showFrameDB[i] = *_frameDB[i % _frameDB.size()];

    /////////////////////////////////////////////////////////////////
    /// Main frame processing cycle begin
    /////////////////////////////////////////////////////////////////

    computeFrames();

    /////////////////////////////////////////////////////////////////
    /// Main frame processing cycle end
    /////////////////////////////////////////////////////////////////

    // Terminate if DB is depleted and no winning rule was found
    if (_frameDB.size() == 0)
    {
      printf("[Jaffar] Frame database depleted with no winning frames, finishing...\n");
      terminate = true;
    }

    // Terminate if a winning rule was found
    if (_winFrameFound == true)
    {
      printf("[Jaffar] Winning frame reached in %lu moves, finishing...\n", _currentStep+1);
      terminate = true;
    }

    // Terminate if maximum number of frames was reached
    if (_currentStep > _maxSteps)
    {
      printf("[Jaffar] Maximum frame number reached, finishing...\n");
      terminate = true;
    }

    // Advancing step
    _currentStep++;
  }

  // Print winning frame if found
  if (_winFrameFound == true)
  {
    printf("[Jaffar] Win Frame Information:\n");
    size_t timeStep = _currentStep-1;
    size_t curMins = timeStep / 720;
    size_t curSecs = (timeStep - (curMins * 720)) / 12;
    size_t curMilliSecs = ceil((double)(timeStep - (curMins * 720) - (curSecs * 12)) / 0.012);
    printf("[Jaffar]  + Solution IGT:  %2lu:%02lu.%03lu\n", curMins, curSecs, curMilliSecs);

    _winFrame.getFrameDataFromDifference(_sourceFrameData, _state[0]->_stateData);
    _state[0]->pushState();
    _miniPop[0]->printFrameInfo();

    printRuleStatus(_winFrame);

    // Print Move History
    if (_storeMoveList)
    {
     printf("[Jaffar]  + Move List: ");
     for (size_t i = 0; i <= _currentStep; i++)
       printf("%s ", _possibleMoves[_winFrame.getMove(i)].c_str());
     printf("\n");
    }
  }

  // Marking the end of the run
  _hasFinalized = true;

  // Stopping show thread
  pthread_join(_showThreadId, NULL);
}

void Train::printRuleStatus(const Frame &frame)
{
 printf("[Jaffar]  + Rule Status: ");
 for (size_t i = 0; i < frame.rulesStatus.size(); i++)
 {
   if (i > 0 && i % 60 == 0) printf("\n                         ");
   printf("%d", frame.rulesStatus[i] ? 1 : 0);
 }
 printf("\n");
}

void Train::computeFrames()
{
  // Initializing counters
  _stepFramesProcessedCounter = 0;
  _newCollisionCounter = 0;

  // Creating new fresh hash database, discarding an old one
  delete _hashDatabases[0];
  _hashDatabases.add(new absl::flat_hash_set<uint64_t>());

  // Initializing step timers
  _stepHashCalculationTime = 0.0;
  _stepHashCheckingTime = 0.0;
  _stepFrameAdvanceTime = 0.0;
  _stepFrameSerializationTime = 0.0;
  _stepFrameDeserializationTime = 0.0;

  // Creating thread-local storage for new frames
  std::vector<std::vector<std::unique_ptr<Frame>>> newThreadFrames(_threadCount);

  // Processing frame database in parallel
  auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling
  #pragma omp parallel
  {
    // Getting thread id
    int threadId = omp_get_thread_num();

    // Profiling timers
    double threadHashCalculationTime = 0.0;
    double threadHashCheckingTime = 0.0;
    double threadFrameAdvanceTime = 0.0;
    double threadFrameSerializationTime = 0.0;
    double threadFrameDeserializationTime = 0.0;
    double threadFrameDecodingTime = 0.0;
    double threadFrameEncodingTime = 0.0;

    // Computing always the last frame while resizing the database to reduce memory footprint
    #pragma omp for schedule(guided)
    for (size_t baseFrameIdx = 0; baseFrameIdx < _frameDB.size(); baseFrameIdx++)
    {
      // Storage for the base frame
      const auto& baseFrame = _frameDB[baseFrameIdx];

      // Getting possible moves for the current frame
      std::vector<uint8_t> possibleMoveIds = getPossibleMoveIds(*baseFrame);

      // Running possible moves
      for (size_t idx = 0; idx < possibleMoveIds.size(); idx++)
      {
        // Increasing  frames processed counter
        _stepFramesProcessedCounter++;

        // Getting possible move id
        auto moveId = possibleMoveIds[idx];

        // Getting possible move string
        std::string move = _possibleMoves[moveId].c_str();

        // Loading frame state
        auto t0 = std::chrono::steady_clock::now(); // Profiling
        baseFrame->getFrameDataFromDifference(_sourceFrameData, _state[threadId]->_stateData);
        auto tf = std::chrono::steady_clock::now();
        threadFrameDecodingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

        t0 = std::chrono::steady_clock::now(); // Profiling
        _state[threadId]->pushState();
        tf = std::chrono::steady_clock::now();
        threadFrameDeserializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

        // Getting current level
        auto curLevel = current_level;

        // Perform the selected move
        t0 = std::chrono::steady_clock::now(); // Profiling
        _miniPop[threadId]->performMove(move);

        // Advance a single frame
        _miniPop[threadId]->advanceFrame();
        tf = std::chrono::steady_clock::now();
        threadFrameAdvanceTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

        // Getting new level (if changed)
        auto newLevel = current_level;

        // Compute hash value
        t0 = std::chrono::steady_clock::now(); // Profiling
        auto hash = _state[threadId]->computeHash();
        tf = std::chrono::steady_clock::now();
        threadHashCalculationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

        // Checking for the existence of the hash in the hash databases (except the current one)
        t0 = std::chrono::steady_clock::now(); // Profiling
        bool collisionDetected = false;
        for (ssize_t i = _hashAgeThreshold-2; i >= 0 && collisionDetected == false; i--)
         collisionDetected |= _hashDatabases[i]->contains(hash);

        // If no collision detected with the normal databases, check the newest
        if (collisionDetected == false)
         #pragma omp critical
         {
           collisionDetected |= _hashDatabases[_hashAgeThreshold-1]->contains(hash);

           // If now there, add it now
           if (collisionDetected == false) _hashDatabases[_hashAgeThreshold-1]->insert(hash);
         }
        tf = std::chrono::steady_clock::now();
        threadHashCheckingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

        // If collision detected, increase collision counter, more or less
        if (collisionDetected) _newCollisionCounter++;

        // If collision detected, discard this frame
        if (collisionDetected) continue;

        // Creating new frame, mixing base frame information and the current sdlpop state
        auto newFrame = std::make_unique<Frame>();
        newFrame->rulesStatus = baseFrame->rulesStatus;

        // If required, store move history
        if (_storeMoveList == true)
        {
         newFrame->moveHistory = baseFrame->moveHistory;
         newFrame->setMove(_currentStep, moveId);
        }

        // Evaluating rules on the new frame
        evaluateRules(*newFrame);

        // Checks whether any fail rules were activated
        bool isFailFrame = checkFail(*newFrame);

        // If frame has failed, discard it and proceed to the next one
        if (isFailFrame) continue;

        // Storing the frame data, only if if belongs to the same level
        if (curLevel == newLevel)
        {
         t0 = std::chrono::steady_clock::now(); // Profiling
         _state[threadId]->getState();
         tf = std::chrono::steady_clock::now(); // Profiling
         threadFrameSerializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

         t0 = std::chrono::steady_clock::now(); // Profiling
         newFrame->computeFrameDifference(_sourceFrameData, _state[threadId]->_stateData);
         tf = std::chrono::steady_clock::now(); // Profiling
         threadFrameEncodingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
        }

        // Calculating current reward
        newFrame->reward = getFrameReward(*newFrame);

        // Check if the frame triggers a win condition
        bool isWinFrame = checkWin(*newFrame);

        // If frame has succeded, then flag it
        if (isWinFrame)
        {
          _winFrameFound = true;
           #pragma omp critical(winFrame)
           _winFrame = *newFrame;
        }

        // Adding novel frame in the next frame database
        newThreadFrames[threadId].push_back(std::move(newFrame));
      }

      // Freeing memory for the used base frame
      _frameDB[baseFrameIdx].reset();
    }

    // Updating timers
    #pragma omp critical
    {
     _stepHashCalculationTime += threadHashCalculationTime;
     _stepHashCheckingTime += threadHashCheckingTime;
     _stepFrameAdvanceTime += threadFrameAdvanceTime;
     _stepFrameSerializationTime += threadFrameSerializationTime;
     _stepFrameDeserializationTime += threadFrameDeserializationTime;
     _stepFrameEncodingTime += threadFrameEncodingTime;
     _stepFrameDecodingTime += threadFrameDecodingTime;
    }
  }

  auto frameComputationTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
  _frameComputationTime = std::chrono::duration_cast<std::chrono::nanoseconds>(frameComputationTimeEnd - frameComputationTimeBegin).count(); // Profiling

  // Updating timer averages
  _stepHashCalculationTime /= _threadCount;
  _stepHashCheckingTime /= _threadCount;
  _stepFrameAdvanceTime /= _threadCount;
  _stepFrameSerializationTime /= _threadCount;
  _stepFrameDeserializationTime /= _threadCount;
  _stepFrameEncodingTime /= _threadCount;
  _stepFrameDecodingTime /= _threadCount;

  // Postprocessing steps
  auto framePostprocessingTimeBegin = std::chrono::steady_clock::now(); // Profiling

  // Getting total new frames and displacements
  size_t totalNewFrameCount = 0;
  std::vector<size_t> totalNewFrameDisplacements(_threadCount);
  for (int i = 0; i < _threadCount; i++) { totalNewFrameDisplacements[i] = totalNewFrameCount; totalNewFrameCount += newThreadFrames[i].size(); }

  // Passing new frames into the new frame database
  _frameDB.resize(totalNewFrameCount);
  #pragma omp parallel for
  for (int i = 0; i < _threadCount; i++)
   for (size_t j = 0; j < newThreadFrames[i].size(); j++) _frameDB[j + totalNewFrameDisplacements[i]] = std::move(newThreadFrames[i][j]);

  // Sorting local DB frames by reward
  auto DBSortingTimeBegin = std::chrono::steady_clock::now(); // Profiling
  boost::sort::block_indirect_sort(_frameDB.begin(), _frameDB.end(), [](const auto &a, const auto &b) { return a->reward > b->reward; });
  auto DBSortingTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
  _DBSortingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(DBSortingTimeEnd - DBSortingTimeBegin).count(); // Profiling

  // If global frames exceed the maximum allowed, sort and truncate all excessive frames
  if (_frameDB.size() > _maxDatabaseSize) _frameDB.resize(_maxDatabaseSize);

  // Storing best frame
  if (_frameDB.empty() == false) _bestFrame = *_frameDB[0];

  // Summing frame processing counters
  _totalFramesProcessedCounter += _stepFramesProcessedCounter;

  // Re-calculating global collision counter
  _hashCollisions += _newCollisionCounter;

  auto framePostprocessingTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
  _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePostprocessingTimeEnd - framePostprocessingTimeBegin).count(); // Profiling
}

void Train::evaluateRules(Frame &frame)
{
  // Getting thread id
  int threadId = omp_get_thread_num();

  for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  {
    // Evaluate rule only if it's active
    if (frame.rulesStatus[ruleId] == false)
    {
      // Checking dependencies first. If not met, continue to the next rule
      bool dependenciesMet = true;
      for (size_t i = 0; i < _rules[threadId][ruleId]->_dependenciesIndexes.size(); i++)
        if (frame.rulesStatus[_rules[threadId][ruleId]->_dependenciesIndexes[i]] == false)
          dependenciesMet = false;

      // If dependencies aren't met, then continue to next rule
      if (dependenciesMet == false) continue;

      // Checking if conditions are met
      bool isSatisfied = _rules[threadId][ruleId]->evaluate();

      // If it's achieved, update its status and run its actions
      if (isSatisfied) satisfyRule(frame, ruleId);
    }
  }
}

bool Train::checkFail(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
   if (_rules[threadId][ruleId]->_isFailRule == true)
    return true;

 return false;
}

bool Train::checkWin(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
   if (_rules[threadId][ruleId]->_isWinRule == true)
    return true;

 return false;
}

float Train::getRuleRewards(const Frame &frame)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 float reward = 0.0;

 for (size_t ruleId = 0; ruleId < _rules[threadId].size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
    reward += _rules[threadId][ruleId]->_reward;
 return reward;
}

void Train::satisfyRule(Frame &frame, const size_t ruleId)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Recursively run actions for the yet unsatisfied rules that are satisfied by this one and mark them as satisfied
 for (size_t satisfiedIdx = 0; satisfiedIdx < _rules[threadId][ruleId]->_satisfiesIndexes.size(); satisfiedIdx++)
 {
  // Obtaining index
  size_t subRuleId = _rules[threadId][ruleId]->_satisfiesIndexes[satisfiedIdx];

  // Only activate it if it hasn't been activated before
  if(frame.rulesStatus[subRuleId] == false) satisfyRule(frame, subRuleId);
 }

 // Setting status to satisfied
 frame.rulesStatus[ruleId] = true;
}

void Train::printTrainStatus()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Current Step #: %lu / %lu\n", _currentStep, _maxSteps);

  size_t timeStep = _currentStep-1;
  size_t curMins = timeStep / 720;
  size_t curSecs = (timeStep - (curMins * 720)) / 12;
  size_t curMilliSecs = ceil((double)(timeStep - (curMins * 720) - (curSecs * 12)) / 0.012);

  size_t maxStep = _maxSteps-1;
  size_t maxMins = maxStep / 720;
  size_t maxSecs = (maxStep - (maxMins * 720)) / 12;
  size_t maxMilliSecs = ceil((double)(maxStep - (maxMins * 720) - (maxSecs * 12)) / 0.012);

  printf("[Jaffar] Current IGT:  %2lu:%02lu.%03lu / %2lu:%02lu.%03lu\n", curMins, curSecs, curMilliSecs, maxMins, maxSecs, maxMilliSecs);
  printf("[Jaffar] Best Reward: %f\n", _bestFrameReward);
  printf("[Jaffar] Database Size: %lu / %lu\n", _frameDB.size(), _maxDatabaseSize);
  printf("[Jaffar] Frames Processed: (Step/Total): %lu / %lu\n", _stepFramesProcessedCounter, _totalFramesProcessedCounter);
  printf("[Jaffar] Elapsed Time (Step/Total):   %3.3fs / %3.3fs\n", _currentStepTime / 1.0e+9, _searchTotalTime / 1.0e+9);
  printf("[Jaffar]   + Frame Computation:       %3.3fs\n", _frameComputationTime / 1.0e+9);
  printf("[Jaffar]     + Hash Calculation:        %3.3fs\n", _stepHashCalculationTime / 1.0e+9);
  printf("[Jaffar]     + Hash Checking:           %3.3fs\n", _stepHashCheckingTime / 1.0e+9);
  printf("[Jaffar]     + Frame Advance:           %3.3fs\n", _stepFrameAdvanceTime / 1.0e+9);
  printf("[Jaffar]     + Frame Serialization:     %3.3fs\n", _stepFrameSerializationTime / 1.0e+9);
  printf("[Jaffar]     + Frame Deserialization:   %3.3fs\n", _stepFrameDeserializationTime / 1.0e+9);
  printf("[Jaffar]     + Frame Encoding:          %3.3fs\n", _stepFrameEncodingTime / 1.0e+9);
  printf("[Jaffar]     + Frame Decoding:          %3.3fs\n", _stepFrameDecodingTime / 1.0e+9);
  printf("[Jaffar]   + Frame Postprocessing:    %3.3fs\n", _framePostprocessingTime / 1.0e+9);
  printf("[Jaffar]     + DB Sorting               %3.3fs\n", _DBSortingTime / 1.0e+9);
  printf("[Jaffar] Performance: %.3f Frames/s\n", (double)_stepFramesProcessedCounter / (_currentStepTime / 1.0e+9));
  printf("[Jaffar] Max Frame State Difference: %lu\n", _maxFrameDiff);
  printf("[Jaffar] Hash DB Collisions: %lu\n", _hashCollisions);

  size_t hashDatabasesEntries = 0;
  for (size_t i = 0; i < _hashDatabases.size(); i++) hashDatabasesEntries += _hashDatabases[i]->size();
  printf("[Jaffar] Hash DB Entries: %lu\n", hashDatabasesEntries);
  printf("[Jaffar] Frame DB Size: %.3fmb\n", (double)(_frameDB.size() * Frame::getSerializationSize()) / (1024.0 * 1024.0));
  printf("[Jaffar] Hash DB Size: %.3fmb\n", (double)(hashDatabasesEntries * sizeof(uint64_t)) / (1024.0 * 1024.0));
  printf("[Jaffar] Best Frame Information:\n");

  _bestFrame.getFrameDataFromDifference(_sourceFrameData, _state[0]->_stateData);
  _state[0]->pushState();
  _miniPop[0]->printFrameInfo();
  printRuleStatus(_bestFrame);

  // Getting kid room
  int kidCurrentRoom = Kid.room;

  // Getting magnet values for the kid
  auto kidMagnet = getKidMagnetValues(_bestFrame, kidCurrentRoom);

  printf("[Jaffar]  + Kid Horizontal Magnet Intensity / Position: %.1f / %.0f\n", kidMagnet.intensityX, kidMagnet.positionX);
  printf("[Jaffar]  + Kid Vertical Magnet Intensity: %.1f\n", kidMagnet.intensityY);

  // Getting guard room
  int guardCurrentRoom = Guard.room;

  // Getting magnet values for the guard
  auto guardMagnet = getGuardMagnetValues(_bestFrame, guardCurrentRoom);

  printf("[Jaffar]  + Guard Horizontal Magnet Intensity / Position: %.1f / %.0f\n", guardMagnet.intensityX, guardMagnet.positionX);
  printf("[Jaffar]  + Guard Vertical Magnet Intensity: %.1f\n", guardMagnet.intensityY);

  // Print Move History
  if (_storeMoveList)
  {
   printf("[Jaffar]  + Last 30 Moves: ");
   size_t startMove = (size_t)std::max((int)0, (int)_currentStep-30);
   for (size_t i = startMove; i <= _currentStep; i++)
     printf("%s ", _possibleMoves[_bestFrame.getMove(i)].c_str());
   printf("\n");
  }
}

magnetInfo_t Train::getKidMagnetValues(const Frame &frame, const int room)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < frame.rulesStatus.size(); ruleId++)
 {
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[threadId][ruleId];

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetPositionX.size(); i++)
     if (rule->_kidMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_kidMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetIntensityX.size(); i++)
     if (rule->_kidMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_kidMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_kidMagnetIntensityY.size(); i++)
     if (rule->_kidMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_kidMagnetIntensityY[i].value;
  }
 }

 return magnetInfo;
}

magnetInfo_t Train::getGuardMagnetValues(const Frame &frame, const int room)
{
 // Getting thread id
 int threadId = omp_get_thread_num();

 // Storage for magnet information
 magnetInfo_t magnetInfo;
 magnetInfo.positionX = 0.0f;
 magnetInfo.intensityX = 0.0f;
 magnetInfo.intensityY = 0.0f;

 // Iterating rule vector
 for (size_t ruleId = 0; ruleId < frame.rulesStatus.size(); ruleId++)
  if (frame.rulesStatus[ruleId] == true)
  {
    const auto& rule = _rules[threadId][ruleId];

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetPositionX.size(); i++)
     if (rule->_guardMagnetPositionX[i].room == room)
      magnetInfo.positionX = rule->_guardMagnetPositionX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetIntensityX.size(); i++)
     if (rule->_guardMagnetIntensityX[i].room == room)
      magnetInfo.intensityX = rule->_guardMagnetIntensityX[i].value;

    for (size_t i = 0; i < _rules[threadId][ruleId]->_guardMagnetIntensityY.size(); i++)
     if (rule->_guardMagnetIntensityY[i].room == room)
      magnetInfo.intensityY = rule->_guardMagnetIntensityY[i].value;
  }

 return magnetInfo;
}

float Train::getFrameReward(const Frame &frame)
{
  // Accumulator for total reward
  float reward = getRuleRewards(frame);

  // Getting kid room
  int kidCurrentRoom = Kid.room;

  // Getting magnet values for the kid
  auto kidMagnet = getKidMagnetValues(frame, kidCurrentRoom);

  // Getting kid's current frame
  const auto curKidFrame = Kid.frame;

  // Evaluating kidMagnet's reward on the X axis
  const float kidDiffX = std::abs(Kid.x - kidMagnet.positionX);
  reward += (float) kidMagnet.intensityX * (256.0f - kidDiffX);

  // For positive Y axis kidMagnet, rewarding climbing frames
  if ((float) kidMagnet.intensityY > 0.0f)
  {
    // Jumphang, because it preludes climbing (Score + 1-20)
    if (curKidFrame >= 67 && curKidFrame <= 80)
     reward += (float) kidMagnet.intensityY * (curKidFrame - 66);

    // Hang, because it preludes climbing (Score +21)
    if (curKidFrame == 91) reward += 21.0f * (float) kidMagnet.intensityY;

    // Climbing (Score +22-38)
    if (curKidFrame >= 135 && curKidFrame <= 149) reward += (float) kidMagnet.intensityY * (22.0f + (curKidFrame - 134));

    // Adding absolute reward for Y position
    reward += (float) kidMagnet.intensityY * (256.0f - Kid.y);
  }

  // For negative Y axis kidMagnet, rewarding falling/climbing down frames
  if ((float) kidMagnet.intensityY < 0.0f)
  {
    // Turning around, because it generally preludes climbing down
    if (curKidFrame >= 45 && curKidFrame <= 52) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hanging, because it preludes falling
    if (curKidFrame >= 87 && curKidFrame <= 99) reward += -0.5f * (float) kidMagnet.intensityY;

    // Hang drop, because it preludes falling
    if (curKidFrame >= 81 && curKidFrame <= 85) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling start
    if (curKidFrame >= 102 && curKidFrame <= 105) reward += -1.0f * (float) kidMagnet.intensityY;

    // Falling itself
    if (curKidFrame == 106) reward += -2.0f + (float) kidMagnet.intensityY;

    // Climbing down
    if (curKidFrame == 148) reward += -2.0f + (float) kidMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * kidMagnet.intensityY * (Kid.y);
  }

  // Getting guard room
  int guardCurrentRoom = Guard.room;

  // Getting magnet values for the guard
  auto guardMagnet = getGuardMagnetValues(frame, guardCurrentRoom);

  // Getting guard's current frame
  const auto curGuardFrame = Guard.frame;

  // Evaluating guardMagnet's reward on the X axis
  const float guardDiffX = std::abs(Guard.x - guardMagnet.positionX);
  reward += (float) guardMagnet.intensityX * (256.0f - guardDiffX);

  // For positive Y axis guardMagnet
  if ((float) guardMagnet.intensityY > 0.0f)
  {
   // Adding absolute reward for Y position
   reward += (float) guardMagnet.intensityY * (256.0f - Guard.y);
  }

  // For negative Y axis guardMagnet, rewarding falling/climbing down frames
  if ((float) guardMagnet.intensityY < 0.0f)
  {
    // Falling start
    if (curGuardFrame >= 102 && curGuardFrame <= 105) reward += -1.0f * (float) guardMagnet.intensityY;

    // Falling itself
    if (curGuardFrame == 106) reward += -2.0f + (float) guardMagnet.intensityY;

    // Adding absolute reward for Y position
    reward += (float) -1.0f * guardMagnet.intensityY * (Guard.y);
  }

  // Apply bonus when kid is inside a non-visible room
  if (kidCurrentRoom == 0 || kidCurrentRoom >= 25) reward += 128.0f;

  // Returning reward
  return reward;
}

std::vector<uint8_t> Train::getPossibleMoveIds(const Frame &frame)
{
  // Move Ids =        0    1    2    3    4    5     6     7     8    9     10    11    12    13    14
  //_possibleMoves = {".", "S", "U", "L", "R", "D", "LU", "LD", "RU", "RD", "SR", "SL", "SU", "SD", "CA" };

  // Getting thread id
  int threadId = omp_get_thread_num();

  // Loading frame state
  frame.getFrameDataFromDifference(_sourceFrameData, _state[threadId]->_stateData);
  _state[threadId]->pushState();

  // If dead, do nothing
  if (Kid.alive >= 0)
    return {0};

  // For level 1, if kid touches ground and music plays, try restarting level
  if (Kid.frame == 109 && need_level1_music == 33)
    return {0, 14};

  // If bumped, nothing to do
  if (Kid.action == actions_5_bumped)
    return {0};

  // If in mid air or free fall, hope to grab on to something
  if (Kid.action == actions_3_in_midair || Kid.action == actions_4_in_freefall)
    return {0, 1};

  // Move, sheath, attack, parry
  if (Kid.sword == sword_2_drawn)
    return {0, 1, 2, 3, 4, 5};

  // Kid is standing or finishing a turn, try all possibilities
  if (Kid.frame == frame_15_stand || (Kid.frame >= frame_50_turn && Kid.frame < 53))
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

  // Turning frame, try all possibilities
  if (Kid.frame == frame_48_turn)
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

  // Start running animation, all movement without shift
  if (Kid.frame < 4)
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

  // Starting jump up, check directions, jump and shift
  if (Kid.frame >= frame_67_start_jump_up_1 && Kid.frame < frame_70_jumphang)
    return {0, 1, 2, 3, 4, 5, 6, 8, 12};

  // Running, all movement without shift
  if (Kid.frame < 15)
    return {0, 2, 3, 4, 5, 6, 7, 8, 9};

  // Hanging, up and shift are only options
  if (Kid.frame >= frame_87_hanging_1 && Kid.frame < 100)
    return {0, 1, 2, 12};

  // Crouched, can only stand, drink, or hop
  if (Kid.frame == frame_109_crouch)
    return {0, 1, 3, 4, 5, 7, 9, 13};

  // Default, no nothing
  return {0};
}

Train::Train(int argc, char *argv[])
{
  // Getting number of openMP threads
  _threadCount = omp_get_max_threads();

  // Setting SDL env variables to use the dummy renderer
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);

  // Profiling information
  _searchTotalTime = 0.0;
  _currentStepTime = 0.0;
  _frameComputationTime = 0.0;

  // Initializing counters
  _stepFramesProcessedCounter = 0;
  _totalFramesProcessedCounter = 0;
  _newCollisionCounter = 0;

  // Setting starting step
  _currentStep = 0;

  // Parsing max hash DB entries
  if (const char *hashAgeThresholdString = std::getenv("JAFFAR_HASH_AGE_THRESHOLD")) _hashAgeThreshold = std::stol(hashAgeThresholdString);
  else EXIT_WITH_ERROR("[Jaffar] JAFFAR_HASH_AGE_THRESHOLD environment variable not defined.\n");

  // Parsing max frame DB entries
  size_t maxDBSizeMb = 0;
  if (const char *MaxDBMBytesEnvString = std::getenv("JAFFAR_MAX_FRAME_DATABASE_SIZE_MB")) maxDBSizeMb = std::stol(MaxDBMBytesEnvString);
  else EXIT_WITH_ERROR("[Jaffar] JAFFAR_MAX_FRAME_DATABASE_SIZE_MB environment variable not defined.\n");

  // Parsing file output frequency
  _outputSaveBestSeconds = -1.0;
  if (const char *outputSaveBestSecondsEnv = std::getenv("JAFFAR_SAVE_BEST_EVERY_SECONDS")) _outputSaveBestSeconds = std::stof(outputSaveBestSecondsEnv);
  _outputSaveCurrentSeconds = -1.0;
  if (const char *outputSaveCurrentSecondsEnv = std::getenv("JAFFAR_SAVE_CURRENT_EVERY_SECONDS")) _outputSaveCurrentSeconds = std::stof(outputSaveCurrentSecondsEnv);

  // Parsing savegame files output path
  _outputSaveBestPath = "/tmp/jaffar.best.sav";
  if (const char *outputSaveBestPathEnv = std::getenv("JAFFAR_SAVE_BEST_PATH")) _outputSaveBestPath = std::string(outputSaveBestPathEnv);
  _outputSaveCurrentPath = "/tmp/jaffar.current.sav";
  if (const char *outputSaveCurrentPathEnv = std::getenv("JAFFAR_SAVE_CURRENT_PATH")) _outputSaveCurrentPath = std::string(outputSaveCurrentPathEnv);

  // Parsing solution files output path
  _outputSolutionBestPath = "/tmp/jaffar.best.sol";
  if (const char *outputSolutionBestPathEnv = std::getenv("JAFFAR_SOLUTION_BEST_PATH")) _outputSolutionBestPath = std::string(outputSolutionBestPathEnv);
  _outputSolutionCurrentPath = "/tmp/jaffar.current.sol";
  if (const char *outputSolutionCurrentPathEnv = std::getenv("JAFFAR_SOLUTION_CURRENT_PATH")) _outputSolutionCurrentPath = std::string(outputSolutionCurrentPathEnv);

  // Parsing command line arguments
  argparse::ArgumentParser program("jaffar-train", JAFFAR_VERSION);

  program.add_argument("--savFile")
    .help("Specifies the path to the SDLPop savefile (.sav) from which to start.")
    .default_value(std::string("quicksave.sav"))
    .required();

  program.add_argument("--maxSteps")
    .help("Specifies the maximum number of steps to run jaffar for.")
    .action([](const std::string& value) { return std::stoul(value); })
    .required();

  program.add_argument("--RNGSeed")
    .help("Specifies a user-defined RNG seed to use.")
    .default_value(std::string("Default"));

  program.add_argument("--disableHistory")
    .help("Do not store the move history during training to save memory.")
    .default_value(false)
    .implicit_value(true);

  program.add_argument("jaffarFile")
    .help("path to the Jaffar configuration script (.jaffar) file to run.")
    .required();

  // Try to parse arguments
  try { program.parse_args(argc, argv);  }
  catch (const std::runtime_error &err) { EXIT_WITH_ERROR("%s\n%s", err.what(), program.help().str().c_str()); }

  // Establishing whether to store move history
  _storeMoveList = program.get<bool>("--disableHistory") == false;

  // Getting savefile path
  auto saveFilePath = program.get<std::string>("--savFile");

  // Getting user-defined RNG seed
  auto RNGSeedSetting = program.get<std::string>("--RNGSeed");
  bool overrideRNGSeedActive = false;
  int overrideRNGSeedValue = 0;
  if (RNGSeedSetting != "Default") { overrideRNGSeedActive = true; overrideRNGSeedValue = std::stoi(RNGSeedSetting); }

  // Loading save file contents
  std::string sourceString;
  bool status = loadStringFromFile(sourceString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());
  if (sourceString.size() != _FRAME_DATA_SIZE) EXIT_WITH_ERROR("[ERROR] Wrong size of input state %s. Expected: %lu, Read: %lu bytes.\n", saveFilePath.c_str(), _FRAME_DATA_SIZE, sourceString.size());

  // If size is correct, copy it to the source frame value
  memcpy(_sourceFrameData, sourceString.data(), _FRAME_DATA_SIZE);

  // Parsing max steps
  _maxSteps = program.get<size_t>("--maxSteps");

  // The move list contains two moves per byte
  _moveListStorageSize = (_maxSteps >> 1) + 1;

  // Calculating DB sizes
  _maxDatabaseSize = floor(((double)maxDBSizeMb * 1024.0 * 1024.0) / ((double)Frame::getSerializationSize()));

  // Parsing config files
  _scriptFile = program.get<std::string>("jaffarFile");
  nlohmann::json scriptFileJs;
  std::string scriptString;
  status = loadStringFromFile(scriptString, _scriptFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n%s \n", _scriptFile.c_str(), program.help().str().c_str());

  nlohmann::json scriptJs;
  try { scriptJs = nlohmann::json::parse(scriptString); }
  catch (const std::exception &err) { EXIT_WITH_ERROR("[ERROR] Parsing configuration file %s. Details:\n%s\n", _scriptFile.c_str(), err.what()); }

  // Checking whether it contains the rules field
  if (isDefined(scriptJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Train configuration file '%s' missing 'Rules' key.\n", _scriptFile.c_str());

  // Resizing containers based on thread count
  _miniPop.resize(_threadCount);
  _state.resize(_threadCount);
  _rules.resize(_threadCount);

  // Creating a first instance of SDLPop that will serve as cache for all the others
  std::string fileCache;

  // Initializing thread-specific SDL instances
  #pragma omp parallel
  {
   // Getting thread id
   int threadId = omp_get_thread_num();
  _miniPop[threadId] = new miniPoPInstance();
  _miniPop[threadId]->initialize();

  // Initializing State Handler
   _state[threadId] = new State(_miniPop[threadId], sourceString);

   //If overriding seed, do it now
   if (overrideRNGSeedActive == true)
   {
    _miniPop[threadId]->setSeed(overrideRNGSeedValue);
    _state[threadId]->getState();
    delete(_state[threadId]);
    _state[threadId] = new State(_miniPop[threadId], _state[threadId]->_stateData);
   }

   // Adding rules, pointing to the thread-specific sdlpop instances
   for (size_t ruleId = 0; ruleId < scriptJs["Rules"].size(); ruleId++)
    _rules[threadId].push_back(new Rule(scriptJs["Rules"][ruleId], _miniPop[threadId]));

   // Setting global rule count
   _ruleCount = _rules[threadId].size();

   // Checking for repeated rule labels
   std::set<size_t> ruleLabelSet;
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
   {
    size_t label = _rules[threadId][ruleId]->_label;
    ruleLabelSet.insert(label);
    if (ruleLabelSet.size() < ruleId + 1)
     EXIT_WITH_ERROR("[ERROR] Rule label %lu is repeated in the configuration file.\n", label);
   }

   // Looking for rule dependency indexes that match their labels
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
    for (size_t depId = 0; depId < _rules[threadId][ruleId]->_dependenciesLabels.size(); depId++)
    {
     bool foundLabel = false;
     size_t label = _rules[threadId][ruleId]->_dependenciesLabels[depId];
     if (label == _rules[threadId][ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in dependencies vector.\n", label);
     for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
      if (_rules[threadId][subRuleId]->_label == label)
      {
       _rules[threadId][ruleId]->_dependenciesIndexes[depId] = subRuleId;
       foundLabel = true;
       break;
      }
     if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as dependency by rule %lu.\n", label, _rules[threadId][ruleId]->_label);
    }

   // Looking for rule satisfied sub-rules indexes that match their labels
   for (size_t ruleId = 0; ruleId < _ruleCount; ruleId++)
    for (size_t satisfiedId = 0; satisfiedId < _rules[threadId][ruleId]->_satisfiesLabels.size(); satisfiedId++)
    {
     bool foundLabel = false;
     size_t label = _rules[threadId][ruleId]->_satisfiesLabels[satisfiedId];
     if (label == _rules[threadId][ruleId]->_label) EXIT_WITH_ERROR("[ERROR] Rule %lu references itself in satisfied vector.\n", label);

     for (size_t subRuleId = 0; subRuleId < _ruleCount; subRuleId++)
      if (_rules[threadId][subRuleId]->_label == label)
      {
       _rules[threadId][ruleId]->_satisfiesIndexes[satisfiedId] = subRuleId;
       foundLabel = true;
       break;
      }
     if (foundLabel == false) EXIT_WITH_ERROR("[ERROR] Could not find rule label %lu, specified as satisfied by rule %lu.\n", label, satisfiedId);
    }
  }

  printf("[Jaffar] SDLPop initialized.\n");

  // Setting initial status for each rule
  std::vector<char> rulesStatus(_ruleCount);
  for (size_t i = 0; i < _ruleCount; i++) rulesStatus[i] = false;

  // Setting initial values
  _hasFinalized = false;
  _hashCollisions = 0;
  _bestFrameReward = 0;

  // Maximum difference between explored frames and the pivot frame
  _maxFrameDiff = 0;

  // Setting win status
  _winFrameFound = false;

  // Adding hash databases (one per move up to the given threshold).
  _hashDatabases.resize(_hashAgeThreshold);
  for (size_t i = 0; i < _hashAgeThreshold; i++) _hashDatabases.add(new absl::flat_hash_set<uint64_t>());

  // Computing initial hash
  const auto hash = _state[0]->computeHash();

  auto initialFrame = std::make_unique<Frame>();
  _state[0]->getState();
  initialFrame->computeFrameDifference(_sourceFrameData, _state[0]->_stateData);
  initialFrame->rulesStatus = rulesStatus;

  // Evaluating Rules on initial frame
  evaluateRules(*initialFrame);

  // Evaluating Score on initial frame
  initialFrame->reward = getFrameReward(*initialFrame);

  // Registering hash for initial frame
  _hashDatabases[_hashAgeThreshold-1]->insert({ hash, 0 });

  // Copying initial frame into the best frame
  _bestFrame = *initialFrame;

  // Filling show database
  _showFrameDB.resize(SHOW_FRAME_COUNT);
  for (size_t i = 0; i < SHOW_FRAME_COUNT; i++) _showFrameDB[i] = *initialFrame;

  // Adding frame to the current database
  _frameDB.push_back(std::move(initialFrame));

  // Initializing show thread
  if (pthread_create(&_showThreadId, NULL, showThreadFunction, this) != 0)
    EXIT_WITH_ERROR("[ERROR] Could not create show thread.\n");
}

// Functions for the show thread
void *Train::showThreadFunction(void *trainPtr)
{
  ((Train *)trainPtr)->showSavingLoop();
  return NULL;
}

void Train::showSavingLoop()
{
  // Timer for saving frames
  auto bestFrameSaveTimer = std::chrono::steady_clock::now();
  auto currentFrameSaveTimer = std::chrono::steady_clock::now();

  // Counter for the current best frame id
  size_t currentFrameId = 0;

  while (_hasFinalized == false)
  {
    // Sleeping for one second to prevent excessive overheads
    sleep(1);

    // Checking if we need to save best frame
    if (_outputSaveBestSeconds > 0.0)
    {
      double bestFrameTimerElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - bestFrameSaveTimer).count();
      if (bestFrameTimerElapsed / 1.0e+9 > _outputSaveBestSeconds)
      {
        // Saving best frame data
        std::string bestFrameData;
        bestFrameData.resize(_FRAME_DATA_SIZE);
        _bestFrame.getFrameDataFromDifference(_sourceFrameData, bestFrameData.data());
        saveStringToFile(bestFrameData, _outputSaveBestPath.c_str());

        // Storing the solution sequence
        if (_storeMoveList)
        {
         std::string solutionString;
         solutionString += _possibleMoves[_bestFrame.getMove(0)];
         for (size_t i = 1; i <= _currentStep; i++)
          solutionString += std::string(" ") + _possibleMoves[_bestFrame.getMove(i)];
         saveStringToFile(solutionString, _outputSolutionBestPath.c_str());
        }

        // Resetting timer
        bestFrameSaveTimer = std::chrono::steady_clock::now();
      }
    }

    // Checking if we need to save current frame
    if (_outputSaveCurrentSeconds > 0.0)
    {
      double currentFrameTimerElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - currentFrameSaveTimer).count();
      if (currentFrameTimerElapsed / 1.0e+9 > _outputSaveCurrentSeconds)
      {
        // Saving best frame data
       std::string showFrameData;
       showFrameData.resize(_FRAME_DATA_SIZE);
       _showFrameDB[currentFrameId].getFrameDataFromDifference(_sourceFrameData, showFrameData.data());
       saveStringToFile(showFrameData, _outputSaveCurrentPath.c_str());

        // Storing the solution sequence
        if (_storeMoveList)
        {
         std::string solutionString;
         solutionString += _possibleMoves[_showFrameDB[currentFrameId].getMove(0)];
         for (size_t i = 1; i <= _currentStep; i++)
          solutionString += std::string(" ") + _possibleMoves[_showFrameDB[currentFrameId].getMove(i)];
         saveStringToFile(solutionString, _outputSolutionCurrentPath.c_str());
        }

        // Resetting timer
        currentFrameSaveTimer = std::chrono::steady_clock::now();

        // Increasing counter
        currentFrameId = (currentFrameId + 1) % SHOW_FRAME_COUNT;
      }
    }
  }

  // If it has finalized with a win, save the winning frame
  if (_outputSaveBestSeconds > 0.0)
  {
   auto lastFrame = _winFrameFound ? _winFrame : _bestFrame;

   // Saving best frame data
   std::string winFrameData;
   winFrameData.resize(_FRAME_DATA_SIZE);
   lastFrame.getFrameDataFromDifference(_sourceFrameData, winFrameData.data());
   saveStringToFile(winFrameData, _outputSaveBestPath.c_str());

   // Storing the solution sequence
   if (_storeMoveList)
   {
    std::string solutionString;
    solutionString += _possibleMoves[lastFrame.getMove(0)];
    for (size_t i = 1; i <= _currentStep; i++)
     solutionString += std::string(" ") + _possibleMoves[lastFrame.getMove(i)];
    saveStringToFile(solutionString, _outputSolutionBestPath.c_str());
   }
  }
}

int main(int argc, char *argv[])
{
  Train train(argc, argv);

  // Running Search
  train.run();
}
