#include "MqManagerThread.h"
#include "MqThreadPool.h"
#include "MqProducerThreadPool.h"
#include "MqConsumerThreadPool.h"
#include "MqThread.h"
#include "RuleLoadThread.h"
#include "CMQProducerThread.h"
#include "LogMacro.h"
#include "commonmessage.h"
#include "Fmt.h"

#include "boost/foreach.hpp"

#define foreach BOOST_FOREACH

MqManagerThread* g_pMqManagerThread = NULL;

const UInt64 TIMER_ID_MQ_PROCESS_THREAD_SUMMARY = 1000;

extern UInt64 g_uComponentId;

#define BETWEEN(val, min, max) (((val) >= min) && ((val) <= max))
#define CHK_INT_RETURN(var, min, max) CHK_INT_(var, min, max, return)
#define CHK_INT_(var, min, max, code)                                       \
        if (((var) < (min)) || ((var) > (max)))                            \
        {                                                                   \
            LogError("Invalid system parameter(%s) value(%s), %s(%d).",     \
                strSysPara.c_str(),                                         \
                iter->second.c_str(),                                       \
                VNAME(var),                                                 \
                var);                                                       \
            code;                                                           \
        }

//////////////////////////////////////////////////////////////////////////////////////////

bool publishMqMsg_(cs_t strExchange, cs_t strRoutingkey, cs_t strkey, cs_t strData, int iMiddlewareType)
{
    CHK_NULL_RETURN_FALSE(g_pMqManagerThread);

    MiddleMsg<MqPublishReqMsg>* pReq = new MiddleMsg<MqPublishReqMsg>();
    CHK_NULL_RETURN_FALSE(pReq);
    pReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;

    MqPublishReqMsg* pMqReq = pReq->m_pMsg;
    CHK_NULL_RETURN_FALSE(pMqReq);
    pMqReq->m_strData = strData;
    pMqReq->m_strExchange = strExchange;
    pMqReq->m_strRoutingKey = strRoutingkey;
    pMqReq->m_strKey = strkey;
    pMqReq->m_uiMiddlewareType = iMiddlewareType;
    pMqReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;

    g_pMqManagerThread->PostMsg(pReq);
    return true;
}

bool publishMqC2sIoMsg(cs_t strExchange, cs_t strRoutingkey, cs_t strData)
{
    return publishMqMsg_(strExchange, strRoutingkey, "", strData, MIDDLEWARE_TYPE_MQ_C2S_IO);
}

bool publishMqSendIoMsg(cs_t strExchange, cs_t strRoutingkey, cs_t strData)
{
    return publishMqMsg_(strExchange, strRoutingkey, "", strData, MIDDLEWARE_TYPE_MQ_SEND_IO);
}

bool publishMqC2sDbMsg(cs_t strKey, char* sql)
{
    return publishMqMsg_("", "", strKey, string(sql) + "RabbitMQFlag=" + strKey, MIDDLEWARE_TYPE_MQ_C2S_DB);
}

bool publishMqC2sDbMsg(cs_t strKey, cs_t strSql)
{
    return publishMqMsg_("", "", strKey, strSql, MIDDLEWARE_TYPE_MQ_C2S_DB);
}

bool publishMqSendDbMsg(cs_t strKey, cs_t strData)
{
    return publishMqMsg_("", "", strKey, strData, MIDDLEWARE_TYPE_MQ_SNED_DB);
}

//////////////////////////////////////////////////////////////////////////////////////////

MqManagerThread::MqManagerThread()
    :   CThread("MqManagerThread"),
        m_pMqThreadProcessSummary(NULL),
        m_ulMqC2sIoPThdTotalQueueSize(0)
{
    for (UInt32 i = 0; i < MQ_USE_NUM; ++i)
    {
        m_aryThreadPoolMgr[i] = NULL;
        m_aryMqConsumeCb[i] = NULL;
    }
}

MqManagerThread::~MqManagerThread()
{
}

bool MqManagerThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    GET_RULELOAD_DATA_MAP(t_sms_middleware_configure, m_mapMiddlewareCfg);
    GET_RULELOAD_DATA_MAP(t_sms_mq_configure, m_mapMqCfg);
    GET_RULELOAD_DATA_MAP(t_sms_mq_thread_configure, m_mapMqThreadCfg);
    GET_RULELOAD_DATA_MAP(t_sms_component_configure, m_mapComponentCfg);
    GET_RULELOAD_DATA_MAP(t_sms_component_ref_mq, m_mapComponentRefMq);

    INIT_CHK_FUNC_RET_FALSE(initMqThreadPool());

    m_pMqThreadProcessSummary = SetTimer(TIMER_ID_MQ_PROCESS_THREAD_SUMMARY, "", 10*1000);

    return true;
}

bool MqManagerThread::initMqThreadPool()
{
    foreach (AppDataMapValueType& iter, m_mapComponentRefMq)
    {
        const TSmsComponentRefMqCfg& refMq = *(TSmsComponentRefMqCfg*)(iter.second);

        if (g_uComponentId == refMq.m_uComponentId)
        {
            LogDebug("ComponentId:%lu, MqId:%u, Mode:%u.",
                g_uComponentId, refMq.m_uMqId, refMq.m_uMode);

            AppDataMapCIter iter1;
            INIT_CHK_MAP_FIND_STR_RET_FALSE(m_mapMqCfg, iter1, Fmt("%lu", refMq.m_uMqId));
            const TSmsMqCfg& mqCfg = *(TSmsMqCfg*)(iter1->second);

            AppDataMapCIter iter2;
            INIT_CHK_MAP_FIND_STR_RET_FALSE(m_mapMiddlewareCfg, iter2, to_string(mqCfg.m_uMiddleWareId));
            const TSmsMiddlewareCfg& middlewareCfg = *(TSmsMiddlewareCfg*)(iter2->second);

            INIT_CHK_FUNC_RET_FALSE(createMqThreadPool(middlewareCfg, refMq.m_uMode));
        }
    }

    return true;
}

bool MqManagerThread::createMqThreadPool(const TSmsMiddlewareCfg& mw, UInt32 uiMode)
{
    LogDebug("MiddleWareId[%lu], MiddleWareType[%u], Mode[%u].",
        mw.m_ulMiddlewareId, mw.m_uMiddleWareType, uiMode);

    int type = (PRODUCT_MODE == uiMode)?(mw.m_uMiddleWareType - 1):(mw.m_uMiddleWareType + 3);
    if ((type < 0) || (type > 7)) return true;

    MqThreadPool* pMqThreadPool = m_aryThreadPoolMgr[type];
    if (NULL != pMqThreadPool) return true;

    //////////////////////////////////////////////////////////////////////

    switch (type)
    {
        case PRODUCE_TO_MQ_C2S_IO:
        case PRODUCE_TO_MQ_SEND_IO:
        case PRODUCE_TO_MQ_C2S_DB:
        case PRODUCE_TO_MQ_SEND_DB:
            pMqThreadPool = new MqProducerThreadPool();
            break;
        case CONSUME_FROM_MQ_C2S_IO:
        case CONSUME_FROM_MQ_SEND_IO:
        case CONSUME_FROM_MQ_C2S_DB:
        case CONSUME_FROM_MQ_SEND_DB:
            pMqThreadPool = new MqConsumerThreadPool();
            break;
        default:
            return false;
    }

    pMqThreadPool->m_pMgr = this;
    pMqThreadPool->m_uiMqThreadPoolType = type;
    pMqThreadPool->m_uiMode = uiMode;
    pMqThreadPool->m_middleWareConfig = mw;
    INIT_CHK_FUNC_RET_FALSE(pMqThreadPool->init());
    INIT_CHK_FUNC_RET_FALSE(pMqThreadPool->start());

    m_aryThreadPoolMgr[type] = pMqThreadPool;

    return true;
}

vector<CThread*> MqManagerThread::getAllThreads()
{
    vector<CThread*> vecThreads;

    for (UInt32 i = 0; i < MQ_USE_NUM; ++i)
    {
        MqThreadPool* pMqThreadPool = m_aryThreadPoolMgr[i];

        if (NULL != pMqThreadPool)
        {
            foreach (CThread* pThread, pMqThreadPool->getAllThreads())
            {
                vecThreads.push_back(pThread);
            }
        }
    }

    return vecThreads;
}

UInt32 MqManagerThread::getMqC2sIoPThdTotalQueueSize()
{
    for (UInt32 i = 0; i < MQ_USE_NUM; ++i)
    {
        MqThreadPool* pMqThreadPool = m_aryThreadPoolMgr[i];

        if (PRODUCE_TO_MQ_C2S_IO == pMqThreadPool->m_uiMqThreadPoolType)
        {
            if (pMqThreadPool->getLinkStatus())
            {
                return 0;
            }
            else
            {
                // 链接不OK的时候再去获取队列堆积的消息数量
                return pMqThreadPool->getTotalQueueSize();
            }
        }
    }

    return 0;
}

void MqManagerThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_TIMEOUT:
        {
            handleTimeoutReq(pMsg);
            break;
        }
        case MSGTYPE_MQ_PUBLISH_REQ:
        {
            handleMqPublishReq(pMsg);
            break;
        }
        case MSGTYPE_MQ_GETMQMSG_REQ:
        {
            handleMqGetMsgReq(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_DB_UPDATE_REQ:
        {
            handleDbUpdateReq(pMsg);
            break;
        }
        default:
        {
            LogError("Invalid MsgType(%lu).", pMsg->m_iMsgType);
            break;
        }
    }
}

void MqManagerThread::handleTimeoutReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    string strInfo;
    foreach (CThread* pThread, getAllThreads())
    {
        strInfo.append("[" + pThread->GetThreadName() + ":");
        strInfo.append("(" + to_string(pThread->GetMsgSize())  + ")");
        strInfo.append("(" + to_string(pThread->GetMsgCount()) + ")] ");

        pThread->SetMsgCount(0);
    }

    LogFatal("QueueSize %s", strInfo.data());

    SAFE_DELETE(m_pMqThreadProcessSummary);
    m_pMqThreadProcessSummary = SetTimer(TIMER_ID_MQ_PROCESS_THREAD_SUMMARY, "", 10*1000);
}

void MqManagerThread::handleMqPublishReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    MiddleMsg<MqPublishReqMsg>* pMiddleMsg = (MiddleMsg<MqPublishReqMsg>*)pMsg;

    MqPublishReqMsg* pReq = pMiddleMsg->m_pMsg;
    CHK_NULL_RETURN(pReq);

    int type = pReq->m_uiMiddlewareType - 1;
    if ((type < 0) || (type > 3))
    {
        LogError("Invalid MiddlewareType[%u], discard message.", pReq->m_uiMiddlewareType);
        return;
    }

    MqProducerThreadPool* pMqProducerThreadPool = (MqProducerThreadPool*)(m_aryThreadPoolMgr[type]);
    CHK_NULL_RETURN(pMqProducerThreadPool);
    pMqProducerThreadPool->postMsg(pReq);
}

void MqManagerThread::handleMqGetMsgReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    MiddleMsg<MqConsumeReqMsg>* pMiddleMsg = (MiddleMsg<MqConsumeReqMsg>*)pMsg;

    MqConsumeReqMsg* pReq = pMiddleMsg->m_pMsg;
    CHK_NULL_RETURN(pReq);

    CThread* pThread = m_aryMqConsumeCb[pReq->m_uiType];

    if (NULL != pThread)
    {
        LogDebug("==>send mq msg to %s<==", pThread->GetThreadName().data());
        pThread->PostMsg(pReq);
    }
}

void MqManagerThread::handleDbUpdateReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    DbUpdateReq* pReq = (DbUpdateReq*)pMsg;

    switch (pReq->m_uiTableId)
    {
        case t_sms_mq_thread_configure:
        {
            clearAppData(m_mapMqThreadCfg);
            m_mapMqThreadCfg = pReq->m_mapAppData;
            break;
        }
        case t_sms_component_configure:
        {
            clearAppData(m_mapComponentCfg);
            m_mapComponentCfg = pReq->m_mapAppData;
            break;
        }
        case t_sms_component_ref_mq:
        {
            clearAppData(m_mapComponentRefMq);
            m_mapComponentRefMq = pReq->m_mapAppData;
            break;
        }
        case t_sms_param:
        {
            handleSmsParamUpdateReq(pReq);
            break;
        }
        default:
        {
            LogError("Invalid TableId:%u.", pReq->m_uiTableId);
            break;
        }
    }


    for (UInt32 i = 0; i < MQ_USE_NUM; ++i)
    {
        MqThreadPool* pMqThreadPool = m_aryThreadPoolMgr[i];

        if (NULL != pMqThreadPool)
        {
            pMqThreadPool->handleDbUpdateReq(pReq);
        }
    }
}

void MqManagerThread::handleSmsParamUpdateReq(DbUpdateReq* pReq)
{
    CHK_NULL_RETURN(pReq);
}

