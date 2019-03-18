#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <string.h>

char *base64_encode(unsigned char *data, size_t len)
{
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *mem = BIO_new(BIO_s_mem());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	BIO_push(b64, mem);
	BIO_write(b64, data, len);
	BIO_flush(b64);

	BUF_MEM *buf;
	BIO_get_mem_ptr(mem, &buf);

	char *result = malloc((buf->length + 1) * sizeof(char));
	memcpy(result, buf->data, buf->length);
	*(result + buf->length) = 0;

	BIO_set_close(b64, BIO_NOCLOSE);
	BIO_free_all(b64);
	BUF_MEM_free(buf);

	return result;
}

unsigned char *base64_decode(char *data, size_t len)
{
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *mem = BIO_new(BIO_s_mem());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	BIO_push(b64, mem);
	BIO_write(mem, data, len);
	BIO_flush(mem);

	BUF_MEM *buf = BUF_MEM_new();
	BUF_MEM_grow(buf, 1);

	while (BIO_read(b64, buf->data + buf->length - 1, 1) > 0)
	{
		BUF_MEM_grow(buf, buf->length + 1);
	}

	unsigned char *result = malloc((buf->length + 1) * sizeof(char));
	memcpy(result, buf->data, buf->length);
	*(result + buf->length) = 0;

	BIO_set_close(b64, BIO_NOCLOSE);
	BIO_free_all(b64);
	BUF_MEM_free(buf);

	return result;
}

char *base64url_encode(unsigned char *data, size_t len, int padding)
{
	char *b64 = base64_encode(data, len);

	int pad_chars = 0;
	size_t b64_len = strlen(b64);

	for (int i = 0; i < b64_len; i++)
	{
		if (*(b64 + i) == '/')
		{
			*(b64 + i) = '_';
		}
		else if (*(b64 + i) == '+')
		{
			*(b64 + i) = '-';
		}
		else if (*(b64 + i) == '=')
		{
			pad_chars++;
		}
	}

	if (padding)
	{
		return b64;
	}
	else
	{
		char *b64_url = malloc((b64_len - pad_chars + 1) * sizeof(char));
		strncpy(b64_url, b64, b64_len - pad_chars);
		*(b64_url + b64_len - pad_chars) = 0;
		return b64_url;
	}
}

unsigned char *base64url_decode(char *data, size_t len)
{
	int pad_chars = 4 - (len % 4);

	if (pad_chars)
	{	
		char *b64 = malloc(len + pad_chars + 1);
		strncpy(b64, data, len);

		for (int i = 0; i < pad_chars; i++)
			*(b64 + len + i) = '=';

		*(b64 + len + pad_chars) = 0;

		return base64_decode(b64, len + pad_chars);
	}
	else
	{
		return base64_decode(data, len);
	}
}
