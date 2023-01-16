#include <vector>
#include <string>

namespace art
{

std::vector<std::string> SplitString(std::string& str, const char* delim);

uint64_t NowMicros();

}