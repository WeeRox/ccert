#ifndef __LLIST_H
#define __LLIST_H

typedef struct llist
{
	void *data;
	struct llist *next;
} llist;

llist *llist_last(llist *list);
llist *llist_append(llist *list, void *data);

#endif /* __LLIST_H */
