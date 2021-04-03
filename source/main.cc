#include <stdio.h>
#include "SDLPopInstance.h"
#include "utils.h"
#include <mpi.h>

//const std::string moveString = ". . . . . . . RD . . . . RD . . . . . . . . . . . . . . . RU . . . . . . . . . . . . . . . . . RD . . . . RD . . . . RD . . . . D RD . . . . . S . . . . . . . . . . . . . . . . . . . . R . . . . . . R . U . . . . . . . . . . . R . . . RU . . . . . . . . . . R . . . . . . R . . . R . . . . . . . . . L . . . . . U . . . . . . . . . . . . . . . . . . . . R . . . R . . . . . . R . . . R . . . R . . D . . . . . . . . . . . . . U";
const std::string moveString = ". . . . . . . RD";

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
 s.initialize();

 int frame = 0;
 printf("Initial_seed: %d\n", *s.random_seed);
 //getchar();
 int last_room = 0;

 const auto moveList = split(moveString, ' ');
 for (const auto move : moveList)
 {
   s.performMove(move);
   s.advanceFrame();
   s.printFrameInfo();
   // state.Quicksave("/tmp/cat/" + std::to_string(frame));
   /* std::cout << "Frame " << frame++ << " hash " << state.ComputeHash()
             << " seed " << **s.random_seed << " move " << int(move)
             << " exit_timer:" << exit_room_timer
             << " dif_room:" << different_room; */
   // prince.PrepareGameAfterLoad();
   s.draw();
   /* std::cout << " exit_timer2:" << exit_room_timer
             << " iswin?:" << scorer.IsWin() << " isloss?:" << scorer.IsLoss()
             << std::endl; */
 }

 printf("Done.\n");
} 
