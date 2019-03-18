#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <string.h>

#include "base64.h"
#include "error.h"
#include "http.h"
#include "prompt.h"
#include "url.h"
#include "json.h"
#include "jws.h"

acmee account_jwk_rsa(EVP_PKEY *pkey, struct json_token *jwk);
acmee account_find_rsa(EVP_PKEY *pkey);

acmee account_new(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *jwk, *protected, *payload, *key;

	/* create the protected header */

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "jwk");
	jwk = json_add_object(key);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, CA_NEW_ACCOUNT);

	switch (EVP_PKEY_base_id(pkey))
	{
		case EVP_PKEY_RSA:
			result = account_jwk_rsa(pkey, jwk);
			break;
		default:
			result = ACMEE_KEY_UNSUPPORTED_TYPE;
			break;
	}

	/* create the payload */

	payload = json_empty();

	printf("The terms of service for this certificate authority\n");
	printf("can be found on\n");
	printf("%s\n", CA_TERMS_OF_SERVICE);

	key = json_add_key(payload, "termsOfServiceAgreed");
	json_add_boolean(key, prompt_yesno("Do you agree to the terms of service?", 1));

	printf("By supplying an email address the certificate authority\n");
	printf("can contact you about issues related to your certificates.\n");
	printf("Write one email per row and end with an empty row when you are done.\n");
	printf("Just press 'Enter' if you don't want to supply an email.\n");

	key = json_add_key(payload, "contact");
	struct json_token *contact = json_add_array(key);

	char *email, *mailto;

	while (*(email = prompt_text(NULL)) != 0)
	{
		asprintf(&mailto, "mailto:%s", email);
		json_add_string(contact, mailto);
	}

	jws = generate_jws(protected, payload, pkey);

	request_new(CA_NEW_ACCOUNT);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();

	if (get_status_code() == 201)
	{
		CA_ACCOUNT = get_header("Location");
	}
	else
	{
		result = ACMEE_ACCOUNT_NOTCREATED;
	}

	return result;
}

acmee account_find(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	switch (EVP_PKEY_base_id(pkey))
	{
		case EVP_PKEY_RSA:
			result = account_find_rsa(pkey);
			break;

		default:
			result = ACMEE_KEY_UNSUPPORTED_TYPE;
	}
	return result;
}

acmee account_jwk_rsa(EVP_PKEY *pkey, struct json_token *jwk)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *key;

	RSA *rsa = EVP_PKEY_get0_RSA(pkey);	

	const BIGNUM *n = RSA_get0_n(rsa);
	const BIGNUM *e = RSA_get0_e(rsa);

	unsigned char *buf;

	key = json_add_key(jwk, "kty");
	json_add_string(key, "RSA");

	key = json_add_key(jwk, "n");
	buf = malloc(BN_num_bytes(n) * sizeof(unsigned char));
	BN_bn2bin(n, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(n), 0));
	free(buf);

	key = json_add_key(jwk, "e");
	buf = malloc(BN_num_bytes(e) * sizeof(unsigned char));
	BN_bn2bin(e, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(e), 0));
	free(buf);

	return result;
}

acmee account_find_rsa(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *jwk, *protected, *payload, *key;

	/* create the protected header */

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "jwk");
	jwk = json_add_object(key);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, CA_NEW_ACCOUNT);

	/* create the jwk for a RSA key */

	RSA *rsa = EVP_PKEY_get0_RSA(pkey);	

	const BIGNUM *n = RSA_get0_n(rsa);
	const BIGNUM *e = RSA_get0_e(rsa);

	unsigned char *buf;

	key = json_add_key(jwk, "kty");
	json_add_string(key, "RSA");

	key = json_add_key(jwk, "n");
	buf = malloc(BN_num_bytes(n) * sizeof(unsigned char));
	BN_bn2bin(n, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(n), 0));
	free(buf);

	key = json_add_key(jwk, "e");
	buf = malloc(BN_num_bytes(e) * sizeof(unsigned char));
	BN_bn2bin(e, buf);
	json_add_string(key, base64url_encode(buf, BN_num_bytes(e), 0));
	free(buf);

	/* create the payload */

	payload = json_empty();
	key = json_add_key(payload, "onlyReturnExisting");
	json_add_boolean(key, 1);

	jws = generate_jws(protected, payload, pkey);

	request_new(CA_NEW_ACCOUNT);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();

	if (get_status_code() == 200)
	{
		printf("Account exists\n");
		CA_ACCOUNT = get_header("Location");
	}
	else if (get_status_code() == 400)
	{
		struct json_token *error = json_tokenize(get_body());
		if (strcmp(json_get_string(error, "type"), "urn:ietf:params:acme:error:accountDoesNotExist") == 0)
		{
			result = ACMEE_ACCOUNT_NOTEXIST;
		}
		else
		{
			result = ACMEE_OTHER;
		}
	}
	else
	{
		result = ACMEE_OTHER;
	}

	request_cleanup();
	return result;
}
