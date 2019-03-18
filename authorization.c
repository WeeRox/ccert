#include <string.h>

#include <openssl/evp.h>

#include "authorization.h"
#include "challenge.h"
#include "error.h"
#include "http.h"
#include "identifier.h"
#include "json.h"
#include "jws.h"
#include "url.h"

acmee authorization_get(EVP_PKEY *pkey, char *url)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *protected, *key;

	/* create the protected header */
	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "kid");
	json_add_string(key, CA_ACCOUNT);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, url);

	jws = generate_jws(protected, NULL, pkey);

	request_new(url);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	struct json_token *json = json_tokenize(get_body());
	authorization.expires = json_get_string(json, "expires");

	char *status = json_get_string(json, "status");

	if (strcmp(status, "invalid") == 0)
		authorization.status = INVALID;
	else if (strcmp(status, "valid") == 0)
		authorization.status = VALID;
	else if (strcmp(status, "pending") == 0)
		authorization.status = PENDING;
	else if (strcmp(status, "revoked") == 0)
		authorization.status = REVOKED;
	else if (strcmp(status, "deactivated") == 0)
		authorization.status = DEACTIVATED;
	else if (strcmp(status, "expired") == 0)
		authorization.status = EXPIRED; 

	struct json_token *identifier = json_get(json, "identifier");
	char *type = json_get_string(identifier, "type");

	if (strcmp(type, "dns") == 0)
		authorization.identifier.type = DNS;

	authorization.identifier.value = json_get_string(identifier, "value");

	struct json_token *challenges = json_get(json, "challenges");
	int i = 0;
	struct acme_challenge *challenge;
	struct json_token *obj;
	char *t;

	while ((obj = json_get_index(challenges, i)) != NULL)
	{
		challenge = malloc(sizeof(struct acme_challenge));
		challenge->status = json_get_string(obj, "status");
		challenge->url = json_get_string(obj, "url");
		challenge->validated = json_get_string(obj, "validated");

		t = json_get_string(obj, "type");

		if (strcmp(t, "http-01") == 0)
		{
			challenge->type = HTTP_01;
			challenge->http_01.token = json_get_string(obj, "token");
		}
		else if (strcmp(t, "dns-01") == 0)
		{
			challenge->type = DNS_01;
			challenge->dns_01.token = json_get_string(obj, "token");
		}
		else
		{
			challenge->type = UNKNOWN;
		}

		authorization.challenges = llist_append(authorization.challenges, challenge);
		i++;
	}

	authorization.wildcard = json_get_boolean(json, "wildcard");

	return result;
}
