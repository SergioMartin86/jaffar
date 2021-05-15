#include "argparse.hpp"
#include "common.h"
#include "nlohmann/json.hpp"
#include "state.h"
#include "utils.h"
#include <ncurses.h>
#include <unistd.h>

// Function to check for keypress taken from https://github.com/ajpaulson/learning-ncurses/blob/master/kbhit.c
int kbhit()
{
  int ch, r;

  // turn off getch() blocking and echo
  nodelay(stdscr, TRUE);
  noecho();

  // check for input
  ch = getch();
  if (ch == ERR) // no input
    r = FALSE;
  else // input
  {
    r = TRUE;
    ungetch(ch);
  }

  // restore block and echo
  echo();
  nodelay(stdscr, FALSE);

  return (r);
}

int getKeyPress()
{
  while (!kbhit())
  {
    usleep(100000ul);
    refresh();
  }
  return getch();
}

int main(int argc, char *argv[])
{
  // Defining arguments
  argparse::ArgumentParser program("jaffar-play", JAFFAR_VERSION);

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

  // Getting savefile path
  std::string saveFilePath = program.get<std::string>("savFile");

  // Loading save file contents
  std::string saveString;
  bool status = loadStringFromFile(saveString, saveFilePath.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not load save state from file: %s\n", saveFilePath.c_str());


  // If sequence file defined, load it and play it
  std::string moveSequence;
  std::string solutionFile = program.get<std::string>("solutionFile");
  status = loadStringFromFile(moveSequence, solutionFile.c_str());
  if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from solution file: %s\n%s \n", solutionFile.c_str(), program.help().str().c_str());

  // Initializing ncurses screen
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  scrollok(stdscr, TRUE);

  // Getting move list
  const auto moveList = split(moveSequence, ' ');

  // Getting sequence size
  const int sequenceLength = moveList.size();

  // Printing info
  printw("[Jaffar] Playing sequence file: %s\n", solutionFile.c_str());
  printw("[Jaffar] Sequence length: %d frames\n", sequenceLength);
  printw("[Jaffar] Generating frame sequence...\n");

  refresh();

  // Initializing replay generating SDLPop Instance
  SDLPopInstance genSDLPop("libsdlPopLib.so", false);
  genSDLPop.initialize(false);

  // Storage for sequence frames
  std::vector<std::string> frameSequence;

  // Starting replay creation
  genSDLPop.init_record_replay();
  genSDLPop.start_recording();

  // Initializing generating State Handler
  State genState(&genSDLPop, saveString);

  // Saving initial frame
  frameSequence.push_back(genState.saveState());

//  nlohmann::json sequenceJs;

  // Iterating move list in the sequence
  for (int i = 0; i < sequenceLength; i++)
  {
//     // Kid.Room Kid.x Kid.y Kid.frame Kid.Orientation
//    sequenceJs["Kid Room"][i] = int(genSDLPop.Kid->room);
//    sequenceJs["Kid X"][i] = int(genSDLPop.Kid->x);
//    sequenceJs["Kid Y"][i] = int(genSDLPop.Kid->y);
//    sequenceJs["Kid Frame"][i] = int(genSDLPop.Kid->frame);
//    sequenceJs["Kid Direction"][i] = int(genSDLPop.Kid->direction);

    genSDLPop.performMove(moveList[i]);
    genSDLPop.advanceFrame();

    // Storing new frame
    frameSequence.push_back(genState.saveState());
  }

//  saveStringToFile(sequenceJs.dump(2).c_str(), "sequence.js");
  printw("[Jaffar] Opening SDLPop window...\n");

  // Initializing showing SDLPop Instance
  SDLPopInstance showSDLPop("libsdlPopLib.so", false);
  showSDLPop.initialize(true);

  // Initializing State Handler
  State showState(&showSDLPop, saveString);

  // Setting timer for a human-visible animation
  showSDLPop.set_timer_length(timer_1, 16);

  // Setting window title
  SDL_SetWindowTitle(*showSDLPop.window_, "Jaffar Play");

  // Printing initial frame info
  showSDLPop.draw();

  // Variable for current step in view
  int currentStep = 1;

  // Print command list
  printw("[Jaffar] Available commands:\n");
  printw("[Jaffar]  n: -1 m: +1 | h: -10 | j: +10 | y: -100 | u: +100 \n");
  printw("[Jaffar]  g: set RNG | l: loose tile sound | s: quicksave | r: create replay | q: quit  \n");
  printw("[Jaffar]  1: set lvl1 music \n");

  // Flag to display frame information
  bool showFrameInfo = true;

  // Interactive section
  int command;
  do
  {
    // Loading requested step
    showState.loadState(frameSequence[currentStep - 1]);

    // Draw requested step
    showSDLPop.draw();

    // Printing frame information
    if (showFrameInfo)
    {
      printw("[Jaffar] ----------------------------------------------------------------\n");
      printw("[Jaffar] Current Step #: %d / %d\n", currentStep, sequenceLength);
      printw("[Jaffar]  + Move: %s\n", moveList[currentStep - 1].c_str());

      printw("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Fall.x: %d, Fall.y: %d, Frame: %3d, HP: %d/%d, Direction: %d\n",
             int(showSDLPop.Kid->room),
             int(showSDLPop.Kid->x),
             int(showSDLPop.Kid->y),
             int(showSDLPop.Kid->fall_x),
             int(showSDLPop.Kid->fall_y),
             int(showSDLPop.Kid->frame),
             int(*showSDLPop.hitp_curr),
             int(*showSDLPop.hitp_max),
             int(showSDLPop.Kid->direction));

      printw("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Fall.x: %d, Fall.y: %d, Frame: %3d, HP: %d/%d, Direction: %d\n",
             int(showSDLPop.Guard->room),
             int(showSDLPop.Guard->x),
             int(showSDLPop.Guard->y),
             int(showSDLPop.Guard->fall_x),
             int(showSDLPop.Guard->fall_y),
             int(showSDLPop.Guard->frame),
             int(*showSDLPop.guardhp_curr),
             int(*showSDLPop.guardhp_max),
             int(showSDLPop.Guard->direction));

      printw("[Jaffar]  + Exit Door Open: %s\n", showSDLPop.isLevelExitDoorOpen() ? "Yes" : "No");
      printw("[Jaffar]  + Reached Checkpoint: %s (%d)\n", *showSDLPop.checkpoint ? "Yes" : "No", *showSDLPop.checkpoint);
      printw("[Jaffar]  + Feather Fall: %d\n", *showSDLPop.is_feather_fall);
      printw("[Jaffar]  + Need Lvl1 Music: %d\n", *showSDLPop.need_level1_music);
      printw("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", *showSDLPop.random_seed, *showSDLPop.last_loose_sound);
    }

    // Resetting show frame info flag
    showFrameInfo = true;

    // Get command
    command = getKeyPress();

    // Advance/Rewind commands
    if (command == 'n') currentStep = currentStep - 1;
    if (command == 'm') currentStep = currentStep + 1;
    if (command == 'h') currentStep = currentStep - 10;
    if (command == 'j') currentStep = currentStep + 10;
    if (command == 'y') currentStep = currentStep - 100;
    if (command == 'u') currentStep = currentStep + 100;

    // Correct current step if requested more than possible
    if (currentStep < 1) currentStep = 1;
    if (currentStep > sequenceLength) currentStep = sequenceLength;

    // Replay creation command
    if (command == 'r')
    {
      // Storing replay file
      std::string replayFileName = "jaffar.p1r";
      genSDLPop.save_recorded_replay(replayFileName.c_str());
      printw("[Jaffar] Replay saved in '%s'.\n", replayFileName.c_str());

      // Do no show frame info again after this action
      showFrameInfo = false;
    }

    // Quicksave creation command
    if (command == 's')
    {
      // Storing replay file
      std::string saveFileName = "jaffar.sav";

      // Saving frame info to file
      bool status = saveStringToFile(frameSequence[currentStep - 1], saveFileName.c_str());
      if (status == true) printw("[Jaffar] State saved in '%s'.\n", saveFileName.c_str());
      if (status == false) printw("[Jaffar] Error saving file '%s'.\n", saveFileName.c_str());

      // Do no show frame info again after this action
      showFrameInfo = false;
    }

    // RNG setting command
    if (command == 'g')
    {
      // Obtaining RNG state
      printw("Enter new RNG state: ");

      // Setting input as new rng
      char str[80];
      getstr(str);
      *showSDLPop.random_seed = std::stol(str);

      // Replacing current sequence
      frameSequence[currentStep-1] = showState.saveState();
    }

    // loose tile sound setting command
    if (command == 'l')
    {
      // Obtaining RNG state
      printw("Enter new last loose tile sound id: ");

      // Setting input as new rng
      char str[80];
      getstr(str);
      *showSDLPop.last_loose_sound = std::stoi(str);

      // Replacing current sequence
      frameSequence[currentStep-1] = showState.saveState();
    }

    // loose tile sound setting command
    if (command == '1')
    {
      // Obtaining RNG state
      printw("Enter new need level 1 music value: ");

      // Setting input as new rng
      char str[80];
      getstr(str);
      *showSDLPop.need_level1_music = std::stoi(str);

      // Replacing current sequence
      frameSequence[currentStep-1] = showState.saveState();
    }

  } while (command != 'q');

  // Ending ncurses window
  endwin();
}
