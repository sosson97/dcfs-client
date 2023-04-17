#ifndef OPTIONS_HPP_
#define OPTIONS_HPP_

#include <map>
namespace Util {

extern std::map<std::string, std::string> option_map;	
std::string load_client_ip();
std::string load_dcserver_ip();


}

#endif /* OPTIONS_HPP_ */