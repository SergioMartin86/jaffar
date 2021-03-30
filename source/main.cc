#include <stdio.h>
#include "SDLPopInstance.h"
#include <mpi.h>

int main(int argc, char* argv[])
{
 // Initialize the MPI environment
 MPI_Init(&argc, &argv);

 // Get the number of processes
 int commSize;
 MPI_Comm_size(MPI_COMM_WORLD, &commSize);

 // Get the rank of the process
 int commRank;
 MPI_Comm_rank(MPI_COMM_WORLD, &commRank);

 printf("Rank: %d/%d - Loading SDLPop...\n", commRank, commSize);
 SDLPopInstance s;

 printf("Done.\n");
} 
