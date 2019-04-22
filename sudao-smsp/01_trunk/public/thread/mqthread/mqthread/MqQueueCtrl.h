#ifndef THREAD_MQTHREAD_MQTHREAD_MQQUEUESPEEDCTRL_H
#define THREAD_MQTHREAD_MQTHREAD_MQQUEUESPEEDCTRL_H

#include "platform.h"
#include "Thread.h"

#include <sys/time.h>
#include <time.h>

#include <string>
#include <vector>
#include <map>

extern "C"
{
    #include "amqp_tcp_socket.h"
    #include "amqp.h"
    #include "amqp_framing.h"
}

using std::string;
using std::vector;
using std::map;


class MqConsumerThread;

class MqQueueCtrl
{
    struct Message
    {
        UInt16 m_usChannel;
        UInt64 m_ulDeliveryTag;
        string m_strData;
    };

    typedef vector<Message> MessageVec;
    typedef MessageVec::iterator MessageVecIter;

public:

    MqQueueCtrl();

    MqQueueCtrl(UInt64 ulMqId, cs_t strQueue, UInt32 uiSpeed, UInt32 uiPrefetchCount, UInt32 uiMultipleAck);

    ~MqQueueCtrl();

    bool init();

    bool connect();

    bool close();

    void print();

    bool consume(UInt32 uiOneSecondConsumeTimes);

private:

    void initMqQueueCtrl();

    UInt32 consumeMsg(UInt32 uiConsumeCount);

    int amqpConsumeMsg();

    int multipleAck(const amqp_envelope_t& envelope);

    int singleAck(const amqp_envelope_t& envelope);

    int releaseCachedMsg();

    void sendCachedMsg();

    void sendMsg(UInt16 usChannel, UInt64 ulDeleveryTag, cs_t strData);

public:
    MqConsumerThread* m_pThread;

    UInt64 m_ulMqId;

    string m_strQueue;

    UInt32 m_uiSpeed;

    UInt32 m_uiPrefetchCount;

    UInt32 m_uiMultipleAck;

private:
    amqp_connection_state_t m_conn;

    amqp_channel_t m_channel_id;

    bool m_bLinkOk;

    MessageVec m_vecMessage;

    bool m_bFirstConsumeOk;

    UInt32 m_uiAlreadyCousume;
};

typedef map<UInt64, MqQueueCtrl> MqQueueCtrlMap;
typedef MqQueueCtrlMap::value_type MqQueueCtrlMapValueType;
typedef MqQueueCtrlMap::iterator MqQueueCtrlMapIter;
typedef vector<MqQueueCtrl> MqQueueCtrlVec;


class MqQueueCtrlUpdateReq : public TMsg
{
public:
    MqQueueCtrlUpdateReq() {}
    ~MqQueueCtrlUpdateReq() {}

    MqQueueCtrlMap m_mapMqQueueCtrl;

private:
    MqQueueCtrlUpdateReq(const MqQueueCtrlUpdateReq&);
    MqQueueCtrlUpdateReq& operator=(const MqQueueCtrlUpdateReq&);
};

#endif // THREAD_MQTHREAD_MQTHREAD_MQQUEUESPEEDCTRL_H
