#include "global.h"
#include "LogMacro.h"
#include "ssl/md5.h"

#include "boost/algorithm/string.hpp"


string join(const vector<string>& vec, cs_t strSeparator)
{
    return boost::algorithm::join(vec, strSeparator);
}
UInt32 md5Hash(cs_t str, UInt32 uiNumber)
{
    if (str.empty() || (0 == uiNumber))
    {
//        LogDebug("str:%s, num:%u.", str.data(), uiNumber);
        return 0;
    }

    unsigned char md5[17] = {0};
    MD5((const unsigned char*)str.data(), str.length(), md5);

//    for (int i = 0; i < 17; i++)
//    {
//        LogDebug("SmsId[%s], num[%u], md5[%x].", str.data(), uiNumber, md5[i]);
//    }

//    string strMd5;
//    std::string HEX_CHARS = "0123456789abcdef";
//    for (int i = 0; i < 16; i++)
//    {
//        strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
//        strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
//    }
//    LogDebug("strMd5[%s].", strMd5.data());
//    UInt64 uKey = stoull(strMd5.substr(0, 4).data(), 0, 16);

    // After optimization
    UInt32 key = (md5[0] << 8) + md5[1];

    return key % uiNumber;
}


int json_format_string(cs_t src_str, string& out_json)
{
    typedef struct str_t
    {
        char *text; /*<! char c-string */
        unsigned int size;  /*<! usable memory allocated to text minus the space for the nul character */
    } str_t;

    unsigned int pos = 0;
    unsigned int start_pos = 0;
    unsigned int end_pos = 0;
    unsigned int cnt_pos = 0;
    str_t text_str;

    if (src_str.empty())
        return -1;

    text_str.size = src_str.length();
    text_str.text = (char*)src_str.data();

    while (pos < text_str.size)
    {
        if (text_str.text[pos] == '{')
        {
            start_pos = pos;
            break;
        }
        pos++;
    }

    if (pos == text_str.size)
        return -1;

    pos = text_str.size-1;
    while (pos > 0)
    {
        if (text_str.text[pos] == '}')
        {
            end_pos = pos;
            break;
        }
        pos--;
    }

    if (pos == 0)
        return -1;

    cnt_pos = end_pos - start_pos + 1;
    out_json.append(text_str.text+start_pos, cnt_pos);
    return 0;
}

UInt32 getTimeFromNowToMidnight()
{
    // 获取当前时间点距零点的秒数
    long curtime = time(NULL);
    struct tm tblock = {0};
    localtime_r(&curtime, &tblock);
    int h = tblock.tm_hour;
    return ((24 - h) * 60 * 60 - tblock.tm_min * 60 - tblock.tm_sec);
}

void trim(string& s)
{
    if (s.empty()) return;

    string::iterator c;

    for (c = s.begin(); c != s.end() && iswspace(*c++);)
        ;

    s.erase(s.begin(), --c);

    for (c = s.end(); c != s.begin() && iswspace(*--c);)
        ;

    s.erase(++c, s.end());
}

string getCurrentDateTime(cs_t strFormat)
{
    // "%Y-%m-%d %H:%M:%S"
    time_t iCurtime = time(NULL);
    char szreachtime[64] = { 0 };
    struct tm pTime = {0};
    localtime_r((time_t*) &iCurtime,&pTime);

    strftime(szreachtime, sizeof(szreachtime), strFormat.data(), &pTime);
    return szreachtime;
}

