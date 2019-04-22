#ifndef AGENTINFO_H_
#define AGENTINFO_H_

#include "platform.h"

#include <string>
#include <map>
#include <iostream>

using namespace std;

enum AGENTTYPE
{
    AGENT_OTHER = 0,
    XIAOSHOU_DLS = 1,       // 销售代理商
    PINPAI_DLS = 2,         // 品牌代理商
    ZIYUAN_HZS = 3,         // 资源合作商
    DLS_ZIYUAN_HEZUO = 4,   // 代理商和资源合作
    OEM_DLS = 5,            // OEM代理商
    CLOUD_PT = 6,           // 云平台
};

namespace models
{
class AgentInfo
{
public:
    AgentInfo();
    virtual ~AgentInfo();
    AgentInfo(const AgentInfo& other);
    AgentInfo& operator =(const AgentInfo& other);

public:

    int m_iAgent_type;
    int m_iOperation_mode;
    int m_iAgentPaytype;
};

typedef std::map<UInt64, AgentInfo> 	AgentInfoMap;

}

#endif
