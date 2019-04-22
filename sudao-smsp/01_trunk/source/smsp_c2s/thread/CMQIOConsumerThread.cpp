#include "CMQIOConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>

using namespace std;

CMQIOConsumerThread::CMQIOConsumerThread(const char *name):CMQConsumerThread(name)
{
}

CMQIOConsumerThread::~CMQIOConsumerThread()
{
}

bool CMQIOConsumerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogError("CMQConsumerThread::Init() fail");
        return false;
    }
    
    if(false == InitChannels())
    {
        return false;
    }

    return true;
}

bool CMQIOConsumerThread::InitChannels()
{
    /* select * from t_sms_component_configure where component_id = g_uComponentId */
    g_pRuleLoadThread->getComponentConfig(m_componentConfigInfo);

    m_uMqId = m_componentConfigInfo.m_uMqId;
    
    m_bConsumerSwitch = m_componentConfigInfo.m_uComponentSwitch;
    LogNotice("ComponentId[%d] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON" : "OFF");

    if(false == UpdateChannelSpeedCtrl(m_uMqId))
    {
        return false;
    }

    if(false == amqpBasicConsume(SMSP_C2S_CHANNELID, m_uMqId))
    {
        LogError("amqpBasicConsume fail! ChannelId[%d], MqId[%lu]", SMSP_C2S_CHANNELID, m_uMqId);
        return false;
    }   
    return true;
}

bool CMQIOConsumerThread::UpdateChannelSpeedCtrl(UInt64 uMqId)
{
    //m_componentRefMqInfo.clear();
    map<std::string,ComponentRefMq> mapComponentRefMqInfo;
    mapComponentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(mapComponentRefMqInfo);

    /*根据Key查找当前组件与mq的关联信息*/
    char Key[32] = {0};
    snprintf(Key, sizeof(Key), "%lu_%s_%u", g_uComponentId, MESSAGE_TYPE_MO.data(), CONSUMER_MODE);
    string strKey;
    strKey.append(Key);
    map<string,ComponentRefMq>::iterator itor = mapComponentRefMqInfo.find(strKey);

    if(itor == mapComponentRefMqInfo.end())
    {
        LogError("Can not find [component_id[%lu]__message_type[%s]__mode[%d]] in t_sms_component_ref_mq",
            g_uComponentId, MESSAGE_TYPE_MO.data(), CONSUMER_MODE);
        return false;
    }
    else
    {
        if(itor->second.m_uMqId != uMqId)
        {
            LogError("t_sms_component_configure MqId[%lu], t_sms_component_ref_mq MqId[%lu], not the same!", 
                uMqId, itor->second.m_uMqId);
            
            return false;
        }
        else
        {
            m_componentRefMqInfo = itor->second;
            
            m_mapChannelSpeedCtrl.clear();
            
            ChannelSpeedCtrl stuChannelSpeedCtrl;
            stuChannelSpeedCtrl.uChannelSpeed = m_componentRefMqInfo.m_uGetRate;

            map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(uMqId);
            if(itrMq != m_mapMqConfig.end())
            {
                stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
                stuChannelSpeedCtrl.uMqId = uMqId;
                SetChannelSpeedCtrl(SMSP_C2S_CHANNELID, stuChannelSpeedCtrl);
                return true;
            }
            else
            {
                LogError("Can not find MqId[%d] in m_mapMqConfig", uMqId);
                return false;
            }
        }
    }
}

void CMQIOConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }
    
    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            HandleQueueUpdateRep(pMsg);
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

void CMQIOConsumerThread::HandleComponentRefMqRep(TMsg * pMsg)
{
    TUpdateComponentRefMqReq * pRefMq = (TUpdateComponentRefMqReq *)pMsg;
    map<std::string,ComponentRefMq> mapComponentRefMqInfo;
    mapComponentRefMqInfo.clear();
    mapComponentRefMqInfo = pRefMq->m_componentRefMqInfo;

    m_mapChannelSpeedCtrl.clear();

    /*根据Key查找当前组件与mq的关联信息*/
    char Key[32] = {0};
    snprintf(Key, sizeof(Key), "%lu_%s_%u", g_uComponentId, MESSAGE_TYPE_MO.data(), CONSUMER_MODE);
    string strKey;
    strKey.append(Key);
    map<string,ComponentRefMq>::iterator itor = mapComponentRefMqInfo.find(strKey);

    if(itor == mapComponentRefMqInfo.end())
    {
        LogError("Can not find [component_id[%lu]__message_type[%s]__mode][%d] in t_sms_component_ref_mq",
            g_uComponentId, MESSAGE_TYPE_MO.data(), CONSUMER_MODE);
    }
    else
    {
        if(itor->second.m_uMqId != m_uMqId)
        {
            LogNotice("t_sms_component_configure MqId[%lu], t_sms_component_ref_mq MqId[%lu], not the same!", 
                m_uMqId, itor->second.m_uMqId);
            //先取消订阅
            amqp_channel_close(m_mqConnectionState, m_uMqId, AMQP_REPLY_SUCCESS);

            //再订阅
            if(false == amqpBasicConsume(SMSP_C2S_CHANNELID, itor->second.m_uMqId))
            {
                LogError("ReConsume Fail!!!");
            }
            
            m_uMqId = itor->second.m_uMqId;
        }
        
        m_componentRefMqInfo = itor->second;
        
        ChannelSpeedCtrl stuChannelSpeedCtrl;
        stuChannelSpeedCtrl.uChannelSpeed = m_componentRefMqInfo.m_uGetRate;

        map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(m_uMqId);
        if(itrMq != m_mapMqConfig.end())
        {
            /*一切正常*/
            stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
            stuChannelSpeedCtrl.uMqId = m_uMqId;
            SetChannelSpeedCtrl(SMSP_C2S_CHANNELID, stuChannelSpeedCtrl);
        }
        else
        {
            LogError("Can not find MqId[%d] in m_mapMqConfig", m_uMqId);
        }
    }
}

void CMQIOConsumerThread::HandleQueueUpdateRep(TMsg* pMsg)
{
    TUpdateComponentConfigReq* pMqConfig = (TUpdateComponentConfigReq*)(pMsg);

    LogNotice("update m_componentRefMqInfo");

    m_componentConfigInfo = pMqConfig->m_componentConfig;
    
    if(m_componentConfigInfo.m_uMqId != m_uMqId)
    {
        LogNotice("c2s RtAndMo mq id has changed,oldMqid:%lu,newMqid:%lu.", 
            m_uMqId, m_componentConfigInfo.m_uMqId);
        
        amqp_rpc_reply_t rResult = amqp_channel_close(m_mqConnectionState, SMSP_C2S_CHANNELID, AMQP_REPLY_SUCCESS);
        if (AMQP_RESPONSE_NORMAL != rResult.reply_type)
        {
            LogError("amqp_channel_close fail! ChannelId[%d]", SMSP_C2S_CHANNELID);
        }

        if(false == amqpBasicConsume(SMSP_C2S_CHANNELID, m_componentConfigInfo.m_uMqId))
        {
            LogError("amqpBasicConsume fail! ChannelId[%d], MqId[%lu]", SMSP_C2S_CHANNELID, m_componentConfigInfo.m_uMqId);
        }
        else
        {
            LogNotice("amqpBasicConsume success! ChannelId[%d], MqId[%lu]", SMSP_C2S_CHANNELID, m_componentConfigInfo.m_uMqId);
        }
    }

    m_bConsumerSwitch = m_componentConfigInfo.m_uComponentSwitch;
    
    LogNotice("ComponentId[%d] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON" : "OFF");

    m_uMqId = m_componentConfigInfo.m_uMqId;
    
    UpdateChannelSpeedCtrl(m_uMqId);
}

int CMQIOConsumerThread::GetMemoryMqMessageSize()
{
    return g_pReportReceiveThread->GetMsgSize();
}

void CMQIOConsumerThread::processMessage(string& strData)
{
    LogElk("==c2s== get one report or mo message data:%s.",strData.data());
    TReportReciveRequest* pMqmsg = new TReportReciveRequest();
    pMqmsg->m_iMsgType = MSGTYPE_REPORTRECIVE_REQ;
    pMqmsg->m_strRequest.assign(strData);
    g_pReportReceiveThread->PostMsg(pMqmsg);
}
