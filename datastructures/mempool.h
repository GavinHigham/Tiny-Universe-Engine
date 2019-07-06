#ifndef MEMPOOL_H
#define MEMPOOL_H
#include <stdbool.h>
#include <stddef.h>

/*
Note: Check the comment in "hmempool.h", see that it is in sync with this one.
A mempool is a basic "pool" of items.
Items are added to the end of the pool.
Items can be removed from anywhere in the pool; they will be overwritten by the last item in the pool,
and the pool item count will be decremented.

Items are *not* guaranteed to have a stable address - in particular, the most-recently-added item is
likely to be moved to overwrite a deleted item.

There are several conditions that should be handled by the caller:
Any pointers to the deleted item should be set to NULL.
If the deleted item was not the last item in the pool, any pointers to the moved item should point to its new location.
*/

struct mempool {
	bool from_malloc;
	size_t num, max, size;
	unsigned char *pool;
};

//Create a new mempool that holds "num" items of "size" size (in bytes). 
struct mempool mempool_new(size_t num, size_t size);
//Delete a mempool.
void mempool_delete(struct mempool *m);
//Copy an item into the mempool, return an index to the location at which it was inserted.
int mempool_add(struct mempool *m, void *item);
//Claim a mempool slot, return an index to its location.
int mempool_add_raw(struct mempool *m);
//Resize the mempool so it can hold at least num items.
void * mempool_resize(struct mempool *m, int num);
//Enlarge each item in the pool to "size" size in bytes. Extra space after each item is uninitialized.
void * mempool_stretch(struct mempool *m, size_t size);
//Combined mempool_resize and mempool_stretch, with only one realloc.
void * mempool_resize_stretch(struct mempool *m, size_t num, size_t size);
//Remove an item from the mempool. Return the old index of the pool item that overwrites it.
//If pointers to the moved item are updated before pointers to the old item are set to NULL,
//it handles the case where the item was the last one in the pool (the item "overwrites itself" before deletion).
int mempool_remove(struct mempool *m, int i);
//Returns a pointer to the i-th item in the mempool.
void * mempool_get(struct mempool *m, int i);
//Copies out the data of the last item in the pool, then deletes it from the pool.
void mempool_pop(struct mempool *m, void *item);

#endif