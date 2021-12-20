#include "tester.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>

void Tester::run()
{
  //  Printing move list
  for (size_t i = 0; i < _moveList.size(); i++)
  {
   int threadId = omp_get_thread_num();
   printf("Move %lu: %s\n", i+1, _moveList[i+1].c_str());
   _state[threadId]->_miniPop->advanceFrame(_moveList[i]);

   printf("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Row: %2d, Col: %2d, Fall.y: %d, Frame: %3d, HP: %d/%d, Dir: %d, SeqId: %d\n",
          int(Kid.room),
          int(Kid.x),
          int(Kid.y),
          int(Kid.curr_row),
          int(Kid.curr_col),
          int(Kid.fall_y),
          int(Kid.frame),
          int(hitp_curr),
          int(hitp_max),
          int(Kid.direction),
          Kid.curr_seq);

   printf("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Row: %2d, Col: %2d, Fall.y: %d, Frame: %3d, HP: %d/%d, Dir: %d, SeqId: %d\n",
          int(Guard.room),
          int(Guard.x),
          int(Guard.y),
          int(Guard.curr_row),
          int(Guard.curr_col),
          int(Guard.fall_y),
          int(Guard.frame),
          int(guardhp_curr),
          int(guardhp_max),
          int(Guard.direction),
          Guard.curr_seq);

   printf("[Jaffar]  + Guard Can See Kid: %d\n", can_guard_see_kid);
   printf("[Jaffar]  + Is Guard Notice: %d\n", is_guard_notice);
   printf("[Jaffar]  + Guard Refrac: %d\n", guard_refrac);
   printf("[Jaffar]  + Reached Checkpoint: %s (%d)\n", checkpoint ? "Yes" : "No", checkpoint);
   printf("[Jaffar]  + Feather Fall: %d\n", is_feather_fall);
   printf("[Jaffar]  + Need Lvl1 Music: %d\n", need_level1_music);
   printf("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", random_seed, last_loose_sound);
   printf("[Jaffar]  + Demo Index: %d, Time: %d\n", demo_index, demo_time);
   printf("[Jaffar]  + Exit Room Timer: %d\n", exit_room_timer);

  }

 exit(0);

 size_t failCount = 0;
 size_t winCount = 0;

// #pragma omp parallel
 {
  // Getting thread id
  int threadId = omp_get_thread_num();

  // Remember win seed
  dword winSeed = 0;

//  #pragma omp for reduction(+:failCount) reduction(+:winCount)
  for (dword curSeed = 0; curSeed < 3200000; curSeed++)
  {
   // Resetting state
   _state[threadId]->pushState();

   // Changing initial rng
   _state[threadId]->_miniPop->setSeed(curSeed);

   // Creating new current frame
   Frame curFrame(*_baseFrame);

   // Running entire sequence
   for (const auto& move : _moveList)
   {
    _state[threadId]->_miniPop->advanceFrame(move);
    _state[threadId]->evaluateRules(curFrame.rulesStatus);
    frameType type = _state[threadId]->getFrameType(curFrame.rulesStatus);

    if (type == f_fail)
    {
     #pragma omp atomic
     failCount++;
     break;
    }

    if (type == f_win)
    {
     #pragma omp atomic
     winCount++;
     winSeed = curSeed;
     printf("Win Seed: %u\n", winSeed); // exit(0);
     break;
    }
   }
   if (winCount > 1000) exit(0);
  }

  printf("Thread %d - Win Seed: %u\n", threadId, winSeed);
 }

 printf("Fail: %lu, Win: %lu\n", failCount, winCount);
 printf("1/P = %.9f\n", ((double)failCount+(double)winCount) / (double)winCount);

}

Tester::Tester(int argc, char *argv[])
{
  // Getting number of openMP threads
  _threadCount = omp_get_max_threads();

  // Setting SDL env variables to use the dummy renderer
  setenv("SDL_VIDEODRIVER", "dummy", 1);
  setenv("SDL_AUDIODRIVER", "dummy", 1);

  // Parsing command line arguments
  argparse::ArgumentParser program("jaffar-tester", JAFFAR_VERSION);

  program.add_argument("savFile")
    .help("Specifies the path to the SDLPop savefile (.sav) from which to start.")
    .default_value(std::string("quicksave.sav"))
    .required();

  program.add_argument("jaffarFile")
    .help("path to the Jaffar configuration script (.jaffar) file to run.")
    .required();

  program.add_argument("solutionFile")
    .help("path to the solution sequence (.sol) to test.")
    .required();

  // Try to parse arguments
  try { program.parse_args(argc, argv);  }
  catch (const std::runtime_error &err) { EXIT_WITH_ERROR("%s\n%s", err.what(), program.help().str().c_str()); }

  // If sequence file defined, load it and play it
  std::string solutionFile = program.get<std::string>("solutionFile");
  std::string moveSequence;
  bool status = loadStringFromFile(moveSequence, solutionFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from solution file: %s\n%s \n", solutionFile.c_str(), program.help().str().c_str());

  // Getting move list
  moveSequence = std::string(". . . . . . . . . . . . . . . . L . . . L . . . . . . L . U . . . . . . . . . . . L . . . L . . U . . . . . . . . . . . . . . . . L . . . L . . L . . D . . . . . L . . . L . . . . . . L . . . LU . . . .");
  _moveList = split(moveSequence, ' ');

  // Getting savefile path
  auto saveFilePath = program.get<std::string>("savFile");

  // Loading save file contents
  std::string sourceString;
  status = loadStringFromFile(sourceString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());
  if (sourceString.size() != _FRAME_DATA_SIZE) EXIT_WITH_ERROR("[ERROR] Wrong size of input state %s. Expected: %lu, Read: %lu bytes.\n", saveFilePath.c_str(), _FRAME_DATA_SIZE, sourceString.size());

  // If size is correct, copy it to the source frame value
  memcpy(_sourceFrameData, sourceString.data(), _FRAME_DATA_SIZE);

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
    _state[threadId] = new State(sourceString, scriptJs["State Configuration"], scriptJs["Rules"]);
  }

  printf("[Jaffar] miniPop initialized.\n");

  // Check rule count does not exceed maximum
  _ruleCount = _state[0]->_rules.size();
  if (_ruleCount > _MAX_RULE_COUNT) EXIT_WITH_ERROR("[ERROR] Configured Jaffar to run %lu rules, but the specified script contains %lu. Modify frame.h and rebuild to run this level.\n", _MAX_RULE_COUNT, _ruleCount);

  // Creating base frame
  _baseFrame = std::make_unique<Frame>();
  _state[0]->popState();
  _baseFrame->computeFrameDifference(_sourceFrameData, _state[0]->_outputStateData);
  for (size_t i = 0; i < _ruleCount; i++) _baseFrame->rulesStatus[i] = false;

  // Evaluating Rules on initial frame
  _state[0]->evaluateRules(_baseFrame->rulesStatus);

  // Evaluating Score on initial frame
  _baseFrame->reward = _state[0]->getFrameReward(_baseFrame->rulesStatus);
}

int main(int argc, char *argv[])
{
  Tester tester(argc, argv);

  // Running Search
  tester.run();
}
