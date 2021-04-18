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

 if (_jaffarConfig.mpiRank == 0)
 {
  printf("[Jaffar] ----------------------------------------------------------------\n");
  printf("[Jaffar] Using configuration file: %s.\n", _jaffarConfig.inputConfigFile.c_str());
  printf("[Jaffar] Opening SDLPop...\n");
 }

 // Wait for all workers to be ready
 MPI_Barrier(MPI_COMM_WORLD);

 // If this is to play a sequence, simply play it
 if (_jaffarConfig.mpiRank == 0 && _jaffarConfig.opMode == m_play)
 {
  // Parsing profiling verbosity
  if (isDefined(_jaffarConfig.configJs["Playback Configuration"], "Pause After Every Frame") == false) EXIT_WITH_ERROR("[ERROR] Playback configuration missing 'Pause After Every Frame' key.\n");
  bool pauseAfterFrame = _jaffarConfig.configJs["Playback Configuration"]["Pause After Every Frame"].get<bool>();

  // Parsing debugging verbosity
  if (isDefined(_jaffarConfig.configJs["Playback Configuration"], "Produce Replay File") == false) EXIT_WITH_ERROR("[ERROR] Playback configuration missing 'Produce Replay File' key.\n");
  bool produceReplay = _jaffarConfig.configJs["Playback Configuration"]["Produce Replay File"].get<bool>();

  // Parsing debugging verbosity
  if (isDefined(_jaffarConfig.configJs["Playback Configuration"], "Frame Duration") == false) EXIT_WITH_ERROR("[ERROR] Playback configuration missing 'Frame Duration' key.\n");
  size_t frameDuration = _jaffarConfig.configJs["Playback Configuration"]["Frame Duration"].get<size_t>();

  // Initializing SDLPop Instance
  SDLPopInstance sdlpop;
  sdlpop.initialize(1, true);

  // Initializing State Handler
  State state(&sdlpop);

  // Creating move list
  const auto moveList = split(_jaffarConfig.moveSequence, ' ');

  // Printing info
  printf("[Jaffar] Playing sequence file: %s\n", _jaffarConfig.inputSequenceFile.c_str());
  printf("[Jaffar] Sequence length: %lu frames\n", moveList.size());

  // Setting timer for a human-visible animation
  sdlpop.set_timer_length(timer_1, frameDuration);

  // Printing initial frame info
  sdlpop.printFrameInfo();
  sdlpop.draw();

  // If replay is requested, start process
  if (produceReplay)
  {
   sdlpop.init_record_replay();
   sdlpop.start_recording();
  }

  // Iterating move list in the sequence
  for (size_t i = 0; i < moveList.size(); i++)
  {
   // Pause if user requested it
   if (pauseAfterFrame)
   {
    printf("[Jaffar] Playback paused. Press any key to continue...\n");
    getchar();
   }

   printf("[Jaffar] ----------------------------------------------------------------\n");
   printf("[Jaffar] Current Step #: %lu / %lu\n", i, moveList.size());

   sdlpop.performMove(moveList[i]);
   sdlpop.advanceFrame();
   sdlpop.printFrameInfo();
   sdlpop.draw();

   if (produceReplay) sdlpop.add_replay_move();
  }

  printf("[Jaffar] Sequence Finished.\n");

  if (produceReplay == true)
  {
   std::string replayFileName = "jaffar.p1r";
   sdlpop.save_recorded_replay(replayFileName.c_str());
   printf("[Jaffar] Replay saved in '%s'.\n", replayFileName.c_str());
  }

  printf("[Jaffar] Press any key to exit...\n");
  getchar();
 }

 // If this is to find a sequence, run the searcher
 if (_jaffarConfig.opMode == m_train)
 {
  if (_jaffarConfig.mpiRank == 0) printf("[Jaffar] Starting search with %d MPI Ranks.\n", _jaffarConfig.mpiSize);

  // Initializing search module
  Search search;

  // Running Search
  search.run();
 }

 // Synchronize all workers
 MPI_Barrier(MPI_COMM_WORLD);

 if (_jaffarConfig.mpiRank == 0) printf("[Jaffar] Finished.\n");
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
