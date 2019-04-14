#include "test_main.h"
#include "macros.h"
#include "mempool.ctest"

#define RUN_TEST(testfn) {if ((ret = testfn)) { printf(ANSI_COLOR_RED "Test failed: " ANSI_COLOR_YELLOW #testfn " reported %d errors.\n" ANSI_COLOR_RESET, ret); any_failed = 1; } \
else { printf(ANSI_COLOR_GREEN "Test succeeded - " #testfn ANSI_COLOR_RESET "\n"); }}

int test_main(int argc, char **argv)
{
	int ret = 0, any_failed = 0;
	RUN_TEST(mempool_test1())
	return 0;
}