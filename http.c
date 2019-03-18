#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include <curl/curl.h>

struct string
{
	char *ptr;
	size_t size;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, struct string *s)
{
	s->ptr = realloc(s->ptr, s->size + (size * nmemb) + 1);
	strcat(s->ptr + s->size, ptr);
	s->size += size * nmemb;
	return size * nmemb;
};

struct string body, headers;

struct curl_slist *list;

char *nonce = NULL;

CURL *curl_def, *curl;

CURL *get_curl()
{
	return curl;
}

char *get_header(char *header)
{
	regex_t regex;
	regmatch_t match[2];
	int result;
	char *pattern = malloc(strlen(header) + 9 + 1);
	strcpy(pattern, header);
	strcat(pattern, ":[ ](.*)\r");
	*(pattern + strlen(header) + 9) = 0;

	result = regcomp(&regex, pattern, REG_EXTENDED | REG_NEWLINE);
	result = regexec(&regex, headers.ptr, 2, match, 0);	

	if (result != 0)
		return NULL;

	char *val = malloc(match[1].rm_eo - match[1].rm_so + 1);
	strncpy(val, headers.ptr + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
	*(val + (match[1].rm_eo - match[1].rm_so)) = 0;
	return val;
}

char *get_nonce()
{
	return nonce;
}

char *get_headers()
{
	return headers.ptr;
}

char *get_body()
{
	return body.ptr;
}

long get_status_code()
{
	long code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	return code;
}

void request_init()
{
	char *CMD_VERSION = "1.0.0";
	const char *CURL_VERSION = curl_version_info(CURLVERSION_NOW)->version;
	char *USER_AGENT;

	asprintf(&USER_AGENT, "acme/%s curl/%s", CMD_VERSION, CURL_VERSION);

	list = NULL;
	list = curl_slist_append(list, "Content-Type: application/jose+json");

	headers.ptr = calloc(1, 1);
	headers.size = 0;

	body.ptr = calloc(1, 1);
	body.size = 0;

	curl_def = curl_easy_init();
	curl_easy_setopt(curl_def, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl_def, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_def, CURLOPT_WRITEDATA, &body);
	curl_easy_setopt(curl_def, CURLOPT_HEADERDATA, &headers);
	curl_easy_setopt(curl_def, CURLOPT_HTTPHEADER, list);
}

void request_new(char *url)
{
	curl = curl_easy_duphandle(curl_def);
	curl_easy_setopt(curl, CURLOPT_URL, url);
}

void request_perform()
{
	free(headers.ptr);
	free(body.ptr);

	headers.ptr = calloc(1, 1);
	headers.size = 0;

	body.ptr = calloc(1, 1);
	body.size = 0;

	curl_easy_perform(curl);
	nonce = get_header("Replay-Nonce");
}

void request_cleanup()
{
	curl_easy_cleanup(curl);
}
