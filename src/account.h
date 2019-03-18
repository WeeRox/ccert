#ifndef __ACCOUNT_H
#define __ACCOUNT_H

#include "error.h"

acmee account_find(EVP_PKEY *pkey);
acmee account_new(EVP_PKEY *pkey);

#endif /* __ACCOUNT_H */
