#ifndef HMEMPOOL_H
#define HMEMPOOL_H
#include "mempool.h"
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

/*
Note: Check the comment in "mempool.h", see that it is in sync with this one.
A mempool is a basic "pool" of items.
Items are added to the end of the pool.
Items can be removed from anywhere in the pool; they will be overwritten by the last item in the pool,
and the pool item count will be decremented.

Items are *not* guaranteed to have a stable address - in particular, the most-recently-added item is
likely to be moved to overwrite a deleted item.

There are several conditions that should be handled by the caller:
Any pointers to the deleted item should be set to NULL.
If the deleted item was not the last item in the pool, any pointers to the moved item should point to its new location.

This is a wrapper around the mempool implementation to provide stable handles to elements of a mempool.
Mempool elements will still be moved around when items are removed or added (triggering a resize),
but the new location will always be accessible using the handle.
*/

struct hmempool {
	struct mempool pool, free_handles;
	size_t indirection_len; //Length of the indirection tables. They can't be made smaller (easily).
	uint32_t *htoi; //Handle to item table.
	uint32_t *itoh; //item to handle table.
	bool freed, managed;
};

//Create a new hmempool that holds "num" items of "size" size (in bytes). 
struct hmempool hmempool_new(size_t num, size_t size);
//Delete an hmempool.
void hmempool_delete(struct hmempool *hm);
//Claim the mempool slot owned by h, copy an item into the hmempool, return h.
uint32_t hmempool_claim(struct hmempool *hm, uint32_t h, void *item);
//Claim the mempool slot owned by h, return h.
uint32_t hmempool_claim_raw(struct hmempool *hm, uint32_t h);
//Copy an item into the hmempool, return a handle.
uint32_t hmempool_add(struct hmempool *hm, void *item);
//Claim a mempool slot, return a handle.
uint32_t hmempool_add_raw(struct hmempool *hm);
//Resize the mempool so it can hold at least num items.
void hmempool_resize(struct hmempool *hm, size_t num);
//Enlarge each item in the pool to "size" size in bytes. Extra space after each item is uninitialized.
void hmempool_stretch(struct hmempool *m, size_t size);
//Combined hmempool_resize and hmempool_stretch, with only one realloc.
void hmempool_resize_stretch(struct hmempool *hm, size_t num, size_t size);
//Remove the item with handle h from the hmempool.
void hmempool_remove(struct hmempool *hm, uint32_t h);
//Get a pointer to an item with handle h from the hmempool.
void * hmempool_get(struct hmempool *hm, uint32_t h);

#endif