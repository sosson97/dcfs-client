#ifndef CRYPTO_HPP_
#define CRYPTO_HPP_

#include <cstddef>
#include <string>
#include <set>
#include <string>

namespace Util {
	unsigned char * hash256(void *data, size_t len, unsigned char *res);            // Helper for SHA256
}
#endif /* CRYPTO_HPP_ */