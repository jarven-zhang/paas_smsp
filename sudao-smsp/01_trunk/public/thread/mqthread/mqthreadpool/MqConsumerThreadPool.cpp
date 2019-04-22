#include "MqConsumerThreadPool.h"
#include "MqConsumerThread.h"
#include "MqManagerThread.h"
#include "commonmessage.h"
#include "Fmt.h"

#include "boost/algorithm/string.hpp"
#include "boost/foreach.hpp"

#define foreach BOOST_FOREACH

extern UInt64 g_uComponentId;


MqConsumerThreadPool::MqConsumerThreadPool()
{
}

MqConsumerThreadPool::~MqConsumerThreadPool()
{
}

bool MqConsumerThreadPool::createThread()
{
    if (m_vecMqQueueCtrl.empty())
    {
        // 初始化时需要获取获取队列
        CHK_FUNC_RET_FALSE(getConsumerQueue());
    }

    UInt32 uiIndex = m_vecThreads.size();
    string strThreadName = Fmt<32>("%s%u", m_strThreadPoolName.data(), uiIndex);
    boost::replace_all(strThreadName, "ConsumerThreadPool", "CThd");

    MqConsumerThread* pThread = new MqConsumerThread();
    CHK_NULL_RETURN_FALSE(pThread);
    pThread->setName(strThreadName);
    pThread->m_uiIndexInThreadPool = uiIndex;
    pThread->m_uiMqThreadType = m_uiMqThreadPoolType;
    pThread->m_strIp = m_middleWareConfig.m_strHostIp;
    pThread->m_usPort = m_middleWareConfig.m_uPort;
    pThread->m_strUserName = m_middleWareConfig.m_strUserName;
    pThread->m_strPassword = m_middleWareConfig.m_strPassword;
    pThread->m_mapMqQueueCtrl = getThreadConsumerQueue(uiIndex);
    pThread->m_bConsumerSwitch = getComponentSwitch();
    pThread->m_pMqThreadPool = this;

    if (!pThread->init() || !pThread->CreateThread())
    {
        LogError("Call init or CreateThread failed.");
        SAFE_DELETE(pThread);
        return false;
    }

    m_vecThreads.push_back(pThread);

    LogWarn("[%s] ===> Start %s, index:%u <===",
        m_strThreadPoolName.data(),
        strThreadName.data(),
        uiIndex);

    return true;
}

bool MqConsumerThreadPool::reduceThread()
{
    if (m_vecThreads.empty()) return true;

    MqThread* pMqThread = (MqThread*)(m_vecThreads[m_vecThreads.size() - 1]);
    CHK_NULL_RETURN_FALSE(pMqThread);

    LogWarn("[%s] ===> Stop %s, Remaining thread num:%u, Now waitting thread exit <===",
        m_strThreadPoolName.data(),
        pMqThread->GetThreadName().data(),
        m_vecThreads.size());

    CommonMsg<bool>* pReq = new CommonMsg<bool>();
    pReq->m_value = true;
    pReq->m_iMsgType = MSGTYPE_MQ_THREAD_EXIT_REQ;
    pMqThread->PostMngMsg(pReq);

    m_vecThreads.pop_back();
    return true;
}

bool MqConsumerThreadPool::getConsumerQueue()
{
    m_vecMqQueueCtrl.clear();

    foreach (AppDataMapValueType& iter, m_pMgr->m_mapComponentRefMq)
    {
        const TSmsComponentRefMqCfg& refMq = *(TSmsComponentRefMqCfg*)(iter.second);

        if ((g_uComponentId == refMq.m_uComponentId) && (CONSUMER_MODE == refMq.m_uMode))
        {
            AppDataMapCIter iter1;
            INIT_CHK_MAP_FIND_STR_RET_FALSE(m_pMgr->m_mapMqCfg, iter1, to_string(refMq.m_uMqId));
            const TSmsMqCfg& mqCfg = *(TSmsMqCfg*)(iter1->second);

            AppDataMapCIter iter2;
            INIT_CHK_MAP_FIND_STR_RET_FALSE(m_pMgr->m_mapMiddlewareCfg, iter2, Fmt("%lu", mqCfg.m_uMiddleWareId));
            const TSmsMiddlewareCfg& middlewareCfg = *(TSmsMiddlewareCfg*)(iter2->second);

            if ((middlewareCfg.m_uMiddleWareType + 3) == m_uiMqThreadPoolType)
            {
                m_vecMqQueueCtrl.push_back(MqQueueCtrl(mqCfg.m_uMqId,
                                                       mqCfg.m_strQueue,
                                                       refMq.m_uGetRate,
                                                       refMq.m_uiPrefetchCount,
                                                       refMq.m_uiMultipleAck));

//                LogNoticeP("Queue[id:%lu, name:%s, speed:%u] has assigned to %s.",
//                    mqCfg.m_uMqId,
//                    mqCfg.m_strQueue.data(),
//                    refMq.m_uGetRate,
//                    m_strThreadPoolName.data());
            }
        }
    }

    if (m_vecMqQueueCtrl.empty())
    {
        LogErrorP("Component[id:%lu] has no queue to consumer.", g_uComponentId);
        return false;
    }

    return true;
}

MqQueueCtrlMap MqConsumerThreadPool::getThreadConsumerQueue(UInt32 index)
{
    MqQueueCtrlMap mapMqQueueCtrl;

    for (std::size_t i = 0; i < m_vecMqQueueCtrl.size(); ++i)
    {
        MqQueueCtrl& mqQueueCtrl = m_vecMqQueueCtrl[i];

        if (index == (i % m_uiThreadNum))
        {
            mapMqQueueCtrl[mqQueueCtrl.m_ulMqId] = mqQueueCtrl;

            LogNotice("Queue[id:%lu, name:%s, speed:%u] has assigned to %s:%u.",
                mqQueueCtrl.m_ulMqId,
                mqQueueCtrl.m_strQueue.data(),
                mqQueueCtrl.m_uiSpeed,
                m_strThreadPoolName.data(),
                index);
        }
    }

    return mapMqQueueCtrl;
}

void MqConsumerThreadPool::handleComponentRefMqUpdateReq()
{
    CHK_FUNC_RET(getConsumerQueue());

    for (std::size_t i = 0; i < m_vecThreads.size(); ++i)
    {
        MqThread* pMqThread = (MqThread*)m_vecThreads[i];
        CHK_NULL_RETURN(pMqThread);

        MqQueueCtrlUpdateReq* pReq = new MqQueueCtrlUpdateReq();
        CHK_NULL_RETURN(pReq);
        pReq->m_mapMqQueueCtrl = getThreadConsumerQueue(i);
        pReq->m_iMsgType = MSGTYPE_MQ_QUEUE_CTRL_UPDATE_REQ;
        pMqThread->PostMngMsg(pReq);
    }
}

