#include <stdio.h>
#include "SDLPopInstance.h"
#include "utils.h"
#include "state.h"
#include "search.h"
#include <mpi.h>

const std::string moveString = ". . . . . . . RD . . . . RD . . . . . . . . . . . . . . . RU . . . . . . . . . . . . . . . . . RD . . . . RD . . . . RD . . . . D RD . . . . . S . . . . . . . . . . . . . . . . . . . . R . . . . . . R . U . . . . . . . . . . . R . . . RU . . . . . . . . . . R . . . . . . R . . . R . . . . . . . . . L . . . . . U . . . . . . . . . . . . . . . . . . . . R . . . R . . . . . . R . . . R . . . R . . D . . . . . . . . . . . . . U";

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
 SDLPopInstance s;
 State state(&s);
 Search search(&s);

 s.initialize(true);

 const auto moveList = split(moveString, ' ');
 for (const auto move : moveList)
 {
   s.performMove(move);
   s.advanceFrame();
   s.printFrameInfo();
   s.draw();
   printf("Hash: 0x%016lX\n", state.computeHash());
 }

 printf("Done.\n");
} 
