#include "CMQAuditConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include "CEmojiString.h"
#include "Comm.h"
#include "LogMacro.h"



using namespace std;

CMQAuditConsumerThread::CMQAuditConsumerThread(const char *name):CMQAckConsumerThread(name)
{
    m_uDBSwitch = 2;
}

CMQAuditConsumerThread::~CMQAuditConsumerThread()
{
}

bool CMQAuditConsumerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if(false == CMQAckConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogError("CMQNoAckConsumerThread::Init() fail");
        return false;
    }

    if(false == InitChannels())
    {
        return false;
    }

    //
    m_uFetchCount = 1;
    LogNotice("reset frtchCount[%u]",m_uFetchCount);
    return true;
}

bool CMQAuditConsumerThread::AllChannelConsumerPause()
{
    if(true == m_bConsumerSwitch)
    {
        LogWarn("m_bConsumerSwitch is ON. Can't pause.");
        return false;
    }

    map<string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin();
    // cancel consume
    bool bRet = true;

    for(; itr != m_componentRefMqInfo.end(); ++itr)
    {
        std::string strChannelId = Comm::int2str(itr->second.m_uId);
        amqp_basic_cancel(m_mqConnectionState, itr->second.m_uId, amqp_cstring_bytes(strChannelId.c_str()));
        amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
        if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
        {
            LogNotice("ChannelId[%d] Mqid[%lu] pause consume failed. reply_type:%d", itr->second.m_uId, itr->second.m_uMqId, rConsume.reply_type);
            bRet = false;
            continue;
        }
        else
        {
            LogNotice("ChannelId[%d] Mqid[%lu] pause consume success.", itr->second.m_uId, itr->second.m_uMqId);
        }
    }
    return bRet;
}

bool CMQAuditConsumerThread::AllChannelConsumerResume()
{
    if(false == m_bConsumerSwitch)
    {
        LogWarn("m_bConsumerSwitch is OFF. Can't resume.");
        return false;
    }

    bool bRet = true;
    map<string,ComponentRefMq>::iterator itr;
    for(itr = m_componentRefMqInfo.begin(); itr != m_componentRefMqInfo.end(); ++itr)
    {
        if (false == amqpBasicConsume(itr->second.m_uId, itr->second.m_uMqId))
        {
            LogError("ChannelId[%d] Mqid[%lu] resume consume failed!!!", itr->second.m_uId, itr->second.m_uMqId);
            bRet = false;
            continue;
        }
        else
        {
            LogNotice("ChannelId[%d] Mqid[%lu] Consume resume succ.", itr->second.m_uId, itr->second.m_uMqId);
        }
    }

    return bRet;
}

bool CMQAuditConsumerThread::UpdateMqConsumer(map <string,ComponentRefMq>& mapComponentRefMq)
{
    map<string,ComponentRefMq>::iterator itold = m_componentRefMqInfo.begin();
    for(; itold != m_componentRefMqInfo.end(); )
    {
        map <string,ComponentRefMq>::iterator itdel = mapComponentRefMq.find(itold->first);
        if(itdel == mapComponentRefMq.end())//mq关系解除
        {
            LogNotice("ChannelId[%d] Mqid[%lu] close Consume...", itold->second.m_uId, itold->second.m_uMqId);
            //先取消订阅
            amqp_channel_close(m_mqConnectionState, itold->second.m_uId, AMQP_REPLY_SUCCESS);
            m_componentRefMqInfo.erase(itold++);
            map<UInt32, ChannelSpeedCtrl>::iterator it = m_mapChannelSpeedCtrl.find(itold->second.m_uId);
            if(it != m_mapChannelSpeedCtrl.end())
                m_mapChannelSpeedCtrl.erase(it);
        }
        else
        {
            itold++;
        }
    }

    map <string,ComponentRefMq>::iterator itor = mapComponentRefMq.begin();
    for(; itor != mapComponentRefMq.end(); itor++)
    {
        if((g_uComponentId == itor->second.m_uComponentId) &&
            (MESSAGE_CONTENT_AUDIT_RESULT == itor->second.m_strMessageType) &&
            (CONSUMER_MODE == itor->second.m_uMode))
        {
            map <UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
            if (itrMq == m_mapMqConfig.end())
            {
                LogError("MqId:%lu is not find in m_mapMqConfig", itor->second.m_uMqId);
                continue;
            }

            //update channel speed
            ChannelSpeedCtrl stuChannelSpeedCtrl;
            stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
            stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uGetRate;
            stuChannelSpeedCtrl.uMqId = itor->second.m_uMqId;
            SetChannelSpeedCtrl(itor->second.m_uId, stuChannelSpeedCtrl);

            map <string,ComponentRefMq>::iterator itr = m_componentRefMqInfo.find(itor->first);
            if(itr == m_componentRefMqInfo.end())
            {//add new mq consmer
                if((true == m_bConsumerSwitch) && (false == amqpBasicChannelOpen(itor->second.m_uId, itor->second.m_uMqId)))
                {
                    LogError("add newmq[%lu] open failed",itor->second.m_uMqId);
                    return false;
                }
                if((true == m_bConsumerSwitch) && (false == amqpBasicConsume(itor->second.m_uId, itor->second.m_uMqId)))
                {
                    LogError("add newmq[%lu] amqpBasicConsume failed",itor->second.m_uMqId);
                    return false;
                }
                LogNotice("Add new mq[%u] consumer success!!",itor->second.m_uMqId);
                m_componentRefMqInfo[itor->first] = itor->second;
            }
        }
    }
    LogNotice("Init mqqueue Over! m_componentRefMqInfo size[%d], m_mapChannelSpeedCtrl size[%d]",
                m_componentRefMqInfo.size(), m_mapChannelSpeedCtrl.size());
    return true;
}
bool CMQAuditConsumerThread::InitChannels()
{
    m_componentRefMqInfo.clear();
    m_mapChannelSpeedCtrl.clear();
    m_iSumChannelSpeed = 0;
    map <string,ComponentRefMq> tempRefMqInfo;
    tempRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(tempRefMqInfo);

    return UpdateMqConsumer(tempRefMqInfo);
}

void CMQAuditConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQAckConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            std::map<std::string, std::string>::iterator iterDB = pSysParamReq->m_SysParamMap.find("DB_SWITCH");
            if (iterDB == pSysParamReq->m_SysParamMap.end())
            {
                LogWarn("DB_SWITCH is not find in sysParamMap.");
                return;
            }
            m_uDBSwitch = atoi(iterDB->second.data());
            LogNotice("==MQ== DB_SWITCH[%d].",m_uDBSwitch);
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            HandleComponentRefMqRep(pMsg);
            break;
        }
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
}

void CMQAuditConsumerThread::HandleComponentRefMqRep(TMsg * pMsg)
{
    TUpdateComponentRefMqReq* pMqCon = (TUpdateComponentRefMqReq*)pMsg;
    //m_mapChannelSpeedCtrl.clear();
    m_iSumChannelSpeed = 0;
    map <string,ComponentRefMq>& tempRefMqInfo = pMqCon->m_componentRefMqInfo;

    UpdateMqConsumer(tempRefMqInfo);
}

int CMQAuditConsumerThread::GetMemoryMqMessageSize()
{
    return g_pAuditThread->GetMsgSize();
}

void CMQAuditConsumerThread::processMessage(string& strData)
{
    LogNotice("==access== get one audit message data:%s.",strData.data());
    TMsg* pMqmsg = new TMsg();
    pMqmsg->m_iMsgType = MSGTYPE_MQ_GETMQMSG_REQ;
    pMqmsg->m_strSessionID.assign(strData);
    g_pAuditThread->PostMsg(pMqmsg);
}
