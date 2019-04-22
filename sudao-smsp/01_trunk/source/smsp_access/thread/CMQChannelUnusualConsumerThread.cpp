#include "CMQChannelUnusualConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "HttpParams.h"
#include "BusTypes.h"

using namespace std;


CMQChannelUnusualConsumerThread::CMQChannelUnusualConsumerThread(const char *name):CMQConsumerThread(name)
{
}

CMQChannelUnusualConsumerThread::~CMQChannelUnusualConsumerThread()
{
}

bool CMQChannelUnusualConsumerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
    	LogError("CMQConsumerThread::Init() fail");
        return false;
    }
	
	m_mapChannelInfo.clear();
	g_pRuleLoadThread->getChannlelUnusualMap(m_mapChannelInfo);//SELECT * FROM t_sms_channel WHERE state='0';

	/*初始化拉取速度*/
	map<UInt64, MqConfig>::iterator itrMq;
	for(map<int, models::Channel>::iterator itor = m_mapChannelInfo.begin(); itor != m_mapChannelInfo.end(); itor++)
	{
		ChannelSpeedCtrl stuChannelSpeedCtrl;
		stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uSpeed;

		itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
		if(itrMq != m_mapMqConfig.end())
		{
			stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
			stuChannelSpeedCtrl.uMqId = itor->second.m_uMqId;
			SetChannelSpeedCtrl(itor->first, stuChannelSpeedCtrl);
		}
		else
		{
			LogError("Can not find MqId[%lu] of channel[%d] in m_mapMqConfig", itor->second.m_uMqId, itor->first);
			printf("[WARN] Can not find MqId[%lu] of channel[%d] in m_mapMqConfig\n", itor->second.m_uMqId, itor->first);
			continue;
		}
	}

	m_componentRefMqInfo.clear();
	g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);

	if (false == InitChannels())
	{
		printf("InitChannels is failed.\n");
		return false;
	}
	
    return true;
}

bool CMQChannelUnusualConsumerThread::InitChannels()
{
	for (map<int, models::Channel>::iterator itrChannel = m_mapChannelInfo.begin(); itrChannel !=m_mapChannelInfo.end(); ++itrChannel)
	{
		if (false == amqpBasicConsume(itrChannel->first, itrChannel->second.m_uMqId)) 
		{
			LogError("amqpBasicConsume channel [%d] failed", itrChannel->first);
			printf("[ERROR] amqpBasicConsume channel [%d] failed\n", itrChannel->first);
			continue;
		}
	}
	
	return true;
}

void CMQChannelUnusualConsumerThread::HandleMsg(TMsg* pMsg)
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
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
    return;
}

void CMQChannelUnusualConsumerThread::HandleChannelUpdateRep(TMsg* pMsg)
{
	TUpdateChannelReq* pChannel = (TUpdateChannelReq*)pMsg;
	LogNotice("update close channel size:%d",pChannel->m_ChannelMap.size());

	///close_channel delete,close MQ channels has been not closed channel.
	for (map<int, models::Channel>::iterator itrChannel = m_mapChannelInfo.begin();itrChannel !=m_mapChannelInfo.end();)
	{
		map<int, models::Channel>::iterator itr = pChannel->m_ChannelMap.find(itrChannel->first);
		if (itr == pChannel->m_ChannelMap.end())
		{
			LogWarn("Channelid[%d] is not close, close this channel mq channels now.", itrChannel->first);

			amqp_rpc_reply_t ret = amqp_channel_close(m_mqConnectionState, itrChannel->first, AMQP_REPLY_SUCCESS);

			if (AMQP_RESPONSE_NORMAL != ret.reply_type)
			{
				LogFatal("amqp_channel_close fail[reply_type:%d ChannelId:%d]", ret.reply_type, itrChannel->first);
				itrChannel++;
			}
			else
			{
				m_mapChannelInfo.erase(itrChannel++);
			}
		}
		else
		{
			itrChannel++;
		}
	}

	////close_channel add or update
    for(map<int, models::Channel>::iterator itrNew = pChannel->m_ChannelMap.begin();itrNew != pChannel->m_ChannelMap.end(); ++itrNew)
    {
		map<int, models::Channel>::iterator itrOld = m_mapChannelInfo.find(itrNew->first);
		if (itrOld == m_mapChannelInfo.end())
		{
			LogNotice("add close_channel channelid:%d.",itrNew->first);

			if (false == amqpBasicConsume(itrNew->first, itrNew->second.m_uMqId))
			{
				LogFatal("channelid:%d,channels is create failed.",itrNew->first);
			}
			else
			{
				m_mapChannelInfo.insert(make_pair(itrNew->first, itrNew->second));
			}
		}
		else
		{
			itrOld->second = itrNew->second;
		}
	}

	/*更新速度*/
	m_mapChannelSpeedCtrl.clear();
	map<UInt64, MqConfig>::iterator itrMq;
	for(map<int, models::Channel>::iterator itor = m_mapChannelInfo.begin(); itor != m_mapChannelInfo.end(); itor++)
	{
		ChannelSpeedCtrl stuChannelSpeedCtrl;
		stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uSpeed;

		itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
		if(itrMq != m_mapMqConfig.end())
		{
			stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
			stuChannelSpeedCtrl.uMqId = itor->second.m_uMqId;
			SetChannelSpeedCtrl(itor->first, stuChannelSpeedCtrl);
		}
		else
		{
			LogError("Can not find MqId[%lu] in m_mapMqConfig");
		}
	}
}

int CMQChannelUnusualConsumerThread::GetMemoryMqMessageSize()
{
	return g_pMQIOProducerThread->GetMsgSize();
}

void CMQChannelUnusualConsumerThread::processMessage(string& strData)
{
	strData.append("&errordate=");
	UInt64 uNowTime = time(NULL);
	char strErrorDate[32] = {0};
	snprintf(strErrorDate, sizeof(strErrorDate), "%lu", uNowTime);
	strData.append(strErrorDate);

	string strTemp = strData;
	web::HttpParams param;
	param.Parse(strTemp);

	UInt32 uOperater = atoi(param._map["operater"].c_str());
	UInt32 uSmsType = atoi(param._map["smsType"].c_str());

	/*
	11：异常移动行业，
	12：异常移动营销，
	13：异常联通行业，
	14：异常联通营销，
	15：异常电信行业，
	16：异常电信营销
	*/

	string strMessageType = "";
	if (YIDONG == uOperater)
	{
		if (5 == uSmsType || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
		{
			strMessageType.assign("12");
		}
		else
		{
			strMessageType.assign("11");
		}
	}
	else if (DIANXIN == uOperater)
	{
		if (5 == uSmsType || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
		{
			strMessageType.assign("16");
		}
		else
		{
			strMessageType.assign("15");
		}
	}
	else if ((LIANTONG == uOperater) || (FOREIGN == uOperater))
	{
		if (5 == uSmsType || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
		{
			strMessageType.assign("14");
		}
		else
		{
			strMessageType.assign("13");
		}
	}
	else 
	{	
		LogError("==except== phoneType:%u is invalid.",uOperater);
		return;
	}

	string strKey = "";
	char temp[250] = {0};
	snprintf(temp,250,"%lu_%s_0",g_uComponentId,strMessageType.data());
	strKey.assign(temp);

	map<string,ComponentRefMq>::iterator itReq =  m_componentRefMqInfo.find(strKey);
	if (itReq == m_componentRefMqInfo.end())
	{
		LogError("==except== strKey:%s is not find in m_componentRefMqInfo.",strKey.data());
		return;
	}

	map<UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(itReq->second.m_uMqId);
	if (itrMq == m_mapMqConfig.end())
	{
		LogError("==except== mqid:%lu is not find in m_mqConfigInfo.",itReq->second.m_uMqId);
		return;
	}

	string strExchange = itrMq->second.m_strExchange;
	string strRoutingKey = itrMq->second.m_strRoutingKey;

	LogNotice("==direct push except== exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());

	TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
	pMQ->m_strData = strData;
	pMQ->m_strExchange = strExchange;		
	pMQ->m_strRoutingKey = strRoutingKey;	
	pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
	g_pMQIOProducerThread->PostMsg(pMQ);
}
