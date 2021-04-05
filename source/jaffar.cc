#include <stdio.h>
#include "SDLPopInstance.h"
#include "jaffar.h"
#include "utils.h"
#include "state.h"
#include "search.h"
#include "argparse.hpp"
#include <mpi.h>

// Configuration singleton
jaffarConfigStruct _jaffarConfig;

int main(int argc, char* argv[])
{
 // Initialize the MPI environment
 //MPI_Init(&argc, &argv);

 // Get the number of processes
 //int commSize;
 //MPI_Comm_size(MPI_COMM_WORLD, &commSize);

 // Get the rank of the process
 //int commRank;
 //MPI_Comm_rank(MPI_COMM_WORLD, &commRank);

 //printf("Rank: %d/%d - Loading SDLPop...\n", commRank, commSize);

 parseArgs(argc, argv);

 SDLPopInstance sdlpop;
 State state(&sdlpop);
 Scorer scorer(&sdlpop, &state, _jaffarConfig.configJs);

 // Initializing SDLPop Instance
 sdlpop.initialize(1, true);

 // Loading save file
 state.quickLoad(_jaffarConfig.inputSaveFile);

 // Setting seed, if override was selected
 if (_jaffarConfig.overrideSeedEnabled)
  sdlpop.setSeed(_jaffarConfig.overrideSeedValue);

 // If this is to play a sequence, simply play it
 if (_jaffarConfig.opMode == m_play)
 {
  // Setting timer for a sane animation
  sdlpop.set_timer_length(timer_1, 20);

  // Printing initial frame info
  sdlpop.printFrameInfo();
  sdlpop.draw();
  getchar();

  const auto moveList = split(_jaffarConfig.moveSequence, ' ');
  for (const auto move : moveList)
  {
   sdlpop.performMove(move);
   sdlpop.advanceFrame();
   sdlpop.printFrameInfo();
   sdlpop.draw();
  }
 }

 // If this is to train, instantiate the search module
 if (_jaffarConfig.opMode == m_train)
 {
  Search search(&sdlpop, &state, &scorer, _jaffarConfig.configJs);
  search.run();
 }

 printf("Done.\n");
}

using namespace argparse;

void parseArgs(int argc, char* argv[])
{
 ArgumentParser program("jaffar", "1.0.0");

 program.add_argument("--config", "-c")
   .help("path to the Jaffar configuration (.config) file to run.")
   .default_value(std::string("jaffar.config"))
   .required();

 program.add_argument("--save", "-s")
    .help("path to the SDLPop savegame (.sav) file to run.")
    .default_value(std::string("quicksave.sav"))
    .required();

 program.add_argument("--play", "-p")
   .help("plays a given sequence (.seq) file.")
   .default_value(std::string(""))
   .action([](const std::string& value) { return std::string(value); });

 try
 {
    program.parse_args(argc, argv);
 }
 catch (const std::runtime_error& err)
 {
   fprintf(stderr, "%s\n%s", err.what(), program.help().str().c_str());
   exit(-1);
 }

 _jaffarConfig.inputConfigFile = program.get<std::string>("--config");
 _jaffarConfig.inputSaveFile = program.get<std::string>("--save");
 _jaffarConfig.inputSequenceFile = program.get<std::string>("--play");

 // By default do train
 _jaffarConfig.opMode = m_train;

 // If sequence file defined, load it and play it
 if (_jaffarConfig.inputSequenceFile != "")
 {
  _jaffarConfig.opMode = m_play;
  bool status = loadStringFromFile(_jaffarConfig.moveSequence, _jaffarConfig.inputSequenceFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from sequence input file: %s\n%s \n", _jaffarConfig.inputSequenceFile.c_str(), program.help().str().c_str());
 }

 // Reading config file
 std::string configString;
 bool status = loadStringFromFile(configString, _jaffarConfig.inputConfigFile.c_str());
 if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n%s \n", _jaffarConfig.inputConfigFile.c_str(), program.help().str().c_str());

 // Parsing JSON from config file
 try
 {
  _jaffarConfig.configJs = nlohmann::json::parse(configString);
 }
 catch (const std::exception& err)
 {
   fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", _jaffarConfig.inputConfigFile.c_str(), err.what());
   exit(-1);
 }

 // Parsing random seed information
 if (isDefined(_jaffarConfig.configJs, "Random Seed", "Override") == false) EXIT_WITH_ERROR("[ERROR] Config file missing 'Random Seed', 'Override' key.\n");
 if (isDefined(_jaffarConfig.configJs, "Random Seed", "Value") == false) EXIT_WITH_ERROR("[ERROR] Config file missing 'Random Seed, 'Value' key.\n");
 _jaffarConfig.overrideSeedEnabled = _jaffarConfig.configJs["Random Seed"]["Override"].get<bool>();
 _jaffarConfig.overrideSeedValue = _jaffarConfig.configJs["Random Seed"]["Value"].get<dword>();

 // Making sure Rules entry exists
 if (isDefined(_jaffarConfig.configJs, "Rules") == false) EXIT_WITH_ERROR("[ERROR] Config file missing 'Rules' key.\n");
}
