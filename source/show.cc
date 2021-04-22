#include "common.h"
#include "state.h"
#include "utils.h"
#include "argparse.hpp"
#include <unistd.h>
#include "nlohmann/json.hpp"

int main(int argc, char* argv[])
{
 // Defining arguments
 argparse::ArgumentParser program("jaffar-show", JAFFAR_VERSION);

 program.add_argument("saveFile")
   .help("path to the Prince of Persia Save file (.sav) to display.")
   .required();

 // Parsing command line
 try
 {
  program.parse_args(argc, argv);
 }
 catch (const std::runtime_error& err)
 {
   fprintf(stderr, "[Jaffar] Error parsing command line arguments: %s\n%s", err.what(), program.help().str().c_str());
   exit(-1);
 }

 double updateEverySeconds = 1.0;
 if(const char* updateEverySecondsEnv = std::getenv("JAFFAR_SHOW_UPDATE_EVERY_SECONDS"))
  updateEverySeconds  = std::stof(updateEverySecondsEnv);
 else
   fprintf(stderr, "[Jaffar] Warning: Environment variable JAFFAR_SHOW_UPDATE_EVERY_SECONDS is not defined. Using default: 1.0s\n");

 // Getting savefile path
 std::string saveFile = program.get<std::string>("saveFile");

 // Initializing showing SDLPop Instance
 SDLPopInstance showSDLPop;
 showSDLPop.initialize(true);

 // Defining initial configuration
 nlohmann::json stateConfig;
 stateConfig["Path"] = saveFile;
 stateConfig["Random Seed"] = 0;
 stateConfig["Last Loose Tile Sound"] = 0;

 // Initializing State Handler
 State showState(&showSDLPop, stateConfig);

 // Setting timer for a human-visible animation
 showSDLPop.set_timer_length(timer_1, 16);

 // Constant loop of updates
 while(true)
 {
  // Reloading save file
  showState.quickLoad(saveFile);

  // Printing initial frame info
  showSDLPop.draw();

  // Adding sanity pause
  useconds_t waitLength = std::floor(updateEverySeconds * 1000000.0);
  usleep(waitLength);
 }
}
