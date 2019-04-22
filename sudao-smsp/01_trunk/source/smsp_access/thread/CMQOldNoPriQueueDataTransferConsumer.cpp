#include "CMQOldNoPriQueueDataTransferConsumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include <base64.h>
#include "HttpParams.h"
#include "ChannelPriorityHelper.h"
using namespace std;


CMQOldNoPriQueueDataTransferConsumer::CMQOldNoPriQueueDataTransferConsumer(const char *name):CMQConsumerThread(name)
{
}

CMQOldNoPriQueueDataTransferConsumer::~CMQOldNoPriQueueDataTransferConsumer()
{
}

bool CMQOldNoPriQueueDataTransferConsumer::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
    	LogError("CMQConsumerThread::Init() fail");
        return false;
    }
	
	m_mapChannelInfo.clear();
	g_pRuleLoadThread->getChannlelMap(m_mapChannelInfo);

    m_mapMqConfig.clear();
	g_pRuleLoadThread->getMqConfig(m_mapMqConfig);

	m_componentRefMqInfo.clear();
	g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);

    m_clientPriorities.clear();
    g_pRuleLoadThread->getClientPriorities(m_clientPriorities);

    return true;
}

bool CMQOldNoPriQueueDataTransferConsumer::InitChannels()
{
	for (map<int, ChannelSpeedCtrl>::iterator itSpeed = m_mapChannelSpeedCtrl.begin(); 
            itSpeed !=m_mapChannelSpeedCtrl.end(); ++itSpeed)
	{
        LogNotice("Init Channel[%d]", itSpeed->first);
		if (false == amqpBasicConsume(itSpeed->first, itSpeed->second.uMqId)) 
		{
			LogError("amqpBasicConsume is failed");
			return false;
		}
	}
	
	return true;
}

void CMQOldNoPriQueueDataTransferConsumer::HandleMsg(TMsg* pMsg)
{
    if(true == CMQConsumerThread::HandleCommonMsg(pMsg))
    {
		return;
	}

    switch(pMsg->m_iMsgType)
    {
		case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
		{
			HandleChannelUpdateRep(pMsg);
			break;
		}
		case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
		{
			TUpdateComponentRefMqReq* pMqCom = (TUpdateComponentRefMqReq*)pMsg;
			m_componentRefMqInfo.clear();

			m_componentRefMqInfo = pMqCom->m_componentRefMqInfo;

			LogNotice("update t_sms_component_ref_mq size:%d.",m_componentRefMqInfo.size());
			break;
		}
        case MSGTYPE_RULELOAD_CLIENT_PRIORITY_UPDATE_REQ:
        {
            TUpdateClientPriorityReq *pReq = (TUpdateClientPriorityReq*) pMsg;
            m_clientPriorities = pReq->m_clientPriorities;
			LogNotice("update t_sms_client_priority");
            break;
        }

        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
    return;
}

void CMQOldNoPriQueueDataTransferConsumer::HandleChannelUpdateRep(TMsg* pMsg)
{
	TUpdateChannelReq* pChannel = (TUpdateChannelReq*)pMsg;
    const std::map<int, Channel>& newChannelMap = pChannel->m_ChannelMap;
    LogNotice("channel update size %u", newChannelMap.size());

	/*更新速度*/
    for (map<int , ChannelSpeedCtrl>::iterator itSpeed = m_mapChannelSpeedCtrl.begin();
            itSpeed != m_mapChannelSpeedCtrl.end(); ++itSpeed)
    {
        map<int, models::Channel>::const_iterator itChnlTmp = newChannelMap.find(itSpeed->first);
        if (itChnlTmp != newChannelMap.end())
        {
            if ( itSpeed->second.uChannelSpeed != itChnlTmp->second.m_uSpeed)
            {
                itSpeed->second.uChannelSpeed = itChnlTmp->second.m_uSpeed;
                LogNotice("Channel[%d] speed changed to %d", itSpeed->first, itSpeed->second.uChannelSpeed);
            }
        }
    }

    std::map<int, Channel>::const_iterator cIterNew, cIterOld;
    for (cIterNew = newChannelMap.begin(); cIterNew != newChannelMap.end(); ++cIterNew)
    {
        int iChannelId = cIterNew->first;

        cIterOld = m_mapChannelInfo.find(iChannelId);
        // Go on when channel's mq_id changed
        if (cIterOld == m_mapChannelInfo.end()
                || cIterOld->second.m_uMqId == cIterNew->second.m_uMqId)
        {
            LogDebug("ignore channel[%d]'s change", iChannelId);
            continue;
        }

        map<UInt64,MqConfig>::const_iterator cIterNewMqCfg, cIterOldMqCfg;
        cIterNewMqCfg = m_mapMqConfig.find(cIterNew->second.m_uMqId);
        cIterOldMqCfg = m_mapMqConfig.find(cIterOld->second.m_uMqId);

        if (cIterNewMqCfg == m_mapMqConfig.end()
                || cIterOldMqCfg == m_mapMqConfig.end())
        {
            LogWarn("channel[%d]: mq configure of id %lu or %lu not found!", 
                    iChannelId, cIterNew->second.m_uMqId, cIterOld->second.m_uMqId);
            continue;
        }

        // Move MQ data from no-priority-queue to new priority-queue
        if ( ! ((cIterOldMqCfg->second.m_uMqType & MQTYPE_NORMAL) == MQTYPE_NORMAL
                && ((cIterNewMqCfg->second.m_uMqType & MQTYPE_PRIORITY) == MQTYPE_PRIORITY)))
        {
            LogDebug("channel[%d]: no need to transfer mq data. oldMqType[%lu:%u] VS newMqType[%lu:%u]", 
                    iChannelId,
                    cIterOldMqCfg->second.m_uMqId, cIterOldMqCfg->second.m_uMqType, 
                    cIterNewMqCfg->second.m_uMqId, cIterNewMqCfg->second.m_uMqType);
            continue;
        }

        LogNotice("Transfer channel[%d]'s mq data from mq_id[%s] to [%s]", 
                  iChannelId,
                  cIterOldMqCfg->second.m_strQueue.c_str(),
                  cIterNewMqCfg->second.m_strQueue.c_str());

        map<int, UInt64>::iterator chnlCacheIter = m_transferedChannels.find(iChannelId);
        // If it's new priority-channel-switch, 
        // OR old-chnl-config changed ( NoPriOldA->PriNewB->NoPriOldC => C diff with A, close A and consume C)
        if (chnlCacheIter == m_transferedChannels.end()
                || (chnlCacheIter->second != cIterOldMqCfg->second.m_uMqId))
        {
            // Old-chnl-config changed
            if (chnlCacheIter != m_transferedChannels.end())
            {
                LogNotice("old-chnl-mqId[%lu] of channel[%d] not match old-chnl-cache[%lu], close and replace it.", 
                          cIterOldMqCfg->second.m_uMqId, iChannelId, chnlCacheIter->second);
                amqp_channel_close(m_mqConnectionState, iChannelId, AMQP_REPLY_SUCCESS);
                m_transferedChannels.erase(chnlCacheIter);
            }

            // here use iChannelId as param of amqp-API's channelid
            if(false == amqpBasicConsume(iChannelId, cIterOldMqCfg->second.m_uMqId))
            {
                LogError("[try move channel '%d' mq-data] consume '%s' failed", 
                        iChannelId, 
                        cIterOldMqCfg->second.m_strQueue.c_str());
                continue;
            }

            m_transferedChannels[iChannelId] = cIterOldMqCfg->second.m_uMqId;

            LogNotice("Add new channel speed[%d]", iChannelId);
            ChannelSpeedCtrl stuChannelSpeedCtrl;
            stuChannelSpeedCtrl.uChannelSpeed = cIterNew->second.m_uSpeed;
            stuChannelSpeedCtrl.strQueue = cIterOldMqCfg->second.m_strQueue;
            stuChannelSpeedCtrl.uMqId = cIterOldMqCfg->second.m_uMqId;
            SetChannelSpeedCtrl(iChannelId, stuChannelSpeedCtrl);
        }
    }

    m_mapChannelInfo = pChannel->m_ChannelMap;
}

int CMQOldNoPriQueueDataTransferConsumer::GetMemoryMqMessageSize()
{
	return GetMsgSize();
}

void CMQOldNoPriQueueDataTransferConsumer::processMessage(string& strData)
{
	string strTemp = strData;
	web::HttpParams param;
	param.Parse(strTemp);

	int iChannelId = atoi(param._map["channelid"].c_str());
    std::string strSmsType = param._map["smsType"];
    std::string strSign = Base64::Decode(param._map["sign"]);
    std::string strClientId = param._map["clientId"];

    std::map<int, Channel>::const_iterator cIterChnl = m_mapChannelInfo.find(iChannelId);

    if (cIterChnl == m_mapChannelInfo.end())
    {
        LogError("Not found channel [%d]", iChannelId);
        return;
    }
     
	map<UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(cIterChnl->second.m_uMqId);
	if (itrMq == m_mapMqConfig.end())
	{
		LogError("==except== mqid:%lu is not find in m_mapMqConfig.", cIterChnl->second.m_uMqId);
		return;
	}

	string strExchange = itrMq->second.m_strExchange;
	string strRoutingKey = itrMq->second.m_strRoutingKey;

	LogNotice("==transfer old MQ data== exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());

    smsSessionInfo smsInfo;
    smsInfo.m_uChannelExValue = cIterChnl->second.m_uExValue;
    smsInfo.m_uChannleId = iChannelId;
    smsInfo.m_strSmsType = strSmsType;
    smsInfo.m_strSign = strSign;
    smsInfo.m_strClientId = strClientId;

	TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
	pMQ->m_strData = strData;
	pMQ->m_strExchange = strExchange;		
	pMQ->m_strRoutingKey = strRoutingKey;	
	pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
	pMQ->m_iPriority = ChannelPriorityHelper::GetClientChannelPriority(&smsInfo, m_clientPriorities);
	g_pMQSendIOProducerThread->PostMsg(pMQ);
}
