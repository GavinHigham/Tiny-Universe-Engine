#include "mempool.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MEMPOOL_GET(m, i, size) (m->pool + size * i)

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

void * mempool_resize(struct mempool *m, int num)
{
	return mempool_resize_stretch(m, num, m->size);
}

void * mempool_stretch(struct mempool *m, size_t size)
{
	return mempool_resize_stretch(m, m->num, size);
}

void * mempool_resize_stretch(struct mempool *m, size_t num, size_t size)
{
	void *new_pool = realloc(m->pool, num * size);
	if (new_pool) {
		// memset(new_pool, 0, num * size);
		m->pool = new_pool;
		if (size > m->size)
			for (int i = m->num; i-- > 0;) {
				memmove(MEMPOOL_GET(m, i, size), MEMPOOL_GET(m, i, m->size), m->size);
				memset(MEMPOOL_GET(m, i, size) + m->size, 0, size - m->size);
			}
		m->max = num;
		m->size = size;
	}
	return new_pool;
}

void mempool_delete(struct mempool *m)
{
	if (m->from_malloc)
		free(m->pool);
}

int mempool_add(struct mempool *m, void *item)
{
	assert(m->num < m->max); //Making sure the pool is not full is the responsibility of the caller.
	memcpy(mempool_get(m, m->num), item, m->size);
	return m->num++;
}

int mempool_add_raw(struct mempool *m)
{
	return m->num++;
}

int mempool_remove(struct mempool *m, int i)
{
	assert(m->num > 0);
	assert(i < m->max);
	assert(i >= 0);

	m->num--;
	memcpy(m->pool + m->size * i, mempool_get(m, m->num), m->size);
	return m->num;
}

void * mempool_get(struct mempool *m, int i)
{
	return MEMPOOL_GET(m, i, m->size);
}

void mempool_pop(struct mempool *m, void *item)
{
	memcpy(item, mempool_get(m, m->num-1), m->size);
	m->num--;
}

void mempool_fill_uint32_t_descending(struct mempool *m, uint32_t lowest, uint32_t highest)
{
	for (int i = highest; i >= lowest; i--) {
		mempool_add(m, &i);
	}
}
