#ifndef TABLE_H
#define TABLE_H
#include <stdbool.h>

#ifndef LENGTH
	#define LENGTH(x) (sizeof(x)/sizeof(x[0]))
#endif

//Linked-list declarations.
struct list_node;
typedef struct list_node {
	char *key;
	void *data;
	struct list_node *next;
} LISTNODE;

typedef struct hashtable {
	unsigned int len;
	unsigned int max;
	LISTNODE **table;
	void (*cleanup)(void *);
} TABLE;

//Create a new node, storing a pointer to key.
LISTNODE * listnode_new(char *key);
//Free n, and free the string node holds a pointer to.
void listnode_free(LISTNODE *n);
//Free every node in the list, and all referenced strings.
void list_free(LISTNODE *n);
//Set n to point at list, then return it.
LISTNODE * list_prepend(LISTNODE *list, LISTNODE *n);
//Clone a string into new memory. Should be freed by the caller.
char * strclone(char *str);

//Hashtable declarations
struct tbl_itr {
	TABLE *t;
	LISTNODE *curr;
	int i;
};

enum {
	HASHMULT = 31
};

//Generate a hash of str, which should be null-terminated.
unsigned int strhash(char *str);
//Returns a new opaque TABLE object of size "len". Should be freed by the caller with table_free.
TABLE * table_new(int len);
//Find a string key in the hashtable and return a pointer to the containing node.
//If insert is true and the key is not found, key will be cloned and inserted,
//and a pointer to the new containing node will be returned.
//If the key is found, returns a pointer to the containing node.
//If the key is not found, and insert is false, returns NULL.
LISTNODE * table_find(TABLE *t, char *key, bool insert);
//Call operation() on each node in the table.
void table_apply(TABLE *t, void (*operation)(LISTNODE *));
//Frees all data associated with a table, including all cloned strings, and inner linked-lists.
//If cleanup is not NULL, it will be called on every data member stored in the table.
void table_free(TABLE *t, void (*cleanup)(void *));
//Prints all the sub-lists of t in this format:
//prefix[index]->str1->str2->NULL
//Example:
//mytable[5]->spock->shatner->NULL
//mytable[31]->lizard->enterprise->NULL
void table_print(TABLE *t, const char *prefix);
//Returns an iterator structure to t. The returned iterator can be used with table_next.
struct tbl_itr table_itr(TABLE *t);
//Returns a pointer to the next node in the table itr was created from, or NULL if none are left.
//Behaviour is undefined if the table is modified before itr has visited the entire table.
LISTNODE * table_next(struct tbl_itr *itr);
//Traverse the table and returns the number of elements.
int table_count(TABLE *);
//Dumps a pointer to each string in t into buf.
//buf should be large enough to hold every node in t.
void table_dump_keys(TABLE *t, char *buf[]);

#endif