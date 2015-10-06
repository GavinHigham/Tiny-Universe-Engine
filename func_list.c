#include <stdio.h>
#include "func_list.h"

struct func_list func_list_new(struct counted_func *func_storage, int len)
{
	return (struct func_list){func_storage, 0, len};
}

int func_list_add(struct func_list *list, int repeat, void (*func)(void))
{
	if (list->num_funcs < list->max_funcs) {
			list->funcs[list->num_funcs] = (struct counted_func){func, repeat};
			list->num_funcs++;
	} else {
		return -1; //no room for more funcs
	}
	return 0;
}

int func_list_remove(struct func_list *list, void (*func)(void))
{
	for (int i = 0; i < list->num_funcs; i++) {
		if (list->funcs[i].func == func) { //find the first func that matches the one passed in
			list->funcs[i] = list->funcs[list->num_funcs]; //delete it
			list->num_funcs--;
			return 0; //only delete one func
		}
	}
	return -1; //could not find func
}

void func_list_call(struct func_list *list)
{
	for (int i = 0; i < list->num_funcs; i++) {
		list->funcs[i].func(); //call each func
		if (list->funcs[i].count > 0) //count of CALL_EVERY_TIME, aka -1, is never decremented.
			list->funcs[i].count--;
		if (list->funcs[i].count == 0) { //a func has run its alloted number of times
			list->funcs[i] = list->funcs[list->num_funcs-1]; //delete the func
			list->num_funcs--;
		}
	}
}
