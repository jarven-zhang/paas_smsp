#include "MqProducerThreadPool.h"
#include "MqProducerThread.h"
#include "MqManagerThread.h"
#include "commonmessage.h"
#include "LogMacro.h"
#include "ssl/md5.h"
#include "Fmt.h"

#include "boost/algorithm/string.hpp"

MqProducerThreadPool::MqProducerThreadPool()
{
}

MqProducerThreadPool::~MqProducerThreadPool()
{
}

bool MqProducerThreadPool::createThread()
{
    UInt32 uiIndex = m_vecThreads.size();
    string strThreadName = Fmt<32>("%s%u", m_strThreadPoolName.data(), uiIndex);
    boost::replace_all(strThreadName, "ProducerThreadPool", "PThd");

    MqProducerThread* pThread = new MqProducerThread();
    CHK_NULL_RETURN_FALSE(pThread);
    pThread->setName(strThreadName);
    pThread->m_uiIndexInThreadPool = uiIndex;
    pThread->m_uiMqThreadType = m_uiMqThreadPoolType;
    pThread->m_strIp = m_middleWareConfig.m_strHostIp;
    pThread->m_usPort = m_middleWareConfig.m_uPort;
    pThread->m_strUserName = m_middleWareConfig.m_strUserName;
    pThread->m_strPassword = m_middleWareConfig.m_strPassword;
    pThread->m_bProducerSwitch = getComponentSwitch();

    if (!pThread->init() || !pThread->CreateThread())
    {
        LogError("Call init or CreateThread failed.");
        SAFE_DELETE(pThread);
        return false;
    }

    m_vecThreads.push_back(pThread);

    LogWarn("[%s] ===> Start %s, index:%u <===",
        m_strThreadPoolName.data(), strThreadName.data(), uiIndex);

    return true;
}

bool MqProducerThreadPool::reduceThread()
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

void MqProducerThreadPool::postMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    MqPublishReqMsg* pReq = (MqPublishReqMsg*)pMsg;

    if ((MIDDLEWARE_TYPE_MQ_C2S_DB == pReq->m_uiMiddlewareType)
    || (MIDDLEWARE_TYPE_MQ_SNED_DB == pReq->m_uiMiddlewareType))
    {
        CHK_FUNC_RET(getIdFromSql(pReq));
        UInt32 uiIndex = hash(pReq->m_strKey, MQ_DB_QUEUE_NUM);

        pReq->m_strExchange = m_vecMqCfgForDbQueue[uiIndex].m_strExchange;
        pReq->m_strRoutingKey = m_vecMqCfgForDbQueue[uiIndex].m_strRoutingKey;

        if (MQ_DB_QUEUE_NUM != m_vecThreads.size())
        {
            uiIndex = hash(pReq->m_strKey, m_vecThreads.size());
        }

        m_vecThreads[uiIndex]->PostMsg(pMsg);
    }
    else
    {
        if (m_vecThreads.empty())
        {
            LogWarn("[%s] no thread in threadpool.", m_strThreadPoolName.data());
            return;
        }

        if (m_uiIndex >= m_vecThreads.size())
        {
            m_uiIndex = 0;
        }

        LogDebug("[%s] choose index:%u.", m_strThreadPoolName.data(), m_uiIndex);

        m_vecThreads[m_uiIndex]->PostMsg(pMsg);
        m_uiIndex++;
    }
}

bool MqProducerThreadPool::getIdFromSql(MqPublishReqMsg* pReq)
{
    CHK_NULL_RETURN_FALSE(pReq);

    if (pReq->m_strKey.empty())
    {
        string::size_type pos = pReq->m_strData.find("RabbitMQFlag=");

        if (pos == string::npos)
        {
            LogError("[%s] Can't find RabbitMQFlag= in %s.",
                m_strThreadPoolName.data(), pReq->m_strData.data());
            return false;
        }

        static int len = strlen("RabbitMQFlag=");
        pReq->m_strKey = pReq->m_strData.substr(pos + len, 36);
    }

    return true;
}

UInt32 MqProducerThreadPool::hash(cs_t str, UInt32 uiNumber)
{
    if (str.empty() || (0 == uiNumber))
    {
        LogDebug("str:%s, num:%u.", str.data(), uiNumber);
        return 0;
    }

    unsigned char md5[17] = {0};
    MD5((const unsigned char*)str.data(), str.length(), md5);

//    for (int i = 0; i < 17; i++)
//    {
//        LogDebug("SmsId[%s], num[%u], md5[%x].", str.data(), uiNumber, md5[i]);
//    }

//    string strMd5;
//    std::string HEX_CHARS = "0123456789abcdef";
//    for (int i = 0; i < 16; i++)
//    {
//        strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
//        strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
//    }
//    LogDebug("strMd5[%s].", strMd5.data());
//    UInt64 uKey = stoull(strMd5.substr(0, 4).data(), 0, 16);

    // After optimization
    UInt32 key = (md5[0] << 8) + md5[1];

    return key % uiNumber;
}

void MqProducerThreadPool::handleComponentRefMqUpdateReq()
{
}

