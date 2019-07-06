#include "hmempool.h"

#include "mempool.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct hmempool hmempool_new(size_t max, size_t size)
{
	struct hmempool tmp = {
		.pool = mempool_new(max, size),
		.free_handles = mempool_new(max, sizeof(uint32_t)),
		.indirection_len = max,
		.freed = false,
		.managed = true,
	};
	tmp.htoi = calloc(max, sizeof(uint32_t));
	tmp.itoh = calloc(max, sizeof(uint32_t));
	//The handle with value 0 is invalid
	for (uint32_t i = max; i > 0; i--)
		mempool_add(&tmp.free_handles, &i);
	return tmp;
}

struct hmempool hmempool_new_unmanaged(size_t max, size_t size, size_t num_handles)
{
	struct hmempool tmp = {
		.pool = mempool_new(max, size),
		.indirection_len = num_handles,
		.freed = false,
	};
	tmp.htoi = calloc(num_handles, sizeof(uint32_t));
	tmp.itoh = calloc(num_handles, sizeof(uint32_t));
	return tmp;
}

void hmempool_resize(struct hmempool *hm, size_t num)
{
	hmempool_resize_stretch(hm, num, hm->pool.size);
}

void hmempool_stretch(struct hmempool *hm, size_t size)
{
	mempool_stretch(&hm->pool, size);
}

void hmempool_resize_stretch(struct hmempool *hm, size_t num, size_t size)
{
	assert(num >= hm->indirection_len); //Making the pool smaller is such a headache, let's not bother for now.
	mempool_resize_stretch(&hm->pool, num, size);
	if (num > hm->indirection_len) {
		void *new_htoi = realloc(hm->htoi, num*sizeof(uint32_t));
		void *new_itoh = realloc(hm->itoh, num*sizeof(uint32_t));
		hm->htoi = new_htoi ? new_htoi : hm->htoi;
		hm->itoh = new_itoh ? new_itoh : hm->itoh;

		if (hm->managed) {
			mempool_resize(&hm->free_handles, num);
			//Generate some new handles (handles start at 1)
			for (uint32_t i = num; i > hm->indirection_len; i--)
				mempool_add(&hm->free_handles, &i);
		}
		for (uint32_t i = num; i > hm->indirection_len; i--) {
			hm->htoi[i-1] = 0;
			hm->itoh[i-1] = 0;
		}

		hm->indirection_len = num;

		if (!new_htoi || !new_itoh)
			printf("%s: We're probably running out of memory here.\n", __FUNCTION__);
	}
}

void hmempool_delete(struct hmempool *hm)
{
	if (!hm->freed) {
		mempool_delete(&hm->pool);
		if (hm->managed)
			mempool_delete(&hm->free_handles);
		free(hm->htoi);
		free(hm->itoh);
		hm->freed = true;
	}
}

uint32_t hmempool_claim(struct hmempool *hm, uint32_t h, void *item)
{
	uint32_t i = mempool_add(&hm->pool, item);
	hm->htoi[h-1] = i;
	hm->itoh[i] = h;
	return h;
}

uint32_t hmempool_claim_raw(struct hmempool *hm, uint32_t h)
{
	uint32_t i = mempool_add_raw(&hm->pool);
	hm->htoi[h-1] = i;
	hm->itoh[i] = h;
	return h;
}

uint32_t hmempool_add(struct hmempool *hm, void *item)
{
	//Get new handle from free handle pool
	uint32_t h;
	mempool_pop(&hm->free_handles, &h);
	return hmempool_claim(hm, h, item);
}

uint32_t hmempool_add_raw(struct hmempool *hm)
{
	//Get new handle from free handle pool
	uint32_t h;
	mempool_pop(&hm->free_handles, &h);
	return hmempool_claim_raw(hm, h);
}

void hmempool_unclaim(struct hmempool *hm, uint32_t h)
{
	//Get item index
	int i = hm->htoi[h-1];
	//Invalidate handle and index
	hm->htoi[h-1] = 0;
	hm->itoh[i] = 0;
	mempool_remove(&hm->pool, i);
}

void hmempool_remove(struct hmempool *hm, uint32_t h)
{
	//Add handle to free handle pool
	mempool_add(&hm->free_handles, &h);
	hmempool_unclaim(hm, h);
}

void * hmempool_get(struct hmempool *hm, uint32_t h)
{
	return mempool_get(&hm->pool, hm->htoi[h-1]);
}
