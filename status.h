#ifndef __STATUS_H
#define __STATUS_H

enum acme_status
{
	PENDING,
	PROCESSING,
	VALID,
	INVALID,
	REVOKED,
	DEACTIVATED,
	EXPIRED,
	READY
};

#endif /* __STATUS_H */
