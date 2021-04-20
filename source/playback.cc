#include "playback.h"
#include "state.h"
#include "utils.h"
#include <ncurses.h>

Playback::Playback()
{
 // Parsing whether to introduce pauses
 _pauseAfterFrame = _jaffarConfig.configJs["Playback Configuration"]["Pause After Every Frame"].get<bool>();

 // Parsing debugging verbosity
 _produceReplay = _jaffarConfig.configJs["Playback Configuration"]["Produce Replay File"].get<bool>();

 // Parsing frame duration
 _frameDuration = _jaffarConfig.configJs["Playback Configuration"]["Frame Duration"].get<size_t>();
}

void Playback::play()
{
 // Initializing ncurses screen
 initscr();
 cbreak();
 noecho();
 nodelay(stdscr, TRUE);
 scrollok(stdscr, TRUE);

 // Getting move list
 const auto moveList = split(_jaffarConfig.moveSequence, ' ');

 // Getting sequence size
 const int sequenceLength = moveList.size();

 // Printing info
 printw("[Jaffar] Playing sequence file: %s\n", _jaffarConfig.inputSequenceFile.c_str());
 printw("[Jaffar] Sequence length: %d frames\n", sequenceLength);
 printw("[Jaffar] Generating frame sequence...\n");

 refresh();

 // Initializing generating SDLPop Instance
 SDLPopInstance genSDLPop;
 genSDLPop.initialize(false);

 // Initializing generating State Handler
 State genState(&genSDLPop);

 // Storage for sequence frames
 std::vector<std::string> frameSequence;

 // If replay is requested, start process
 if (_produceReplay)
 {
  genSDLPop.init_record_replay();
  genSDLPop.start_recording();
 }

 // Saving initial frame
 frameSequence.push_back(genState.saveFrame());

 // Iterating move list in the sequence
 for (int i = 0; i < sequenceLength; i++)
 {
  genSDLPop.performMove(moveList[i]);
  genSDLPop.advanceFrame();

  // Storing new frame
  frameSequence.push_back(genState.saveFrame());

  if (_produceReplay) genSDLPop.add_replay_move();
 }

 if (_produceReplay == true)
 {
  std::string replayFileName = "jaffar.p1r";
  genSDLPop.save_recorded_replay(replayFileName.c_str());
  printw("[Jaffar] Replay saved in '%s'.\n", replayFileName.c_str());
 }

 printw("[Jaffar] Opening SDLPop window...\n");

 // Initializing showing SDLPop Instance
 SDLPopInstance showSDLPop;
 showSDLPop.initialize(true);

 // Initializing State Handler
 State showState(&showSDLPop);

 // Setting timer for a human-visible animation
 showSDLPop.set_timer_length(timer_1, _frameDuration);

 // Printing initial frame info
 showSDLPop.draw();

 // Variable for current step in view
 int currentStep = 1;

 // Interactive section
 int command;
 do
 {
  // Loading requested step
  showState.loadFrame(frameSequence[currentStep-1]);

  // Draw requested step
  showSDLPop.draw();

  // Printing step information
  printw("[Jaffar] ----------------------------------------------------------------\n");
  printw("[Jaffar] Current Step #: %d / %d\n", currentStep, sequenceLength);
  printw("[Jaffar]  + Move: %s\n", moveList[currentStep-1].c_str()) ;
  printw("[Jaffar]  + [showSDLPop.Kid]   Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Sequence: %4d\n", int(showSDLPop.Kid->room), int(showSDLPop.Kid->x), int(showSDLPop.Kid->y), int(showSDLPop.Kid->frame), int(*showSDLPop.hitp_curr), int(*showSDLPop.hitp_max), int(showSDLPop.Kid->curr_seq));
  printw("[Jaffar]  + [showSDLPop.Guard] Room: %d, Pos.x: %3d, Pos.y: %3d, Frame: %3d, HP: %d/%d, Sequence: %4d\n", int(showSDLPop.Guard->room), int(showSDLPop.Guard->x), int(showSDLPop.Guard->y), int(showSDLPop.Guard->frame), int(*showSDLPop.guardhp_curr), int(*showSDLPop.guardhp_max), int(showSDLPop.Guard->curr_seq));
  printw("[Jaffar]  + Exit Door Open: %s\n", showSDLPop.isLevelExitDoorOpen() ? "Yes" : "No");
  printw("[Jaffar]  + RNG State: 0x%08X (Last Loose Tile Sound Id: %d)\n", *showSDLPop.random_seed, *showSDLPop.last_loose_sound);

  // Print command list
  printw("[Jaffar] Available commands: n: -1 m: +1 | h: -10 | j: +10 | y: -100 | u: +100 | q: quit \n");

  // Get command
  command = getKeyPress();

  // Parse command
  if (command == 'n') currentStep = currentStep-1;
  if (command == 'm') currentStep = currentStep+1;
  if (command == 'h') currentStep = currentStep-10;
  if (command == 'j') currentStep = currentStep+10;
  if (command == 'y') currentStep = currentStep-100;
  if (command == 'u') currentStep = currentStep+100;

  // Correct current step if requested more than possible
  if (currentStep < 1) currentStep = 1;
  if (currentStep > sequenceLength) currentStep = sequenceLength;

 } while (command != 'q');

 // Ending ncurses window
 endwin();
}
