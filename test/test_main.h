#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include "macros.h"
#include "stdio.h"

//Soft assert, meaning don't abort the rest of the program, just continue.
#define TEST_SOFT_ASSERT(numfailed, x) if (x) {printf(ANSI_COLOR_YELLOW "Soft assert failed: " ANSI_COLOR_RED "(" #x ")" ANSI_COLOR_YELLOW " in " __FILE__ ":%d" ANSI_COLOR_RESET "\n", __LINE__); numfailed++;}

int test_main(int argc, char **argv);

#endif