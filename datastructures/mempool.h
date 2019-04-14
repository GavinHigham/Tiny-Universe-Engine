#ifndef MEMPOOL_H
#define MEMPOOL_H
#include <stdbool.h>
#include <stddef.h>

/*
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
	void *pool;
};

//Create a new mempool that holds "num" items of "size" size (in bytes). 
struct mempool mempool_new(size_t num, size_t size);
//Delete a mempool.
void mempool_delete(struct mempool *m);
//Copy an item into the mempool, return a pointer to the location at which it was inserted.
void * mempool_add(struct mempool *m, void *item);
//Remove an item from the mempool. Return the old index of the pool item that overwrites it.
//If pointers to the moved item are updated before pointers to the old item are set to NULL,
//it handles the case where the item was the last one in the pool (the item "overwrites itself" before deletion).
int mempool_remove(struct mempool *m, int i);
//Returns a pointer to the i-th item in the mempool.
void * mempool_get(struct mempool *m, int i);

#endif