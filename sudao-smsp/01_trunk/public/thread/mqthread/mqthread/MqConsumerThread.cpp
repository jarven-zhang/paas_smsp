#include "MqConsumerThread.h"
#include "MqManagerThread.h"
#include "MqThreadPool.h"
#include "LogMacro.h"
#include "commonmessage.h"

#include "boost/foreach.hpp"


#define foreach BOOST_FOREACH


MqConsumerThread::MqConsumerThread()
{
    m_bConsumerSwitch = false;
}

MqConsumerThread::~MqConsumerThread()
{
}

bool MqConsumerThread::init()
{
    INIT_CHK_FUNC_RET_FALSE(MqThread::init());
    INIT_CHK_FUNC_RET_FALSE(initConsumerQueue());

    return true;
}

bool MqConsumerThread::getLinkStatus()
{
    return true;
}

bool MqConsumerThread::initConsumerQueue()
{
    foreach (MqQueueCtrlMapValueType& iter, m_mapMqQueueCtrl)
    {
        CHK_FUNC_RET_FALSE(initOneQueue(iter.second));
    }

    return true;
}

bool MqConsumerThread::initOneQueue(MqQueueCtrl& mqQueueCtrl)
{
    mqQueueCtrl.m_pThread = this;

    if (!mqQueueCtrl.init())
    {
        LogErrorT("initialize queue[id:%lu, name:%s, speed:%u] failed.",
            mqQueueCtrl.m_ulMqId,
            mqQueueCtrl.m_strQueue.data(),
            mqQueueCtrl.m_uiSpeed);

        return false;
    }

    return true;
}

void MqConsumerThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (1)
    {
        handleMngMsg();

        if (m_bConsumerSwitch)
        {
            consumeMessage();
        }

        if (m_bStop && m_msgMngQueue.empty() && m_msgQueue.empty())
        {
            break;
        }
    }

    LogWarnT("========> now exit <========");
    delete this;
}

void MqConsumerThread::handleMngMsg()
{
    TMsg* pMsg = GetMngMsg();

    if (NULL != pMsg)
    {
        HandleMsg(pMsg);
        delete pMsg;
    }
    else
    {
        if (!m_bConsumerSwitch)
        {
            select_sleep(SLEEP_TIME);
        }
    }
}

void MqConsumerThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_MQ_QUEUE_CTRL_UPDATE_REQ:
        {
            handleMqQueueCtrlUpdateReq(pMsg);
            break;
        }
        case MSGTYPE_MQ_SWITCH_UPDATE_REQ:
        {
            CommonMsg<bool>* pReq = (CommonMsg<bool>*)pMsg;

            if (m_bConsumerSwitch != pReq->m_value)
            {
                LogWarnT("ConsumerSwitch: %s -> %s.",
                    m_bConsumerSwitch?"ON":"OFF",
                    (pReq->m_value)?"ON":"OFF");

                m_bConsumerSwitch = pReq->m_value;
            }

            break;
        }
        case MSGTYPE_MQ_THREAD_EXIT_REQ:
        {
            CommonMsg<bool>* pReq = (CommonMsg<bool>*)pMsg;

            if (m_bStop != pReq->m_value)
            {
                LogWarnT("Thread Stop Flag: %s -> %s.",
                    m_bStop?"ON":"OFF",
                    (pReq->m_value)?"ON":"OFF");

                m_bStop = pReq->m_value;
            }

            break;
        }
        default:
        {
            LogErrorT("Invalid MsgType[%x].", pMsg->m_iMsgType);
            break;
        }
    }
}

void MqConsumerThread::consumeMessage()
{
    // 计算本轮消费开始时间
    UInt64 ulConsumeStart = now_microseconds();

    // 本轮消费次数
    UInt32 uiContinueConsumeTimes = 0;

    while(1)
    {
        bool bAllConsumeOk = true;

        for (MqQueueCtrlMapIter iter = m_mapMqQueueCtrl.begin(); iter != m_mapMqQueueCtrl.end(); ++iter)
        {
            MqQueueCtrl& mqQueueCtrl = iter->second;

            if (!mqQueueCtrl.consume(uiContinueConsumeTimes))
            {
                bAllConsumeOk = false;
            }
        }

        Int64 diff = now_microseconds() - ulConsumeStart;

        if (diff >= ONE_SECOND_FOR_MICROSECOND)
        {
//            LogNoticeT("consume all queue takes %ld usec, %u times, no need sleep.",
//                diff,
//                uiContinueConsumeTimes + 1);

            break;
        }
        else
        {
            if (bAllConsumeOk)
            {
                UInt64 sleep_usec = ONE_SECOND_FOR_MICROSECOND - diff;

//                LogNoticeT("consume all queue takes %ld usec, %u times, sleep %lu usec.",
//                    diff,
//                    uiContinueConsumeTimes + 1,
//                    sleep_usec);

                select_sleep(sleep_usec);
                break;
            }
            else
            {
                uiContinueConsumeTimes++;
            }
        }
    }
}

void MqConsumerThread::sendMsg(cs_t strData)
{
    MiddleMsg<MqConsumeReqMsg>* pMiddleMsg = new MiddleMsg<MqConsumeReqMsg>();
    CHK_NULL_RETURN(pMiddleMsg);
    pMiddleMsg->m_iMsgType = MSGTYPE_MQ_GETMQMSG_REQ;

    MqConsumeReqMsg* pReq = pMiddleMsg->m_pMsg;
    CHK_NULL_RETURN(pReq);
    pReq->m_uiType = m_uiMqThreadType;
    pReq->m_strData = strData;
    pReq->m_iMsgType = MSGTYPE_MQ_GETMQMSG_REQ;

    m_pMqThreadPool->m_pMgr->PostMsg(pMiddleMsg);
}

void MqConsumerThread::printMqQueueCtr()
{
    foreach (MqQueueCtrlMapValueType& iter, m_mapMqQueueCtrl)
    {
        MqQueueCtrl& mqQueueCtrl = iter.second;
        mqQueueCtrl.print();
    }
}

// FIX ME
// 这种分配方式会导致消费者线程消费的队列在更新时变动较大
// 而且可能会瞬间出现，两个线程同时消费一个队列的情况，但是不会重复消费
void MqConsumerThread::handleMqQueueCtrlUpdateReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    MqQueueCtrlUpdateReq* pReq = (MqQueueCtrlUpdateReq*)pMsg;

    // 处理新增的MqQueueCtrl
    for (MqQueueCtrlMapIter iter = pReq->m_mapMqQueueCtrl.begin(); iter != pReq->m_mapMqQueueCtrl.end(); ++iter)
    {
        MqQueueCtrl& mqQueueCtrl = iter->second;

        if (m_mapMqQueueCtrl.end() == m_mapMqQueueCtrl.find(mqQueueCtrl.m_ulMqId))
        {
            LogNoticeT("add MqQueue[id:%lu, name:%s, speed:%u].",
                mqQueueCtrl.m_ulMqId,
                mqQueueCtrl.m_strQueue.data(),
                mqQueueCtrl.m_uiSpeed);

            initOneQueue(mqQueueCtrl);
            m_mapMqQueueCtrl[mqQueueCtrl.m_ulMqId] = mqQueueCtrl;
        }
    }

    for (MqQueueCtrlMapIter iterOld = m_mapMqQueueCtrl.begin(); iterOld != m_mapMqQueueCtrl.end();)
    {
        MqQueueCtrl& mqQueueCtrlOld = iterOld->second;
        MqQueueCtrlMapIter iterNew = pReq->m_mapMqQueueCtrl.find(mqQueueCtrlOld.m_ulMqId);

        if (pReq->m_mapMqQueueCtrl.end() == iterNew)
        {
            LogNoticeT("delete MqQueue[id:%lu, name:%s, speed:%u].",
                mqQueueCtrlOld.m_ulMqId,
                mqQueueCtrlOld.m_strQueue.data(),
                mqQueueCtrlOld.m_uiSpeed);

            // 处理删除的MqQueueCtrl
            m_mapMqQueueCtrl.erase(iterOld++);
        }
        else
        {
            // 处理修改的MqQueueCtrl
            const MqQueueCtrl& mqQueueCtrlNew = iterNew->second;

            LogNoticeT("update MqQueue[id:%lu, name:%s], "
                "Speed[%u->%u], PrefetchCount[%u->%u], MultipleAck[%u->%u].",
                mqQueueCtrlOld.m_ulMqId,
                mqQueueCtrlOld.m_strQueue.data(),
                mqQueueCtrlOld.m_uiSpeed,
                mqQueueCtrlNew.m_uiSpeed,
                mqQueueCtrlOld.m_uiPrefetchCount,
                mqQueueCtrlNew.m_uiPrefetchCount,
                mqQueueCtrlOld.m_uiMultipleAck,
                mqQueueCtrlNew.m_uiMultipleAck);

            if ((mqQueueCtrlOld.m_uiSpeed != mqQueueCtrlNew.m_uiSpeed)
            || (mqQueueCtrlOld.m_uiMultipleAck != mqQueueCtrlNew.m_uiMultipleAck))
            {
                mqQueueCtrlOld.m_uiSpeed = mqQueueCtrlNew.m_uiSpeed;
                mqQueueCtrlOld.m_uiMultipleAck = mqQueueCtrlNew.m_uiMultipleAck;
            }

            if (mqQueueCtrlOld.m_uiPrefetchCount != mqQueueCtrlNew.m_uiPrefetchCount)
            {
                mqQueueCtrlOld.m_uiPrefetchCount = mqQueueCtrlNew.m_uiPrefetchCount;

                mqQueueCtrlOld.close();
                mqQueueCtrlOld.connect();

                select_sleep(1000*1000);
            }

            iterOld++;
        }
    }

    printMqQueueCtr();
}

