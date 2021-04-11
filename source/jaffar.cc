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
 MPI_Init(&argc, &argv);

 // Get the number of processes
 MPI_Comm_size(MPI_COMM_WORLD, &_jaffarConfig.mpiSize);

 // Get the rank of the process
 MPI_Comm_rank(MPI_COMM_WORLD, &_jaffarConfig.mpiRank);

 parseArgs(argc, argv);

 bool useSDLPopGUI = false;
 if (_jaffarConfig.mpiRank == 0)
 {
  useSDLPopGUI = true; // Only show GUI if this is root MPI rank (0)
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Running script: %s.\n", _jaffarConfig.inputConfigFile.c_str());
  printf("[Jaffar] Using %d MPI Ranks.\n", _jaffarConfig.mpiSize);
 }

 // Initializing SDLPop Instance
 SDLPopInstance sdlpop;
 sdlpop.initialize(1, useSDLPopGUI);

 // Initializing State Handler
 State state(&sdlpop, _jaffarConfig.configJs["Savefile Configuration"]);

 // Wait for all workers to be ready
 MPI_Barrier(MPI_COMM_WORLD);

 // If this is to play a sequence, simply play it
 if (_jaffarConfig.opMode == m_play)
 {
  // Setting timer for a sane animation
  sdlpop.set_timer_length(timer_1, 20);

  // Printing initial frame info
  sdlpop.printFrameInfo();
  sdlpop.draw();

  // Iterating move list in the sequence
  const auto moveList = split(_jaffarConfig.moveSequence, ' ');
  for (const auto move : moveList)
  {
   sdlpop.performMove(move);
   sdlpop.advanceFrame();
   sdlpop.printFrameInfo();
   sdlpop.draw();
  }
 }

 // If this is to find a sequence, run the searcher
 if (_jaffarConfig.opMode == m_train)
 {
  // Initializing search module
  Search search(&sdlpop, &state, _jaffarConfig.configJs["Search Configuration"]);

  // Running Search
  search.run();
 }

 if (_jaffarConfig.mpiRank == 0) printf("[Jaffar] Finished.\n");
 MPI_Barrier(MPI_COMM_WORLD);
 return MPI_Finalize();
}

using namespace argparse;

void parseArgs(int argc, char* argv[])
{
 ArgumentParser program("jaffar", "1.0.0");

 program.add_argument("--config", "-c")
   .help("path to the Jaffar configuration (.config) file to run.")
   .default_value(std::string("jaffar.config"))
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
   if (_jaffarConfig.mpiRank == 0) fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", _jaffarConfig.inputConfigFile.c_str(), err.what());
   exit(-1);
 }
}
