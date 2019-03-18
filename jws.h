#ifndef __JWS_H
#define __JWS_H

#include <openssl/evp.h>

struct json_token *generate_jws(struct json_token *protected, struct json_token *payload, EVP_PKEY *pkey);

#endif /* __JWS_H */
