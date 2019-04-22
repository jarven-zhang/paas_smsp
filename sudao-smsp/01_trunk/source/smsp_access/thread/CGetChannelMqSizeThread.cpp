#include "CGetChannelMqSizeThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "LogMacro.h"

using namespace std;


CGetChannelMqSizeThread::CGetChannelMqSizeThread(const char *name):CThread(name)
{
	m_linkState = false;
	m_pGetChannelMqSizeTimer = NULL;
}

CGetChannelMqSizeThread::~CGetChannelMqSizeThread()
{
		amqp_channel_close(m_MQconn, 1, AMQP_REPLY_SUCCESS);
		amqp_connection_close(m_MQconn, AMQP_REPLY_SUCCESS);
		amqp_destroy_connection(m_MQconn);
}


bool CGetChannelMqSizeThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{

    if(!CThread::Init())
    {
    	printf("CThread::Init() fail.\n");
        return false;
    }

	m_strIp.assign(strIp);
	m_uPort = uPort;
	m_strUserName.assign(strUserName);
	m_strPassWord.assign(strPassWord);

	if (!RabbitMQConnect())
	{
		printf("RabbitMQConnect is failed.\n");
		return false;
	}

	m_mapMqConfig.clear();
	g_pRuleLoadThread->getMqConfig(m_mapMqConfig);

	m_mapChannelInfo.clear();
	g_pRuleLoadThread->getChannlelMap(m_mapChannelInfo);

	getChannelMqSize();

	m_pGetChannelMqSizeTimer = SetTimer(111, "GET_CHANNEL_MQ_SIZE", 1500);

    return true;
}

void CGetChannelMqSizeThread::getChannelMqSize()
{
	map<UInt32,UInt32> mapChannelMqSize;

	for (map<int, models::Channel>::iterator itrChannel = m_mapChannelInfo.begin(); itrChannel != m_mapChannelInfo.end(); ++itrChannel)
	{
		map<UInt64,MqConfig>::iterator itrMq =  m_mapMqConfig.find(itrChannel->second.m_uMqId);
		if (m_mapMqConfig.end() == itrMq)
		{
			LogError("mqid:%lu,is not find in m_mapMqConfig.");
			continue;
		}

		string strQueue = itrMq->second.m_strQueue;

		amqp_queue_declare_ok_t* pResult = amqp_queue_declare(m_MQconn, 1, amqp_cstring_bytes(strQueue.c_str()), 1, 0, 0, 1,amqp_empty_table);
		if (NULL == pResult)
		{
			LogError("pResult is NULL,channelid:%d is not create mq or mq server is cut link.",itrChannel->first);
			close();
			RabbitMQConnect();
			usleep(1000*100);
			break;
		}

		UInt32 uChannelMqSize = pResult->message_count;

		/////LogDebug("==test== channelid:%d,channelMqSize:%d.",itrChannel->first,uChannelMqSize);

		mapChannelMqSize.insert(make_pair(itrChannel->first,uChannelMqSize));
	}

	if ( 0 != mapChannelMqSize.size() )
	{
		CGetChannelMqSizeReqMsg* pGet = new CGetChannelMqSizeReqMsg();
		pGet->m_iMsgType = MSGTYPE_GET_CHANNEL_MQ_SIZE_REQ;
		pGet->m_mapGetChannelMqSize = mapChannelMqSize;
        CommPostMsg( STATE_ROUTE, pGet );
	}

	return;
}


bool CGetChannelMqSizeThread::close()
{
	amqp_channel_close(m_MQconn, 1, AMQP_REPLY_SUCCESS);
	amqp_connection_close(m_MQconn, AMQP_REPLY_SUCCESS);

	int ret = amqp_destroy_connection(m_MQconn);
	if(AMQP_STATUS_OK != ret)
	{
		printf("reply_type:%d is amqp_destroy_connection failed.\n",ret);
		LogWarn("reply_type:%d is amqp_destroy_connection failed.\n",ret);
		return false;
	}

	m_linkState = false;
	return true;
}

bool CGetChannelMqSizeThread::RabbitMQConnect()
{
	amqp_socket_t* pSocket = NULL;
	int iStatus = 0;
	m_MQconn = amqp_new_connection();
	pSocket = amqp_tcp_socket_new(m_MQconn);

	if (NULL == pSocket)
	{
		printf("amqp_tcp_socket_new is failed.\n");
		LogWarn("amqp_tcp_socket_new is failed.\n");
		return false;
	}

	iStatus = amqp_socket_open(pSocket, m_strIp.data(), m_uPort);
	if (iStatus < 0)
	{
		printf("amqp_socket_open is failed.\n");
		LogWarn("amqp_socket_open is failed.\n");
		return false;
	}

	amqp_rpc_reply_t rLonin =  amqp_login(m_MQconn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_strUserName.data(), m_strPassWord.data());
	if (AMQP_RESPONSE_NORMAL != rLonin.reply_type)
	{
		printf("reply_type:%d is amqp_login failed.\n",rLonin.reply_type);
		LogWarn("reply_type:%d is amqp_login failed.\n",rLonin.reply_type);
		return false;
	}

	amqp_channel_open(m_MQconn, 1);
	amqp_rpc_reply_t rOpenChannel =  amqp_get_rpc_reply(m_MQconn);
	if (AMQP_RESPONSE_NORMAL != rOpenChannel.reply_type)
	{
		printf("reply_type:%d is amqp_channel_open failed.\n",rOpenChannel.reply_type);
		LogWarn("reply_type:%d is amqp_channel_open failed.\n",rOpenChannel.reply_type);
		return false;
	}

	m_Props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	m_Props.content_type = amqp_cstring_bytes("text/plain");
	m_Props.delivery_mode = 2;

	m_linkState = true;
	return true;

}

void CGetChannelMqSizeThread::MainLoop(void)
{
    WAIT_MAIN_INIT_OVER

	while(true)
	{
		m_pTimerMng->Click();

		pthread_mutex_lock(&m_mutex);
		TMsg* pMsg = m_msgQueue.GetMsg();
		pthread_mutex_unlock(&m_mutex);

		if(pMsg == NULL)
		{
			usleep(10);
		}
		else
		{
			HandleMsg(pMsg);
			delete pMsg;
		}
	}
}

void CGetChannelMqSizeThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    switch (pMsg->m_iMsgType)
    {
		case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
		{
			TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)(pMsg);
			m_mapMqConfig.clear();

			m_mapMqConfig = pMqCon->m_mapMqConfig;

			LogNotice("update t_sms_mq_configure size:%d.",m_mapMqConfig.size());
			break;
		}
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
		{
			TUpdateChannelReq* msg = (TUpdateChannelReq*)pMsg;
			LogNotice("RuleUpdate channelMap update. map.size[%d]", msg->m_ChannelMap.size());
			m_mapChannelInfo.clear();

			m_mapChannelInfo = msg->m_ChannelMap;

			getChannelMqSize();

			delete m_pGetChannelMqSizeTimer;
   		 	m_pGetChannelMqSizeTimer = NULL;
			m_pGetChannelMqSizeTimer = SetTimer(111, "GET_CHANNEL_MQ_SIZE", 1500);

			break;
		}
        case MSGTYPE_TIMEOUT:
        {
			HandleTimeOut(pMsg);
            break;
        }

        default:
        {
			LogWarn("msgType:0x%x is invalid.",pMsg->m_iMsgType);
            break;
        }
    }
}

void CGetChannelMqSizeThread::HandleTimeOut(TMsg* pMsg)
{
	if(0 == pMsg->m_strSessionID.compare("GET_CHANNEL_MQ_SIZE"))
	{
		getChannelMqSize();
	}
	else
	{
		LogError("timer type:%s,is invalid.",pMsg->m_strSessionID.data());
	}

	delete m_pGetChannelMqSizeTimer;
 	m_pGetChannelMqSizeTimer = NULL;
	m_pGetChannelMqSizeTimer = SetTimer(111, "GET_CHANNEL_MQ_SIZE", 1500);
}



