#ifndef CRYPTO_HPP_
#define CRYPTO_HPP_
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ecerr.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <cstddef>
#include <string>
#include <set>
#include <string>

#define CRYPTO_RSA_KEY_LEN_4096 4096
#define CRYPTO_RSA_KEY_LEN_2048 2048
#define CRYPTO_RSA_KEY_LEN_1024 1024
#define CRYPTO_RSA_KEY_EXP      65535

namespace Util {
	unsigned char * hash256(void *data, size_t len, unsigned char *res);            // Helper for SHA256

	unsigned char *sign_dsa(EVP_PKEY *pkey, unsigned char *data, size_t len);

	// int verify_dsa(EVP_PKEY *verify_key, void *data, size_t len, unsigned char *signature, size_t sig_len);
	int verify_dsa(EVP_PKEY *pkey, void *data, size_t len, unsigned char *signature, size_t sig_len);

	int encrypt_symmetric(unsigned char *key, unsigned char *iv, unsigned char *inbuf, int inlen, unsigned char *outbuf);

	int decrypt_symmetric(unsigned char *key, unsigned char *iv, unsigned char *ciphertext, 
		int ciphertext_len, unsigned char *plaintext);

	int generate_symmetric_key(unsigned char* key);

	EVP_PKEY * generate_evp_pkey_dsa();

	// int sign(void *data, size_t len, void *pkey);
}
#endif /* CRYPTO_HPP_ */