#ifndef MQCONSUMERTHREADPOOL_H
#define MQCONSUMERTHREADPOOL_H

#include "MqThreadPool.h"
#include "MqQueueCtrl.h"

class MqConsumerThreadPool : public MqThreadPool
{
public:

    MqConsumerThreadPool();

    ~MqConsumerThreadPool();

private:

    virtual bool createThread();

    virtual bool reduceThread();

    virtual void handleComponentRefMqUpdateReq();

    bool getConsumerQueue();

    MqQueueCtrlMap getThreadConsumerQueue(UInt32 index);

private:

    MqQueueCtrlVec m_vecMqQueueCtrl;
};

#endif // MQCONSUMERTHREADPOOL_H
