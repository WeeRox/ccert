#ifndef __ORDER_H
#define __ORDER_H

#include "error.h"
#include "llist.h"

struct acme_order
{
	char *status;
	char *expires;
	llist *identifiers;
	char *not_before;
	char *not_after;
	llist *authorizations;
	char *finalize;
	char *certificate;
};

struct acme_order order;

acmee order_new(EVP_PKEY *pkey);
acmee order_get(EVP_PKEY *pkey);
acmee order_finalize(EVP_PKEY *pkey);
acmee order_certificate(EVP_PKEY *pkey);

#endif /* __ORDER_H */
