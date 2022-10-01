#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hashtable.h"

#ifndef LENGTH
	#define LENGTH(x) (sizeof(x)/sizeof(x[0]))
#endif

struct hashtable {
	unsigned int len;
	unsigned int max;
	hashtable_listnode **table;
	void (*cleanup)(void *);
};

//Hashtable declarations
struct hashtable_iterator {
	hashtable *t;
	hashtable_listnode *curr;
	int i;
};

enum {
	HASHMULT = 31
};

//Create a new node, storing a pointer to key.
hashtable_listnode * listnode_new(const char *key)
{
	hashtable_listnode *n = malloc(sizeof(hashtable_listnode));
	n->key = key;
	n->data = NULL;
	n->handle = 0;
	n->next = NULL;
	return n;
}

//Free n and n's string key.
void listnode_free(hashtable_listnode *n)
{
	free(n->key);
	free(n);
}

//Free every node in the list, and all key strings.
void list_free(hashtable_listnode *n)
{
	if (n != NULL) {
		list_free(n->next);
		listnode_free(n);
	}
}

//Set n to point at list, then return it.
hashtable_listnode * list_prepend(hashtable_listnode *list, hashtable_listnode *n)
{
	n->next = list;
	return n;
}

unsigned int strhash(const char *str)
{
	unsigned int h = 0;
	for (unsigned char *p = (unsigned char *)str; *p != '\0'; p++)
		h = HASHMULT * h + *p;
	return h;
}

hashtable * hashtable_new(int len)
{
	hashtable *t = malloc(sizeof(hashtable));
	if (t != NULL) {
		t->len = 0;
		t->max = len;
		t->table = calloc(len, sizeof(hashtable_listnode *));
		if (t->table == NULL) {
			free(t);
			return NULL;
		}
	}
	return t;
}

static void hashtable_data_apply(hashtable *t, void (*operation)(void *, void *), void *userdata)
{
	struct hashtable_iterator itr = hashtable_itr(t);
	for (hashtable_listnode *n = hashtable_next(&itr); n != NULL; n = hashtable_next(&itr))
		operation(n->data, userdata);
}

void hashtable_apply(hashtable *t, void (*operation)(hashtable_listnode *, void *), void *userdata)
{
	struct hashtable_iterator itr = hashtable_itr(t);
	for (hashtable_listnode *n = hashtable_next(&itr); n != NULL; n = hashtable_next(&itr))
		operation(n, userdata);
}

void hashtable_free(hashtable *t, void (*cleanup)(void *, void *), void *userdata)
{
	if (cleanup != NULL) //Clean up any stored data in the hashtable.
		hashtable_data_apply(t, cleanup, userdata);
	for (int i = 0; i < t->max; i++)
	 	list_free(t->table[i]);
	free(t->table);
	free(t);
}

hashtable_listnode * hashtable_find(hashtable *t, const char *key, bool insert)
{
	unsigned int i = strhash(key) % t->max;
	hashtable_listnode *n;

	for (n = t->table[i]; n != NULL; n = n->next)
		if (strcmp(key, n->key) == 0)
			break;

	if (insert && n == NULL) {
		n = t->table[i] = list_prepend(t->table[i], listnode_new(strdup(key)));
		t->len++;
	}
	return n;
}

void hashtable_print(hashtable *t, const char *prefix)
{
	for (int i = 0; i < t->max; i++) {
		if (t->table[i] != NULL) {
			printf("%s[%d]->", prefix, i);
			for (hashtable_listnode *n = t->table[i]; n != NULL; n = n->next)
				printf("%s->", n->key);
			printf("NULL\n");
		}
	}
}

struct hashtable_iterator hashtable_itr(hashtable *t)
{
	return (struct hashtable_iterator) {
		.t = t,
		.curr = NULL,
		.i = 0
	};
}

hashtable_listnode * hashtable_next(struct hashtable_iterator *itr)
{
	if (itr->curr != NULL) //If we have a current node, grab the next one in the list.
		itr->curr = itr->curr->next;
	while (itr->curr == NULL && itr->i < itr->t->max) { //If curr is still NULL, find the next list.
		itr->curr = itr->t->table[itr->i];
		itr->i++;
	}
	return itr->curr; //Returns NULL once itr has reached the end of the hashtable.
}

int hashtable_count(hashtable *t)
{
	return t->len;
}

void hashtable_dump_keys(hashtable *t, const char *buf[])
{
	int i = 0;
	struct hashtable_iterator itr = hashtable_itr(t);
	for (hashtable_listnode *n = hashtable_next(&itr); n != NULL; n = hashtable_next(&itr))
		buf[i++] = n->key;
}