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

    unsigned char *sign_dsa(EVP_PKEY *pkey, void *data, size_t len) {
        EVP_MD_CTX *mdctx = NULL;
        int ret = 0;
        
        unsigned char *sig;
        *sig = NULL;
        
        /* Create the Message Digest Context */
        if(!(mdctx = EVP_MD_CTX_create())) {
            printf("Creating Message Digest CTX failed");
            goto err;
        }
        
        /* Initialise the DigestSign operation - SHA-256 has been selected as the message digest function in this example */
        if(1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey)) {
            printf("DigestSign op failed.");
            goto err;
        }
        
        /* Call update with the message */
        if(1 != EVP_DigestSignUpdate(mdctx, data, len)) {
            printf("DigestSignUpdate op failed.");
            goto err;
        }
        
        /* Finalise the DigestSign operation */
        /* First call EVP_DigestSignFinal with a NULL sig parameter to obtain the length of the
        * signature. Length is returned in slen */
        size_t *slen;
        if(1 != EVP_DigestSignFinal(mdctx, NULL, slen)) {
            printf("Obtaining len of sig failed.");
            goto err;
        }
        /* Allocate memory for the signature based on size in slen */
        if(!(*sig = OPENSSL_malloc(sizeof(unsigned char) * (*slen)))) {
            printf("Allocating sig failed.");
            goto err;
        }
        /* Obtain the signature */
        if(1 != EVP_DigestSignFinal(mdctx, *sig, slen)) {
            printf("Sig step failed.");
            goto err;
        }
        
        /* Success */
        ret = 1;
        
        err:
        if(ret != 1)
        {
            return 0;
        }
        
        /* Clean up */
        if(*sig && !ret) OPENSSL_free(*sig);
        if(mdctx) EVP_MD_CTX_destroy(mdctx);
        return sig;
    }

    int verify_dsa(EVP_PKEY *pkey, void *data, size_t len, unsigned char *signature, size_t sig_len)
    {
        //probably only want to create the context once regardless of how many times you want to -- Ujjaini
        EVP_MD_CTX *mdctx = NULL;

        /* Sets up verification context mdctx to use a digest with public key pkey and type sha256 */
        if(1 != EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pkey)) {
            printf("Verification Error");
            return 0;
        }

        /* Initialize `key` with a public key */
        if(1 != EVP_DigestVerifyUpdate(mdctx, data, len)) {
            printf("Verification Error");
            return 0;
        }

        return (EVP_DigestVerifyFinal(mdctx, signature, sig_len));
    }

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

        /* Generate the key */
        EVP_PKEY *pkey = NULL;
        if (!EVP_PKEY_keygen(kctx, &pkey)) {
            printf("Key generation failed.");
        }

        return pkey;
    }

    int encrypt_symmetric(unsigned char *key, unsigned char *iv, char *inbuf, int inlen, char *outbuf, int outlen) {
        EVP_CIPHER_CTX *ctx;
        EVP_CIPHER_CTX_init(ctx);

        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL,
                1);
        OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);
        OPENSSL_assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);

        EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 1);
        if(!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, &inlen)) {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        if(!EVP_CipherFinal_ex(ctx, outbuf, &outlen))
        {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }

        EVP_CIPHER_CTX_cleanup(ctx);
        return 1;
    }

    int decrypt_symmetric(unsigned char* key,  char *inbuf, int inlen, char *outbuf, int outlen) {

        EVP_CIPHER_CTX *ctx;
        EVP_CIPHER_CTX_init(ctx);

        EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL,
                0);
        OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);

        EVP_CipherInit_ex(ctx, NULL, NULL, key, NULL, 0);

        if(!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, &inlen)) {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }
        if(!EVP_CipherFinal_ex(ctx, outbuf, &outlen))
        {
            /* Error */
            EVP_CIPHER_CTX_cleanup(ctx);
            return -1; //UJJAINI TODO: replace with correct error
        }

        EVP_CIPHER_CTX_cleanup(ctx);
        return 1;

    }


    int generate_symmetric_key(unsigned char* key) {
        if (!RAND_bytes(key, 16*sizeof(char))) {
            return -1; //UJJAINI TODO: replace with correct error
        }
    }
}