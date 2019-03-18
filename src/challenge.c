#include <string.h>
#include <stdio.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "base64.h"
#include "challenge.h"
#include "error.h"
#include "http.h"
#include "json.h"
#include "jws.h"
#include "prompt.h"
#include "url.h"

acmee challenge_thumbprint_rsa(EVP_PKEY *pkey, char **thumbprint64);

acmee challenge_validate(EVP_PKEY *pkey, struct acme_challenge *challenge)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *protected, *payload, *key;

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "kid");
	json_add_string(key, CA_ACCOUNT);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, challenge->url);

	payload = json_empty();

	jws = generate_jws(protected, payload, pkey);

	request_new(challenge->url);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	return result;
}

acmee challenge_by_type(llist *challenges, enum acme_challenge_type type, struct acme_challenge **challenge)
{
	acmee result = ACMEE_SUCCESS;
	llist *curr = challenges;

	while (curr != NULL)
	{
		if (((struct acme_challenge *) curr->data)->type == type)
		{
			*challenge = (struct acme_challenge *) curr->data;
			return result;
		}
		curr = curr->next;
	}

	result = ACMEE_CHALLENGE_NOTFOUND;
	return result;
}

acmee challenge_http_01(EVP_PKEY *pkey, struct acme_challenge *challenge)
{
	acmee result = ACMEE_SUCCESS;

	char *thumbprint64;
	char *path;
	char *key_authorization;

	switch (EVP_PKEY_base_id(pkey))
	{
		case EVP_PKEY_RSA:
			result = challenge_thumbprint_rsa(pkey, &thumbprint64);
			break;
		default:
			result = ACMEE_KEY_UNSUPPORTED_TYPE;
			return result;
	}

	path = malloc((28 + strlen(challenge->http_01.token) + 1) * sizeof(char));
	strcpy(path, "/.well-known/acme-challenge/");
	strcat(path, challenge->http_01.token);

	key_authorization = malloc((strlen(challenge->http_01.token) + 1 + strlen(thumbprint64) + 1) * sizeof(char));
	strcpy(key_authorization, challenge->http_01.token);
	strcat(key_authorization, ".");
	strcat(key_authorization, thumbprint64);

	printf("Create a resource at '%s' for the domain being authorizaed\n", path);
	printf("with the value '%s' to validate the challenge.\n", key_authorization);
	prompt_enter("Press 'Enter' when done");

	return result;
}

acmee challenge_dns_01(EVP_PKEY *pkey, struct acme_challenge *challenge)
{
	acmee result = ACMEE_SUCCESS;

	char *thumbprint64;

	switch (EVP_PKEY_base_id(pkey))
	{
		case EVP_PKEY_RSA:
			result = challenge_thumbprint_rsa(pkey, &thumbprint64);
			break;
		default:
			result = ACMEE_KEY_UNSUPPORTED_TYPE;
			return result;
	}

	return result;
}

acmee challenge_thumbprint_rsa(EVP_PKEY *pkey, char **thumbprint64)
{
	acmee result = ACMEE_SUCCESS;
	RSA *rsa = EVP_PKEY_get0_RSA(pkey);

	const BIGNUM *n = RSA_get0_n(rsa);
	const BIGNUM *e = RSA_get0_e(rsa);

	unsigned char *buf;

	/* Create the JWK used for the thumbprint */
	/* the members must be in lexicographic order */
	/* as defined in RFC 7638 section 3.3. */
	/* Since json_add_* will place new elements */
	/* first in an object the members must be */
	/* added in reverse. */

	struct json_token *key, *jwk = json_empty();

	key = json_add_key(jwk, "n");
	buf = malloc(BN_num_bytes(n) * sizeof(unsigned char));
	BN_bn2bin(n, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(n), 0));
	free(buf);

	key = json_add_key(jwk, "kty");
	json_add_string(key, "RSA");

	key = json_add_key(jwk, "e");
	buf = malloc(BN_num_bytes(e) * sizeof(unsigned char));
	BN_bn2bin(e, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(e), 0));
	free(buf);

	char *msg = json_print(jwk, 0, 0);
	unsigned char *thumbprint = malloc(32 * sizeof(unsigned char));
	unsigned int l;

	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

	EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
	EVP_DigestUpdate(mdctx, msg, strlen(msg));
	EVP_DigestFinal_ex(mdctx, thumbprint, &l);

	EVP_MD_CTX_free(mdctx);

	*thumbprint64 = base64url_encode(thumbprint, l, 0);
	return result;
}
