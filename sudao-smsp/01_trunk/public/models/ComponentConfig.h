#ifndef __COMPONENTCONFIG__
#define __COMPONENTCONFIG__

#include "platform.h"

#include <string>
#include <map>

using std::string;
using std::map;

const string COMPONENT_TYPE_C2S      = "00";
const string COMPONENT_TYPE_ACCESS   = "01";
const string COMPONENT_TYPE_SEND     = "02";
const string COMPONENT_TYPE_REPORT   = "03";
const string COMPONENT_TYPE_AUDIT    = "04";
const string COMPONENT_TYPE_CHARGE   = "05";
const string COMPONENT_TYPE_CONSUMER = "06";
const string COMPONENT_TYPE_REBACK   = "07";
const string COMPONENT_TYPE_HTTP     = "08";
const string COMPONENT_TYPE_MONITOR  = "09";

class ComponentConfig
{
public:
    ComponentConfig();
    ~ComponentConfig();

public:
    UInt64  m_uComponentId;
    string  m_strComponentType;
    string  m_strComponentName;
    string  m_strHostIp;
    UInt32  m_uNodeId;
    UInt32  m_uRedisThreadNum;
    UInt32  m_uSgipReportSwitch;  ////0 is close,1 is open
    UInt64  m_uMqId;
    UInt32  m_uComponentSwitch;   //组件开关，0:关闭，1:打开
    UInt32  m_uBlacklistSwitch;  //reload blacklist switch : 0:close ,1:open  only for comsumer_access
    UInt32  m_uMonitorSwitch;   //监控开关, 0: 关闭，1:开启

};

typedef map<string, ComponentConfig> ComponentConfigMap;
typedef ComponentConfigMap::iterator ComponentConfigMapIter;

#endif ////__COMPONENTCONFIG__
