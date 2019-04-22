#include "MqThreadPool.h"
#include "MqThread.h"
#include "MqThreadConfig.h"
#include "MqManagerThread.h"
#include "commonmessage.h"
#include "RuleLoadThread.h"
#include "MqEnum.h"
#include "Fmt.h"
#include "LogMacro.h"

#include "boost/foreach.hpp"


#define foreach BOOST_FOREACH

extern UInt64 g_uComponentId;


MqThreadPool::MqThreadPool()
{
}

MqThreadPool::~MqThreadPool()
{
}

bool MqThreadPool::init()
{
    INIT_CHK_FUNC_RET_FALSE(getName());
    INIT_CHK_FUNC_RET_FALSE(getThreadNum());
    INIT_CHK_FUNC_RET_FALSE(getMqDbQueue());

    LogFatal("======> Start %s, ThreadNum:%u <======",
        m_strThreadPoolName.data(), m_uiThreadNum);

    return true;
}

bool MqThreadPool::getName()
{
    switch (m_uiMqThreadPoolType)
    {
        case PRODUCE_TO_MQ_C2S_IO:
            m_strThreadPoolName = "MqC2sIoProducerThreadPool";
            break;
        case PRODUCE_TO_MQ_SEND_IO:
            m_strThreadPoolName = "MqSendIoProducerThreadPool";
            break;
        case PRODUCE_TO_MQ_C2S_DB:
            m_strThreadPoolName = "MqC2sDbProducerThreadPool";
            break;
        case PRODUCE_TO_MQ_SEND_DB:
            m_strThreadPoolName = "MqSendDbProducerThreadPool";
            break;
        case CONSUME_FROM_MQ_C2S_IO:
            m_strThreadPoolName = "MqC2sIoConsumerThreadPool";
            break;
        case CONSUME_FROM_MQ_SEND_IO:
            m_strThreadPoolName = "MqSendIoConsumerThreadPool";
            break;
        case CONSUME_FROM_MQ_C2S_DB:
            m_strThreadPoolName = "MqC2sDbConsumerThreadPool";
            break;
        case CONSUME_FROM_MQ_SEND_DB:
            m_strThreadPoolName = "MqSendDbConsumerThreadPool";
            break;
        default:
            return false;
    }

    return true;
}

bool MqThreadPool::getThreadNum()
{
    string strComponentType;

#if defined(smsp_c2s)
    strComponentType = COMPONENT_TYPE_C2S;
#elif defined(smsp_http)
    strComponentType = COMPONENT_TYPE_HTTP;
#elif defined(smsp_access)
    strComponentType = COMPONENT_TYPE_ACCESS;
#elif defined(smsp_audit)
    strComponentType = COMPONENT_TYPE_AUDIT;
#elif defined(smsp_send)
    strComponentType = COMPONENT_TYPE_SEND;
#elif defined(smsp_report)
    strComponentType = COMPONENT_TYPE_REPORT;
#elif defined(smsp_reback)
    strComponentType = COMPONENT_TYPE_REBACK;
#elif defined(smsp_consumer)
    strComponentType = COMPONENT_TYPE_CONSUMER;
#endif

    string strKey = Fmt<16>("%u_%u_%s",
                            m_middleWareConfig.m_uMiddleWareType,
                            m_uiMode,
                            strComponentType.data());

    AppDataMapCIter iter = m_pMgr->m_mapMqThreadCfg.find(strKey);

    if (m_pMgr->m_mapMqThreadCfg.end() == iter)
    {
        LogWarn("Can't find MiddleWareType[%u],Mode[%u],ComponentType[%u] "
            "in t_sms_mq_thread_config.",
            m_middleWareConfig.m_uMiddleWareType,
            m_uiMode,
            strComponentType.data());

        m_uiThreadNum = 0;
    }
    else
    {
        const TSmsMqThreadCfg& mqThreadCfg = *(TSmsMqThreadCfg*)(iter->second);
        m_uiThreadNum = mqThreadCfg.m_uiThreadNum;
    }

    LogNotice("%s thread num:%u", m_strThreadPoolName.data(), m_uiThreadNum);
    return true;
}

bool MqThreadPool::getMqDbQueue()
{
    UInt32 uiMode = 0;

    if ((PRODUCE_TO_MQ_C2S_DB == m_uiMqThreadPoolType)
    || (PRODUCE_TO_MQ_SEND_DB == m_uiMqThreadPoolType))
    {
        uiMode = PRODUCT_MODE;
    }
    else if ((CONSUME_FROM_MQ_C2S_DB == m_uiMqThreadPoolType)
        || (CONSUME_FROM_MQ_SEND_DB == m_uiMqThreadPoolType))
    {
        uiMode = CONSUMER_MODE;
    }
    else
    {
        return true;
    }

    set<UInt64> setMqId;
    foreach (AppDataMapValueType& iter, m_pMgr->m_mapComponentRefMq)
    {
        const TSmsComponentRefMqCfg& refMq = *(TSmsComponentRefMqCfg*)(iter.second);

        if ((g_uComponentId != refMq.m_uComponentId) || (uiMode != refMq.m_uMode)) continue;

        AppDataMapCIter iter1;
        INIT_CHK_MAP_FIND_STR_RET_FALSE(m_pMgr->m_mapMqCfg, iter1, to_string(refMq.m_uMqId));
        const TSmsMqCfg& mqCfg = *(TSmsMqCfg*)(iter1->second);

        AppDataMapCIter iter2;
        INIT_CHK_MAP_FIND_STR_RET_FALSE(m_pMgr->m_mapMiddlewareCfg, iter2, to_string(mqCfg.m_uMiddleWareId));
        const TSmsMiddlewareCfg& middlewareCfg = *(TSmsMiddlewareCfg*)(iter2->second);

        if ((MIDDLEWARE_TYPE_MQ_C2S_DB == middlewareCfg.m_uMiddleWareType)
          || (MIDDLEWARE_TYPE_MQ_SNED_DB == middlewareCfg.m_uMiddleWareType))
        {
            setMqId.insert(refMq.m_uMqId);
        }
    }

    foreach (UInt64 ulMqId, setMqId)
    {
        AppDataMapCIter iter;
        INIT_CHK_MAP_FIND_STR_RET_FALSE(m_pMgr->m_mapMqCfg, iter, to_string(ulMqId));
        const TSmsMqCfg& mqCfg = *(TSmsMqCfg*)(iter->second);

        m_vecMqCfgForDbQueue.push_back(mqCfg);
    }

    if (MQ_DB_QUEUE_NUM != m_vecMqCfgForDbQueue.size())
    {
        LogErrorP("The number of MqDbQueue is not %u.", MQ_DB_QUEUE_NUM);
        return false;
    }

    return true;
}

bool MqThreadPool::getComponentSwitch()
{
    AppDataMapCIter iter = m_pMgr->m_mapComponentCfg.find(to_string(g_uComponentId));

    if (m_pMgr->m_mapComponentCfg.end() == iter)
    {
        LogError("Can't find %lu in t_sms_component_cfg.", g_uComponentId);
        return false;
    }

    const TSmsComponentCfg& componentCfg = *(TSmsComponentCfg*)(iter->second);
    return (PRODUCT_MODE == m_uiMode)?componentCfg.m_uProducerSwitch:componentCfg.m_uComponentSwitch;
}

bool MqThreadPool::getLinkStatus()
{
    foreach (CThread* pThread, m_vecThreads)
    {
        MqThread* pMqThread = (MqThread*)pThread;

        if (pMqThread->getLinkStatus())
        {
            return true;
        }
    }

    return false;
}

UInt32 MqThreadPool::getTotalQueueSize()
{
    UInt32 uiTotalQueueSize = 0;

    foreach (CThread* pThread, m_vecThreads)
    {
        uiTotalQueueSize += pThread->GetMsgSize();
    }

    return uiTotalQueueSize;
}

bool MqThreadPool::start()
{
    return adjustThread();
}

bool MqThreadPool::adjustThread()
{
    UInt32 uiNum = m_vecThreads.size();

    if (m_uiThreadNum > uiNum)
    {
        CHK_FUNC_RET_FALSE(createThreadByNum(m_uiThreadNum - uiNum));
    }
    else if (m_uiThreadNum < uiNum)
    {
        CHK_FUNC_RET_FALSE(reduceThreadByNum(uiNum - m_uiThreadNum));
    }

    printThreadPool();
    return true;
}

bool MqThreadPool::createThreadByNum(UInt32 uiThreadNum)
{
    for (UInt32 i = 0; i < uiThreadNum; ++i)
    {
        CHK_FUNC_RET_FALSE(createThread());
    }

    return true;
}

bool MqThreadPool::reduceThreadByNum(UInt32 uiThreadNum)
{
    for (UInt32 i = 0; i < uiThreadNum; ++i)
    {
        CHK_FUNC_RET_FALSE(reduceThread());
    }

    return true;
}

bool MqThreadPool::createThread()
{
    return true;
}

bool MqThreadPool::reduceThread()
{
    return true;
}

void MqThreadPool::printThreadPool()
{
    foreach (CThread* pThread, m_vecThreads)
    {
        LogNotice("==> %s have %s <==",
            m_strThreadPoolName.data(),
            pThread->GetThreadName().data());
    }
}

void MqThreadPool::handleDbUpdateReq(DbUpdateReq* pReq)
{
    CHK_NULL_RETURN(pReq);

    switch (pReq->m_uiTableId)
    {
        case t_sms_mq_thread_configure:
        {
            handleMqThreadCfgUpdateReq();
            break;
        }
        case t_sms_component_configure:
        {
            handleComponentCfgUpdateReq();
            break;
        }
        case t_sms_component_ref_mq:
        {
            handleComponentRefMqUpdateReq();
            break;
        }
        default:
        {
            break;
        }
    }
}

void MqThreadPool::handleMqThreadCfgUpdateReq()
{
    CHK_FUNC_RET(getThreadNum());
    CHK_FUNC_RET(adjustThread());

    // 消费者线程池调整线程数量之后，需要重新分配队列
    if (CONSUMER_MODE == m_uiMode)
    {
        handleComponentRefMqUpdateReq();
    }
}

void MqThreadPool::handleComponentCfgUpdateReq()
{
    bool bSwitch = getComponentSwitch();

    for (std::size_t i = 0; i < m_vecThreads.size(); ++i)
    {
        MqThread* pMqThread = (MqThread*)m_vecThreads[i];
        CHK_NULL_RETURN(pMqThread);

        CommonMsg<bool>* pReq = new CommonMsg<bool>();
        CHK_NULL_RETURN(pReq);
        pReq->m_value = bSwitch;
        pReq->m_iMsgType = MSGTYPE_MQ_SWITCH_UPDATE_REQ;
        pMqThread->PostMngMsg(pReq);
    }
}

void MqThreadPool::handleComponentRefMqUpdateReq()
{
}

