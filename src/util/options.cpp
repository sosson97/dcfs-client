#include "options.hpp"

namespace Util {


std::map<std::string, std::string> option_map;	
std::string load_client_ip() {
	if (option_map.find("client_ip") == option_map.end()) {
		return "";
	}
	return option_map["client_ip"];
}
std::string load_dcserver_ip() {
	if (option_map.find("dcserver_ip") == option_map.end()) {
		return "";
	}
	return option_map["dcserver_ip"];
}


}