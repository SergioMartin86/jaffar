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
  argparse::ArgumentParser program("jaffar-show-snapshot", JAFFAR_VERSION);

  program.add_argument("saveFile")
    .help("path to the Prince of Persia Save file (.sav) to display.")
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

  double updateEverySeconds = 1.0;
  if (const char *updateEverySecondsEnv = std::getenv("JAFFAR_SHOW_UPDATE_EVERY_SECONDS"))
    updateEverySeconds = std::stof(updateEverySecondsEnv);
  else
    fprintf(stderr, "[Jaffar] Warning: Environment variable JAFFAR_SHOW_UPDATE_EVERY_SECONDS is not defined. Using default: 1.0s\n");

  // Getting savefile path
  std::string saveFilePath = program.get<std::string>("saveFile");

  // Loading save file contents
  std::string saveString;
  bool status = loadStringFromFile(saveString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());

  // Initializing showing SDLPop Instance
  SDLPopInstance showSDLPop("libsdlPopLib.so", false);
  showSDLPop.initialize(true);

  // Initializing State Handler
  State showState(&showSDLPop, saveString);

  // Setting timer for a human-visible animation
  //showSDLPop.set_timer_length(timer_1, 16);

  // Setting window title
  std::string windowTitle = "Jaffar Show: " + saveFilePath;
  //SDL_SetWindowTitle(*showSDLPop.window_, windowTitle.c_str());

  // Constant loop of updates
  while (true)
  {
    // Reloading save file
    std::string saveData;
    bool status = loadStringFromFile(saveData, saveFilePath.c_str());

    if (status == true && saveData.size() == _FRAME_DATA_SIZE)
    {
      // Loading data into state
      showState.loadState(saveData);

      // Drawing frame
      showSDLPop.draw();
    }

    // Adding sanity pause
    useconds_t waitLength = std::floor(updateEverySeconds * 1000000.0);
    usleep(waitLength);
  }
}
