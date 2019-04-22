#include "CMQAckConsumerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "base64.h"
#include "Comm.h"
using namespace std;

CMQAckConsumerThread ::CMQAckConsumerThread (const char *name):CThread(name)
{
	m_bConsumerSwitch = false;
	m_mapChannelSpeedCtrl.clear();
	m_iSumChannelSpeed = 0;
	m_iCurrentSpeed = 0;
}

CMQAckConsumerThread ::~CMQAckConsumerThread ()
{
	amqp_connection_close(m_mqConnectionState, AMQP_REPLY_SUCCESS);
	amqp_destroy_connection(m_mqConnectionState);
}

bool CMQAckConsumerThread ::close()
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

bool CMQAckConsumerThread ::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
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

	PropertyUtils::GetValue("mq_prefetchcount", m_uFetchCount);
	m_uFetchCount = (m_uFetchCount <= 0) ? 1 : m_uFetchCount;
	LogNotice("ComponentId[%d] m_iFetchCount is %d", g_uComponentId,m_uFetchCount);
    return true;
}

void CMQAckConsumerThread ::SetChannelSpeedCtrl(UInt32 iChannelId, ChannelSpeedCtrl stuChannelSpeedCtrl)
{
	if(m_mapChannelSpeedCtrl.empty())
	{
		m_iSumChannelSpeed = 0;
	}
	m_iSumChannelSpeed += stuChannelSpeedCtrl.uChannelSpeed;
	map<UInt32, ChannelSpeedCtrl>::iterator itr = m_mapChannelSpeedCtrl.find(iChannelId);
	if(itr == m_mapChannelSpeedCtrl.end())
	{
		m_mapChannelSpeedCtrl.insert(make_pair(iChannelId, stuChannelSpeedCtrl));
	}
	else
	{
		itr->second.strQueue = stuChannelSpeedCtrl.strQueue ;
		itr->second.uChannelSpeed = stuChannelSpeedCtrl.uChannelSpeed ;
		itr->second.uMqId = stuChannelSpeedCtrl.uMqId;
	}

	LogNotice("ChannelId[%d] LimitSpeed[%d] SumChannelSpeed[%d] Queue[%s] MqId[%lu]",
		iChannelId, stuChannelSpeedCtrl.uChannelSpeed, m_iSumChannelSpeed, stuChannelSpeedCtrl.strQueue.data(), stuChannelSpeedCtrl.uMqId);
}
//check sum speed
bool CMQAckConsumerThread::ConsumerSpeedCheck()
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
//check channel speed
bool CMQAckConsumerThread ::ConsumerSpeedCheck(ChannelSpeedCtrl& SpeedInfo)
{
	struct timeval tvStartTime;
	struct timeval tvEndTime;
	UInt64 dwTime = 0;
	if(0 == SpeedInfo.uCurrentSpeed)
	{
		gettimeofday(&tvStartTime, NULL);
		SpeedInfo.uLastTime = 1000000*tvStartTime.tv_sec + tvStartTime.tv_usec;
	}
	if(SpeedInfo.uCurrentSpeed >= SpeedInfo.uChannelSpeed)
	{
		//get time2
		gettimeofday(&tvEndTime, NULL);
		dwTime = 1000000*tvEndTime.tv_sec + tvEndTime.tv_usec - SpeedInfo.uLastTime;

		if(dwTime < 1000*1000)	//	1s
		{
			return false;
		}
		else
		{
			SpeedInfo.uCurrentSpeed = 0;
		}
	}
	return true;
}

bool CMQAckConsumerThread ::AllChannelAckCheck()
{
	map<UInt32, ChannelSpeedCtrl>::iterator itr = m_mapChannelSpeedCtrl.begin();
	for(; itr != m_mapChannelSpeedCtrl.end(); itr++)
	{
		amqpBasicChannelAck(itr->first,itr->second);
	}
	return true;
}
bool CMQAckConsumerThread ::amqpBasicChannelAck(UInt32 uChannelId,ChannelSpeedCtrl& SpeedInfo)
{
	int result;
	int maxcount = 0;
	if(SpeedInfo.uDeliveryTag == 0)
	{
		return true;
	}

	do
	{
		result = amqp_basic_ack(m_mqConnectionState, uChannelId, SpeedInfo.uDeliveryTag, 1);
		if(0 == result)
		{
			break;
		}
		usleep(1000);
	}while(++maxcount <= 3);

	if(result != 0)
	{
		LogWarn("uChannelId[%u],DeiveryTag[%lu] ack fail[%d]",uChannelId,SpeedInfo.uDeliveryTag,result);
		return false;
	}
	LogNotice("uChannelId[%u],DeiveryTag[%lu] ack succ!!",uChannelId,SpeedInfo.uDeliveryTag);
	SpeedInfo.uDeliveryTag = 0;
	SpeedInfo.uCount = 0;
	return true;
}
bool CMQAckConsumerThread::amqpBasicChannelCancel(UInt32 uChannelId)
{
	std::string strChannelId = Comm::int2str(uChannelId);
	amqp_basic_cancel(m_mqConnectionState, uChannelId, amqp_cstring_bytes(strChannelId.c_str()));
	amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
	if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
	{
	    LogWarn("ChannelId[%d] pause consume failed. reply_type:%d", uChannelId, rConsume.reply_type);
	   	return false;
	}

	LogNotice("ChannelId[%d] pause consume success.", uChannelId);
	return true;
}
bool CMQAckConsumerThread ::amqpBasicChannelOpen(UInt32 uChannelId, UInt64 uMqId)
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

	amqp_basic_qos(m_mqConnectionState, uChannelId, 0, m_uFetchCount, 0);

	amqp_rpc_reply_t rSetQos = amqp_get_rpc_reply(m_mqConnectionState);
	if (AMQP_RESPONSE_NORMAL != rSetQos.reply_type)
	{
		LogError("channelid:%d,reply_type:%d is amqp_basic_qos failed.",uChannelId,rSetQos.reply_type);
		return false;
	}

	return true;
}
bool CMQAckConsumerThread ::amqpBasicConsume(UInt32 uChannelId, UInt64 uMqId)
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
		0, 0 , 0, amqp_empty_table);
	amqp_rpc_reply_t rConsume = amqp_get_rpc_reply(m_mqConnectionState);
	if (AMQP_RESPONSE_NORMAL != rConsume.reply_type)
	{
		LogError("channelid:%d,reply_type:%d is amqp_basic_consume failed.",uChannelId,rConsume.reply_type);
		return false;
	}

	return true;
}

bool CMQAckConsumerThread ::RabbitMQConnect()
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

bool CMQAckConsumerThread ::HandleCommonMsg(TMsg* pMsg)
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
				if(true == m_bConsumerSwitch)
				{
					close();
					RabbitMQConnect();
					InitChannels();
				}
				else
				{
					AllChannelAckCheck();
					AllChannelConsumerPause();
				}
			}
			result = true;
			break;
		}
		#endif
    }
	return result;
}

void CMQAckConsumerThread ::MainLoop()
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
		if(m_bConsumerSwitch)
		{
			if(false == ConsumerSpeedCheck())
				continue;

			string	strData = "";
			int 	result	= AMQP_CONSUME_MESSAGE_READY;
			result = amqpConsumeMessage(strData);
			if(IS_RABBITMQ_EXCEPTION(result))
			{
				close();
				RabbitMQConnect();
				InitChannels();
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
		else
		{
			usleep(1000*1000);
		}
	}
	return;
}

int CMQAckConsumerThread ::amqpConsumeMessage(string& strData)
{
	//int iMemMqQueueSize = GetMemoryMqMessageSize();

	/*检查是否有冻结的通道队列已经超时，释放消息*/
	//ReleaseBlockedChannels(uCurrentTime, iMemMqQueueSize);

	amqp_rpc_reply_t ret;
	amqp_envelope_t  envelope;
	UInt32 	uChannelId = 0;
	UInt64 	uDeliveryTag = 0;
	int     result;
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
				AllChannelAckCheck();
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
	uChannelId = envelope.channel;
	uDeliveryTag = envelope.delivery_tag;
	LogNotice("ChannelID[%d]  exchange[%.*s] routingkey[%.*s]",
		envelope.channel,
	    envelope.exchange.len, (char *)envelope.exchange.bytes,
	    envelope.routing_key.len, (char *)envelope.routing_key.bytes);

	amqp_destroy_envelope(&envelope);

	map<UInt32, ChannelSpeedCtrl>::iterator itor = m_mapChannelSpeedCtrl.find(uChannelId);
	if(itor != m_mapChannelSpeedCtrl.end())
	{
		itor->second.uDeliveryTag = uDeliveryTag;
		itor->second.uCurrentSpeed++;
		itor->second.uCount++;
		if(itor->second.uCount >= m_uFetchCount)
		{
			if(false == amqpBasicChannelAck(uChannelId,itor->second))
			{
				AllChannelAckCheck();
				return AMQP_BASIC_ACK_FAIL;
			}
		}
	}
	else
	{
		LogWarn("rabbitmq send message error, ChannelId[%u]",uChannelId);
		result = amqp_basic_reject(m_mqConnectionState, uChannelId, uDeliveryTag, 0);

		if(result != 0)
		{
			return AMQP_BASIC_REJECT_FAIL;
		}

		/*关闭这条通道取消订阅*/
		amqp_channel_close(m_mqConnectionState, uChannelId, AMQP_REPLY_SUCCESS);

		return AMQP_BASIC_REJECT_OK;
	}



	return AMQP_BASIC_GET_MESSAGE_OK;
}

