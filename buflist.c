#include <stdio.h>
#include "buflist.h"

HEAP new_heap(void *arr[], int len)
{
	return (HEAP) {
		._arr = arr,
		.max = len,
		.len = 0
	};
}

void * peek(HEAP *ph)
{
	if (ph->len < 1)
		return NULL;
	return ph->_arr[0];
}

static int parent_index(int i)
{
	return (i - 1) / 2;
}

//Left Child Index
static int lci(int i)
{
	return i*2 + 1;
}

//Right Child Index
static int rci(int i)
{
	return i*2 + 2;
}

static void swap(void **p1, void **p2) {
	void *tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

static void sift(HEAP *ph, int i, int (*compare)(const void *, const void *))
{
	int pi = parent_index(i);
	if (compare(&ph->_arr[i], &ph->_arr[pi]) > 0) {
		swap(&ph->_arr[i], &ph->_arr[pi]);
		sift(ph, pi, compare);
	}
}

static void sift_down(HEAP *ph, int i, int (*compare)(const void *, const void *))
{
	int l = lci(i);
	int r = rci(i);
	int mi = i;
	
	if (l <= ph->len && compare(ph->_arr[l], ph->_arr[mi]) > 0)
		mi = l;
	if (r <= ph->len && compare(ph->_arr[r], ph->_arr[mi]) > 0)
		mi = r;
	
	if (mi != i) {
		swap(&ph->_arr[i], &ph->_arr[mi]);
		sift_down(ph, mi, compare);
	}
}

int heap_add(HEAP *ph, void *v, int (*compare)(const void *, const void *))
{
	if (ph->len >= ph->max)
		return -1;
	ph->_arr[(ph->len)++] = v;
	sift(ph, (ph->len - 1), compare);
	return 0;
}

void * heap_rem(HEAP *ph, int (*compare)(const void *, const void *))
{
	if (ph->len < 1)
		return NULL;
	void * root = peek(ph);
	ph->_arr[0] = ph->_arr[--(ph->len)];
	sift_down(ph, 0, compare);
	return root;
}

int stack_push(STACK *ps, void *v)
{
	if (ps->len >= ps->max)
		return -1;
	ps->_arr[(ps->len)++] = v;
	return 0;
}

void * stack_pop(STACK *ps)
{
	if (ps->len < 1)
		return NULL;
	return ps->_arr[--(ps->len)];
}

int stack_available(STACK *ps)
{
	return ps->max - ps->len;
}