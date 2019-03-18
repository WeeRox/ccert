#ifndef __JSON_H
#define __JSON_H

struct json_token
{
	char *token;
	struct json_token *next;
};

struct json_token *json_tokenize(char *json);

struct json_token* json_get(struct json_token *json, char *key);
char *json_get_string(struct json_token *json, char *key);
int json_get_boolean(struct json_token *json, char *key);
struct json_token *json_get_index(struct json_token *json, int index);
char *json_get_index_string(struct json_token *json, int index);

struct json_token *json_empty();
struct json_token *json_add_key(struct json_token *json, char *key);
struct json_token *json_add_object(struct json_token *json);
struct json_token *json_add_array(struct json_token *json);
void json_add_string(struct json_token *json, char *string);
void json_add_boolean(struct json_token *json, int b);

char *json_print(struct json_token *json, int space, int newline);

#endif /* __JSON_H */
