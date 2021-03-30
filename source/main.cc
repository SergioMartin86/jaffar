#include <stdio.h>
#include "sdlpop-instance.h"

int main(int argc, char* argv[])
{
 printf("Loading SDLPop...\n");
 SDLPopInstance s;
 s.initialize();
 printf("Done.\n");
} 
