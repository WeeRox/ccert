#include <stdlib.h>

#include "llist.h"

llist *llist_last(llist *list)
{
	if (list == NULL)
		return NULL;

	llist *item = list;

	while (item->next != NULL)
		item = item->next;

	return item;
}

llist *llist_append(llist *list, void *data)
{
	llist *new, *last;

	new = malloc(sizeof(llist));

	if (new == NULL)
		return NULL;

	new->data = data;
	new->next = NULL;

	if (list == NULL)
		return new;

	last = llist_last(list);

	last->next = new;
	return list;
}
