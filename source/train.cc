#include "train.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>
#include <unistd.h>
#include <algorithm>
#include <set>
#include <boost/sort/sort.hpp>
#include <random>

void Train::run()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Launching Jaffar Version %s...\n", JAFFAR_VERSION);
  printf("[Jaffar] Using configuration file: "); printf("%s ", _scriptFile.c_str()); printf("\n");
  printf("[Jaffar] Frame size: %lu\n", sizeof(Frame));
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

    // Updating show frames (random selection)
    std::random_device rd;     // only used once to initialise (seed) engine
    std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int> uni(0, _frameDB.size()-1); // guaranteed unbiased

    for (size_t i = 0; i < SHOW_FRAME_COUNT; i++) _showFrameDB[i] = *_frameDB[uni(rng)];

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
      printf("[Jaffar] Winning frame reached in %lu moves, finishing...\n", _currentStep);
      terminate = true;
    }

    // Terminate if maximum number of frames was reached
    if (_currentStep > _MAX_MOVELIST_SIZE-1)
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

    _winFrame.getFrameDataFromDifference(_sourceFrameData, _state[0]->_inputStateData);
    _state[0]->pushState();
    _state[0]->_miniPop->printFrameInfo();
    _state[0]->printRuleStatus(_winFrame);

    #ifndef JAFFAR_DISABLE_MOVE_HISTORY

    // Print Move History
    printf("[Jaffar]  + Move List: ");
    for (size_t i = 0; i <= _currentStep; i++)
      printf("%s ", _possibleMoves[_winFrame.getMove(i)].c_str());
    printf("\n");

    #endif
  }

  // Marking the end of the run
  _hasFinalized = true;

  // Stopping show thread
  pthread_join(_showThreadId, NULL);
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

  // Creating storage for flush-trigger frames
  std::vector<std::unique_ptr<Frame>> flushDBFrames;

  // Processing frame database in parallel
  auto frameComputationTimeBegin = std::chrono::steady_clock::now(); // Profiling
  #pragma omp parallel
  {
    // Getting thread id
    int threadId = omp_get_thread_num();

    // Reserving space for frame pointers
    newThreadFrames[threadId].reserve(_maxDatabaseSize / _threadCount);

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
      const auto baseFrame = *_frameDB[baseFrameIdx];

      // Freeing memory for the used base frame
      _frameDB[baseFrameIdx].reset();

      // Loading frame state
      auto t0 = std::chrono::steady_clock::now(); // Profiling
      char baseFrameData[_FRAME_DATA_SIZE];
      baseFrame.getFrameDataFromDifference(_sourceFrameData, baseFrameData);
      auto tf = std::chrono::steady_clock::now();
      threadFrameDecodingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

      // Getting possible moves for the current frame
      t0 = std::chrono::steady_clock::now(); // Profiling
      memcpy(_state[threadId]->_inputStateData, baseFrameData, _FRAME_DATA_SIZE);
      _state[threadId]->pushState();
      std::vector<uint8_t> possibleMoveIds = _state[threadId]->getPossibleMoveIds(baseFrame);
      tf = std::chrono::steady_clock::now();
      threadFrameDeserializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

      // Running possible moves
      for (size_t idx = 0; idx < possibleMoveIds.size(); idx++)
      {
        // Increasing  frames processed counter
        _stepFramesProcessedCounter++;

        // Getting possible move id
        auto moveId = possibleMoveIds[idx];

        // Getting possible move string
        std::string move = _possibleMoves[moveId].c_str();

        // If this comes after the first move, we need to reload the base state
        if (idx > 0)
        {
         t0 = std::chrono::steady_clock::now(); // Profiling
         _state[threadId]->pushState();
         tf = std::chrono::steady_clock::now();
         threadFrameDeserializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
        }

        // Getting current level
        auto curLevel = current_level;

        // Perform the selected move
        t0 = std::chrono::steady_clock::now(); // Profiling
        _state[threadId]->_miniPop->performMove(move);

        // Advance a single frame
        _state[threadId]->_miniPop->advanceFrame();
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
        auto newFrame = std::make_unique<Frame>(baseFrame);

        #ifndef JAFFAR_DISABLE_MOVE_HISTORY

        // If required, store move history
        newFrame->setMove(_currentStep, moveId);

        #endif

        // Evaluating rules on the new frame
        _state[threadId]->evaluateRules(*newFrame);

        // If frame has failed, discard it and proceed to the next one
        if (newFrame->_type == f_fail) continue;

        // Storing the frame data, only if if belongs to the same level
        if (curLevel == newLevel)
        {
         t0 = std::chrono::steady_clock::now(); // Profiling
         _state[threadId]->popState();
         tf = std::chrono::steady_clock::now(); // Profiling
         threadFrameSerializationTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();

         t0 = std::chrono::steady_clock::now(); // Profiling
         newFrame->computeFrameDifference(_sourceFrameData, _state[threadId]->_outputStateData);
         tf = std::chrono::steady_clock::now(); // Profiling
         threadFrameEncodingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(tf - t0).count();
        }

        // Calculating current reward
        newFrame->reward = _state[threadId]->getFrameReward(*newFrame);

        // If frame has succeded, then flag it
        if (newFrame->_type == f_win)
        {
          _winFrameFound = true;
           #pragma omp critical(winFrame)
           _winFrame = *newFrame;
        }

        // Adding novel frame in the next frame database, if regular
        if (newFrame->_type == f_regular)
        {
         newThreadFrames[threadId].push_back(std::move(newFrame));
         continue;
        }

        if (newFrame->_type == f_flush) // Else flush database if it's tagged to flush the database
        {
         // Setting new frame as regular from now on
         newFrame->_type = f_regular;

         // Pushing it into the special flush DB, in case there are more like it in this frame
         #pragma omp critical(flushDB)
         flushDBFrames.push_back(std::move(newFrame));
         continue;
        }
      }
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

  // Clearing all old frames
  _frameDB.clear();

  // If there are no flush type frames, continue normally
  if (flushDBFrames.size() == 0)
  {
   // Getting total new frames and displacements
   size_t totalNewFrameCount = 0;
   std::vector<size_t> totalNewFrameDisplacements(_threadCount);
   for (int i = 0; i < _threadCount; i++) { totalNewFrameDisplacements[i] = totalNewFrameCount; totalNewFrameCount += newThreadFrames[i].size(); }

   // Passing new frames into the new frame database
   _frameDB.resize(totalNewFrameCount);
   #pragma omp parallel for
   for (int i = 0; i < _threadCount; i++)
    for (size_t j = 0; j < newThreadFrames[i].size(); j++) _frameDB[j + totalNewFrameDisplacements[i]] = std::move(newThreadFrames[i][j]);
  }

  // If there are flush type frames, push them into the database.
  if (flushDBFrames.size() > 0) _frameDB = std::move(flushDBFrames);

  // Sorting local DB frames by reward
  auto DBSortingTimeBegin = std::chrono::steady_clock::now(); // Profiling
  boost::sort::block_indirect_sort(_frameDB.begin(), _frameDB.end(), [](const auto &a, const auto &b) { return a->reward > b->reward; });
  auto DBSortingTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
  _DBSortingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(DBSortingTimeEnd - DBSortingTimeBegin).count(); // Profiling

  // If global frames exceed the maximum allowed, sort and truncate all excessive frames
  if (_frameDB.size() > _maxDatabaseSize) _frameDB.resize(_maxDatabaseSize);

  // Storing best frame
  _bestFrameReward = -1.0;
  if (_frameDB.empty() == false) { _bestFrame = *_frameDB[0]; _bestFrameReward = _bestFrame.reward; }

  // Summing frame processing counters
  _totalFramesProcessedCounter += _stepFramesProcessedCounter;

  // Re-calculating global collision counter
  _hashCollisions += _newCollisionCounter;

  auto framePostprocessingTimeEnd = std::chrono::steady_clock::now();                                                                           // Profiling
  _framePostprocessingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(framePostprocessingTimeEnd - framePostprocessingTimeBegin).count(); // Profiling
}



void Train::printTrainStatus()
{
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Current Step #: %lu (Max: %u)\n", _currentStep, _MAX_MOVELIST_SIZE);

  size_t timeStep = _currentStep-1;
  size_t curMins = timeStep / 720;
  size_t curSecs = (timeStep % 720) / 12;
  size_t curMilliSecs = floor((double)(timeStep % 12) / 0.012);

  size_t maxStep = _MAX_MOVELIST_SIZE-1;
  size_t maxMins = maxStep / 720;
  size_t maxSecs = (maxStep % 720) / 12;
  size_t maxMilliSecs = floor((double)(maxStep % 12) / 0.012);

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
  printf("[Jaffar] Frame DB Size: %.3fmb\n", (double)(_frameDB.size() * sizeof(Frame)) / (1024.0 * 1024.0));
  printf("[Jaffar] Hash DB Size: %.3fmb\n", (double)(hashDatabasesEntries * sizeof(uint64_t)) / (1024.0 * 1024.0));
  printf("[Jaffar] Best Frame Information:\n");

  _bestFrame.getFrameDataFromDifference(_sourceFrameData, _state[0]->_inputStateData);
  _state[0]->pushState();
  _state[0]->_miniPop->printFrameInfo();
  _state[0]->printRuleStatus(_bestFrame);

  // Getting kid room
  int kidCurrentRoom = Kid.room;

  // Getting magnet values for the kid
  auto kidMagnet = _state[0]->getKidMagnetValues(_bestFrame, kidCurrentRoom);

  printf("[Jaffar]  + Kid Horizontal Magnet Intensity / Position: %.1f / %.0f\n", kidMagnet.intensityX, kidMagnet.positionX);
  printf("[Jaffar]  + Kid Vertical Magnet Intensity: %.1f\n", kidMagnet.intensityY);

  // Getting guard room
  int guardCurrentRoom = Guard.room;

  // Getting magnet values for the guard
  auto guardMagnet = _state[0]->getGuardMagnetValues(_bestFrame, guardCurrentRoom);

  printf("[Jaffar]  + Guard Horizontal Magnet Intensity / Position: %.1f / %.0f\n", guardMagnet.intensityX, guardMagnet.positionX);
  printf("[Jaffar]  + Guard Vertical Magnet Intensity: %.1f\n", guardMagnet.intensityY);

  #ifndef JAFFAR_DISABLE_MOVE_HISTORY

  // Print Move History
  printf("[Jaffar]  + Last 30 Moves: ");
  size_t startMove = 0;//(size_t)std::max((int)0, (int)_currentStep-30);
  for (size_t i = startMove; i < _currentStep; i++)
    printf("%s ", _possibleMoves[_bestFrame.getMove(i)].c_str());
  printf("\n");

  #endif
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

  program.add_argument("--RNGSeed")
    .help("Specifies a user-defined RNG seed to use.")
    .default_value(std::string("Default"));

  program.add_argument("jaffarFile")
    .help("path to the Jaffar configuration script (.jaffar) file to run.")
    .required();

  // Try to parse arguments
  try { program.parse_args(argc, argv);  }
  catch (const std::runtime_error &err) { EXIT_WITH_ERROR("%s\n%s", err.what(), program.help().str().c_str()); }

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

  // Calculating DB sizes
  _maxDatabaseSize = floor(((double)maxDBSizeMb * 1024.0 * 1024.0) / ((double)sizeof(Frame)));

  // Parsing config files
  _scriptFile = program.get<std::string>("jaffarFile");
  nlohmann::json scriptFileJs;
  std::string scriptString;
  status = loadStringFromFile(scriptString, _scriptFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from Jaffar script file: %s\n%s \n", _scriptFile.c_str(), program.help().str().c_str());

  nlohmann::json scriptJs;
  try { scriptJs = nlohmann::json::parse(scriptString); }
  catch (const std::exception &err) { EXIT_WITH_ERROR("[ERROR] Parsing configuration file %s. Details:\n%s\n", _scriptFile.c_str(), err.what()); }

  // Checking whether it contains the rules field
  if (isDefined(scriptJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Configuration file '%s' missing 'Rules' key.\n", _scriptFile.c_str());

  // Checking whether it contains the rules field
  if (isDefined(scriptJs, "State Configuration") == false) EXIT_WITH_ERROR("[ERROR] Configuration file '%s' missing 'State Configuration' key.\n", _scriptFile.c_str());

  // Resizing containers based on thread count
  _state.resize(_threadCount);

  // Initializing thread-specific SDL instances
  #pragma omp parallel
  {
   // Getting thread id
   int threadId = omp_get_thread_num();

   #pragma omp critical
    _state[threadId] = new State(sourceString, scriptJs["State Configuration"], scriptJs["Rules"], overrideRNGSeedActive == true ? overrideRNGSeedValue : -1);
  }

  printf("[Jaffar] miniPop initialized.\n");

  // Setting initial values
  _hasFinalized = false;
  _hashCollisions = 0;
  _bestFrameReward = 0;

  // Check rule count does not exceed maximum
  _ruleCount = _state[0]->_rules.size();

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
  _state[0]->popState();
  initialFrame->computeFrameDifference(_sourceFrameData, _state[0]->_outputStateData);
  for (size_t i = 0; i < _ruleCount; i++) initialFrame->rulesStatus[i] = false;

  // Evaluating Rules on initial frame
  _state[0]->evaluateRules(*initialFrame);

  // Evaluating Score on initial frame
  initialFrame->reward = _state[0]->getFrameReward(*initialFrame);

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

        #ifndef JAFFAR_DISABLE_MOVE_HISTORY

        // Storing the solution sequence
        std::string solutionString;
        solutionString += _possibleMoves[_bestFrame.getMove(0)];
        for (size_t i = 1; i <= _currentStep; i++)
         solutionString += std::string(" ") + _possibleMoves[_bestFrame.getMove(i)];
        saveStringToFile(solutionString, _outputSolutionBestPath.c_str());

        #endif

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

        #ifndef JAFFAR_DISABLE_MOVE_HISTORY

        // Storing the solution sequence
        std::string solutionString;
        solutionString += _possibleMoves[_showFrameDB[currentFrameId].getMove(0)];
        for (size_t i = 1; i <= _currentStep; i++)
         solutionString += std::string(" ") + _possibleMoves[_showFrameDB[currentFrameId].getMove(i)];
        saveStringToFile(solutionString, _outputSolutionCurrentPath.c_str());

        #endif

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

   #ifndef JAFFAR_DISABLE_MOVE_HISTORY

   // Storing the solution sequence
   std::string solutionString;
   solutionString += _possibleMoves[lastFrame.getMove(0)];
   for (size_t i = 1; i <= _currentStep; i++)
    solutionString += std::string(" ") + _possibleMoves[lastFrame.getMove(i)];
   saveStringToFile(solutionString, _outputSolutionBestPath.c_str());

   #endif
  }
}

int main(int argc, char *argv[])
{
  Train train(argc, argv);

  // Running Search
  train.run();
}
