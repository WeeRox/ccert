#ifndef __AUTHORIZATION_H
#define __AUTHORIZATION_H

#include "error.h"
#include "identifier.h"
#include "llist.h"
#include "status.h"

struct acme_authorization
{
	enum acme_status status;
	char *expires;
	struct acme_identifier identifier;
	llist *challenges;
	int wildcard;
};

struct acme_authorization authorization;

acmee authorization_get(EVP_PKEY *pkey, char *url);

#endif /* __AUTHORIZATION_H */
