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

    unsigned char *sign_dsa(EVP_PKEY *pkey, unsigned char *data, size_t len) {
        EVP_MD_CTX *mdctx = NULL;
        int ret = 0;
        
        unsigned char *sig;
        *sig = NULL;
        
        /* Create the Message Digest Context */
        printf("Creating Message Digest Context.\n");
        // EVP_MD_CTX_init(mdctx);
        if(!(mdctx = EVP_MD_CTX_create())) {
            printf("Creating Message Digest CTX failed");
            goto err;
        }
        
        /* Initialise the DigestSign operation - SHA-256 has been selected as the message digest function in this example */
        printf("Initializing digestsign op. \n");
        if(1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey)) {
            printf("DigestSign op failed.");
            goto err;
        }
        
        /* Hash len bytes of data into mdctx */
        printf("Calling digest sign update. \n");
        if(1 != EVP_DigestSignUpdate(mdctx, data, len)) {
            printf("DigestSignUpdate op failed.");
            goto err;
        }
        
        /* Finalise the DigestSign operation */
        /* First call EVP_DigestSignFinal with a NULL sig parameter to obtain the length of the
        * signature. Length is returned in slen */
        size_t *slen;
        printf("Obtaining the length of the buffer.\n");
        if(1 != EVP_DigestSignFinal(mdctx, NULL, slen)) { // MILES: this line is segfaulting for some reason
            printf("Obtaining len of sig failed.");
            goto err;
        }
        /* Allocate memory for the signature based on size in slen */
        printf("Allocate memory for the signature based on size in slen. \n");
        if(!(*sig = OPENSSL_malloc(sizeof(unsigned char) * (*slen)))) {
            printf("Allocating sig failed.");
            goto err;
        }
        /* Obtain the signature */
        printf("Obtaining Sig. \n");
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
    
    // int encrypt_symmetric(unsigned char *key, unsigned char *iv, char *inbuf, int inlen, char *outbuf, int outlen) {
    //     EVP_CIPHER_CTX *ctx = NULL;
    //     printf("enc -1\n");

    //     EVP_CIPHER_CTX_init(ctx);
    //     printf("enc 0\n");


    //     EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL, 1);
    //     OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);
    //     OPENSSL_assert(EVP_CIPHER_CTX_iv_length(ctx) == 16);

    //     printf("enc 1\n");

    //     EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 1);
    //     printf("enc 2\n");
    //     if(!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, &inlen)) {
    //         /* Error */
    //         EVP_CIPHER_CTX_cleanup(ctx);
    //         return -1; //UJJAINI TODO: replace with correct error
    //     }
    //     printf("enc 3\n");
    //     if(!EVP_CipherFinal_ex(ctx, outbuf, &outlen))
    //     {
    //         /* Error */
    //         EVP_CIPHER_CTX_cleanup(ctx);
    //         return -1; //UJJAINI TODO: replace with correct error
    //     }
    //     printf("enc 4\n");

    //     EVP_CIPHER_CTX_cleanup(ctx);
    //     return 1;
    // }
    int encrypt_symmetric(unsigned char *key, unsigned char *iv, unsigned char *plaintext, int plaintext_len, 
        unsigned char *ciphertext, int outlen) { 
   
        EVP_CIPHER_CTX *ctx;

        int len;

        int ciphertext_len;

        /* Create and initialise the context */
        if(!(ctx = EVP_CIPHER_CTX_new())) {
            return -1;
        }
            

        /*
        * Initialise the encryption operation. IMPORTANT - ensure you use a key
        * and IV size appropriate for your cipher
        * In this example we are using 256 bit AES (i.e. a 256 bit key). The
        * IV size for *most* modes is the same as the block size. For AES this
        * is 128 bits
        */
        if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
            return -1;

        /*
        * Provide the message to be encrypted, and obtain the encrypted output.
        * EVP_EncryptUpdate can be called multiple times if necessary
        */
        if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
            return -1;
        ciphertext_len = len;

        /*
        * Finalise the encryption. Further ciphertext bytes may be written at
        * this stage.
        */
        if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
            return -1;
        ciphertext_len += len;

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        return ciphertext_len;
    }

    // int decrypt_symmetric(unsigned char* key,  char *inbuf, int inlen, char *outbuf, int outlen) {

    //     EVP_CIPHER_CTX *ctx;
    //     EVP_CIPHER_CTX_init(ctx);

    //     EVP_CipherInit_ex(ctx, EVP_aes_128_cbc(), NULL, NULL, NULL,
    //             0);
    //     OPENSSL_assert(EVP_CIPHER_CTX_key_length(ctx) == 16);

    //     EVP_CipherInit_ex(ctx, NULL, NULL, key, NULL, 0);

    //     if(!EVP_CipherUpdate(ctx, outbuf, &outlen, inbuf, &inlen)) {
    //         /* Error */
    //         EVP_CIPHER_CTX_cleanup(ctx);
    //         return -1; //UJJAINI TODO: replace with correct error
    //     }
    //     if(!EVP_CipherFinal_ex(ctx, outbuf, &outlen))
    //     {
    //         /* Error */
    //         EVP_CIPHER_CTX_cleanup(ctx);
    //         return -1; //UJJAINI TODO: replace with correct error
    //     }

    //     EVP_CIPHER_CTX_cleanup(ctx);
    //     return 1;

    // }
    // decrypt_symmetric(unsigned char* key,  char *inbuf, int inlen, char *outbuf, int outlen) {
    int decrypt_symmetric(unsigned char *key, unsigned char *ciphertext, int ciphertext_len,
            unsigned char *plaintext, unsigned char *iv)
    {
        EVP_CIPHER_CTX *ctx;

        int len;

        int plaintext_len;

        /* Create and initialise the context */
        if(!(ctx = EVP_CIPHER_CTX_new()))
            return -1;

        /*
        * Initialise the decryption operation. IMPORTANT - ensure you use a key
        * and IV size appropriate for your cipher
        * In this example we are using 256 bit AES (i.e. a 256 bit key). The
        * IV size for *most* modes is the same as the block size. For AES this
        * is 128 bits
        */
        if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
            return -1;

        /*
        * Provide the message to be decrypted, and obtain the plaintext output.
        * EVP_DecryptUpdate can be called multiple times if necessary.
        */
        if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
            return -1;
        plaintext_len = len;

        /*
        * Finalise the decryption. Further plaintext bytes may be written at
        * this stage.
        */
        if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
            return -1;
        plaintext_len += len;

        /* Clean up */
        EVP_CIPHER_CTX_free(ctx);

        return plaintext_len;
    }


    int generate_symmetric_key(unsigned char* key) {
        if (!RAND_bytes(key, 16*sizeof(char))) {
            return -1; //UJJAINI TODO: replace with correct error
        }
        return 1;
    }
}