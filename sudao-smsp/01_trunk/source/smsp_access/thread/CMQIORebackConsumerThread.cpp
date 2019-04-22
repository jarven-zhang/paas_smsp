#include "CMQIORebackConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>

using namespace std;

CMQIORebackConsumerThread::CMQIORebackConsumerThread(const char *name):CMQConsumerThread(name)
{
}

CMQIORebackConsumerThread::~CMQIORebackConsumerThread()
{
}

bool CMQIORebackConsumerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if(false == CMQConsumerThread::Init(strIp, uPort, strUserName, strPassWord))
    {
    	LogError("CMQConsumerThread::Init() fail");
        return false;
    }

	if (false == InitChannels())
	{
		LogError("InitChannels is failed");
		return false;
	}

    return true;
}

bool CMQIORebackConsumerThread::InitChannels()
{
	map<string,ComponentRefMq> componentRefMq;
	g_pRuleLoadThread->getComponentRefMq(componentRefMq);
	m_componentRefMqInfo.clear();
	m_mapChannelSpeedCtrl.clear();
	for (map<string,ComponentRefMq>::iterator itr = componentRefMq.begin();itr !=componentRefMq.end(); ++itr)
	{
		if (g_uComponentId == itr->second.m_uComponentId && itr->second.m_uMode == 1)
		{
			if(	itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_YD_HY	||
				itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_YD_YX	||
				itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_LT_HY	||
				itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_LT_YX	||
				itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_DX_HY	||
				itr->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_DX_YX	  )
			{
				m_componentRefMqInfo.insert(make_pair(itr->first, itr->second));
				LogNotice("Componentid[%lu] Queue[%s]", g_uComponentId, itr->first.c_str());
			
				ChannelSpeedCtrl stuChannelSpeedCtrl;
				stuChannelSpeedCtrl.uChannelSpeed = itr->second.m_uGetRate;
				map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(itr->second.m_uMqId);
				if(itrMq != m_mapMqConfig.end())
				{
					stuChannelSpeedCtrl.uMqId = itr->second.m_uMqId;
					stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
					SetChannelSpeedCtrl(itr->second.m_uId, stuChannelSpeedCtrl);
				}
				else
				{
					LogError("m_mapMqConfig can not find MqId[%lu]", itr->second.m_uMqId);
					return false;
				}

				if (false == amqpBasicConsume(itr->second.m_uId, itr->second.m_uMqId))
				{
					LogWarn("amqpBasicConsume is failed");
					return false;
				}
			}
		}
	}

	LogNotice("Init Channel Over! m_componentRefMqInfo size[%d], m_mapChannelSpeedCtrl size[%d]", 
		m_componentRefMqInfo.size(), m_mapChannelSpeedCtrl.size());
	
	return true;
}

void CMQIORebackConsumerThread::HandleMsg(TMsg* pMsg)
{
    if(true == CMQConsumerThread::HandleCommonMsg(pMsg))
    {
		return;
	}

    switch(pMsg->m_iMsgType)
    {
		case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ: 	
		{	
			HandleQueueUpdateRep(pMsg);
			break;
		}
        default:
        {
            LogWarn("msg type[%x] is invalid!",pMsg->m_iMsgType);
            break;
        }
    }
}

void CMQIORebackConsumerThread::HandleQueueUpdateRep(TMsg* pMsg)
{
	TUpdateComponentRefMqReq* pComRefMqUpdate = (TUpdateComponentRefMqReq*)pMsg;

	map<string,ComponentRefMq> tempComponentRefMq;/*最新*/
	for (map<string,ComponentRefMq>::iterator itrTemp = pComRefMqUpdate->m_componentRefMqInfo.begin(); itrTemp !=pComRefMqUpdate->m_componentRefMqInfo.end(); ++itrTemp)
	{
		if ((g_uComponentId == itrTemp->second.m_uComponentId) && (itrTemp->second.m_uMode == 1))
		{
			if(	itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_YD_HY 	||
				itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_YD_YX 	||
				itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_LT_HY 	||
				itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_LT_YX 	||
				itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_DX_HY 	||
				itrTemp->second.m_strMessageType == MESSAGE_TYPE_ABNORMAL_DX_YX  	 )
			{
				tempComponentRefMq.insert(make_pair(itrTemp->first,itrTemp->second));
			}
		}
	}

	LogNotice("m_componentRefMqInfo update, old size[%d], new size[%d]", m_componentRefMqInfo.size(), tempComponentRefMq.size());

	/*新开*/
	for (map<string,ComponentRefMq>::iterator it1 = tempComponentRefMq.begin(); it1 != tempComponentRefMq.end(); it1++)
	{
		map<int, ChannelSpeedCtrl>::iterator it2 = m_mapChannelSpeedCtrl.find(it1->second.m_uId);
		if(it2 == m_mapChannelSpeedCtrl.end())
		{
			if(false == amqpBasicConsume(it1->second.m_uId, it1->second.m_uMqId))
			{
				LogError("amqpBasicConsume fail. ChannelId[%d]", it1->second.m_uId);
			}
			else
			{
				LogNotice("amqpBasicConsume, ChannelId[%d]", it1->second.m_uId);
			}
		}
		else
		{
			//检查是否已经修改MqId
			if(it2->second.uMqId != it1->second.m_uMqId)
			{
				LogNotice("ChannelId[%d] Changed Mqid[%lu] to [%lu], ReConsume...", it1->second.m_uId, it2->second.uMqId, it1->second.m_uMqId);
				
				//先取消订阅
				amqp_channel_close(m_mqConnectionState, it1->second.m_uId, AMQP_REPLY_SUCCESS);
				
				//再重新订阅
				if(false == amqpBasicConsume(it1->second.m_uId, it1->second.m_uMqId))
				{
					LogError("ReConsume Fail!!!");
				}
			}
		}
	}

	/*关闭*/
	for (map<string, ComponentRefMq>::iterator it1 = m_componentRefMqInfo.begin(); it1 != m_componentRefMqInfo.end(); it1++)
	{
		map<string, ComponentRefMq>::iterator it2 = tempComponentRefMq.find(it1->first);

		if(it2 == tempComponentRefMq.end())
		{
			amqp_rpc_reply_t ret = amqp_channel_close(m_mqConnectionState, it1->second.m_uId, AMQP_REPLY_SUCCESS);
			if(ret.reply_type != AMQP_RESPONSE_NORMAL)
			{
				LogError("amqp_channel_close fail, reply_type[%d] ChannelId[%d]", ret.reply_type, it1->second.m_uId);
			}
			else
			{
				LogNotice("amqp_channel_close ChannelId[%d]", it1->second.m_uId);
			}
		}
	}

	/*更新*/
	m_mapChannelSpeedCtrl.clear();
	m_componentRefMqInfo.clear();
	m_componentRefMqInfo = tempComponentRefMq;
	for(map<string, ComponentRefMq>::iterator itor = m_componentRefMqInfo.begin(); itor != m_componentRefMqInfo.end(); itor++)
	{
		ChannelSpeedCtrl stuChannelSpeedCtrl;
		stuChannelSpeedCtrl.uChannelSpeed = itor->second.m_uGetRate;

		map<UInt64, MqConfig>::iterator itrMq = m_mapMqConfig.find(itor->second.m_uMqId);
		if(itrMq != m_mapMqConfig.end())
		{
			stuChannelSpeedCtrl.strQueue = itrMq->second.m_strQueue;
			stuChannelSpeedCtrl.uMqId = itor->second.m_uMqId;
			SetChannelSpeedCtrl(itor->second.m_uId, stuChannelSpeedCtrl);
		}
		else
		{
			LogError("m_mapMqConfig can not find MqId[%lu]", itor->second.m_uMqId);
		}
	}

	LogNotice("Update Over! m_componentRefMqInfo size[%d], m_mapChannelSpeedCtrl size[%d]");
}

int CMQIORebackConsumerThread::GetMemoryMqMessageSize()
{
	return GetMsgSize();
}

void CMQIORebackConsumerThread::processMessage(string& strData)
{
	LogNotice("access get one io mq msg:%s.",strData.data());

	TMsg* pMqmsg = new TMsg();
	pMqmsg->m_iMsgType = MSGTYPE_MQ_GETMQREBACKMSG_REQ;
	pMqmsg->m_strSessionID.assign(strData);
	g_pSessionThread->PostMsg(pMqmsg);
}
