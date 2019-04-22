#ifndef MQTHREAD_MQMANAGERTHREAD_H
#define MQTHREAD_MQMANAGERTHREAD_H

#include "Thread.h"
#include "AppData.h"
#include "MqEnum.h"

#include "boost/array.hpp"

#include <string>
#include <vector>
#include <map>
#include <set>

using std::string;
using std::vector;
using std::map;
using std::set;

//////////////////////////////////////////////////////////////////////////////////////////

bool publishMqC2sIoMsg(cs_t strExchange, cs_t strRoutingkey, cs_t strData);
bool publishMqSendIoMsg(cs_t strExchange, cs_t strRoutingkey, cs_t strData);
bool publishMqC2sDbMsg(cs_t strKey, char* sql);
bool publishMqC2sDbMsg(cs_t strKey, cs_t strSql);
bool publishMqSendDbMsg(cs_t strKey, cs_t strData);

//////////////////////////////////////////////////////////////////////////////////////////

class DbUpdateReq;
class MqThreadPool;

class MqManagerThread : public CThread
{
    const static UInt32 MQ_USE_NUM = 8;

    typedef boost::array<MqThreadPool*, MQ_USE_NUM> MqThreadPoolArray;
    typedef boost::array<CThread*, MQ_USE_NUM> MqConsumeCbArray;
    typedef map<string, UInt64> ExchangeRoutingKeyMap;

public:

    MqManagerThread();

    ~MqManagerThread();

    bool Init();

    void regMqConsumeCallback(CThread* pThread, UInt32 type) {m_aryMqConsumeCb[type] = pThread;}

    vector<CThread*> getAllThreads();

    UInt32 getMqC2sIoPThdTotalQueueSize();

    void isProducerOK();

private:

    MqManagerThread(const MqManagerThread&);

    MqManagerThread& operator=(const MqManagerThread&);

    bool initMqThreadPool();

    bool createMqThreadPool(const TSmsMiddlewareCfg& mw, UInt32 uiMode);

    void HandleMsg(TMsg* pMsg);

    void handleTimeoutReq(TMsg* pMsg);

    void handleMqPublishReq(TMsg* pMsg);

    void handleMqGetMsgReq(TMsg* pMsg);

    void handleDbUpdateReq(TMsg* pMsg);

    void handleSmsParamUpdateReq(DbUpdateReq* pReq);

public:

    AppDataMap m_mapMiddlewareCfg;

    AppDataMap m_mapMqCfg;

    AppDataMap m_mapMqThreadCfg;

    AppDataMap m_mapComponentCfg;

    AppDataMap m_mapComponentRefMq;

private:

    MqThreadPoolArray m_aryThreadPoolMgr;

    MqConsumeCbArray m_aryMqConsumeCb;

    CThreadWheelTimer* m_pMqThreadProcessSummary;

    UInt64 m_ulMqC2sIoPThdTotalQueueSize;
};

extern MqManagerThread* g_pMqManagerThread;

#endif // MQTHREAD_MQMANAGERTHREAD_H
