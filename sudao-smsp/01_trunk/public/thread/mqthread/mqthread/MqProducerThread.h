#ifndef MQPRODUCERTHREAD_H
#define MQPRODUCERTHREAD_H

#include "MqThread.h"

class MqProducerThread : public MqThread
{
public:

    MqProducerThread();

    ~MqProducerThread();

    virtual bool init();

    virtual bool getLinkStatus();

private:

    void initChannelId();

    bool connect();

    void close();

    void reconnect();

    void setLinkStatus(bool bLinkOk);

    inline bool checkExit();

    virtual void MainLoop();

    virtual void HandleMsg(TMsg* pMsg);

    void handleMngMsg();

    void handleMqMsg();

    void handleFailedMqMsg();

    void handleNormalMqMsg();

    void handlePublishMsgReq(TMsg* pMsg);


public:

    bool m_bProducerSwitch;

private:

    amqp_connection_state_t m_conn;

    amqp_basic_properties_t m_properties;

    amqp_channel_t m_channel_id;

    bool m_bLinkOk;

    TMsgQueue m_msgQueueFailed;

};

#endif // MQPRODUCERTHREAD_H
