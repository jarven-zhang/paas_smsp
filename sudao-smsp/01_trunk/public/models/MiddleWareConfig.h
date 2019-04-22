#ifndef __MIDDLEWARECONFIG__
#define __MIDDLEWARECONFIG__

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

const UInt32 MIDDLEWARE_TYPE_REDIS      = 0;
const UInt32 MIDDLEWARE_TYPE_MQ_C2S_IO  = 1;
const UInt32 MIDDLEWARE_TYPE_MQ_SEND_IO = 2;
const UInt32 MIDDLEWARE_TYPE_MQ_C2S_DB  = 3;
const UInt32 MIDDLEWARE_TYPE_MQ_SNED_DB = 4;
const UInt32 MIDDLEWARE_TYPE_KAFKA      = 5;
const UInt32 MIDDLEWARE_TYPE_REDIS_OVER_RATE = 6;
const UInt32 MIDDLEWARE_TYPE_MQ_MONITOR = 7;


class MiddleWareConfig
{
public:
    MiddleWareConfig();
    ~MiddleWareConfig();

public:
    UInt64 m_uMiddleWareId;
    UInt32 m_uMiddleWareType;
    string m_strMiddleWareName;
    string m_strHostIp;
    UInt32 m_uPort;
    string m_strUserName;
    string m_strPassWord;
    UInt32 m_uNodeId;
};

typedef map<UInt64, MiddleWareConfig> MiddleWareConfigMap;

#endif ////__MIDDLEWARECONFIG__