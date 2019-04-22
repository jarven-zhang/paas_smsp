#ifndef MQPRODUCERTHREADPOOL_H
#define MQPRODUCERTHREADPOOL_H

#include "platform.h"
#include "MqThreadPool.h"

class TMsg;
class MqPublishReqMsg;

class MqProducerThreadPool : public MqThreadPool
{
public:
    MqProducerThreadPool();
    ~MqProducerThreadPool();

    void postMsg(TMsg* pMsg);

private:

    virtual bool createThread();

    virtual bool reduceThread();

    virtual void handleComponentRefMqUpdateReq();

    bool getIdFromSql(MqPublishReqMsg* pReq);

    UInt32 hash(cs_t str, UInt32 uiNumber);
};

#endif // MQPRODUCERTHREADPOOL_H
