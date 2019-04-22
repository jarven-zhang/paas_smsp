#ifndef PUBLIC_COMMOM_GLOBAL_H
#define PUBLIC_COMMOM_GLOBAL_H

#include "platform.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <vector>

using std::vector;

string join(const vector<string>& vec, cs_t strSeparator);
int json_format_string(cs_t src_str, string& out_json);
UInt32 md5Hash(cs_t str, UInt32 uiNumber);
UInt32 getTimeFromNowToMidnight();

inline UInt64 now_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (UInt64)tv.tv_sec * 1000000 + (UInt64)tv.tv_usec;
}
void trim(string& s);
string getCurrentDateTime(cs_t strFormat);


#endif // PUBLIC_COMMOM_GLOBAL_H
