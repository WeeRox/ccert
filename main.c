#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#include <curl/curl.h>

#include <openssl/pem.h>
#include <openssl/evp.h>

#include "account.h"
#include "authorization.h"
#include "challenge.h"
#include "order.h"
#include "identifier.h"

#include "json.h"
#include "http.h"
#include "url.h"
#include "base64.h"
#include "prompt.h"

char *CA_DIRECTORY = "https://acme-v02.api.letsencrypt.org/directory";
char *CA_NEW_NONCE;
char *CA_NEW_ACCOUNT;
char *CA_NEW_ORDER;
char *CA_NEW_AUTHZ;
char *CA_REVOKE_CERT;
char *CA_KEY_CHANGE;
char *CA_TERMS_OF_SERVICE;

int main(int argc, char **argv)
{
	request_init();

	acmee result;

	char *path = prompt_text("Path to private key: ");
	FILE *private_key = fopen(path, "r");

	if (private_key == NULL)
	{
		printf("Couldn't open file '%s'\n", path);
		return 1;
	}

	EVP_PKEY *pkey = NULL;
	PEM_read_PrivateKey(private_key, &pkey, NULL, NULL);

	switch (EVP_PKEY_base_id(pkey))
	{
		case EVP_PKEY_RSA:
			break;

		default:
			printf("This type of private key with id %i isn't supported\n", EVP_PKEY_base_id(pkey));
	}

	request_new(CA_DIRECTORY);
	curl_easy_setopt(get_curl(), CURLOPT_HTTPHEADER, NULL);
	request_perform();

	struct json_token *json = json_tokenize(get_body());
	struct json_token *meta = json_get(json, "meta");

	CA_NEW_NONCE = json_get_string(json, "newNonce");
	CA_NEW_ACCOUNT = json_get_string(json, "newAccount");
	CA_NEW_ORDER = json_get_string(json, "newOrder");
	CA_NEW_AUTHZ = json_get_string(json, "newAuthz");
	CA_REVOKE_CERT = json_get_string(json, "revokeCert");
	CA_KEY_CHANGE = json_get_string(json, "keyChange");
	CA_TERMS_OF_SERVICE = json_get_string(meta, "termsOfService");

	request_cleanup();

	request_new(CA_NEW_NONCE);
	curl_easy_setopt(get_curl(), CURLOPT_HTTPHEADER, NULL);
	curl_easy_setopt(get_curl(), CURLOPT_NOBODY, 1);
	request_perform();
	request_cleanup();

	printf("Checking for existing account...\n");

	result = account_find(pkey);

	if (result == ACMEE_ACCOUNT_NOTEXIST)
	{
		printf("Creating account...\n");
		result = account_new(pkey);
		if (result == ACMEE_SUCCESS)
		{
			printf("Account created\n");
		}
		else if (result == ACMEE_ACCOUNT_NOTCREATED)
		{
			printf("Account couldn't be created!\n");
			printf("%s\n", get_body());
			return 1;
		}
	}

	printf("Creating order...\n");

	result = order_new(pkey);

	printf("Order created\n");
	printf("Processing identifier authorizations...\n");

	llist *auth = order.authorizations;

	while (auth != NULL)
	{
		authorization_get(pkey, auth->data);

		printf("Authorizing %s...\n", authorization.identifier.value);

		llist *challenges = authorization.challenges;

		struct acme_challenge *challenge;

		char **choices = malloc(0);
		int *retval = malloc(0);
		int n = 0;

		while (challenges != NULL)
		{
			challenge = (struct acme_challenge *) challenges->data;

			switch (challenge->type)
			{
				case HTTP_01:
					choices = realloc(choices, sizeof(char *) * (n + 1));
					*(choices + n) = malloc(sizeof(char) * 8);
					strcpy(*(choices + n), "HTTP-01");
					retval = realloc(retval, sizeof(int) * (n + 1));
					*(retval + n) = HTTP_01;
					n++;
					break;
				case DNS_01:
					choices = realloc(choices, sizeof(char *) * (n + 1));
					*(choices + n) = malloc(sizeof(char) * 7);
					strcpy(*(choices + n), "DNS-01");
					retval = realloc(retval, sizeof(int) * (n + 1));
					*(retval + n) = DNS_01;
					n++;
					break;
			}

			challenges = challenges->next;
		}

		enum acme_challenge_type chosen_type = prompt_select("Select an authentication method:", choices, retval, n);

		switch (chosen_type)
		{
			case HTTP_01:
				result = challenge_by_type(authorization.challenges, HTTP_01, &challenge);
				result = challenge_http_01(pkey, challenge);
				break;

			case DNS_01:
				result = challenge_by_type(authorization.challenges, DNS_01, &challenge);
				result = challenge_dns_01(pkey, challenge);
				break;
		}

		challenge_validate(pkey, challenge);

		while (authorization.status == PENDING)
		{
			sleep(1);
			authorization_get(pkey, auth->data);
		}

		if (authorization.status == INVALID)
		{
			printf("The domain couldn't be authorized.\n");
			printf("Aborting...\n");
		}

		auth = auth->next;
	}

	/* might want to use the result from this */
	/* to see that the order is 'ready' */
	order_get(pkey);

	order_finalize(pkey);

	order_certificate(pkey);

	return 0;
}
