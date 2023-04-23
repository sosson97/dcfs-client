#include "../src/util/crypto.hpp"

// C headers
#include <cstdio>
#include <cstdlib>

enum result { success, failure };

using namespace Util;

static void init_buf(char *buf, int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = rand() % 256;
    }
}

// return negative if they are equal
// return the index of the first different byte
static int compare_buf(char *buf1, char *buf2, int size) {
    for (int i = 0; i < size; i++) {
        if (buf1[i] != buf2[i]) {
            return i;
        }
    }
    return -1;
}


#ifndef TEST_DATA_LENGTH
#define TEST_DATA_LENGTH 1024
#endif

#define AES_PAD 16

int test_symmetric_encryption() {
    printf("Testing symmetric enc/dec...\n");

    unsigned char data[TEST_DATA_LENGTH];
    unsigned char encrypted_data[TEST_DATA_LENGTH + AES_PAD];
    unsigned char decrypted_data[TEST_DATA_LENGTH + AES_PAD];
    
    // Create test data
    init_buf((char *)data, TEST_DATA_LENGTH);

	unsigned char key[16];
    unsigned char iv[16];

    // Create symmetric keys + iv
    if (generate_symmetric_key(key) <= 0) {
        printf("symmetric key generation failed\n");
        return -1;
    }
    if (generate_symmetric_key(iv) <= 0) {
        printf("iv  generation failed\n");
        return -1;
    }

    // Encrypt
    int enclen = 0;
    if (encrypt_symmetric(key, iv, data, TEST_DATA_LENGTH, encrypted_data, &enclen) <= 0) {
        printf("Encyprtion failed\n");
        return -1;
    }

    printf("Encrypted data length: %d\n", enclen);

    // Decrypt
    int declen = 0;
    if (decrypt_symmetric(key, iv, encrypted_data, enclen, decrypted_data, &declen) <= 0) {
        printf("decryption failed\n");
        return -1;
    }
    printf("Decrypted data length: %d\n", declen);

    if (compare_buf((char *)data, (char *)decrypted_data, TEST_DATA_LENGTH) != -1) {
        printf("Decrypted data does not match original data\n");
        return -1;
    } else {
        printf("Decrypted data matches original data\n");
    }    
    
    return 0;
}

int test_dsa() {
    EC_KEY *eckey;
    char data[TEST_DATA_LENGTH];
    char out[TEST_DATA_LENGTH];
    init_buf(data, TEST_DATA_LENGTH);

    eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!eckey) {
        printf("Error creating EC_KEY\n");
        return -1;
    }

    if (!EC_KEY_generate_key(eckey)) {
        printf("Error generating EC_KEY\n");
        return -1;
    }

    int siglen = 0;
    unsigned char * sig = sign(eckey, (unsigned char *)data, TEST_DATA_LENGTH, &siglen);
    if (!sig) {
        printf("Error signing\n");
        return -1;
    }

    bool verified = verify(eckey, (unsigned char *)data, TEST_DATA_LENGTH, sig, siglen);
    if (!verified) {
        printf("Error verifying\n");
        return -1;
    } else {
        printf("Verified\n");
    }

    // const uint8_t kMsg[] = {1, 2, 3, 4};

    //EVP_PKEY *pkey = generate_evp_pkey_dsa();
    // BIO *bp = BIO_new_fp(stdout, BIO_NOCLOSE);
    // EVP_PKEY_print_private(bp, pkey, 1, NULL);

    //unsigned char *test_sig = sign_dsa(pkey, (unsigned char *)data, TEST_DATA_LENGTH); // MILES: This line segfaults

    return 0;
}

int main() {
    printf("Beginning Crypto Test.....\n");

    test_symmetric_encryption();
    test_dsa();
    
    return 0;
}

