#include "hmempool.h"

#include "mempool.h"
#include "math/utility.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Note: Indexes in htoi are stored with a +1 bias, so that an index of 0 can represent a handle not mapping to anything in the pool.

struct hmempool hmempool_new(size_t max, size_t size)
{
	struct hmempool tmp = {
		.pool = mempool_new(max, size),
		.free_handles = mempool_new(max, sizeof(uint32_t)),
		.indirection_len = max,
		.allocated = true,
		.managed = true,
	};
	tmp.htoi = calloc(max, sizeof(uint32_t));
	tmp.itoh = calloc(max, sizeof(uint32_t));
	//The handle with value 0 is invalid
	mempool_fill_uint32_t_descending(&tmp.free_handles, 1, max);
	return tmp;
}

struct hmempool hmempool_new_unmanaged(size_t max, size_t size, size_t num_handles)
{
	assert(num_handles >= max);
	struct hmempool tmp = {
		.pool = mempool_new(max, size),
		.indirection_len = num_handles,
		.allocated = true,
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

void hmempool_handles_resize(struct hmempool *hm, size_t num)
{
	if (num > hm->indirection_len) {
		void *new_htoi = crealloc(hm->htoi, num*sizeof(uint32_t), hm->indirection_len*sizeof(uint32_t));
		void *new_itoh = crealloc(hm->itoh, num*sizeof(uint32_t), hm->indirection_len*sizeof(uint32_t));
		hm->htoi = new_htoi ? new_htoi : hm->htoi;
		hm->itoh = new_itoh ? new_itoh : hm->itoh;

		if (hm->managed) {
			size_t oldmax = hm->free_handles.max;
			mempool_resize(&hm->free_handles, num);
			//Generate some new handles (handles start at 1)
			mempool_fill_uint32_t_descending(&hm->free_handles, oldmax+1, num);
		}

		hm->indirection_len = num;

		if (!new_htoi || !new_itoh)
			printf("%s: We're probably running out of memory here.\n", __FUNCTION__);
	}
}

void hmempool_resize_stretch(struct hmempool *hm, size_t num, size_t size)
{
	assert(num >= hm->indirection_len); //Making the pool smaller is such a headache, let's not bother for now.
	mempool_resize_stretch(&hm->pool, num, size);
	hmempool_handles_resize(hm, num);
}

void hmempool_delete(struct hmempool *hm)
{
	if (hm->allocated) {
		mempool_delete(&hm->pool);
		if (hm->managed)
			mempool_delete(&hm->free_handles);
		free(hm->htoi);
		free(hm->itoh);
		hm->allocated = false;
	}
}

uint32_t hmempool_claim(struct hmempool *hm, uint32_t h, void *item)
{
	uint32_t i = mempool_add(&hm->pool, item);
	hm->htoi[h-1] = i+1;
	hm->itoh[i] = h;
	return h;
}

uint32_t hmempool_claim_raw(struct hmempool *hm, uint32_t h)
{
	uint32_t i = mempool_add_raw(&hm->pool);
	hm->htoi[h-1] = i+1;
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
	int i = hm->htoi[h-1]-1;
	//Invalidate handle
	uint32_t i2 = mempool_remove(&hm->pool, i);
	uint32_t h2 = hm->itoh[i2];
	//The order of these operations is very important to handle edge cases correctly.
	hm->itoh[i] = h2;
	hm->itoh[i2] = 0;
	hm->htoi[h2-1] = i+1;
	hm->htoi[h-1] = 0;
}

void hmempool_remove(struct hmempool *hm, uint32_t h)
{
	//Add handle to free handle pool
	mempool_add(&hm->free_handles, &h);
	hmempool_unclaim(hm, h);
}

void * hmempool_get(struct hmempool *hm, uint32_t h)
{
	uint32_t i = hm->htoi[h-1];
	return h && i ? mempool_get(&hm->pool, i-1) : NULL;
}
