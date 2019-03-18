#include <openssl/evp.h>
#include <string.h>

#include "json.h"
#include "base64.h"

struct json_token *generate_jws(struct json_token *protected, struct json_token *payload, EVP_PKEY *pkey)
{
	struct json_token *jws, *key;

	char *msg;
	unsigned char *sig;
	size_t siglen;
	char *buffer;

	buffer = json_print(protected, 0, 0);

	char *protected64 = base64url_encode(buffer, strlen(buffer), 0);

	char *payload64;
	if (payload != NULL)
	{
		buffer = json_print(payload, 0, 0);
		payload64 = base64url_encode(buffer, strlen(buffer), 0);
	}
	else
	{
		/* POST-as-GET */
		payload64 = base64url_encode(NULL, 0, 0);
	}

	asprintf(&msg, "%s.%s", protected64, payload64);

	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey);
	EVP_DigestSignUpdate(mdctx, msg, strlen(msg));
	EVP_DigestSignFinal(mdctx, NULL, &siglen);

	sig = malloc(siglen * sizeof(unsigned char));

	EVP_DigestSignFinal(mdctx, sig, &siglen);

	EVP_MD_CTX_free(mdctx);

	jws = json_empty();
	key = json_add_key(jws, "protected");
	json_add_string(key, protected64);
	key = json_add_key(jws, "payload");
	json_add_string(key, payload64);
	key = json_add_key(jws, "signature");
	json_add_string(key, base64url_encode(sig, siglen, 0));

	return jws;
}
