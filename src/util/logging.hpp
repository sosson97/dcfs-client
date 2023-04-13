#ifndef __LOGGING_H
#define __LOGGING_H
#include <string>

enum LogLevel
{
    LDEBUG,
    INFO,
    WARNING,
    ERROR
};

namespace Logger
{
    void log(LogLevel level, const std::string &message);
};

#endif // __LOGGING_H