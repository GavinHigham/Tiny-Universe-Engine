#include "test/test_main.h"
#include "datastructures/mempool.h"
#include "string.h"

int mempool_test_add_remove()
{
	int nf = 0; //Number of failures

	int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	struct mempool m = mempool_new(LENGTH(numbers), sizeof(int));
	struct mempool *M = &m;
	for (int i = 0; i < LENGTH(numbers); i++)
		mempool_add(M, &numbers[i]);

	//Numbers added in order
	TEST_SOFT_ASSERT(nf, memcmp(numbers, M->pool, sizeof(numbers)) == 0)
	//2nd removed, replaced with 10th
	mempool_remove(M, 1);
	TEST_SOFT_ASSERT(nf, numbers[9] == *(int *)mempool_get(M, 1))
	TEST_SOFT_ASSERT(nf, M->num == LENGTH(numbers) - 1)

	mempool_delete(M);
	return nf;
}

int mempool_test_resize()
{
	int nf = 0; //Number of failures

	int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	struct mempool m = mempool_new(LENGTH(numbers)/2, sizeof(int));
	struct mempool *M = &m;
	for (int i = 0; i < LENGTH(numbers)/2; i++)
		mempool_add(M, &numbers[i]);

	TEST_SOFT_ASSERT(nf, memcmp(numbers, M->pool, sizeof(numbers)/2) == 0)

	mempool_resize(M, M->num * 2);
	for (int i = LENGTH(numbers)/2; i < LENGTH(numbers); i++)
		mempool_add(M, &numbers[i]);

	TEST_SOFT_ASSERT(nf, memcmp(numbers, M->pool, sizeof(numbers)) == 0)

	mempool_delete(M);
	return nf;
}

int mempool_test_stretch()
{
	int nf = 0; //Number of failures

	int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	struct mempool m = mempool_new(LENGTH(numbers), sizeof(int));
	struct mempool *M = &m;
	for (int i = 0; i < LENGTH(numbers); i++)
		mempool_add(M, &numbers[i]);

	TEST_SOFT_ASSERT(nf, memcmp(numbers, M->pool, sizeof(numbers)) == 0)
	mempool_stretch(M, M->size * 2);

	//Even with gaps, numbers copied successfully
	for (int i = 0; i < LENGTH(numbers); i++)
		TEST_SOFT_ASSERT(nf, numbers[i] == *(int *)mempool_get(M, i));

	mempool_delete(M);
	return nf;
}