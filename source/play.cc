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
  catch (const std::runtime_error &err)
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
  catch (const std::exception &err)
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

  #pragma omp parallel for
  {
   // Initializing generating SDLPop Instance
   SDLPopInstance genSDLPop;
   genSDLPop.initialize(false);

   // Storage for sequence frames
   std::vector<std::string> frameSequence;

   // Initializing generating State Handler
   State genState(&genSDLPop, scriptJs);

   // Saving initial frame
   frameSequence.push_back(genState.saveState());

   // Iterating move list in the sequence
   for (int i = 0; i < sequenceLength; i++)
   {
     genSDLPop.performMove(moveList[i]);
     genSDLPop.advanceFrame();

     // Storing new frame
     frameSequence.push_back(genState.saveState());
   }
  }

  exit(0);
  endwin();
}
