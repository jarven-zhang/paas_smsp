#ifndef __MQCONFIG__
#define __MQCONFIG__

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

enum
{
    MQTYPE_NORMAL = 0,
    MQTYPE_PRIORITY = 1
};

class MqConfig
{
public:
    MqConfig();
    ~MqConfig();

public:
    UInt64  m_uMqId;
    UInt64  m_uMiddleWareId;
    UInt64  m_uMqType;
    string  m_strQueue;
    string  m_strExchange;
    string  m_strRoutingKey;
    string  m_strRemark;
};

typedef map<UInt64, MqConfig> MqConfigMap;
typedef MqConfigMap::iterator MqConfigMapIter;

#endif ////__MQCONFIG__
