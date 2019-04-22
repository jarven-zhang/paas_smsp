#include "AppData.h"
#include "LogMacro.h"


#define SET_DATA_STR(field, dbfield) \
    field = r[dbfield];

#define SET_DATA_UINT(type, field, dbfield) \
    field = to_uint<type>(r[dbfield]);

#define SET_DATA_UINT32(field, dbfield) \
    field = to_uint32(r[dbfield]);

#define SET_DATA_UINT64(field, dbfield) \
    field = to_uint64(r[dbfield]);

#define SET_DATA_DOUBLE(field, dbfield) \
    field = to_double(r[dbfield]);

#define PRE_COPY_DATA(appdata) \
    CHK_NULL_RETURN(pAppData); \
    appdata* pApp = (appdata*)pAppData;

#define COPY_DATA(field) \
    field = pApp->field;

//////////////////////////////////////////////////////////////////////////////////

void TSmsTblUpdateTime::setData(Record& r)
{
    SET_DATA_STR(m_id, "id");
    SET_DATA_STR(m_table_name, "tablename");
    SET_DATA_STR(m_update_time, "updatetime");
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsMiddlewareCfg::setData(Record& r)
{
    SET_DATA_UINT(UInt64, m_ulMiddlewareId, "middleware_id");
    SET_DATA_UINT(UInt32, m_uMiddleWareType, "middleware_type");
    SET_DATA_UINT(UInt16, m_uPort, "port");
    SET_DATA_UINT(UInt32, m_uiNodeId, "node_id");

    SET_DATA_STR(m_strMiddlewareName, "middleware_name");
    SET_DATA_STR(m_strHostIp, "host_ip");
    SET_DATA_STR(m_strUserName, "user_name");
    SET_DATA_STR(m_strPassword, "pass_word");
}

void TSmsMiddlewareCfg::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsMiddlewareCfg);

    COPY_DATA(m_ulMiddlewareId);
    COPY_DATA(m_uMiddleWareType);
    COPY_DATA(m_strMiddlewareName);
    COPY_DATA(m_strHostIp);
    COPY_DATA(m_uPort);
    COPY_DATA(m_strUserName);
    COPY_DATA(m_strPassword);
    COPY_DATA(m_uiNodeId);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsMqCfg::setData(Record& r)
{
    SET_DATA_UINT(UInt64, m_uMqId, "mq_id");
    SET_DATA_UINT(UInt64, m_uMiddleWareId, "middleware_id");
    SET_DATA_UINT(UInt64, m_uMqType, "mqtype");

    SET_DATA_STR(m_strQueue, "mq_queue");
    SET_DATA_STR(m_strExchange, "mq_exchange");
    SET_DATA_STR(m_strRoutingKey, "mq_routingkey");
    SET_DATA_STR(m_strRemark, "remark");
}

void TSmsMqCfg::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsMqCfg);

    COPY_DATA(m_uMqId);
    COPY_DATA(m_uMiddleWareId);
    COPY_DATA(m_uMqType);
    COPY_DATA(m_strQueue);
    COPY_DATA(m_strExchange);
    COPY_DATA(m_strRoutingKey);
    COPY_DATA(m_strRemark);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsMqThreadCfg::setData(Record& r)
{
    SET_DATA_UINT(UInt32, m_uMiddleWareType, "middleware_type");
    SET_DATA_UINT(UInt32, m_uiMode, "mode");
    SET_DATA_UINT(UInt32, m_uiThreadNum, "thread_num");

    SET_DATA_STR(m_strComponentType, "component_type");

    LogDebug("MiddleWareType:%u, Mode:%u, ComponentType:%s, ThreadNum:%u",
        m_uMiddleWareType,
        m_uiMode,
        m_strComponentType.data(),
        m_uiThreadNum);
}

void TSmsMqThreadCfg::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsMqThreadCfg);

    COPY_DATA(m_uMiddleWareType);
    COPY_DATA(m_uiMode);
    COPY_DATA(m_strComponentType);
    COPY_DATA(m_uiThreadNum);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsComponentCfg::setData(Record& r)
{
    SET_DATA_UINT(UInt64, m_uComponentId, "component_id");
    SET_DATA_UINT(UInt32, m_uNodeId, "node_id");
    SET_DATA_UINT(UInt32, m_uRedisThreadNum, "redis_thread_num");
    SET_DATA_UINT(UInt32, m_uSgipReportSwitch, "sgip_report_switch");
    SET_DATA_UINT(UInt64, m_uMqId, "mq_id");
    SET_DATA_UINT(UInt32, m_uProducerSwitch, "producer_switch");
    SET_DATA_UINT(UInt32, m_uComponentSwitch, "component_switch");
    SET_DATA_UINT(UInt32, m_uBlacklistSwitch, "black_list_switch");
    SET_DATA_UINT(UInt32, m_uMonitorSwitch, "monitor_switch");

    SET_DATA_STR(m_strComponentType, "component_type");
    SET_DATA_STR(m_strComponentName, "component_name");
    SET_DATA_STR(m_strHostIp, "host_ip");
}

void TSmsComponentCfg::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsComponentCfg);

    COPY_DATA(m_uComponentId);
    COPY_DATA(m_uNodeId);
    COPY_DATA(m_uRedisThreadNum);
    COPY_DATA(m_uSgipReportSwitch);
    COPY_DATA(m_uMqId);
    COPY_DATA(m_uProducerSwitch);
    COPY_DATA(m_uComponentSwitch);
    COPY_DATA(m_uBlacklistSwitch);
    COPY_DATA(m_uMonitorSwitch);
    COPY_DATA(m_strComponentType);
    COPY_DATA(m_strComponentName);
    COPY_DATA(m_strHostIp);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsComponentRefMqCfg::setData(Record& r)
{
    SET_DATA_UINT(UInt64, m_uId, "id");
    SET_DATA_UINT(UInt64, m_uComponentId, "component_id");
    SET_DATA_UINT(UInt64, m_uMqId, "mq_id");
    SET_DATA_UINT(UInt32, m_uGetRate, "get_rate");
    SET_DATA_UINT(UInt32, m_uMode, "mode");
    SET_DATA_UINT(UInt32, m_uWeight, "weight");
    SET_DATA_UINT(UInt32, m_uiPrefetchCount, "prefetch_count");
    SET_DATA_UINT(UInt32, m_uiMultipleAck, "multiple_ack");

    SET_DATA_STR(m_strMessageType, "message_type");
    SET_DATA_STR(m_strRemark, "remark");
}

void TSmsComponentRefMqCfg::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsComponentRefMqCfg);

    COPY_DATA(m_uId);
    COPY_DATA(m_uComponentId);
    COPY_DATA(m_uMqId);
    COPY_DATA(m_uGetRate);
    COPY_DATA(m_uMode);
    COPY_DATA(m_uWeight);
    COPY_DATA(m_uiPrefetchCount);
    COPY_DATA(m_uiMultipleAck);
    COPY_DATA(m_strMessageType);
    COPY_DATA(m_strRemark);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsParam::setData(Record& r)
{
    SET_DATA_STR(m_strParaKey, "param_key");
    SET_DATA_STR(m_strParaValue, "param_value");
}

void TSmsParam::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsParam);

    COPY_DATA(m_strParaKey);
    COPY_DATA(m_strParaValue);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsAgentInfo::setData(Record& r)
{
    SET_DATA_UINT(UInt32, m_uiAgentId, "agent_id");
    SET_DATA_UINT(UInt32, m_uiOperationMode, "operation_mode");
    SET_DATA_UINT(UInt32, m_uiPayType, "paytype");
}

void TSmsAgentInfo::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsAgentInfo);

    COPY_DATA(m_uiAgentId);
    COPY_DATA(m_uiOperationMode);
    COPY_DATA(m_uiPayType);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsAgentAccount::setData(Record& r)
{
    SET_DATA_UINT32(m_uiAgentId, "agent_id");
    SET_DATA_DOUBLE(m_dBalance, "balance");
    SET_DATA_DOUBLE(m_dCurrentCredit, "current_credit");
    SET_DATA_DOUBLE(m_dNoBackPayment, "no_back_payment");
}

void TSmsAgentAccount::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsAgentAccount);

    COPY_DATA(m_uiAgentId);
    COPY_DATA(m_dBalance);
    COPY_DATA(m_dCurrentCredit);
    COPY_DATA(m_dNoBackPayment);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsDirectClientGivePool::setData(Record& r)
{
    SET_DATA_UINT64(m_ulGivePoolId, "give_pool_id");
    SET_DATA_STR(m_strClientId, "clientid");
    SET_DATA_STR(m_strDueTime, "due_time");
    SET_DATA_UINT32(m_uiTotalNumber, "total_number");
    SET_DATA_UINT32(m_uiRemainNumber, "remain_number");
    SET_DATA_DOUBLE(m_dUnitPrice, "unit_price");
}

void TSmsDirectClientGivePool::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsDirectClientGivePool);

    COPY_DATA(m_ulGivePoolId);
    COPY_DATA(m_strClientId);
    COPY_DATA(m_strDueTime);
    COPY_DATA(m_uiTotalNumber);
    COPY_DATA(m_uiRemainNumber);
    COPY_DATA(m_dUnitPrice);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsUserPropertyLog::setData(Record& r)
{
    SET_DATA_UINT32(m_uiId, "id");
    SET_DATA_STR(m_strClientId, "clientid");
    SET_DATA_STR(m_strProperty, "property");
    SET_DATA_STR(m_strValue, "value");
    SET_DATA_STR(m_strEffectDate, "effect_date");
}

void TSmsUserPropertyLog::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsUserPropertyLog);

    COPY_DATA(m_uiId);
    COPY_DATA(m_strClientId);
    COPY_DATA(m_strProperty);
    COPY_DATA(m_strValue);
    COPY_DATA(m_strEffectDate);
}

//////////////////////////////////////////////////////////////////////////////////

void TSmsAccount::setData(Record& r)
{
    SET_DATA_STR(m_strClientId, "clientid");
    SET_DATA_UINT32(m_uiSmsType, "smstype");
}

void TSmsAccount::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TSmsAccount);

    COPY_DATA(m_strClientId);
    COPY_DATA(m_uiSmsType);
}

//////////////////////////////////////////////////////////////////////////////////

void TsmsClientSendLimit::setData(Record& r)
{
    SET_DATA_STR(m_strClientId, "clientid");
    SET_DATA_UINT32(m_uiLimitType, "limit_type");
    SET_DATA_UINT32(m_uiSmsType, "smstype");
    SET_DATA_DOUBLE(m_dLimitRate, "limit_rate");
    SET_DATA_STR(m_strLimitTimeStart, "limit_time_start");
    SET_DATA_STR(m_strLimitTimeEnd, "limit_time_end");
    SET_DATA_STR(m_strUpdateTime, "update_time");

    LogDebug("clientid[%s],limit_type[%u],smstype[%u],limit_rate[%lf],"
        "limit_time_start[%s],limit_time_end[%s],update_time[%s].",
        m_strClientId.data(),
        m_uiLimitType,
        m_uiSmsType,
        m_dLimitRate,
        m_strLimitTimeStart.data(),
        m_strLimitTimeEnd.data(),
        m_strUpdateTime.data());
}

void TsmsClientSendLimit::copyData(AppData* pAppData)
{
    PRE_COPY_DATA(TsmsClientSendLimit);

    COPY_DATA(m_strClientId);
    COPY_DATA(m_uiLimitType);
    COPY_DATA(m_uiSmsType);
    COPY_DATA(m_dLimitRate);
    COPY_DATA(m_strLimitTimeStart);
    COPY_DATA(m_strLimitTimeEnd);
    COPY_DATA(m_strUpdateTime);
}


