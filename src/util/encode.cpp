#include "encode.hpp"

namespace Util {
	std::string binary_to_hex_string(const char *data, size_t len) {
		std::string res;
		for (size_t i = 0; i < len; i++) {
			char buf[3];
			snprintf(buf, 3, "%02x", (unsigned char)data[i]);
			res += buf;
		}
		return res;
	}

	std::string hex_string_to_binary(std::string hex) {
		std::string res;
		for (size_t i = 0; i < hex.length(); i += 2) {
			std::string byte = hex.substr(i, 2);
			char chr = (char) (int) strtol(byte.c_str(), NULL, 16);
			res += chr;
		}
		return res;
	}
}