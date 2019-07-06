#include "test/test_main.h"
#include "datastructures/hmempool.h"
#include <string.h>

int hmempool_test_add_remove()
{
	int nf = 0; //Number of failures

	int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	size_t len = LENGTH(numbers);
	uint32_t handles[len];
	struct hmempool hm = hmempool_new(len, sizeof(int));
	struct hmempool *Hm = &hm;
	for (int i = 0; i < len; i++)
		handles[i] = hmempool_add(Hm, &numbers[i]);

	//Numbers added in order
	TEST_SOFT_ASSERT(nf, memcmp(numbers, Hm->pool.pool, sizeof(numbers)) == 0)

	//Remove first half
	size_t half = len/2;
	for (int i = 0; i < half; i++)
		hmempool_remove(Hm, handles[i]);

	//Handles are still stable, even though elements have been moved behind the scenes
	for (int i = half; i < len; i++)
		TEST_SOFT_ASSERT(nf, numbers[i] == *(int *)hmempool_get(Hm, handles[i]))

	TEST_SOFT_ASSERT(nf, Hm->pool.num == (len - half))

	hmempool_delete(Hm);
	return nf;
}

int hmempool_test_resize()
{
	int nf = 0; //Number of failures

	int numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	size_t len = LENGTH(numbers);
	uint32_t handles[len];
	size_t half = len/2;
	TEST_SOFT_ASSERT(nf, half * 2 == len);
	struct hmempool hm = hmempool_new(half, sizeof(int));
	struct hmempool *Hm = &hm;
	for (int i = 0; i < half; i++)
		handles[i] = hmempool_add(Hm, &numbers[i]);

	//Half added successfully
	TEST_SOFT_ASSERT(nf, memcmp(numbers, Hm->pool.pool, sizeof(numbers)/2) == 0)

	//Add other half
	hmempool_resize(Hm, len);
	for (int i = half; i < len; i++)
		handles[i] = hmempool_add(Hm, &numbers[i]);

	//Numbers added in order, despite resize
	TEST_SOFT_ASSERT(nf, memcmp(numbers, Hm->pool.pool, sizeof(numbers)) == 0)

	//All handles are valid still
	for (int i = 0; i < len; i++)
		TEST_SOFT_ASSERT(nf, numbers[i] == *(int *)hmempool_get(Hm, handles[i]))

	hmempool_delete(Hm);
	return nf;
}

int hmempool_test_resize_stretch()
{
	int nf = 0; //Number of failures

	struct two_numbers {
		int number[2];
	};
	struct two_numbers numbers[] = {{1,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {7,7}, {8,8}, {9,9}, {10,10}};
	size_t len = LENGTH(numbers);
	size_t half = len/2;
	uint32_t handles[len];
	struct hmempool hm = hmempool_new(half, sizeof(int));
	struct hmempool *Hm = &hm;
	for (int i = 0; i < half; i++)
		handles[i] = hmempool_add(Hm, &numbers[i].number[0]);

	//Numbers added in order
	for (int i = 0; i < half; i++)
		TEST_SOFT_ASSERT(nf, numbers[i].number[0] == *(int *)hmempool_get(Hm, handles[i]))

	hmempool_resize_stretch(Hm, len, sizeof(struct two_numbers));

	//Handles are still stable, even though elements have been moved behind the scenes
	for (int i = 0; i < half; i++)
		TEST_SOFT_ASSERT(nf, numbers[i].number[0] == *(int *)hmempool_get(Hm, handles[i]))

	//Add larger elements
	for (int i = half; i < len; i++)
		handles[i] = hmempool_add(Hm, &numbers[i]);

	//Larger elements added successfully
	for (int i = half; i < len; i++)
		TEST_SOFT_ASSERT(nf, memcmp(&numbers[i], hmempool_get(Hm, handles[i]), sizeof(struct two_numbers)) == 0)

	//All initial elements still there
	for (int i = 0; i < half; i++)
		TEST_SOFT_ASSERT(nf, numbers[i].number[0] == *(int *)hmempool_get(Hm, handles[i]))

	//Fill in second number for first half
	for (int i = 0; i < half; i++)
		((struct two_numbers *)hmempool_get(Hm, handles[i]))->number[1] = numbers[i].number[1];

	//Things are all added where expected
	TEST_SOFT_ASSERT(nf, memcmp(numbers, Hm->pool.pool, sizeof(numbers)) == 0)

	//Remove first half for kicks
	for (int i = 0; i < half; i++)
		hmempool_remove(Hm, handles[i]);

	//All later handles are valid still
	for (int i = half; i < len; i++)
		TEST_SOFT_ASSERT(nf, memcmp(&numbers[i], hmempool_get(Hm, handles[i]), sizeof(struct two_numbers)) == 0)

	hmempool_delete(Hm);
	return nf;
}