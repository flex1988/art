#include "util.h"
#include <sys/time.h>

namespace art
{

std::vector<std::string> SplitString(std::string& str, const char* delim)
{
    std::vector<std::string> str_vec;
    std::string tmp = "";
    for (uint32_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == *delim)
        {
            str_vec.push_back(tmp);
            tmp.clear();
            continue;
        }

        tmp += str[i];
    }

    str_vec.push_back(tmp);
    return str_vec;
}

uint64_t NowMicros()
{
    static constexpr uint64_t kUsecondsPerSecond = 1000000;
    struct ::timeval tv;
    ::gettimeofday(&tv, nullptr);
    return static_cast<uint64_t>(tv.tv_sec) * kUsecondsPerSecond + tv.tv_usec;
}

}