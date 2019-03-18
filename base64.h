#ifndef __BASE64_H
#define __BASE64_H

char *base64_encode(unsigned char *data, size_t len);
unsigned char *base64_decode(char *data, size_t len);

char *base64url_encode(unsigned char *data, size_t len, int padding);
unsigned char *base64url_decode(char *data, size_t len);

#endif /* __BASE64_H */
