#include "HttpParams.h"

//LOG("smsp");
namespace web
{
    HttpParams::HttpParams()
    {
        // TODO Auto-generated constructor stub

    }

    HttpParams::~HttpParams()
    {
        // TODO Auto-generated destructor stub
    }

    void HttpParams::Parse(const string& query)
    {
        vector<string> pair;
        splitEx(query, "&", pair);
        std::string::size_type pos = 0;
        for (std::vector<std::string>::iterator ite = pair.begin();
             ite != pair.end(); ite++)
        {
//      LogDebug("param:%s", (*ite).data());
            pos = (*ite).find('=');
            if (pos != std::string::npos)
            {
                _map[(*ite).substr(0, pos)] = (*ite).substr(pos + 1);
//          LogInfo("key:%s,value:%s", (*ite).substr(0, pos).data(),
//                  _map[(*ite).substr(0, pos)].data());
            }
            else
            {
                //LogCrit("parse url param err");
            }
        }
    }
    string HttpParams::getValue(string key)
    {
        return _map[key];
    }

    void HttpParams::splitEx(const string& src, string separate_character,
                             vector<string>& strs)
    {
        int separate_characterLen = separate_character.size(); //分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
        int lastPosition = 0, index = -1;
        while (-1 != (index = src.find(separate_character, lastPosition)))
        {
            strs.push_back(src.substr(lastPosition, index - lastPosition));
            lastPosition = index + separate_characterLen;
        }
        string lastString = src.substr(lastPosition); //截取最后一个分隔符后的内容
        if (!lastString.empty())
            strs.push_back(lastString); //如果最后一个分隔符后还有内容就入队

    }

}
