#include <stdio.h>
#include "SDLPopInstance.h"
#include "utils.h"
#include "state.h"
#include "search.h"
#include "argparse.hpp"
#include <mpi.h>

using namespace argparse;

const std::string moveString = ". . . . . . . RD . . . . RD . . . . . . . . . . . . . . . RU . . . . . . . . . . . . . . . . . RD . . . . RD . . . . RD . . . . D RD . . . . . S . . . . . . . . . . . . . . . . . . . . R . . . . . . R . U . . . . . . . . . . . R . . . RU . . . . . . . . . . R . . . . . . R . . . R . . . . . . . . . L . . . . . U . . . . . . . . . . . . . . . . . . . . R . . . R . . . . . . R . . . R . . . R . . D . . . . . . . . . . . . . U";

void parseArgs(int argc, char* argv[])
{
 ArgumentParser program("jaffar", "1.0.0");

 program.add_argument("-f", "--config")
   .help("path to the Jaffar configuration (.config) file to run.")
   .default_value("jaffar.config")
   .required();

 program.add_argument("-s", "--sav")
    .help("path to the SDLPop savegame (.sav) file to run.")
    .default_value("quicksave.sav")
    .required();

 program.add_argument("-p", "--play")
   .help("plays a given sequence (.seq) file.");

 try
 {
    program.parse_args(argc, argv);
 }
 catch (const std::runtime_error& err)
 {
   fprintf(stderr, "%s\n%s", err.what(), program.help().str().c_str());
   exit(-1);
 }
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

 s.initialize(14, true);
 s.setSeed(0xF0F0F0F0);
 s.printFrameInfo();
 s.draw();
 state.quickSave("current.sav");
 getchar();

 const auto moveList = split(moveString, ' ');
 for (const auto move : moveList)
 {
   s.performMove(move);
   s.advanceFrame();
   s.printFrameInfo();
   s.draw();
   getchar();
 }

 printf("Done.\n");
} 
