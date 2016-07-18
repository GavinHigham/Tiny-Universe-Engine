#ifndef QUEUE_H
#define QUEUE_H

typedef struct counted_buf
{
	void **_arr; //Internal array for our circular buffer.
	int len;    //Current size of the array.
	int max;   //Size of the internal array, including the empty space.
} HEAP, STACK;

//Returns a new HEAP of length "len."
HEAP new_heap(void *arr[], int len);

//Shows us what the next item to be dequeued is without actually dequeueing it.
void * peek(HEAP *ph);

//Adds a void * v into the heap ph, percolating it up to the correct level, based on the provided comparison function.
//Returns 0 if v was successfully added to the heap.
//Returns -1 if the heap was full and could not be enlarged. (v was not added in this case)
int heap_add(HEAP *ph, void *v, int (*compare)(const void *, const void *));

//Removes a void * from the heap ph, maintaining heapiness.
//Returns a void *, or NULL if there are no more items in the queue.
void * heap_rem(HEAP *ph, int (*compare)(const void *, const void *));

//Pushes an element onto the stack.
//Returns 0 if v was successfully added to the stack.
//Returns -1 if the stack was full and could not be enlarged. (v was not added in this case)
int stack_push(STACK *ps, void *v);

//Pops an item off the stack.
//Returns NULL if there are no more items on the stack.
void * stack_pop(STACK *ps);

//Returns the number of free slots in the stack.
int stack_available(STACK *ps);

#endif