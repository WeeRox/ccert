AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = ccert

ccert_SOURCES = main.c json.c jws.c http.c prompt.c base64.c llist.c account.c authorization.c challenge.c order.c
ccert_LDADD = -L/usr/local/lib -lcurl -lssl -lcrypto
