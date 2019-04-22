#include "MqProducerThread.h"
#include "commonmessage.h"
#include "LogMacro.h"
#include "Fmt.h"


MqProducerThread::MqProducerThread()
{
    m_bProducerSwitch = false;
    m_conn = NULL;
    m_channel_id = 0;
    m_bLinkOk = false;
}

MqProducerThread::~MqProducerThread()
{
    close();
}

bool MqProducerThread::init()
{
    initChannelId();
    INIT_CHK_FUNC_RET_FALSE(MqThread::init());
    INIT_CHK_FUNC_RET_FALSE(connect());

    return true;
}

bool MqProducerThread::getLinkStatus()
{
    boost::lock_guard<boost::mutex> lk(m_mutexMq);
    return m_bLinkOk;
}

void MqProducerThread::setLinkStatus(bool bLinkOk)
{
    boost::lock_guard<boost::mutex> lk(m_mutexMq);
    m_bLinkOk = bLinkOk;
}

void MqProducerThread::initChannelId()
{
    // 在MQ的web前端channels页面，查看一个连接的channelId，规则：如下
    // 10.10.201.180:5330 (10)，即1(mq_c2s_io),0(线程池中0号线程)
    // 10.10.201.180:5330 (11)，即1(mq_c2s_io),1(线程池中1号线程)
    // 10.10.201.180:5330 (21)，即2(mq_send_io),1(线程池中1号线程)
    // 10.10.201.180:5330 (31)，即3(mq_c2s_db),1(线程池中1号线程)
    // 10.10.201.180:5330 (41)，即4(mq_send_db),1(线程池中1号线程)
    string strChannelId = Fmt<16>("%u%u", m_uiMqThreadType + 1, m_uiIndexInThreadPool);
    m_channel_id = to_uint<UInt16>(strChannelId);
}

bool MqProducerThread::connect()
{
    if (getLinkStatus())
    {
        return true;
    }

    m_conn = amqp_new_connection();
    CHK_NULL_RETURN_FALSE(m_conn);

    amqp_socket_t* pSocket = amqp_tcp_socket_new(m_conn);
    CHK_NULL_RETURN_FALSE(pSocket);

    if (amqp_socket_open(pSocket, m_strIp.data(), m_usPort) < 0)
    {
        LogNoticeT("Call amqp_socket_open failed. Ip:%s, Port:%u.",
            m_strIp.data(), m_usPort);

        return false;
    }

    CHK_AMQP_RPC_REPLY(amqp_login(m_conn,
                                "/",
                                0,
                                131072,
                                0,
                                AMQP_SASL_METHOD_PLAIN,
                                m_strUserName.data(),
                                m_strPassword.data()));

    m_properties.delivery_mode = 2;
    m_properties.content_type = amqp_cstring_bytes("text/plain");
    m_properties._flags = AMQP_BASIC_CONTENT_TYPE_FLAG
                        | AMQP_BASIC_DELIVERY_MODE_FLAG
                        | AMQP_BASIC_PRIORITY_FLAG ;

    amqp_channel_open(m_conn, m_channel_id);
    CHK_AMQP_RPC_REPLY(amqp_get_rpc_reply(m_conn));
    setLinkStatus(true);

    LogNoticeT("==> connect rabbitmq[%s:%u] channelid[%u], success <==",
        m_strIp.data(), m_usPort, m_channel_id);
    return true;
}

void MqProducerThread::close()
{
    if (!getLinkStatus()) return;

    amqp_channel_close(m_conn, m_channel_id, AMQP_REPLY_SUCCESS);
    amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(m_conn);
    setLinkStatus(false);

    LogNoticeT("==> close rabbitmq[%s:%u] channelid[%u], success <==",
        m_strIp.data(), m_usPort, m_channel_id);
}

void MqProducerThread::reconnect()
{
    close();
    connect();
}

bool MqProducerThread::checkExit()
{
    return m_bStop && m_msgMngQueue.empty() && m_msgQueueFailed.empty() && m_msgQueue.empty();
}

void MqProducerThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (1)
    {
        handleMngMsg();
        handleMqMsg();

        if (checkExit()) break;
    }

    LogWarnT("========> now exit <========");
    delete this;
}

void MqProducerThread::handleMngMsg()
{
    TMsg* pMsg = GetMngMsg();

    if (NULL != pMsg)
    {
        HandleMsg(pMsg);
        delete pMsg;
    }
    else
    {
        if (m_msgQueueFailed.empty() && m_msgQueue.empty())
        {
            select_sleep(SLEEP_TIME);
        }
    }
}

void MqProducerThread::handleMqMsg()
{
    if (!m_bProducerSwitch)
    {
        select_sleep(SLEEP_TIME);
        return;
    }

    if (!getLinkStatus())
    {
        reconnect();
    }
    else
    {
        handleFailedMqMsg();
        handleNormalMqMsg();
    }
}

void MqProducerThread::handleFailedMqMsg()
{
    TMsg* pMsg = m_msgQueueFailed.GetMsg();

    if (NULL != pMsg)
    {
        HandleMsg(pMsg);
        delete pMsg;
    }
}

void MqProducerThread::handleNormalMqMsg()
{
    pthread_mutex_lock(&m_mutex);
    TMsg* pMsg = m_msgQueue.GetMsg();
    pthread_mutex_unlock(&m_mutex);

    if (NULL != pMsg)
    {
        HandleMsg(pMsg);
        delete pMsg;
    }
}

void MqProducerThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_MQ_PUBLISH_REQ:
        {
            handlePublishMsgReq(pMsg);
            break;
        }
        case MSGTYPE_MQ_SWITCH_UPDATE_REQ:
        {
            CommonMsg<bool>* pReq = (CommonMsg<bool>*)pMsg;

            if (m_bProducerSwitch != pReq->m_value)
            {
                LogWarnT("ProducerSwitch %s -> %s.",
                    m_bProducerSwitch?"ON":"OFF", (pReq->m_value)?"ON":"OFF");

                m_bProducerSwitch = pReq->m_value;
            }

            break;
        }
        case MSGTYPE_MQ_THREAD_EXIT_REQ:
        {
            CommonMsg<bool>* pReq = (CommonMsg<bool>*)pMsg;

            if (m_bStop != pReq->m_value)
            {
                LogWarnT("Thread Stop Flag %s -> %s.",
                    m_bStop?"ON":"OFF", (pReq->m_value)?"ON":"OFF");

                m_bStop = pReq->m_value;
            }

            break;
        }
        default:
        {
            LogErrorT("Invalid MsgType[%x].", pMsg->m_iMsgType);
            break;
        }
    }
}

void MqProducerThread::handlePublishMsgReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    MqPublishReqMsg* pReq = (MqPublishReqMsg*)pMsg;
    m_properties.priority = pReq->m_iPriority;

    int iRet = amqp_basic_publish(m_conn,
                                m_channel_id,
                                amqp_cstring_bytes(pReq->m_strExchange.data()),
                                amqp_cstring_bytes(pReq->m_strRoutingKey.data()),
                                0,
                                0,
                                &m_properties,
                                amqp_cstring_bytes(pReq->m_strData.data()));
    if (iRet < 0)
    {
        LogElk("==publish MQ failed== channelid[%u],"
            "Exchange[%s],RoutingKey[%s],priority[%d],data[%s].",
            m_channel_id,
            pReq->m_strExchange.data(),
            pReq->m_strRoutingKey.data(),
			pReq->m_iPriority,
            pReq->m_strData.data());

        MqPublishReqMsg* pFailedMsg = new MqPublishReqMsg();
        pFailedMsg->m_strExchange = pReq->m_strExchange;
        pFailedMsg->m_strRoutingKey = pReq->m_strRoutingKey;
        pFailedMsg->m_strData = pReq->m_strData;
        pFailedMsg->m_iPriority = pReq->m_iPriority;
        m_msgQueueFailed.AddMsg(pFailedMsg);

        reconnect();
    }
    else
    {
        LogElk("==publish MQ success== channelid[%u],"
            "Exchange[%s],RoutingKey[%s],priority[%d],data[%s].",
            m_channel_id,
            pReq->m_strExchange.data(),
            pReq->m_strRoutingKey.data(),
            pReq->m_iPriority,
            pReq->m_strData.data());
    }
}

