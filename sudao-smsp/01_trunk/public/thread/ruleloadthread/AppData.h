#ifndef THREAD_RULELOADTHREAD_APP_DATA_H
#define THREAD_RULELOADTHREAD_APP_DATA_H

#include "platform.h"
#include "CThreadSQLHelper.h"
#include "Fmt.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

/////////////////////////////////////////////////////////////////////////////

class AppData
{
public:
    AppData() {}
    virtual ~AppData() {}

    virtual void setData(Record& r) = 0;
    virtual void copyData(AppData* pAppData) = 0;
    virtual string getKey() = 0;
    virtual bool specialHandle() { return true; }

public:
    // table field

public:
    // other field
};


class TSmsTblUpdateTime : public AppData
{
public:
    TSmsTblUpdateTime() {}
    virtual ~TSmsTblUpdateTime() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData) {}
    virtual string getKey() {return m_table_name;}

public:
    string m_id;
    string m_table_name;
    string m_update_time;
};

class TSmsMiddlewareCfg : public AppData
{
public:
    TSmsMiddlewareCfg()
        :m_ulMiddlewareId(0),m_uMiddleWareType(0),m_uPort(0),m_uiNodeId(0) {}
    virtual ~TSmsMiddlewareCfg() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_ulMiddlewareId);}

public:
    UInt64 m_ulMiddlewareId;
    UInt32 m_uMiddleWareType;
    string m_strMiddlewareName;
    string m_strHostIp;
    UInt16 m_uPort;
    string m_strUserName;
    string m_strPassword;
    UInt32 m_uiNodeId;
};

class TSmsMqCfg : public AppData
{
public:
    TSmsMqCfg():m_uMqId(0),m_uMiddleWareId(0),m_uMqType(0) {}
    virtual ~TSmsMqCfg() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_uMqId);}

public:
    UInt64  m_uMqId;
    UInt64  m_uMiddleWareId;
    UInt64  m_uMqType;
    string  m_strQueue;
    string  m_strExchange;
    string  m_strRoutingKey;
    string  m_strRemark;
};

class TSmsMqThreadCfg : public AppData
{
public:
    TSmsMqThreadCfg():m_uMiddleWareType(0),m_uiMode(0),m_uiThreadNum(0) {}
    virtual ~TSmsMqThreadCfg() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return Fmt<32>("%u_%u_%s",m_uMiddleWareType,m_uiMode,m_strComponentType.data());}

public:
    UInt32 m_uMiddleWareType;
    UInt32 m_uiMode;
    string m_strComponentType;
    UInt32 m_uiThreadNum;
};

class TSmsComponentCfg : public AppData
{
public:
    TSmsComponentCfg()
        :m_uComponentId(0),m_uNodeId(0),m_uRedisThreadNum(0),
        m_uSgipReportSwitch(0),m_uMqId(0),m_uProducerSwitch(0),
        m_uComponentSwitch(0),m_uBlacklistSwitch(0),m_uMonitorSwitch(0) {}
    virtual ~TSmsComponentCfg() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_uComponentId);}

public:
    UInt64  m_uComponentId;
    string  m_strComponentType;
    string  m_strComponentName;
    string  m_strHostIp;
    UInt32  m_uNodeId;
    UInt32  m_uRedisThreadNum;
    UInt32  m_uSgipReportSwitch;  ////0 is close,1 is open
    UInt64  m_uMqId;
    UInt32  m_uProducerSwitch;   // 组件生产者开关，0:关闭，1:打开
    UInt32  m_uComponentSwitch;   //组件开关，0:关闭，1:打开
    UInt32  m_uBlacklistSwitch;  //reload blacklist switch : 0:close ,1:open  only for comsumer_access
    UInt32  m_uMonitorSwitch;   //监控开关, 0: 关闭，1:开启
};

class TSmsComponentRefMqCfg : public AppData
{
public:
    TSmsComponentRefMqCfg()
        :m_uId(0),m_uComponentId(0),m_uMqId(0),
        m_uGetRate(0),m_uMode(0),m_uWeight(0),
        m_uiPrefetchCount(0),m_uiMultipleAck(0) {}
    virtual ~TSmsComponentRefMqCfg() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_uId);}

public:
    UInt64  m_uId;
    UInt64  m_uComponentId;
    string  m_strMessageType;
    UInt64  m_uMqId;
    UInt32  m_uGetRate;
    UInt32  m_uMode;
    string  m_strRemark;
    UInt32  m_uWeight;
    UInt32 m_uiPrefetchCount;
    UInt32 m_uiMultipleAck;
};

class TSmsParam : public AppData
{
public:
    TSmsParam() {}
    virtual ~TSmsParam() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return m_strParaKey;}

public:
    string  m_strParaKey;
    string  m_strParaValue;
};

class TSmsAgentInfo : public AppData
{
public:
    TSmsAgentInfo():m_uiAgentId(0),m_uiOperationMode(0),m_uiPayType(0) {}
    virtual ~TSmsAgentInfo() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_uiAgentId);}

public:
    UInt32 m_uiAgentId;
    UInt32 m_uiOperationMode;
    UInt32 m_uiPayType;
};

class TSmsAgentAccount : public AppData
{
public:
    TSmsAgentAccount()
        :m_uiAgentId(0),m_dBalance(0),m_dCurrentCredit(0),m_dNoBackPayment(0) {}
    virtual ~TSmsAgentAccount() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return to_string(m_uiAgentId);}

public:
    UInt32 m_uiAgentId;
    double m_dBalance;
    double m_dCurrentCredit;
    double m_dNoBackPayment;
};

class TSmsDirectClientGivePool : public AppData
{
public:
    TSmsDirectClientGivePool()
        :m_ulGivePoolId(0),m_uiTotalNumber(0),m_uiRemainNumber(0),m_dUnitPrice(0) {}
    virtual ~TSmsDirectClientGivePool() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return m_strClientId;}

public:
    UInt64 m_ulGivePoolId;
    string m_strClientId;
    string m_strDueTime;
    UInt32 m_uiTotalNumber;
    UInt32 m_uiRemainNumber;
    double m_dUnitPrice;
};

class TSmsUserPropertyLog : public AppData
{
public:
    TSmsUserPropertyLog():m_uiId(0) {}
    virtual ~TSmsUserPropertyLog() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return m_strClientId + ":" + m_strProperty;}

public:
    UInt32 m_uiId;
    string m_strClientId;
    string m_strProperty;
    string m_strValue;
    string m_strEffectDate;
};

class TSmsAccount : public AppData
{
public:
    TSmsAccount():m_uiSmsType(0) {}
    virtual ~TSmsAccount() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return m_strClientId;}

public:
    string m_strClientId;
    UInt32 m_uiSmsType;
};

class TsmsClientSendLimit : public AppData
{
public:
    TsmsClientSendLimit():m_uiLimitType(0),m_uiSmsType(0),m_dLimitRate(0) {}
    virtual ~TsmsClientSendLimit() {}

    virtual void setData(Record& r);
    virtual void copyData(AppData* pAppData);
    virtual string getKey() {return Fmt<64>("%s_%u_%u", m_strClientId.data(), m_uiLimitType, m_uiSmsType);}

public:
    string m_strClientId;
    UInt32 m_uiLimitType;
    UInt32 m_uiSmsType;
    double m_dLimitRate;
    string m_strLimitTimeStart;
    string m_strLimitTimeEnd;
    string m_strUpdateTime;
};















/////////////////////////////////////////////////////////////////////////////

typedef map<string, AppData*> AppDataMap;
typedef map<string, AppData*>::iterator AppDataMapIter;
typedef map<string, AppData*>::const_iterator AppDataMapCIter;
typedef map<string, AppData*>::value_type AppDataMapValueType;

typedef vector<AppData*> AppDataVec;
typedef vector<AppData*>::iterator AppDataVecIter;

typedef map<string, AppDataVec> AppDataVecMap;
typedef map<string, AppDataVec>::iterator AppDataVecMapIter;
typedef map<string, AppDataVec>::value_type AppDataVecMapValueType;

/////////////////////////////////////////////////////////////////////////////

typedef TSmsTblUpdateTime           AppData_t_sms_table_update_time;
typedef TSmsMiddlewareCfg           AppData_t_sms_middleware_configure;
typedef TSmsMqCfg                   AppData_t_sms_mq_configure;
typedef TSmsMqThreadCfg             AppData_t_sms_mq_thread_configure;
typedef TSmsComponentCfg            AppData_t_sms_component_configure;
typedef TSmsComponentRefMqCfg       AppData_t_sms_component_ref_mq;
typedef TSmsParam                   AppData_t_sms_param;
typedef TSmsAgentInfo               AppData_t_sms_agent_info;
typedef TSmsAgentAccount            AppData_t_sms_agent_account;
typedef TSmsDirectClientGivePool    AppData_t_sms_direct_client_give_pool;
typedef TSmsUserPropertyLog         AppData_t_sms_user_property_log;
typedef TSmsAccount                 AppData_t_sms_account;
typedef TsmsClientSendLimit         AppData_t_sms_client_send_limit;




#endif // THREAD_RULELOADTHREAD_APP_DATA_H
