#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <stdbool.h>
#include <inttypes.h>

typedef struct hashtable hashtable;
typedef struct hashtable_listnode hashtable_listnode;
struct hashtable_listnode {
	const char *key;
	union {
		void *data;
		uint32_t handle;
	};
	struct hashtable_listnode *next;
};
struct hashtable_iterator;

//Generate a hash of str, which should be null-terminated.
unsigned int strhash(const char *str);
//Returns a new opaque hashtable object of size "len". Should be freed by the caller with hashtable_free.
hashtable * hashtable_new(int len);
//Find a string key in the hashtable and return a pointer to the containing node.
//If insert is true and the key is not found, key will be cloned and inserted,
//and a pointer to the new containing node will be returned.
//If the key is found, returns a pointer to the containing node.
//If the key is not found, and insert is false, returns NULL.
hashtable_listnode * hashtable_find(hashtable *t, const char *key, bool insert);
//Call operation(node, userdata) on each node in the hashtable.
void hashtable_apply(hashtable *t, void (*operation)(hashtable_listnode *, void *), void *userdata);
//Frees all data associated with a hashtable, including all cloned strings, and inner linked-lists.
//If cleanup is not NULL, it will be called on every data member stored in the hashtable:
//cleanup(node.data, userdata)
void hashtable_free(hashtable *t, void (*cleanup)(void *, void *), void *userdata);
//Prints all the sub-lists of t in this format:
//prefix[index]->str1->str2->NULL
//Example:
//mytable[5]->spock->shatner->NULL
//mytable[31]->lizard->enterprise->NULL
void hashtable_print(hashtable *t, const char *prefix);
//Returns an iterator structure to t. The returned iterator can be used with hashtable_next.
struct hashtable_iterator hashtable_itr(hashtable *t);
//Returns a pointer to the next node in the hashtable itr was created from, or NULL if none are left.
//Behaviour is undefined if the hashtable is modified before itr has visited the entire hashtable.
hashtable_listnode * hashtable_next(struct hashtable_iterator *itr);
//Traverse the hashtable and return the number of elements.
int hashtable_count(hashtable *);
//Dumps a pointer to each string in t into buf.
//buf should be large enough to hold every node in t.
void hashtable_dump_keys(hashtable *t, const char *buf[]);

#endif