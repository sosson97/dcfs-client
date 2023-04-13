#include <cstdio>
#include <chrono>
#include "logging.hpp"

auto log_stream = fopen("/tmp/dcfs.log", "a");

namespace Logger
{
    unsigned long int get_current_time(){
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    void log(LogLevel level, const std::string &message)
    {
        switch (level)
        {
        case LDEBUG:
#ifdef DEBUG 
			fprintf(log_stream, "%lu [ DEBUG ] %s\n", get_current_time(), message.c_str());
#endif
            break;
        case INFO:
			fprintf(log_stream, "%lu [ INFO ] %s\n", get_current_time(), message.c_str());
            break;
        case WARNING:
			fprintf(log_stream, "%lu [ WARNING ] %s\n", get_current_time(), message.c_str());
            break;
        case ERROR:
			fprintf(log_stream, "%lu [ ERROR ] %s\n", get_current_time(), message.c_str());
            break;
        default:
            return;
        }
		fflush(log_stream);
	}
}
