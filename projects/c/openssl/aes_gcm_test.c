#include <openssl/bio.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

/* gcc aes_gcm_test.c -o aes_gcm_test -lssl -lcrypto */

#define handleErrors() do { \
	printf("Handle Errors %d\n", __LINE__); \
	exit(1); \
} while (0)

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
	unsigned char *ciphertext, unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;


	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length if default 12 bytes (96 bits) is not appropriate */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

	/* Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
		handleErrors();

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
	ciphertext_len = len;

	/* Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
	int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	int iv_len, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

	/* Initialise the decryption operation. */
	if(!EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

	/* Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
		handleErrors();

	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
		handleErrors();

	/* Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);


	if(ret > 0)
	{
		/* Success */
		plaintext_len += len;
		return plaintext_len;
	}
	else
	{
		/* Verify failed */
		return -1;
	}
}

static char *minidump(unsigned char *data, int len)
{
	static char retval[256];
	int i;

	memset(retval, 0x0, sizeof(retval));
	for (i = 0; i < len; i++)
		sprintf(retval + i*3, "%02x ", data[i]);

	return retval;
}

int main()
{
	unsigned char plaintext[30];
	int plaintext_len = sizeof(plaintext);
	unsigned char aad[16];
	int aad_len = sizeof(aad);
	unsigned char key[16];
	unsigned char iv[12];
	int iv_len = sizeof(iv);
	unsigned char ciphertext[30];
	int ciphertext_len = sizeof(ciphertext);
	unsigned char tag[16];
	int ret;

	memset(plaintext, 0x0, sizeof(plaintext));
	memset(aad, 0x0, sizeof(aad));
	memset(key, 0x0, sizeof(key));
	memset(iv, 0x0, sizeof(iv));
	memset(ciphertext, 0x0, sizeof(ciphertext));
	memset(tag, 0x0, sizeof(tag));

	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));
	printf("aad        %s\n", minidump(aad, sizeof(aad)));
	printf("key        %s\n", minidump(key, sizeof(key)));
	printf("iv         %s\n", minidump(iv, sizeof(iv)));
	printf("ciphertext %s\n", minidump(ciphertext, sizeof(ciphertext)));
	printf("tag        %s\n", minidump(tag, sizeof(tag)));

	ret = encrypt(plaintext, plaintext_len, aad, aad_len, key, iv, iv_len, ciphertext, tag);
	printf("========= encrypt %d\n", ret);
	memset(plaintext, 0x1, sizeof(plaintext));
	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));
	printf("ciphertext %s\n", minidump(ciphertext, sizeof(ciphertext)));
	printf("tag        %s\n", minidump(tag, sizeof(tag)));

	ret = decrypt(ciphertext, ciphertext_len, aad, aad_len, tag, key, iv, iv_len, plaintext);
	printf("========= decrypt %d\n", ret);
	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));

#if 0
	memset(plaintext, 0x1, sizeof(plaintext));
	tag[0]++;
	ret = decrypt(ciphertext, ciphertext_len, aad, aad_len, tag, key, iv, iv_len, plaintext);
	tag[0]--;
	printf("tag ++========= decrypt %d\n", ret);
	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));


	memset(plaintext, 0x1, sizeof(plaintext));
	iv[11]++;
	ret = decrypt(ciphertext, ciphertext_len, aad, aad_len, tag, key, iv, iv_len, plaintext);
	iv[11]--;
	printf("iv[11] ++========= decrypt %d\n", ret);
	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));

	memset(plaintext, 0x1, sizeof(plaintext));
	aad[0]++;
	ret = decrypt(ciphertext, ciphertext_len, aad, aad_len, tag, key, iv, iv_len, plaintext);
	aad[0]--;
	printf("aad[0] ++========= decrypt %d\n", ret);
	printf("plaintext  %s\n", minidump(plaintext, sizeof(plaintext)));
#endif
	return 0;
}
