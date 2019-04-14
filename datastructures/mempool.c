#include "datastructures/mempool.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct mempool mempool_new(size_t max, size_t size)
{
	return (struct mempool){
		.from_malloc = true,
		.num = 0,
		.max = max,
		.size = size,
		.pool = malloc(max * size)
	};
}

struct mempool mempool_init(size_t num, size_t size, void *storage)
{
	return (struct mempool){
		.from_malloc = false,
		.max = num,
		.size = size,
		.pool = storage
	};
}

void mempool_delete(struct mempool *m)
{
	if (m->from_malloc)
		free(m->pool);
}

void * mempool_add(struct mempool *m, void *item)
{
	assert(m->num < m->max); //Making sure the pool is not full is the responsibility of the caller.
	void *tmp = mempool_get(m, m->num++);
	memcpy(tmp, item, m->size);
	return tmp;
}

int mempool_remove(struct mempool *m, int i)
{
	assert(m->num > 0);
	assert(i < m->max);
	assert(i >= 0);

	memcpy(m->pool + m->size * i, m->pool + m->size * (m->num - 1), m->size);
	m->num--;
	return m->num;
}

void * mempool_get(struct mempool *m, int i)
{
	return m->pool + m->size * i;
}
