#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "table.h"

LISTNODE * listnode_new(char *key)
{
	LISTNODE *n = malloc(sizeof(LISTNODE));
	n->key = key;
	n->data = NULL;
	n->next = NULL;
	return n;
}

void listnode_free(LISTNODE *n)
{
	free(n->key);
	free(n);
}

void list_free(LISTNODE *n)
{
	if (n != NULL) {
		list_free(n->next);
		listnode_free(n);
	}
}

LISTNODE * list_prepend(LISTNODE *list, LISTNODE *n)
{
	n->next = list;
	return n;
}

unsigned int strhash(char *str)
{
	unsigned int h = 0;
	for (unsigned char *p = (unsigned char *)str; *p != '\0'; p++)
		h = HASHMULT * h + *p;
	return h;
}

TABLE * table_new(int len)
{
	TABLE *t = malloc(sizeof(TABLE));
	if (t != NULL) {
		t->len = 0;
		t->max = len;
		t->table = calloc(len, sizeof(LISTNODE *));
		if (t->table == NULL) {
			free(t);
			return NULL;
		}
	}
	return t;
}

static void table_data_apply(TABLE *t, void (*operation)(void *))
{
	struct tbl_itr itr = table_itr(t);
	for (LISTNODE *n = table_next(&itr); n != NULL; n = table_next(&itr))
		operation(n->data);
}

void table_apply(TABLE *t, void (*operation)(LISTNODE *))
{
	struct tbl_itr itr = table_itr(t);
	for (LISTNODE *n = table_next(&itr); n != NULL; n = table_next(&itr))
		operation(n);
}

void table_free(TABLE *t, void (*cleanup)(void *))
{
	if (cleanup != NULL) //Clean up any stored data in the table.
		table_data_apply(t, cleanup);
	for (int i = 0; i < t->max; i++)
	 	list_free(t->table[i]);
	free(t->table);
	free(t);
}

LISTNODE * table_find(TABLE *t, char *key, bool insert)
{
	unsigned int i = strhash(key) % t->max;
	LISTNODE *n;

	for (n = t->table[i]; n != NULL; n = n->next)
		if (strcmp(key, n->key) == 0)
			break;

	if (insert && n == NULL) {
		n = t->table[i] = list_prepend(t->table[i], listnode_new(strdup(key)));
		t->len++;
	}
	return n;
}

void table_print(TABLE *t, const char *prefix)
{
	for (int i = 0; i < t->max; i++) {
		if (t->table[i] != NULL) {
			printf("%s[%d]->", prefix, i);
			for (LISTNODE *n = t->table[i]; n != NULL; n = n->next)
				printf("%s->", n->key);
			printf("NULL\n");
		}
	}
}

struct tbl_itr table_itr(TABLE *t)
{
	return (struct tbl_itr) {
		.t = t,
		.curr = NULL,
		.i = 0
	};
}

LISTNODE * table_next(struct tbl_itr *itr)
{
	if (itr->curr != NULL) //If we have a current node, grab the next one in the list.
		itr->curr = itr->curr->next;
	while (itr->curr == NULL && itr->i < itr->t->max) { //If curr is still NULL, find the next list.
		itr->curr = itr->t->table[itr->i];
		itr->i++;
	}
	return itr->curr; //Returns NULL once itr has reached the end of the table.
}

int table_count(TABLE *t)
{
	return t->len;
}

void table_dump_keys(TABLE *t, char *buf[])
{
	int i = 0;
	struct tbl_itr itr = table_itr(t);
	for (LISTNODE *n = table_next(&itr); n != NULL; n = table_next(&itr))
		buf[i++] = n->key;
}
