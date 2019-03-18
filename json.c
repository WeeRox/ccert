#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "json.h"

char *json_print_object(struct json_token **json, int space, int newline);
char *json_print_array(struct json_token **json, int space, int newline);

struct json_token *json_tokenize(char *json)
{
	regex_t regex;
	int result;
	int offset;	
	regmatch_t match[1];
	char *tok;
	size_t tok_len;

	struct json_token *tokens = NULL;
	struct json_token *curr;

	offset = 0;

	result = regcomp(&regex, "[][{}]|\"([^\"\\]|\\.)*\"|true|false|null|[0-9.]+", REG_EXTENDED);
	
	while (regexec(&regex, json + offset, 1, match, 0) == 0)
	{
		tok_len = match[0].rm_eo - match[0].rm_so;
		tok = malloc((tok_len + 1) * sizeof(char));
		strncpy(tok, json + offset + match[0].rm_so, tok_len);
		*(tok + tok_len) = 0;
		offset += tok_len + match[0].rm_so;

		if (tokens == NULL)
		{
			tokens = malloc(sizeof(struct json_token));
			tokens->token = tok;
			tokens->next = NULL;
			curr = tokens;
		}
		else
		{
			struct json_token *tmp = malloc(sizeof(struct json_token));
			tmp->token = tok;
			tmp->next = NULL;
			curr->next = tmp;
			curr = tmp;
		}
	}

	return tokens;
}

struct json_token *json_get(struct json_token *json, char *key)
{
	int c = 0;
	int b = 0;

	char *key_q = malloc(strlen(key) + 2 + 1);
	strcpy(key_q, "\"");
	strcat(key_q, key);
	strcat(key_q, "\"");

	struct json_token *curr = json;

	while (curr != NULL)
	{
		if (strcmp(curr->token, "{") == 0)
			c++;
		else if (strcmp(curr->token, "}") == 0)
			c--;
		else if (strcmp(curr->token, "[") == 0)
			b++;
		else if (strcmp(curr->token, "]") == 0)
			b--;
		else if (c == 1 && b == 0)
		{
			if (strcmp(curr->token, key_q) == 0)
			{
				return curr->next;
			}
		}
		curr = curr->next;
	}
	return NULL;
}

char *json_get_string(struct json_token *json, char *key)
{
	struct json_token *result = json_get(json, key);

	if (result == NULL)
		return NULL;

	char *string = calloc(strlen(result->token) - 2 + 1, sizeof(char));
	strncpy(string, result->token + 1, strlen(result->token) - 2);
	return string;
}

int json_get_boolean(struct json_token *json, char *key)
{
	struct json_token *result = json_get(json, key);

	if (result == NULL)
		return 0;
	else if (strcmp(result->token, "true") == 0)
		return 1;
	else if (strcmp(result->token, "false") == 0)
		return 0;

	return 0;
}

struct json_token *json_get_index(struct json_token *json, int index)
{
	struct json_token *curr = json->next;

	for (int i = 0; i < index; i++)
	{
		if (strcmp(curr->token, "{") == 0)
		{
			json_print_object(&curr, 0, 0);
		}
		else if (strcmp(curr->token, "[") == 0)
		{
			json_print_array(&curr, 0, 0);
		}
		curr = curr->next;

	}

	if (strcmp(curr->token, "]") == 0)
	{
		return NULL;
	}

	return curr;
}

char *json_get_index_string(struct json_token *json, int index)
{
	struct json_token *result = json_get_index(json, index);

	if (result == NULL)
		return NULL;

	char *string = calloc(strlen(result->token) - 2 + 1, sizeof(char));
	strncpy(string, result->token + 1, strlen(result->token) - 2);
	return string;
}

struct json_token *json_empty()
{
	struct json_token *begin = malloc(sizeof(struct json_token));
	struct json_token *end = malloc(sizeof(struct json_token));
	begin->token = malloc(2 * sizeof(char));
	begin->token = "{";
	begin->next = end;

	end->token = malloc(2 * sizeof(char));
	end->token = "}";
	end->next = NULL;

	return begin;
}

char *json_print_object(struct json_token **json, int space, int newline)
{
	int c = 1;
	size_t i = 0;

	struct json_token *curr = *json;

	char *result = malloc(sizeof(char));
	*result = 0;
	size_t len = 0;

	char *tmp;
	size_t tmp_len;

	do
	{
		if (strcmp(curr->token, "{") == 0)
		{
			if (i == 0)
			{
				tmp = curr->token;
			}
			else
			{
				tmp = json_print_object(&curr, space, newline);

				result = realloc(result, len + 1 + 1);
				strcpy(result + len, ":");
				len++;

				if (space)
				{
					result = realloc(result, len + 1 + 1);
					strcpy(result + len, " ");
					len++;
				}
			}
		}
		else if (strcmp(curr->token, "}") == 0)
		{
			c = 0;
			tmp = curr->token;
		}
		else if (strcmp(curr->token, "[") == 0)
		{
			tmp = json_print_array(&curr, space, newline);

			result = realloc(result, len + 1 + 1);
			strcpy(result + len, ":");
			len++;

			if (space)
			{
				result = realloc(result, len + 1 + 1);
				strcpy(result + len, " ");
				len++;
			}
		}
		else
		{
			if ((i - 1) % 2 == 1)
			{
				result = realloc(result, len + 1 + 1);
				strcpy(result + len, ":");
				len++;

				if (space)
				{
					result = realloc(result, len + 1 + 1);
					strcpy(result + len, " ");
					len++;
				}
			}
			else if (i != 1)
			{
				result = realloc(result, len + 1 + 1);
				strcpy(result + len, ",");
				len++;

				if (space)
				{
					result = realloc(result, len + 1 + 1);
					strcpy(result + len, " ");
					len++;
				}
			}

			tmp = curr->token;
		}

		tmp_len = strlen(tmp);
		result = realloc(result, len + tmp_len + 1);
		strcpy(result + len, tmp);
		len += tmp_len;

		if (c)
		{
			curr = curr->next;
			i++;
		}
	}
	while (c);

	*json = curr;
	return result;
}

char *json_print_array(struct json_token **json, int space, int newline)
{
	int c = 1;
	size_t i = 0;

	struct json_token *curr = *json;

	char *result = malloc(sizeof(char));
	*result = 0;
	size_t len = 0;

	char *tmp;
	size_t tmp_len;

	do
	{
		if (strcmp(curr->token, "[") == 0)
		{
			if (i == 0)
			{
				tmp = curr->token;
			}
			else
			{
				tmp = json_print_array(&curr, space, newline);

				result = realloc(result, len + 1 + 1);
				strcpy(result + len, ":");
				len++;

				if (space)
				{
					result = realloc(result, len + 1 + 1);
					strcpy(result + len, " ");
					len++;
				}
			}
		}
		else if (strcmp(curr->token, "]") == 0)
		{
			c = 0;
			tmp = curr->token;
		}
		else if (strcmp(curr->token, "{") == 0)
		{
			tmp = json_print_object(&curr, space, newline);
		}
		else
		{
			tmp = curr->token;
		}

		if (i > 1 && c != 0)
		{
			result = realloc(result, len + 1 + 1);
			strcpy(result + len, ",");
			len++;

			if (space)
			{
				result = realloc(result, len + 1 + 1);
				strcpy(result + len, " ");
				len++;
			}
		}

		tmp_len = strlen(tmp);
		result = realloc(result, len + tmp_len + 1);
		strcpy(result + len, tmp);
		len += tmp_len;

		if (c)
		{
			curr = curr->next;
			i++;
		}
	}
	while (c);

	*json = curr;
	return result;
}

char *json_print(struct json_token *json, int space, int newline)
{
	struct json_token *curr = json;
	return json_print_object(&curr, space, newline);
}

void json_add_token(struct json_token *after, struct json_token *token)
{
	struct json_token *tmp = after->next;

	after->next = token;
	token->next = tmp;
}

struct json_token *json_add_object(struct json_token *after)
{	
	struct json_token *open = malloc(sizeof(struct json_token));
	struct json_token *close = malloc(sizeof(struct json_token));

	open->token = "{";
	close->token = "}";

	json_add_token(after, open);
	json_add_token(open, close);

	return open;
}

struct json_token *json_add_array(struct json_token *after)
{	
	struct json_token *open = malloc(sizeof(struct json_token));
	struct json_token *close = malloc(sizeof(struct json_token));

	open->token = "[";
	close->token = "]";

	json_add_token(after, open);
	json_add_token(open, close);

	return open;
}

struct json_token *json_add_key(struct json_token *in, char *key)
{
	json_add_string(in, key);
	return in->next;
}

void json_add_string(struct json_token *key, char *string)
{
	struct json_token *tok = malloc(sizeof(struct json_token));
	tok->token = malloc((strlen(string) + 2 + 1) * sizeof(char));
	strcpy(tok->token, "\"");
	strcat(tok->token, string);
	strcat(tok->token, "\"");

	json_add_token(key, tok);
}

void json_add_boolean(struct json_token *key, int boolean)
{
	struct json_token *tok = malloc(sizeof(struct json_token));

	if (boolean)
	{
		tok->token = malloc(5);
		strcpy(tok->token, "true");
	}
	else
	{
		tok->token = malloc(6);
		strcpy(tok->token, "false");
	}

	json_add_token(key, tok);
}
