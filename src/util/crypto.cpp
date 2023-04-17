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

    int verify(EVP_PKEY *verify_key, void *data, size_t len, void *signature, size_t sig_len)
    {
        //probably only want to create the context once regardless of how many times you want to -- Ujjaini
        EVP_PKEY_CTX ctx = EVP_PKEY_CTX_new(verify_key);

        if (!EVP_PKEY_verify_init(&ctx)) {
            return 0;
        }
        return EVP_PKEY_verify(&ctx, sig_len, len, data, len);
    }

    EVP_PKEY * create_evp_pkey() {
        const char* curve_name = "P-384";
		int curve_nid = EC_curve_nist2nid(curve_name);
		if (curve_nid == NID_undef) {
			// try converting the shortname (sn) to nid (numberic id)
			curve_nid = OBJ_sn2nid(curve_name);
		}
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
		if (EVP_PKEY_paramgen_init(ctx) <= 0) {
            printf("Error");
			return NULL;
		}
		if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, curve_nid) <= 0) {
			printf("Error");
			return NULL;
		}
		int ret = EVP_PKEY_CTX_set_ec_param_enc(ctx, OPENSSL_EC_NAMED_CURVE);
		if (ret  <= 0) {
			printf("EVP_PKEY_CTX_set_ec_param_enc retuned: %d\n", ret);
			return NULL;
		}
		EVP_PKEY* params = NULL;
		if (EVP_PKEY_paramgen(ctx, &params) <= 0) {
			printf("Error");
            return NULL;
        }
		EVP_PKEY_CTX* key_ctx = EVP_PKEY_CTX_new(params, NULL);
		if (EVP_PKEY_keygen_init(key_ctx) <= 0) {
			printf("Error");
			return NULL;
		}
		EVP_PKEY* pkey = NULL;
		if (EVP_PKEY_keygen(key_ctx, &pkey) <= 0) {
			printf("Error");
			return NULL;
		}
		EVP_PKEY_CTX_free(ctx);
		EVP_PKEY_CTX_free(key_ctx);
		return pkey;
    }


}