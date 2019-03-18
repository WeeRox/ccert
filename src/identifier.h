#ifndef __IDENTIFIER_H
#define __IDENTIFIER_H

enum acme_identifier_type
{
	DNS
};

struct acme_identifier
{
	enum acme_identifier_type type;
	char *value;
};

#endif /* __IDENTIFIER_H */
