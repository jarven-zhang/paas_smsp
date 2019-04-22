#ifndef BLACKLIST_H_
#define BLACKLIST_H_
#include "hiredis.h"
#include "iostream"
#include "platform.h"

using namespace std;

namespace models
{
    typedef enum{
       LAXINBLACKLIST       = 1,
       HYBLACKLIST          = 2,
       YXBLACKLIST          = 4,
       GLOBALBLACKLIST      = 8,
       
    }blacklist_class_t;

    class BlacklistInfo
    {
    public:
        BlacklistInfo()
        {
        }
        
        ~BlacklistInfo()
        {
        }
        std::string m_strClientId;
        std::string m_strPhone;
    };

    class Blacklist
    {
    public:
        Blacklist()
        {
        }
        
        ~Blacklist()
        {
        }
        
    static Int16 GetSmsTypeFlag(string& strSmsType);
    };
}
#endif

