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

#define AES_KEY_LEN 16
#define AES_PAD_LEN 16

namespace Util {
	unsigned char * hash256(void *data, size_t len, unsigned char *res);            // Helper for SHA256
	/*
	unsigned char *sign_dsa(EVP_PKEY *pkey, unsigned char *data, size_t len);
	int verify_dsa(EVP_PKEY *pkey, void *data, size_t len, unsigned char *signature, size_t sig_len);
	EVP_PKEY * generate_evp_pkey_dsa();
	*/
	
    unsigned char *sign(EC_KEY *sig_k, unsigned char* data, int inlen, int *siglen);
	bool verify(EC_KEY *sig_k, unsigned char* data, int inlen, const unsigned char* sig, int siglen);
	int generate_ECDSA_key(EC_KEY **ec_key);	

	int encrypt_symmetric(unsigned char *key, unsigned char *iv, unsigned char *inbuf, int inlen, unsigned char *outbuf, int *outlen);
	int decrypt_symmetric(unsigned char* key, unsigned char *iv, unsigned char *inbuf, int inlen, unsigned char *outbuf, int *outlen);
	int generate_symmetric_key(unsigned char* key);


	// int sign(void *data, size_t len, void *pkey);
}
#endif /* CRYPTO_HPP_ */