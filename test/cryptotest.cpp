#include "../src/util/crypto.hpp"

enum result { success, failure };

using namespace Util;

int test_symmetric_encryption() {
    
    return 0;
}

int test_dsa() {
    
    unsigned char data[100];
    EVP_PKEY * pkey = generate_evp_pkey_dsa();

    return 0;
}

int main() {
    printf("Beginning Crypto Test...\n");

    test_symmetric_encryption();
    test_dsa();
    
    return 0;
}

