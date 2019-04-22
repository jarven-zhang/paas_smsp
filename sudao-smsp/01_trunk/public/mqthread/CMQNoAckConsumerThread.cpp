#include "CMQNoAckConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "base64.h"

using namespace std;

CMQNoAckConsumerThread::CMQNoAckConsumerThread(const char *name):CThread(name)
{
	m_bConsumerSwitch = false;
	m_mapChannelSpeedCtrl.clear();
	m_iSumChannelSpeed = 0;
}

CMQNoAckConsumerThread::~CMQNoAckConsumerThread()
{
	amqp_connection_close(m_mqConnectionState, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(m_mqConnectionState);
}

bool CMQNoAckConsumerThread::close()
{
	amqp_connection_close(m_mqConnectionState, AMQP_REPLY_SUCCESS);

	int ret = amqp_destroy_connection(m_mqConnectionState);
	if(AMQP_STATUS_OK != ret)
	{
		LogWarn("reply_type:%d is amqp_destroy_connection failed.\n",ret);
		return false;
	}
	return true;
}

bool CMQNoAckConsumerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if(false == CThread::Init())
    {
    	LogError("CThread::Init fail");
        return false;
    }

	m_strIp.assign(strIp);
	m_uPort = uPort;
	m_strUserName.assign(strUserName);
	m_strPassWord.assign(strPassWord);

	if (false == RabbitMQConnect())
	{
		LogError("RabbitMQConnect is failed.");
		return false;
	}

	#ifdef RULELOAD_GET_MQ_CONFIG
	g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
	#endif

	#ifdef RULELOAD_GET_COMPONENT_SWITCH
	m_bConsumerSwitch = g_pRuleLoadThread->getComponentSwitch(g_uComponentId);
	LogNotice("ComponentId[%d] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON" : "OFF");
	#endif

    return true;
}

void CMQNoAckConsumerThread::SetChannelSpeedCtrl(int iChannelId, ChannelSpeedCtrl stuChannelSpeedCtrl)
{
	if(m_mapChannelSpeedCtrl.empty())
	{
		m_iSumChannelSpeed = 0;
	}

	m_iSumChannelSpeed += stuChannelSpeedCtrl.uChannelSpeed;

	m_mapChannelSpeedCtrl.insert(make_pair(iChannelId, stuChannelSpeedCtrl));

	LogNotice("ChannelId[%d] LimitSpeed[%d] SumChannelSpeed[%d] Queue[%s] MqId[%lu]",
		iChannelId, stuChannelSpeedCtrl.uChannelSpeed, m_iSumChannelSpeed, stuChannelSpeedCtrl.strQueue.data(), stuChannelSpeedCtrl.uMqId);
}
bool CMQNoAckConsumerThread::ConsumerSpeedCheck()
{
	struct timeval tvEndTime;
	UInt64 dwTime = 0;
	if(0 == m_iCurrentSpeed)
	{
		gettimeofday(&m_tvStartTime, NULL);
	}
	if(m_iCurrentSpeed >= m_iSumChannelSpeed)
	{
		//get time2
		gettimeofday(&tvEndTime, NULL);
		dwTime = 1000000*(tvEndTime.tv_sec-m_tvStartTime.tv_sec)+(tvEndTime.tv_usec-m_tvStartTime.tv_usec);

		if(dwTime < 1000*1000)	//	1s
		{
			LogWarn("consumer read message too fast.currentSpeed[%d]",m_iCurrentSpeed);
			//too fast
			usleep(1000*1000 - dwTime);
			m_iCurrentSpeed = 0;
			gettimeofday(&m_tvStartTime, NULL);
			return false;
		}
		else
		{
			m_iCurrentSpeed = 0;
			gettimeofday(&m_tvStartTime, NULL);
			return true;
		}
	}
	return true;
}
bool CMQNoAckConsumerThread::amqpBasicChannelOpen(UInt32 uChannelId, UInt64 uMqId)
{
	map<UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(uMqId);
	if (itrMq == m_mapMqConfig.end())
	{
		LogError("mqid[%lu] not found in t_sms_mq_configure", uMqId);
		return false;
	}
	amqp_channel_open(m_mqConnectionState, uChannelId);
    amqp_rpc_reply_t rOpenChannel =  amqp_get_rpc_reply(m_mqConnectionState);
    if (AMQP_RESPONSE_NORMAL != rOpenChannel.reply_type)
    {
        LogError("channelid:%d,reply_type:%d is amqp_channel_open failed.",uChannelId,rOpenChannel.reply_type);
        return false;
    }
	return true;
}
bool CMQNoAckConsumerThread::amqpBasicConsume(UInt32 uChannelId, UInt64 uMqId)
{
	map<UInt64,MqConfig>::iterator itrMq = m_mapMqConfig.find(uMqId);
	if (itrMq == m_mapMqConfig.end())
	{
		LogError("mqid[%lu] not found in t_sms_mq_configure", uMqId);
		return false;
	}
	char strChannelId[32];
	memset(strChannelId, 0, sizeof(strChannelId));
	snprintf(strChannelId, sizeof(strChannelId), "%d", uChannelId);
	amqp_basic_consume(m_mqConnectionState, uChannelId, amqp_cstring_bytes(itrMq->second.m_strQueue.data()), amqp_cstring_bytes(strChannelId),
		0, 1 , 0, amqp_empty_table);
	amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
	if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
	{
		LogError("channelid:%d,reply_type:%d is amqp_basic_consume failed.",uChannelId,rConsume.reply_type);
		return false;
	}

	return true;
}

bool CMQNoAckConsumerThread::RabbitMQConnect()
{
	amqp_socket_t* pSocket = NULL;
	int iStatus = 0;
	m_mqConnectionState = amqp_new_connection();
	pSocket = amqp_tcp_socket_new(m_mqConnectionState);

	if (NULL == pSocket)
	{
		LogWarn("amqp_tcp_socket_new is failed");
		return false;
	}

	iStatus = amqp_socket_open(pSocket, m_strIp.data(), m_uPort);
	if (iStatus < 0)
	{
		LogWarn("amqp_socket_open is failed");
		return false;
	}

	amqp_rpc_reply_t rLogin =  amqp_login(m_mqConnectionState, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_strUserName.data(), m_strPassWord.data());
	if (AMQP_RESPONSE_NORMAL != rLogin.reply_type)
	{
		LogWarn("reply_type:%d is amqp_login failed",rLogin.reply_type);
		return false;
	}

	return true;
}

bool CMQNoAckConsumerThread::HandleCommonMsg(TMsg* pMsg)
{
	bool result =false;

    if(NULL == pMsg)
	{
		LogError("pMsg is NULL.");
		return true;
	}

    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
		#ifdef RULELOAD_GET_MQ_CONFIG
		case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
		{
			TUpdateMqConfigReq* pMqConfig = (TUpdateMqConfigReq*)pMsg;
			m_mapMqConfig.clear();
			m_mapMqConfig = pMqConfig->m_mapMqConfig;
			LogNotice("update t_sms_mq_configure size:%d.",m_mapMqConfig.size());
			result = true;
			break;
		}
		#endif

		#ifdef RULELOAD_GET_COMPONENT_SWITCH
		case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
		{
			bool bConsumerSwitch = g_pRuleLoadThread->getComponentSwitch(g_uComponentId);

			if(m_bConsumerSwitch != bConsumerSwitch)
			{
				m_bConsumerSwitch = bConsumerSwitch;
				LogWarn("ComponentID[%llu] m_bConsumerSwitch is %s", g_uComponentId, m_bConsumerSwitch ? "ON":"OFF");
				if(false == m_bConsumerSwitch)
				{
					AllChannelConsumerPause();
				}
				else
				{
					//AllChannelConsumerResume();
					close();
					RabbitMQConnect();
					InitChannels();
				}
			}
			result = true;
			break;
		}
		#endif
    }
	return result;
}

void CMQNoAckConsumerThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

	while(1)
	{
		m_pTimerMng->Click();
		pthread_mutex_lock(&m_mutex);
		TMsg* pMsg = m_msgQueue.GetMsg();
		pthread_mutex_unlock(&m_mutex);

		if(pMsg == NULL)
		{
			//do nothing
		}
		else
		{
			HandleMsg(pMsg);
			delete pMsg;
		}


		if(false == ConsumerSpeedCheck())
			continue;

		string	strData = "";
		int 	result	= AMQP_CONSUME_MESSAGE_READY;
		result = amqpConsumeMessage(strData);
		if(IS_RABBITMQ_EXCEPTION(result))
		{
			if(true == m_bConsumerSwitch)
			{
				close();
				RabbitMQConnect();
				InitChannels();
			}
			usleep(1000*1000);
		}
		else if (AMQP_COMPONENT_SWITCH_OFF == result)
		{
			usleep(1000*1000);
		}
		else
		{
			if(result == AMQP_BASIC_GET_MESSAGE_OK)
			{
				m_iCurrentSpeed++;
				processMessage(strData);
			}
			else
			{
				//其他的消息不用处理
			}
		}
	}
}

int CMQNoAckConsumerThread::amqpConsumeMessage(string& strData)
{
	//int iMemMqQueueSize = GetMemoryMqMessageSize();

	/*检查是否有冻结的通道队列已经超时，释放消息*/
	//ReleaseBlockedChannels(uCurrentTime, iMemMqQueueSize);

	amqp_rpc_reply_t ret;
	amqp_envelope_t  envelope;
	memset(&envelope, 0, sizeof(envelope));
	envelope.message.body.bytes = NULL;
	amqp_maybe_release_buffers(m_mqConnectionState);

	/*rabbitmq超时时间*/
	struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RABBITMQ_TIME_OUT;

	ret = amqp_consume_message(m_mqConnectionState, &envelope, &tv, 0);

	//LogDebug("amqp_consume_message ret.reply_type[%d] ret.reply.id[%x] ret.library_error[-0x%x]",
	//	ret.reply_type, ret.reply.id, 0 - ret.library_error);
	if((AMQP_RESPONSE_NORMAL != ret.reply_type) &&
		(false == m_bConsumerSwitch))
	{
		if(ret.library_error == AMQP_STATUS_TIMEOUT)
			amqp_destroy_envelope(&envelope);

		//LogError("switch off ret.library_error[-0x%x]", 0 - ret.library_error);
		return AMQP_COMPONENT_SWITCH_OFF;
	}
	switch(ret.reply_type)
	{
		case AMQP_RESPONSE_NORMAL:
		{
			break;
		}
		case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		{
			if(ret.library_error == AMQP_STATUS_TIMEOUT)
			{
				amqp_destroy_envelope(&envelope);
				return AMQP_CONSUME_MESSAGE_TIMEOUT;
			}
			else
			{
				LogError("ret.library_error[-0x%x]", 0 - ret.library_error);
				return AMQP_CONSUME_MESSAGE_FAIL;
			}
		}
		default:
		{
			LogError("amqp_consume_message ret.reply_type[%d] ret.reply.id[%x] ret.library_error[-0x%x]",
				ret.reply_type, ret.reply.id, 0 - ret.library_error);

			return AMQP_CONSUME_MESSAGE_FAIL;
		}
	}

	strData.append((char *)envelope.message.body.bytes, envelope.message.body.len);
	LogNotice("ChannelID[%d]  exchange[%.*s] routingkey[%.*s]",
		envelope.channel,
	    envelope.exchange.len, (char *)envelope.exchange.bytes,
	    envelope.routing_key.len, (char *)envelope.routing_key.bytes);

	amqp_destroy_envelope(&envelope);
	return AMQP_BASIC_GET_MESSAGE_OK;

}

