#include "../src/util/crypto.hpp"

enum result { success, failure };

using namespace Util;

void fill_with_random_bytes(unsigned char *arr, const int len) {
    for (int i = 0; i < len; i++) {
        arr[i] = (unsigned char) rand();
    }
}

int test_symmetric_encryption() {
    
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

    unsigned char *test_sig = sign_dsa(pkey, data, TEST_DATA_LENGTH);

    return 0;
}

int main() {
    printf("Beginning Crypto Test...\n");

    test_symmetric_encryption();
    test_dsa();
    
    return 0;
}

