#include "argparse.hpp"
#include "common.h"
#include "nlohmann/json.hpp"
#include "frame.h"
#include "state.h"
#include "utils.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
 // Defining arguments
 argparse::ArgumentParser program("jaffar-show-replay", JAFFAR_VERSION);

 program.add_argument("savFile")
   .help("Specifies the path to the SDLPop savefile (.sav) from which to start.")
   .required();

 program.add_argument("solutionFile")
   .help("path to the Jaffar solution (.sol) file to run.")
   .required();

 // Parsing command line
 try
 {
   program.parse_args(argc, argv);
 }
 catch (const std::runtime_error &err)
 {
   fprintf(stderr, "[Jaffar] Error parsing command line arguments: %s\n%s", err.what(), program.help().str().c_str());
   exit(-1);
 }

 // Initializing replay generating SDLPop Instance
 SDLPopInstance sdlPop("libsdlPopLib.so", false);
 sdlPop.initialize(false);

 // Getting savefile paths
 std::string saveFilePath = program.get<std::string>("savFile");
 std::string solutionFile = program.get<std::string>("solutionFile");

 // Constant loop of updates
 while (true)
 {
  // Loading save file contents
  std::string saveString;
  bool status = loadStringFromFile(saveString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());

  // If sequence file defined, load it and play it
  std::string moveSequence;
  status = loadStringFromFile(moveSequence, solutionFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from solution file: %s\n%s \n", solutionFile.c_str(), program.help().str().c_str());

  // Initializing generating State Handler
  State genState(&sdlPop, saveString);

  // Getting move list
  const auto moveList = split(moveSequence, ' ');

  // Getting sequence size
  const int sequenceLength = moveList.size();

  // Printing info
  printf("[Jaffar] Playing sequence file: %s\n", solutionFile.c_str());
  printf("[Jaffar] Sequence length: %d frames\n", sequenceLength);
  printf("[Jaffar] Generating frame sequence...\n");

  // Storage for sequence frames
  std::vector<std::string> frameSequence;

  // Starting replay creation
  sdlPop.init_record_replay();
  sdlPop.start_recording();


  // Iterating move list in the sequence
  for (int i = 0; i < sequenceLength; i++)
  {
    sdlPop.performMove(moveList[i]);
    sdlPop.advanceFrame();
  }

  // Saving replay into a file
  sdlPop.save_recorded_replay("jaffar.p1r");

  // Sleeping for a couple seconds
  sleep(5);
 }
}
