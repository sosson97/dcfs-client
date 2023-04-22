#include "../src/util/crypto.hpp"

enum result { success, failure };

using namespace Util;

void fill_with_random_bytes(unsigned char *arr, const int len) {
    for (int i = 0; i < len; i++) {
        arr[i] = (unsigned char) rand();
    }
}

void print_char_array(unsigned char *arr, int len) {
    printf("printing");
    for (int i = 0; i < len; i++) {
        printf("%u", arr[i]);
    }
    printf("\n");
}

int test_symmetric_encryption() {
    printf("Testing symmetric enc/dec...\n");

    const int TEST_DATA_LENGTH = 100;
    unsigned char data[TEST_DATA_LENGTH];
    unsigned char encrypted_data[TEST_DATA_LENGTH];
    unsigned char decrypted_data[TEST_DATA_LENGTH];
    
    // Create test data
    fill_with_random_bytes(data, TEST_DATA_LENGTH);

	unsigned char key[16];
    unsigned char iv[16];

    // Create symmetric keys + iv
    if (!generate_symmetric_key(key)) {
        printf("symmetric key generation failed");
        return -1;
    }
    if (!generate_symmetric_key(iv)) {
        printf("iv  generation failed");
        return -1;
    }

    // Encrypt
    if (!encrypt_symmetric(key, iv, data, TEST_DATA_LENGTH, encrypted_data, TEST_DATA_LENGTH)) {
        printf("Encyprtion failed");
        return -1;
    }

    // Decrypt
    if (!decrypt_symmetric(key, encrypted_data, TEST_DATA_LENGTH, decrypted_data, TEST_DATA_LENGTH)) {
        printf("decryption failed");
        return -1;
    }

    print_char_array(data, TEST_DATA_LENGTH);
    print_char_array(decrypted_data, TEST_DATA_LENGTH);

    
    
    return 0;
}

int test_dsa() {
    
    const int TEST_DATA_LENGTH = 100;
    unsigned char data[TEST_DATA_LENGTH];
    fill_with_random_bytes(data, TEST_DATA_LENGTH);
    // const uint8_t kMsg[] = {1, 2, 3, 4};

    EVP_PKEY *pkey = generate_evp_pkey_dsa();
    // BIO *bp = BIO_new_fp(stdout, BIO_NOCLOSE);
    // EVP_PKEY_print_private(bp, pkey, 1, NULL);

    unsigned char *test_sig = sign_dsa(pkey, data, TEST_DATA_LENGTH); // MILES: This line segfaults

    return 0;
}

int main() {
    printf("Beginning Crypto Test.....\n");

    test_symmetric_encryption();
    // test_dsa();
    
    return 0;
}

