#include "CMQReportConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>

using namespace std;

CMQReportConsumerThread::CMQReportConsumerThread(const char *name):CMQConsumerThread(name)
{
}

CMQReportConsumerThread::~CMQReportConsumerThread()
{
}

bool CMQReportConsumerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
        LogError("CMQConsumerThread::Init() fail");
        return false;
    }
    
    if(false == InitChannels())
    {
        //return false;
        LogError("CMQReportConsumerThread::InitChannels() fail");
    }

    return true;
}

bool CMQReportConsumerThread::InitChannels()
{
    map<string,ComponentRefMq> mapComponentRefMq;
    g_pRuleLoadThread->getComponentRefMq(mapComponentRefMq);
    m_mapChannelSpeedCtrl.clear();
    /*根据Key查找当前组件与mq的关联信息*/
    char Key[32] = {0};
    snprintf(Key, sizeof(Key), "%lu_%s_%u", g_uComponentId, MESSAGE_TYPE_REPORT.data(), CONSUMER_MODE);
    string strKey;
    strKey.append(Key);
    map<string,ComponentRefMq>::iterator itr = mapComponentRefMq.find(strKey);
    if(itr == mapComponentRefMq.end())
    {
        LogError("Can not find [component_id[%lu]__message_type[%s]__mode][%d] in t_sms_component_ref_mq",
            g_uComponentId, MESSAGE_TYPE_REPORT.data(), CONSUMER_MODE);
        return false;
    }
    /*保存当前组件的mq信息*/
    m_componentRefMqInfo =  itr->second;
    LogDebug("Componentid[%lu] Queue[%s]", g_uComponentId, itr->first.c_str());

    /*拉取这些队列的消息*/
    ChannelSpeedCtrl stuChannelSpeedCtrl;
    stuChannelSpeedCtrl.uChannelSpeed = itr->second.m_uGetRate;
    map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(itr->second.m_uMqId);
    if(itrMq != m_mapMqConfig.end())
    {
        stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
        stuChannelSpeedCtrl.uMqId = itr->second.m_uMqId;
        SetChannelSpeedCtrl(itr->second.m_uId, stuChannelSpeedCtrl);
        m_uMqId = itr->second.m_uMqId;
    }
    else
    {
        LogError("m_mapMqConfig can not find MqId[%lu]", itr->second.m_uMqId);
        return false;
    }

    /*打开通道队列*/
    if (false == amqpBasicConsume(itr->second.m_uId, itr->second.m_uMqId))
    {
        LogWarn("amqpBasicConsume is failed");
        return false;
    }

    LogNotice("Init Channel Over!m_mapChannelSpeedCtrl size[%d]", m_mapChannelSpeedCtrl.size());
    
    return true;
}

void CMQReportConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQConsumerThread::HandleCommonMsg(pMsg))
    {
        return;
    }
    
    switch(pMsg->m_iMsgType)
    {
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

void CMQReportConsumerThread::HandleComponentRefMqRep(TMsg * pMsg)
{
    TUpdateComponentRefMqConfigReq * pRefMq = (TUpdateComponentRefMqConfigReq *)pMsg;
    map<std::string,ComponentRefMq> mapComponentRefMqInfo;
    mapComponentRefMqInfo.clear();
    mapComponentRefMqInfo = pRefMq->m_componentRefMqInfo;
    LogNotice("upddate ComponentRefMq mapComponentRefMqInfo size[%d]",mapComponentRefMqInfo.size());
    m_mapChannelSpeedCtrl.clear();

    /*根据Key查找当前组件与mq的关联信息*/
    char Key[32] = {0};
    snprintf(Key, sizeof(Key), "%lu_%s_%u", g_uComponentId, MESSAGE_TYPE_REPORT.data(), CONSUMER_MODE);
    string strKey;
    strKey.append(Key);
    map<string,ComponentRefMq>::iterator itor = mapComponentRefMqInfo.find(strKey);

    if(itor == mapComponentRefMqInfo.end())
    {
        LogError("Can not find [component_id[%lu]__message_type[%s]__mode][%d] in t_sms_component_ref_mq",
            g_uComponentId, MESSAGE_TYPE_REPORT.data(), CONSUMER_MODE);
        return;
    }
    
    if(itor->second.m_uMqId != m_uMqId)
    {
        LogNotice("t_sms_component_configure MqId[%lu], t_sms_component_ref_mq MqId[%lu], not the same!", 
            m_uMqId, itor->second.m_uMqId);
        //先取消订阅
        amqp_channel_close(m_mqConnectionState, itor->second.m_uId, AMQP_REPLY_SUCCESS);

        //再订阅
        if(false == amqpBasicConsume(itor->second.m_uId, itor->second.m_uMqId))
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
        SetChannelSpeedCtrl(itor->second.m_uId, stuChannelSpeedCtrl);
    }
    else
    {
        LogError("Can not find MqId[%d] in m_mapMqConfig", m_uMqId);
    }
}


int CMQReportConsumerThread::GetMemoryMqMessageSize()
{
    return g_pReportMQConsumerThread->GetMsgSize();
}

void CMQReportConsumerThread::processMessage(string& strData)
{
    LogNotice("get one report message data:%s.",strData.data());
    map<string,string> mapValue;
    Comm::splitExMap(strData,string("&"),mapValue);
    TReportToDispatchReqMsg* pMsg = new TReportToDispatchReqMsg();
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    pMsg->m_uChannelId = atoi(mapValue["channelid"].data());
    pMsg->m_strChannelSmsId = mapValue["channelsmsid"];
    pMsg->m_strPhone = mapValue["phone"];
    pMsg->m_iStatus = atoi(mapValue["status"].data());
    pMsg->m_strDesc = Base64::Decode(mapValue["reportdesc"]);
    pMsg->m_bSaveReport = false;
    pMsg->m_lReportTime = strtol(mapValue["reporttime"].data(),0,0);
    pMsg->m_uChannelIdentify = atoi(mapValue["identify"].data());
    pMsg->m_uClusterType = atoi(mapValue["cluster_type"].data());
    pMsg->m_iMsgType = MSGTYPE_REPORT_TO_DISPATCH_REQ;
    g_pDispatchThread->PostMsg(pMsg);
    
}
