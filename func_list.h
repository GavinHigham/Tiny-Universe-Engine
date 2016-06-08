#ifndef FUNC_LIST_H
#define FUNC_LIST_H

enum {
	CALL_EVERY_TIME = -1
};

struct counted_func {
	void (*func)(void);
	int count;
};
struct func_list {
	struct counted_func *funcs;
	int num_funcs;
	int max_funcs;
};

struct func_list func_list_new(struct counted_func *func_storage, int len);
int func_list_add(struct func_list *list, int repeat, void (*func)(void));
int func_list_remove(struct func_list *list, void (*func)(void));
void func_list_call(struct func_list *list);

#endif