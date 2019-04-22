#ifndef MQCONSUMERTHREAD_H
#define MQCONSUMERTHREAD_H

#include "MqThread.h"
#include "MqQueueCtrl.h"

#include <vector>
#include <map>

#include <sys/time.h>
#include <time.h>

using std::vector;
using std::map;


class MqConsumerThread : public MqThread
{
public:
    MqConsumerThread();

    ~MqConsumerThread();

    virtual bool init();

    virtual bool getLinkStatus();

    void sendMsg(cs_t strData);

private:

    bool initConsumerQueue();

    virtual void MainLoop();

    virtual void HandleMsg(TMsg* pMsg);

    void handleMngMsg();

    void consumeMessage();

    void handleMqQueueCtrlUpdateReq(TMsg* pMsg);

    bool initOneQueue(MqQueueCtrl& mqQueueCtrl);

    void printMqQueueCtr();

public:

    bool m_bConsumerSwitch;

    MqQueueCtrlMap m_mapMqQueueCtrl;
};

#endif // MQCONSUMERTHREAD_H
