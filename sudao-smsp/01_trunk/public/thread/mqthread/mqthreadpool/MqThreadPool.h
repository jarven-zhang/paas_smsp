#ifndef MQTHREADPOOL_H
#define MQTHREADPOOL_H

#include "platform.h"
#include "ThreadPool.h"
#include "AppData.h"
#include "MiddleWareConfig.h"
#include "ComponentConfig.h"
#include "ComponentRefMq.h"

#include <string>

using std::string;

class DbUpdateReq;
class MqManagerThread;

class MqThreadPool : public ThreadPool
{
public:
    static const UInt32 MQ_DB_QUEUE_NUM = 5;

    typedef vector<TSmsMqCfg> MqCfgVec;

public:

    MqThreadPool();

    virtual ~MqThreadPool();

    bool init();

    virtual bool start();

    bool getName();

    bool getThreadNum();

    bool getMqDbQueue();

    bool getComponentSwitch();

    bool getLinkStatus();

    UInt32 getTotalQueueSize();

    void handleDbUpdateReq(DbUpdateReq* pReq);

private:

    void printThreadPool();

    virtual bool adjustThread();

    virtual bool createThreadByNum(UInt32 uiThreadNum);

    virtual bool reduceThreadByNum(UInt32 uiThreadNum);

    virtual bool createThread();

    virtual bool reduceThread();

    void handleMqThreadCfgUpdateReq();

    void handleComponentCfgUpdateReq();

    virtual void handleComponentRefMqUpdateReq();

public:

    MqManagerThread* m_pMgr;

    UInt32 m_uiMqThreadPoolType;

    UInt32 m_uiMode;

    TSmsMiddlewareCfg m_middleWareConfig;

    MqCfgVec m_vecMqCfgForDbQueue;

protected:

private:

};

#endif // MQTHREADPOOL_H
