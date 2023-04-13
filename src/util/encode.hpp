#include <string>

namespace Util {
	std::string binary_to_hex_string(const char *data, size_t len);
	std::string hex_string_to_binary(std::string hex);
}