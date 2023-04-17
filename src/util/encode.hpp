#ifndef ENCODE_HPP_
#define ENCODE_HPP_

#include <string>

namespace Util {
	std::string binary_to_hex_string(const char *data, size_t len);
	std::string hex_string_to_binary(std::string hex);
}

#endif /* ENCODE_HPP_ */