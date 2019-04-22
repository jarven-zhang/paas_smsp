#ifndef MQTHREADCONFIG_H
#define MQTHREADCONFIG_H

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

class MqThreadConfig
{
public:
    MqThreadConfig()
    {
        m_uMiddleWareType = 0;
        m_uiMode = 0;
        m_uiComponentType = 0;
        m_uiThreadNum = 0;
    }

    ~MqThreadConfig() {}

public:
    UInt32 m_uMiddleWareType;
    UInt32 m_uiMode;
    UInt32 m_uiComponentType;
    UInt32 m_uiThreadNum;
};

typedef map<string, MqThreadConfig> MqThreadCfgMap;
typedef map<string, MqThreadConfig>::iterator MqThreadCfgMapIter;
typedef map<string, MqThreadConfig>::const_iterator MqThreadCfgMapCIter;

#endif // MQTHREADCONFIG_H
