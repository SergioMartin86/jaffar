#include <stdio.h>
#include "SDLPopInstance.h"
#include "jaffar.h"
#include "utils.h"
#include "state.h"
#include "search.h"
#include "argparse.hpp"
#include <mpi.h>

using namespace argparse;

// Configuration singleton
jaffarConfigStruct _jaffarConfig;

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
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from sequence input file: %s\n", _jaffarConfig.inputSequenceFile.c_str());
 }

 // Reading config file
 std::string configString;
 bool status = loadStringFromFile(configString, _jaffarConfig.inputConfigFile.c_str());
 if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n", _jaffarConfig.inputConfigFile.c_str());

 // Parsing JSON from config file
 _jaffarConfig.configJs = nlohmann::json::parse(configString);
}

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

 SDLPopInstance s;
 State state(&s);
 Search search(&s);

 // Initializing SDLPop Instance
 s.initialize(1, true);

 // Loading save file
 state.quickLoad(_jaffarConfig.inputSaveFile);

 s.setSeed(0xF0F0F0F0);
 s.printFrameInfo();
 s.draw();
 getchar();

 // If this is to play a sequence, simply play it
 if (_jaffarConfig.opMode == m_play)
 {
  // Setting timer for a sane animation
  s.set_timer_length(timer_1, 20);

  const auto moveList = split(_jaffarConfig.moveSequence, ' ');
  for (const auto move : moveList)
  {
    s.performMove(move);
    s.advanceFrame();
    s.printFrameInfo();
    s.draw();
  }
 }

 printf("Done.\n");
} 
