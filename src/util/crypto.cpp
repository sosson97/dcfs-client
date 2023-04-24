#include "crypto.hpp"
namespace Util {
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

    int generate_ECDSA_key(EC_KEY **ec_key) {
        if (!ec_key)
            return -1;

        *ec_key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!*ec_key)
            return -1;

        if (!EC_KEY_generate_key(*ec_key))
            return -1;

        return 0;
    }

    unsigned char *sign(EC_KEY *sig_k, unsigned char* data, int inlen, int *siglen) {
#ifdef NO_SIGN
        return NULL;
#else
        if (!sig_k) 
            return NULL;

        *siglen = ECDSA_size(sig_k);

        unsigned char *sig = new unsigned char[*siglen];
        if (ECDSA_sign(0, data, inlen, sig, (unsigned int *)siglen, sig_k) != 1) {
            delete[] sig;
            return NULL;
        }

        return sig;
#endif
    }
 
    bool verify(EC_KEY *sig_k, unsigned char* data, int inlen, const unsigned char* sig, int siglen) {
#ifdef NO_SIGN
        return true;
#else
        if (!sig_k)
            return false;

        return ECDSA_verify(0, data, inlen, sig, siglen, sig_k) == 1;
#endif
    }
    /*    
    EVP_PKEY * generate_evp_pkey_dsa() {

        // Generate params struct
        EVP_PKEY_CTX* pctx;
        EVP_PKEY *pkey_params = NULL;
        if(!(pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DSA, NULL))){
            printf("Failure");
        }
        if(!EVP_PKEY_CTX_set_dsa_paramgen_bits(pctx, 2048)) {
            printf("Failure");
        }
        if(!EVP_PKEY_paramgen_init(pctx)) {
            printf("Failure");
        }
        
        if (!EVP_PKEY_paramgen(pctx, &pkey_params)) {
            printf("Failure");
        }

        // Generate DSA key
        EVP_PKEY_CTX *kctx = NULL;
        if(!(kctx = EVP_PKEY_CTX_new(pkey_params, NULL))) {
            printf("Failure");
        }

        if(!EVP_PKEY_keygen_init(kctx)) {
            printf("Failure");
        }

        EVP_PKEY *pkey = NULL;
        if (!EVP_PKEY_keygen(kctx, &pkey)) {
            printf("Key generation failed.");
        }

        return pkey;
    }
    */
    int encrypt_symmetric(unsigned char *key, unsigned char *iv, unsigned char *inbuf, int inlen, unsigned char *outbuf, int *outlen) {
#ifdef NO_ENC
        memcpy(outbuf, inbuf, inlen);
        *outlen = inlen;
        return 1;
#else
        int retlen = 0;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        EVP_CIPHER_CTX_init(ctx);

        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL,
                1);
        OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);
        OPENSSL_assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);

        EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 1);

        if(!EVP_CipherUpdate(ctx, outbuf, &retlen, inbuf, inlen)) {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        *outlen = retlen;

        if(!EVP_CipherFinal_ex(ctx, outbuf + retlen, &retlen))
        {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        *outlen += retlen;

        EVP_CIPHER_CTX_cleanup(ctx);
        return 1;
    }
#endif

    int decrypt_symmetric(unsigned char* key, unsigned char *iv, unsigned char *inbuf, int inlen, unsigned char *outbuf, int *outlen) {
#ifdef NO_ENC
        memcpy(outbuf, inbuf, inlen);
        *outlen = inlen;
        return 1;
#else
        int retlen;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        
        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL,
                0);
        OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);

        EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 0);

        /*
        * Initialise the decryption operation. IMPORTANT - ensure you use a key
        * and IV size appropriate for your cipher
        * In this example we are using 256 bit AES (i.e. a 256 bit key). The
        * IV size for *most* modes is the same as the block size. For AES this
        * is 128 bits
        */
        if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
            return -1;

        EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 0);

        if(!EVP_CipherUpdate(ctx, outbuf, &retlen, inbuf, inlen)) {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        *outlen = retlen;
        if(!EVP_CipherFinal_ex(ctx, outbuf + retlen, &retlen))
        {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        *outlen += retlen;

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        return 1;
#endif
    }


    int generate_symmetric_key(unsigned char* key) {
        return RAND_bytes(key, 16*sizeof(char)); // 1 on success, 0 on failure, -1 not supported
    }
}