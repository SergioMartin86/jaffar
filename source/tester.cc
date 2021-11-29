#include "tester.h"
#include "argparse.hpp"
#include "utils.h"
#include <omp.h>

void Tester::run()
{
 #pragma omp parallel
 {
  // Getting thread id
  int threadId = omp_get_thread_num();

  #pragma omp for
  for (dword curSeed = 0; curSeed < 4294967295; curSeed++)
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
    _state[threadId]->_miniPop->performMove(move);
    _state[threadId]->_miniPop->advanceFrame();
    _state[threadId]->evaluateRules(curFrame);

    if (curFrame._type == f_fail) break;
    if (curFrame._type == f_win) { printf("Win: %u\n", curSeed); exit(0); }
   }
  }
 }
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
  _state[0]->evaluateRules(*_baseFrame);

  // Evaluating Score on initial frame
  _baseFrame->reward = _state[0]->getFrameReward(*_baseFrame);
}

int main(int argc, char *argv[])
{
  Tester tester(argc, argv);

  // Running Search
  tester.run();
}
