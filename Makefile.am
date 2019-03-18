AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = ccert

ccert_SOURCES = src/main.c src/json.c src/jws.c src/http.c src/prompt.c src/base64.c src/llist.c src/account.c src/authorization.c src/challenge.c src/order.c
ccert_LDADD = -L/usr/local/lib -lcurl -lssl -lcrypto
