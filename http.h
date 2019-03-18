#ifndef __NONCE_H
#define __NONCE_H

#include <curl/curl.h>

void request_init();
void request_new(char *url);
void request_perform();
void request_cleanup();

char *get_header(char *header);
char *get_nonce();
char *get_headers();
char *get_body();
CURL *get_curl();
long get_status_code();

#endif /* __NONCE_H */
