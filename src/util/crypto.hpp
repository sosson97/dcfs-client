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

#include <cstddef>
#include <string>
#include <set>
#include <string>

namespace Util {
	unsigned char * hash256(void *data, size_t len, unsigned char *res);            // Helper for SHA256

	int verify(EVP_PKEY *verify_key, void *data, size_t len, void *signature, size_t sig_len);

	int encrypt();



	// int sign(void *data, size_t len, void *pkey);

	EVP_PKEY *create_evp_pkey();

}
#endif /* CRYPTO_HPP_ */