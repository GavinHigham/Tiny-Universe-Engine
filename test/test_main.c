#include "test_main.h"
#include "macros.h"
#include "mempool.test.c"
#include "hmempool.test.c"
#include "ecs.test.c"
#include "ply_mesh.test.c"
#include <unistd.h>

#define RUN_TEST(testfn) run_test(testfn, #testfn)

bool nofork = false;

int run_test(int (test_fn)(void), char *test_fn_name)
{
	int wstatus = 0;
	if (nofork || !fork()) { //Test process start
		int ret = test_fn();
		if (ret)
			printf(ANSI_COLOR_RED "Test failed: " ANSI_COLOR_YELLOW "%s reported %d error%s."ANSI_COLOR_RESET"\n", test_fn_name, ret, ret==1?"":"s");
		else
			printf(ANSI_COLOR_GREEN "Test succeeded - %s" ANSI_COLOR_RESET "\n", test_fn_name);
		if (!nofork)
			exit(ret); //Test process exit
	} else if (!nofork) {
		waitpid(0, &wstatus, 0);
		if (WIFEXITED(wstatus)) {
			int status = WEXITSTATUS(wstatus);
			if (status) {
				printf(ANSI_COLOR_YELLOW "Test process exited with status %i." ANSI_COLOR_RESET "\n", status);
				return status;
			}
		} else {
			int termsig = -1;
			printf(ANSI_COLOR_RED "Test process did not exit normally");
			if (WIFSIGNALED(wstatus)) {
				termsig = WTERMSIG(wstatus);
				printf(" - terminated with signal %i", termsig);
			}
			printf("." ANSI_COLOR_RESET " See list of signals with \"kill -l\".\n");
			return termsig;
		}
	}
	return 0;
}

int test_main(int argc, char **argv)
{
	if (argc > 2 && !strcmp(argv[2], "nofork"))
		nofork = true;

	RUN_TEST(mempool_test_add_remove);
	RUN_TEST(mempool_test_resize);
	RUN_TEST(mempool_test_stretch);

	RUN_TEST(hmempool_test_add_remove);
	RUN_TEST(hmempool_test_resize);
	RUN_TEST(hmempool_test_resize_stretch);

	RUN_TEST(ecs_test_1);

	RUN_TEST(ply_mesh_load_cube);
	RUN_TEST(ply_mesh_load_newship);

	return 0;
}