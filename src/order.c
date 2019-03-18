#include <string.h>

#include <openssl/asn1t.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "base64.h"
#include "error.h"
#include "http.h"
#include "identifier.h"
#include "json.h"
#include "jws.h"
#include "order.h"
#include "prompt.h"
#include "url.h"

acmee order_parse();

acmee order_new(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *protected, *payload, *key;

	/* create the protected header */

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "kid");
	json_add_string(key, CA_ACCOUNT);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, CA_NEW_ORDER);

	payload = json_empty();
	key = json_add_key(payload, "identifiers");
	struct json_token *identifiers = json_add_array(key);
	struct json_token *obj = json_add_object(identifiers);
	key = json_add_key(obj, "type");
	json_add_string(key, "dns");
	key = json_add_key(obj, "value");
	json_add_string(key, prompt_text("Enter the domain the you want a certificate for: "));

	jws = generate_jws(protected, payload, pkey);

	request_new(CA_NEW_ORDER);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	order_parse();

	CA_ORDER = get_header("Location");

	return result;
}

acmee order_get(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *protected, *key;

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "kid");
	json_add_string(key, CA_ACCOUNT);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, CA_ORDER);

	jws = generate_jws(protected, NULL, pkey);

	request_new(CA_ORDER);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	order_parse();

	return result;
}

acmee order_finalize(EVP_PKEY *pkey)
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
	json_add_string(key, order.finalize);

	/* create key which will be used to sign the CSR */

	EVP_PKEY *csr_key = EVP_PKEY_new();
	RSA *rsa = RSA_new();
	BIGNUM *e = BN_new();

	BN_set_word(e, RSA_F4);

	RSA_generate_key_ex(rsa, 4096, e, NULL);

	EVP_PKEY_assign_RSA(csr_key, rsa);

	/* create CSR */

	X509_REQ *x509_req = X509_REQ_new();
	X509_NAME *x509_name = X509_REQ_get_subject_name(x509_req);

	char *cn = ((struct acme_identifier *) order.identifiers->data)->value;

	X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, cn, strlen(cn), -1, 0);

	X509_REQ_set_pubkey(x509_req, csr_key);
	X509_REQ_sign(x509_req, csr_key, EVP_sha256());

	unsigned char *csr;
	int csr_len;

	csr = NULL;
	csr_len = i2d_X509_REQ(x509_req, &csr);

	/* make finalize request */

	payload = json_empty();
	key = json_add_key(payload, "csr");
	json_add_string(key, base64url_encode(csr, csr_len, 0));

	jws = generate_jws(protected, payload, pkey);

	request_new(order.finalize);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	order_parse();

	/* save the RSA key */

	char *path = prompt_text("Path to store certificate private key: ");
	FILE *private_key = fopen(path, "w+");

	if (private_key == NULL)
	{
		printf("Couldn't open file '%s'\n", path);
		result = ACMEE_FILE_NOACCESS;
	}
	else
	{
		PEM_write_RSAPrivateKey(private_key, rsa, NULL, NULL, 0, NULL, NULL);
	}

	return result;
}

acmee order_certificate(EVP_PKEY *pkey)
{
	acmee result = ACMEE_SUCCESS;
	struct json_token *jws, *protected, *key;

	protected = json_empty();
	key = json_add_key(protected, "alg");
	json_add_string(key, "RS256");
	key = json_add_key(protected, "kid");
	json_add_string(key, CA_ACCOUNT);
	key = json_add_key(protected, "nonce");
	json_add_string(key, get_nonce());
	key = json_add_key(protected, "url");
	json_add_string(key, order.certificate);

	jws = generate_jws(protected, NULL, pkey);

	request_new(order.certificate);
	curl_easy_setopt(get_curl(), CURLOPT_POST, 1);
	curl_easy_setopt(get_curl(), CURLOPT_POSTFIELDS, json_print(jws, 0, 0));
	request_perform();
	request_cleanup();

	/* save the certificate chain */

	char *path = prompt_text("Path to store certificate chain: ");
	FILE *certificate_file = fopen(path, "w+");

	if (certificate_file == NULL)
	{
		printf("Couldn't open file '%s'\n", path);
		result = ACMEE_FILE_NOACCESS;
	}
	else
	{
		fprintf(certificate_file, "%s", get_body());
	}

	return result;
}

acmee order_parse()
{
	acmee result = ACMEE_SUCCESS;

	struct json_token *json = json_tokenize(get_body());
	order.status = json_get_string(json, "status");
	order.expires = json_get_string(json, "expires");
	order.not_before = json_get_string(json, "notBefore");
	order.not_after = json_get_string(json, "notAfter");
	order.finalize = json_get_string(json, "finalize");
	order.certificate = json_get_string(json, "certificate");

	struct json_token *obj;
	struct json_token *identifiers = json_get(json, "identifiers");
	int i = 0;
	struct acme_identifier *id;
	char *t;

	while ((obj = json_get_index(identifiers, i)) != NULL)
	{
		t = json_get_string(obj, "type");
		id = malloc(sizeof(struct acme_identifier)); 

		if (strcmp(t, "dns") == 0)
		{
			id->type = DNS;
		}

		id->value = json_get_string(obj, "value");

		order.identifiers = llist_append(order.identifiers, id);
		i++;
	}

	struct json_token *authorizations = json_get(json, "authorizations");
	i = 0;
	char *authorization;

	while ((authorization = json_get_index_string(authorizations, i)) != NULL)
	{
		order.authorizations = llist_append(order.authorizations, authorization);
		i++;
	}

	return result;
}
