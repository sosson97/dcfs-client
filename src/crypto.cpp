#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ecerr.h>

#include "crypto.hpp"

unsigned char * hash256(void *data, size_t len, unsigned char *hsh)
{
    if (!hsh)
        hsh = new unsigned char[SHA256_DIGEST_LENGTH];
    
    SHA256_CTX hsh_ctx;
    if (!SHA256_Init(&hsh_ctx)){
        delete[] hsh;
        return NULL;         // Fail silently
    }
    if (!SHA256_Update(&hsh_ctx, (void *)data, len)){
        delete[] hsh;
        return NULL;
    }
    if (!SHA256_Final(hsh, &hsh_ctx)){
        delete[] hsh;
        return NULL;
    }

    return hsh;                 // It is your responsibility to delete hsh after you are done.
}
