#ifndef __CHALLENGE_H
#define __CHALLENGE_H

#include "error.h"
#include "llist.h"

struct acme_http_01
{
	char *token;
};

struct acme_dns_01
{
	char *token;
};

enum acme_challenge_type
{
	UNKNOWN = 0,
	HTTP_01,
	DNS_01
};

struct acme_challenge
{
	union {
		struct acme_http_01 http_01;
		struct acme_dns_01 dns_01;
	};
	char *status;
	enum acme_challenge_type type;
	char *url;
	char *validated;
	/* TODO: implement error */
};

acmee challenge_by_type(llist *challenges, enum acme_challenge_type type, struct acme_challenge **challenge);

acmee challenge_validate(EVP_PKEY *pkey, struct acme_challenge *challenge);

acmee challenge_http_01(EVP_PKEY *pkey, struct acme_challenge *challenge);
acmee challenge_dns_01(EVP_PKEY *pkey, struct acme_challenge *challenge);

#endif /* __CHALLENGE_H */
