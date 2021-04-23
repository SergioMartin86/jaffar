#include "common.h"
#include "state.h"
#include "utils.h"
#include <ncurses.h>
#include "argparse.hpp"
#include "nlohmann/json.hpp"
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
 if( ch == ERR)      // no input
   r = FALSE;
 else                // input
 {
  r = TRUE;
  ungetch(ch);
 }

 // restore block and echo
 echo();
 nodelay(stdscr, FALSE);

 return(r);
}

int getKeyPress()
{
 while(!kbhit())
 {
  usleep(100000ul);
  refresh();
 }
 return getch();
}

int main(int argc, char* argv[])
{
 // Defining arguments
 argparse::ArgumentParser program("jaffar-play", JAFFAR_VERSION);

 program.add_argument("jaffarFile")
   .help("path to the Jaffar script (.jaffar) file to run.")
   .required();

 program.add_argument("sequenceFile")
   .help("path to the Jaffar move sequence (.seq) file to run.")
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

 // Reading config file
 std::string scriptFile = program.get<std::string>("jaffarFile");
 std::string scriptString;
 bool status = loadStringFromFile(scriptString, scriptFile.c_str());
 if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from config file: %s\n%s \n", scriptFile.c_str(), program.help().str().c_str());

 // Parsing JSON from config file
 nlohmann::json scriptJs;
 try
 {
  scriptJs = nlohmann::json::parse(scriptString);
 }
 catch (const std::exception& err)
 {
   fprintf(stderr, "[Error] Parsing configuration file %s. Details:\n%s\n", scriptFile.c_str(), err.what());
   exit(-1);
 }

 // If sequence file defined, load it and play it
 std::string moveSequence;
 std::string sequenceFile = program.get<std::string>("sequenceFile");
 status = loadStringFromFile(moveSequence, sequenceFile.c_str());
 if (status == false) EXIT_WITH_ERROR("[ERROR] Could not find or read from sequence input file: %s\n%s \n", sequenceFile.c_str(), program.help().str().c_str());

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
 printw("[Jaffar] Playing sequence file: %s\n", sequenceFile.c_str());
 printw("[Jaffar] Sequence length: %d frames\n", sequenceLength);
 printw("[Jaffar] Generating frame sequence...\n");

 refresh();

 // Initializing generating SDLPop Instance
 SDLPopInstance genSDLPop;
 genSDLPop.initialize(false);

 // Initializing generating State Handler
 State genState(&genSDLPop, scriptJs);

 // Storage for sequence frames
 std::vector<std::string> frameSequence;

 // Starting replay creation
  genSDLPop.init_record_replay();
  genSDLPop.start_recording();

 // Saving initial frame
 frameSequence.push_back(genState.saveFrame());

 // Iterating move list in the sequence
 for (int i = 0; i < sequenceLength; i++)
 {
  genSDLPop.performMove(moveList[i]);
  genSDLPop.advanceFrame();

  // Storing new frame
  frameSequence.push_back(genState.saveFrame());
 }

 printw("[Jaffar] Opening SDLPop window...\n");

 // Initializing showing SDLPop Instance
 SDLPopInstance showSDLPop;
 showSDLPop.initialize(true);

 // Initializing State Handler
 State showState(&showSDLPop, scriptJs);

 // Setting timer for a human-visible animation
 showSDLPop.set_timer_length(timer_1, 16);

 // Setting window title
 SDL_SetWindowTitle(*showSDLPop.window_, "Jaffar Play");

 // Printing initial frame info
 showSDLPop.draw();

 // Variable for current step in view
 int currentStep = 1;

 // Print command list
 printw("[Jaffar] Available commands: n: -1 m: +1 | h: -10 | j: +10 | y: -100 | u: +100 | s: quicksave | r: create replay | q: quit  \n");

 // Flag to display frame information
 bool showFrameInfo = true;

 // Interactive section
 int command;
 do
 {
  // Loading requested step
  showState.loadFrame(frameSequence[currentStep-1]);

  // Draw requested step
  showSDLPop.draw();

  // Printing frame information
  if (showFrameInfo)
  {
   printw("[Jaffar] ----------------------------------------------------------------\n");
   printw("[Jaffar] Current Step #: %d / %d\n", currentStep, sequenceLength);
   printw("[Jaffar]  + Move: %s\n", moveList[currentStep-1].c_str()) ;

   printw("[Jaffar]  + [Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Fall.y: %d, Frame: %3d, HP: %d/%d\n",
     int(showSDLPop.Kid->room),
     int(showSDLPop.Kid->x),
     int(showSDLPop.Kid->y),
     int(showSDLPop.Kid->fall_y),
     int(showSDLPop.Kid->frame),
     int(*showSDLPop.hitp_curr),
     int(*showSDLPop.hitp_max));

   printw("[Jaffar]  + [Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Fall.y: %d,Frame: %3d, HP: %d/%d\n",
     int(showSDLPop.Guard->room),
     int(showSDLPop.Guard->x),
     int(showSDLPop.Guard->y),
     int(showSDLPop.Guard->fall_y),
     int(showSDLPop.Guard->frame),
     int(*showSDLPop.guardhp_curr),
     int(*showSDLPop.guardhp_max));

   printw("[Jaffar]  + Exit Door Open: %s\n", showSDLPop.isLevelExitDoorOpen() ? "Yes" : "No");
   printw("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", *showSDLPop.random_seed, *showSDLPop.last_loose_sound);
  }

  // Resetting show frame info flag
  showFrameInfo = true;

  // Get command
  command = getKeyPress();

  // Advance/Rewind commands
  if (command == 'n') currentStep = currentStep-1;
  if (command == 'm') currentStep = currentStep+1;
  if (command == 'h') currentStep = currentStep-10;
  if (command == 'j') currentStep = currentStep+10;
  if (command == 'y') currentStep = currentStep-100;
  if (command == 'u') currentStep = currentStep+100;

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
    showState.quickSave(saveFileName);
    printw("[Jaffar] State saved in '%s'.\n", saveFileName.c_str());

    // Do no show frame info again after this action
    showFrameInfo = false;
  }

  // Correct current step if requested more than possible
  if (currentStep < 1) currentStep = 1;
  if (currentStep > sequenceLength) currentStep = sequenceLength;

 } while (command != 'q');

 // Ending ncurses window
 endwin();
}
