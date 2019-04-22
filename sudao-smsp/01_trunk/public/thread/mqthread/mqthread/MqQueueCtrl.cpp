#include "MqQueueCtrl.h"
#include "MqConsumerThread.h"
#include "CMQConsumerThread.h"
#include "LogMacro.h"
#include "Fmt.h"


const UInt32 RABBITMQ_WAIT_TIME_OUT = 7*1000;


MqQueueCtrl::MqQueueCtrl()
{
    initMqQueueCtrl();
}

MqQueueCtrl::MqQueueCtrl(UInt64 ulMqId,
                            cs_t strQueue,
                            UInt32 uiSpeed,
                            UInt32 uiPrefetchCount,
                            UInt32 uiMultipleAck)
{
    initMqQueueCtrl();

    m_ulMqId = ulMqId;
    m_strQueue = strQueue;
    m_uiSpeed = uiSpeed;
    m_channel_id = ulMqId;
    m_uiPrefetchCount = uiPrefetchCount;
    m_uiMultipleAck = uiMultipleAck;
}

void MqQueueCtrl::initMqQueueCtrl()
{
    m_pThread = NULL;
    m_ulMqId = 0;
    m_uiSpeed = 0;
    m_uiPrefetchCount = 1;
    m_uiMultipleAck = 0;
    m_conn = NULL;
    m_channel_id = 0;
    m_bLinkOk = false;
    m_bFirstConsumeOk = false;
    m_uiAlreadyCousume = 0;
}

MqQueueCtrl::~MqQueueCtrl()
{
    close();
}

bool MqQueueCtrl::init()
{
    CHK_FUNC_RET_FALSE(connect());
    return true;
}

bool MqQueueCtrl::connect()
{
    if (m_bLinkOk)
        return true;

    m_conn = amqp_new_connection();
    INIT_CHK_NULL_RET_FALSE(m_conn);

    amqp_socket_t* pSocket = amqp_tcp_socket_new(m_conn);
    INIT_CHK_NULL_RET_FALSE(pSocket);

    if (amqp_socket_open(pSocket, m_pThread->m_strIp.data(), m_pThread->m_usPort) < 0)
    {
        LogErrorP("Call amqp_socket_open failed. Ip:%s, Port:%u.",
            m_pThread->m_strIp.data(),
            m_pThread->m_usPort);

        return false;
    }

    CHK_AMQP_RPC_REPLY(amqp_login(m_conn,
                                    "/",
                                    0,
                                    131072,
                                    0,
                                    AMQP_SASL_METHOD_PLAIN,
                                    m_pThread->m_strUserName.data(),
                                    m_pThread->m_strPassword.data()));

    amqp_channel_open(m_conn, m_channel_id);
    CHK_AMQP_RPC_REPLY(amqp_get_rpc_reply(m_conn));

    amqp_basic_qos(m_conn, m_channel_id, 0, m_uiPrefetchCount, 0);
    CHK_AMQP_RPC_REPLY(amqp_get_rpc_reply(m_conn));

    string strConsumerTag = Fmt<64>("Thread[%u],Channel[%u]",
                                    m_pThread->m_uiIndexInThreadPool,
                                    m_channel_id);

    amqp_basic_consume(m_conn,
                        m_channel_id,
                        amqp_cstring_bytes(m_strQueue.data()),
                        amqp_cstring_bytes(strConsumerTag.data()),
                        0,
                        0,
                        0,
                        amqp_empty_table);

    CHK_AMQP_RPC_REPLY(amqp_get_rpc_reply(m_conn));


    m_bLinkOk = true;

    LogNotice("==> %s connect rabbitmq[%s], open Queue"
        "[id:%u,name:%s,speed:%u,PrefetchCount:%u,MultipleAck:%u] success <==",
        m_pThread->GetThreadName().data(),
        m_pThread->m_strIp.data(),
        m_channel_id,
        m_strQueue.data(),
        m_uiSpeed,
        m_uiPrefetchCount,
        m_uiMultipleAck);

    return true;
}

bool MqQueueCtrl::close()
{
    if (m_bLinkOk)
    {
        amqp_channel_close(m_conn, m_channel_id, AMQP_REPLY_SUCCESS);
        amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(m_conn);

        m_bLinkOk = false;

        LogNotice("==> %s close rabbitmq[%s], close Queue"
            "[id:%u,name:%s,speed:%u,PrefetchCount:%u,MultipleAck:%u] success <==",
            m_pThread->GetThreadName().data(),
            m_pThread->m_strIp.data(),
            m_channel_id,
            m_strQueue.data(),
            m_uiSpeed,
            m_uiPrefetchCount,
            m_uiMultipleAck);
    }

    // 需要清空缓存
    m_vecMessage.clear();

    return true;
}

void MqQueueCtrl::print()
{
    if (NULL != m_pThread)
    {
        LogWarn("%s Queue[id:%lu,name:%s,speed:%u, link:%d, "
            "PrefetchCount:%u, MultipleAck:%u].",
            m_pThread->GetThreadName().data(),
            m_ulMqId,
            m_strQueue.data(),
            m_uiSpeed,
            m_bLinkOk,
            m_uiPrefetchCount,
            m_uiMultipleAck);
    }
    else
    {
        LogWarn("Queue[id:%lu,name:%s,speed:%u, link:%d, "
            "PrefetchCount:%u, MultipleAck:%u].",
            m_ulMqId,
            m_strQueue.data(),
            m_uiSpeed,
            m_bLinkOk,
            m_uiPrefetchCount,
            m_uiMultipleAck);
    }
}

bool MqQueueCtrl::consume(UInt32 uiOneSecondConsumeTimes)
{
    // 需要消费的消息数量
    UInt32 uiConsumeCount = m_uiSpeed;

    // 如果是这一秒钟的首次消费
    if (0 == uiOneSecondConsumeTimes)
    {
        // 打印上一秒钟消费到的消息的数量
        LogNotice("[%s][id:%lu,name:%s,speed:%u] Last second consume %u message.",
            m_pThread->GetThreadName().data(),
            m_ulMqId,
            m_strQueue.data(),
            m_uiSpeed,
            m_uiAlreadyCousume);

        m_uiAlreadyCousume = 0;
    }
    else
    {
        if (m_uiAlreadyCousume >= m_uiSpeed)
        {
            return true;
        }

        uiConsumeCount =  m_uiSpeed - m_uiAlreadyCousume;

//        LogDebug("[%s][id:%lu,name:%s,speed:%u] need continue consume "
//            "%u message,OneSecondConsumeTimes[%u].",
//            m_pThread->GetThreadName().data(),
//            m_ulMqId,
//            m_strQueue.data(),
//            m_uiSpeed,
//            uiConsumeCount,
//            uiOneSecondConsumeTimes);
    }

    //////////////////////////////////////////////////////

//    UInt64 ulConsumeStart = now_microseconds();
    UInt32 uiCount = consumeMsg(uiConsumeCount);
//    Int64 timeSpent = now_microseconds() - ulConsumeStart;

    m_uiAlreadyCousume = m_uiAlreadyCousume + uiCount;

//    LogDebug("[%s][id:%lu,name:%s,speed:%u] takes %ld usec,"
//        "got %u messages,AlreadyCousume[%u],OneSecondConsumeTimes[%u].",
//        m_pThread->GetThreadName().data(),
//        m_ulMqId,
//        m_strQueue.data(),
//        m_uiSpeed,
//        timeSpent,
//        uiCount,
//        m_uiAlreadyCousume,
//        uiOneSecondConsumeTimes);

    return (m_uiAlreadyCousume == m_uiSpeed);
}

UInt32 MqQueueCtrl::consumeMsg(UInt32 uiConsumeCount)
{
    UInt32 uiCount = 0;

    for (; uiCount < uiConsumeCount; ++uiCount)
    {
        int result = amqpConsumeMsg();

        if (AMQP_BASIC_ACK_OK == result)
        {
            // do nothing
        }
        else if (AMQP_CONSUME_MESSAGE_TIMEOUT == result)
        {
            break;
        }
        else if ((AMQP_CONSUME_MESSAGE_FAIL == result) || (AMQP_BASIC_ACK_FAIL == result))
        {
            close();
            connect();
            select_sleep(ONE_SECOND_FOR_MICROSECOND);
            break;
        }
    }

    return uiCount;
}

int MqQueueCtrl::amqpConsumeMsg()
{
    int result = AMQP_BASIC_ACK_OK;
    amqp_envelope_t envelope;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RABBITMQ_WAIT_TIME_OUT;

    amqp_maybe_release_buffers(m_conn);

    amqp_rpc_reply_t ret = amqp_consume_message(m_conn, &envelope, &tv, 0);

    if (AMQP_RESPONSE_NORMAL == ret.reply_type)
    {
        if ((m_uiPrefetchCount > 1) && (0 != m_uiMultipleAck) && (m_uiPrefetchCount >= m_uiMultipleAck))
        {
            result = multipleAck(envelope);
        }
        else
        {
            result = singleAck(envelope);
        }

        amqp_destroy_envelope(&envelope);
    }
    else if ((AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type)
         && (AMQP_STATUS_TIMEOUT == ret.library_error))
    {
        // 超时场景下，需要将缓存中的数据发送出去，然后返回超时等待下一轮
        if (AMQP_BASIC_ACK_OK != releaseCachedMsg())
        {
            LogError("Call releaseCachedMsg failed.");
            result = AMQP_BASIC_ACK_FAIL;
        }
        else
        {
            result = AMQP_CONSUME_MESSAGE_TIMEOUT;
        }
    }
    else
    {
        LogError("Call amqp_consume_message failed. ret.reply_type[%d], "
            "ret.reply.id[%x] ret.library_error[-0x%x:%s].",
            ret.reply_type,
            ret.reply.id,
            0 - ret.library_error,
            amqp_error_string2(ret.library_error));

        result = AMQP_CONSUME_MESSAGE_FAIL;
    }

    return result;
}

int MqQueueCtrl::multipleAck(const amqp_envelope_t& envelope)
{
    // 先将消息放入缓冲区
    Message message;
    message.m_usChannel = envelope.channel;
    message.m_ulDeliveryTag = envelope.delivery_tag;
    message.m_strData.append((char *)envelope.message.body.bytes, envelope.message.body.len);
    m_vecMessage.push_back(message);

    // 计算是否需要发送ack,delivery_tag从1开始
    UInt32 remainder = envelope.delivery_tag % m_uiMultipleAck;

    LogDebug("channel[%u], delivery_tag[%lu] push to cache. MultipleAck[%u], remainder[%u].",
        envelope.channel,
        envelope.delivery_tag,
        m_uiMultipleAck,
        remainder);

    if (0 == remainder)
    {
        if (0 != amqp_basic_ack(m_conn, envelope.channel, envelope.delivery_tag, 1))
        {
            LogError("channel[%u], delivery_tag[%lu] Call amqp_basic_ack failed.",
                envelope.channel,
                envelope.delivery_tag);

            m_vecMessage.clear();
            return AMQP_BASIC_ACK_FAIL;
        }
        else
        {
            LogNotice("channel[%u], delivery_tag[%lu] amqp_basic_ack success.",
                envelope.channel,
                envelope.delivery_tag);

            sendCachedMsg();
        }
    }

    return AMQP_BASIC_ACK_OK;
}

int MqQueueCtrl::singleAck(const amqp_envelope_t& envelope)
{
    // 先将缓冲区的消息发送出去
    if (AMQP_BASIC_ACK_OK != releaseCachedMsg())
    {
        LogError("Call releaseCachedMsg failed.");
        return AMQP_BASIC_ACK_FAIL;
    }

    // 再处理当前消息
    if (0 != amqp_basic_ack(m_conn, envelope.channel, envelope.delivery_tag, 0))
    {
        LogError("channel[%u], delivery_tag[%lu] Call amqp_basic_ack failed.",
            envelope.channel,
            envelope.delivery_tag);

        return AMQP_BASIC_ACK_FAIL;
    }

    LogNotice("channel[%u], delivery_tag[%lu] amqp_basic_ack success.",
        envelope.channel,
        envelope.delivery_tag);

    string strData;
    strData.append((char *)envelope.message.body.bytes, envelope.message.body.len);
    sendMsg(envelope.channel, envelope.delivery_tag, strData);

    return AMQP_BASIC_ACK_OK;
}

int MqQueueCtrl::releaseCachedMsg()
{
    if (!m_vecMessage.empty())
    {
        const Message& message = m_vecMessage[m_vecMessage.size() - 1];

        if (0 != amqp_basic_ack(m_conn, message.m_usChannel, message.m_ulDeliveryTag, 1))
        {
            LogError("channel[%u], delivery_tag[%lu] Call amqp_basic_ack failed. m_vecMessage.size[%u].",
                message.m_usChannel,
                message.m_ulDeliveryTag,
                m_vecMessage.size());

            m_vecMessage.clear();
            return AMQP_BASIC_ACK_FAIL;
        }
        else
        {
            LogNotice("channel[%u], delivery_tag[%lu] amqp_basic_ack success. m_vecMessage.size[%u].",
                message.m_usChannel,
                message.m_ulDeliveryTag,
                m_vecMessage.size());

            sendCachedMsg();
        }
    }

    return AMQP_BASIC_ACK_OK;
}

inline void MqQueueCtrl::sendCachedMsg()
{
    for (MessageVecIter iter = m_vecMessage.begin(); iter != m_vecMessage.end(); ++iter)
    {
        const Message& message = *iter;
        sendMsg(message.m_usChannel, message.m_ulDeliveryTag, message.m_strData);
    }

    m_vecMessage.clear();
}

inline void MqQueueCtrl::sendMsg(UInt16 usChannel, UInt64 ulDeleveryTag, cs_t strData)
{
    LogNotice("channel[%u], delivery_tag[%lu] ==send message== data[%s].",
        usChannel,
        ulDeleveryTag,
        strData.data());

    m_pThread->sendMsg(strData);
}


