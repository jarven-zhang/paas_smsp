#include "CMQProducerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>

using namespace std;


CMQProducerThread::CMQProducerThread(const char *name): CThread(name)
{
    m_linkState = false;
    m_errMsg = NULL;
    m_strIp = "";
}

CMQProducerThread::~CMQProducerThread()
{
    amqp_channel_close(m_MQconn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(m_MQconn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(m_MQconn);
}


bool CMQProducerThread::Init(string strIp, unsigned int uPort, string strUserName, string strPassWord)
{
    if( false == CThread::Init())
    {
        printf("CThread::Init() fail.\n");
        return false;
    }

    m_strIp.assign(strIp);
    m_uPort = uPort;
    m_strUserName.assign(strUserName);
    m_strPassWord.assign(strPassWord);

    if ( !strIp.empty() && false == RabbitMQConnect())
    {
        printf("RabbitMQConnect is failed.\n");
        return false;
    }
    return true;
}

bool CMQProducerThread::close()
{
    amqp_channel_close(m_MQconn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(m_MQconn, AMQP_REPLY_SUCCESS);

    int ret = amqp_destroy_connection(m_MQconn);
    if(AMQP_STATUS_OK != ret)
    {
        printf("reply_type:%d is amqp_destroy_connection failed.\n", ret);
        LogWarn("reply_type:%d is amqp_destroy_connection failed.\n", ret);
        return false;
    }

    m_linkState = false;
    return true;
}

bool CMQProducerThread::RabbitMQConnect()
{
    amqp_socket_t *pSocket = NULL;
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
        printf("reply_type:%d is amqp_login failed.\n", rLonin.reply_type);
        LogWarn("reply_type:%d is amqp_login failed.\n", rLonin.reply_type);
        return false;
    }

    amqp_channel_open(m_MQconn, 1);
    amqp_rpc_reply_t rOpenChannel =  amqp_get_rpc_reply(m_MQconn);
    if (AMQP_RESPONSE_NORMAL != rOpenChannel.reply_type)
    {
        printf("reply_type:%d is amqp_channel_open failed.\n", rOpenChannel.reply_type);
        LogWarn("reply_type:%d is amqp_channel_open failed.\n", rOpenChannel.reply_type);
        return false;
    }

    m_Props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_PRIORITY_FLAG ;
    m_Props.content_type = amqp_cstring_bytes("text/plain");
    m_Props.delivery_mode = 2;

    m_linkState = true;
    return true;

}

void CMQProducerThread::MainLoop(void)
{
    WAIT_MAIN_INIT_OVER

    while(1)
    {
        //m_pTimerMng->Click();

        if( m_linkState || m_strIp.empty())
        {
            //add by fangjinxiong20161031 begin. get last failed msg
            if( NULL != m_errMsg && !m_strIp.empty())
            {
                //has msg to send
                HandlePublish(m_errMsg);
                delete m_errMsg;
                m_errMsg = NULL;
            }
            //add over
            pthread_mutex_lock(&m_mutex);
            TMsg *pMsg = m_msgQueue.GetMsg();
            pthread_mutex_unlock(&m_mutex);

            if(pMsg == NULL)
            {
                usleep(1000 * 10);
            }
            else
            {
                HandleMsg(pMsg);
                delete pMsg;
            }
        }
        else
        {
            close();
            RabbitMQConnect();
            usleep(1000 * 10);
        }
    }
}



void CMQProducerThread::HandleMsg(TMsg *pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
    case MSGTYPE_MQ_PUBLISH_REQ:
    {
        HandlePublish(pMsg);
        break;
    }
    default:
    {
        LogWarn("msgType:0x%x is invalid.", pMsg->m_iMsgType);
        break;
    }
    }
}

void CMQProducerThread::HandlePublish(TMsg *pMsg)
{
    if (pMsg == NULL)
    {
        LogError("pMsg is NULL");
        return;
    }
    TMQPublishReqMsg *pMQ = (TMQPublishReqMsg *)pMsg;

    /////?ù?YpMQ->m_uQueueType ??è?Dèòaµ?exchangeoíqueue
    /////string strExChange = "";
    /////string strRoutingKey = "";

    if( true != m_linkState || NULL == m_MQconn )
    {
        LogError("MQ [IP:%s, Port:%u, User:%s ] LinkState Error",
                 m_strIp.data(), m_uPort, m_strUserName.data());
        return ;
    }

    m_Props.priority = pMQ->m_iPriority;
    LogElk("==publish MQ== Exchange[%s],RoutingKey[%s],data[%s],priority[%d].", pMQ->m_strExchange.data(), pMQ->m_strRoutingKey.data(), pMQ->m_strData.data(), m_Props.priority);

    int iRet = amqp_basic_publish(m_MQconn, 1, amqp_cstring_bytes(pMQ->m_strExchange.data()), amqp_cstring_bytes(pMQ->m_strRoutingKey.data()),
                                  0, 0, &m_Props, amqp_cstring_bytes(pMQ->m_strData.data()));
    if (iRet < 0)
    {
        LogError("exchange[%s],routingkey[%s],msg[%s] is publish failed.", pMQ->m_strExchange.data(), pMQ->m_strRoutingKey.data(), pMQ->m_strData.data());
        close();
        RabbitMQConnect();

        //RESEND
        iRet = amqp_basic_publish(m_MQconn, 1, amqp_cstring_bytes(pMQ->m_strExchange.data()), amqp_cstring_bytes(pMQ->m_strRoutingKey.data()),
                                  0, 0, &m_Props, amqp_cstring_bytes(pMQ->m_strData.data()));
        if(iRet < 0)
        {
            LogError("rePublish failed,exchange[%s],routingkey[%s],msg[%s]", pMQ->m_strExchange.data(), pMQ->m_strRoutingKey.data(), pMQ->m_strData.data());

            if(m_errMsg)
            {
                //m_errMsg should be null
                LogError("no reason to go here");
            }
            else
            {
                //save unsend message
                m_errMsg = new TMQPublishReqMsg();
                m_errMsg->m_strExchange = pMQ->m_strExchange;
                m_errMsg->m_strRoutingKey = pMQ->m_strRoutingKey;
                m_errMsg->m_strData = pMQ->m_strData;
            }
        }
        else
        {
            LogError("rePublish suc,exchange[%s],routingkey[%s],msg[%s]", pMQ->m_strExchange.data(), pMQ->m_strRoutingKey.data(), pMQ->m_strData.data());
        }
        return;
    }

    return;
}


