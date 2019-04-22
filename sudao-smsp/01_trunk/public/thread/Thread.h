#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <queue>
#include <string.h>
#include <string>
#include <stdio.h>
#include "platform.h"
#include "ThreadMacro.h"
#include "CThreadWheelTimerManager.h"
#include "httpresponse.h"
#include "linkedblockpool.h"
#include "Cond.h"

#include "boost/bind.hpp"
#include "boost/function.hpp"

using namespace std;

#ifndef REG_MSG_CALLBACK
#define REG_MSG_CALLBACK(thd_, msg_, func) \
    m_mapMsgCallback[msg_] = boost::bind(&thd_::func, this, _1);
#endif


#define __THREAD_INDEX_IN_THREAD_POOL__ m_uiIndexInThreadPool
#define __THREAD_NAME_IN_THREAD_POOL__ GetThreadName()

class CThreadWheelTimerManager;
class CThread;
class CThreadWheelTimer;

class MsgSequenceGenerator
{
    unsigned int m_iSeq;
    pthread_mutex_t m_mutex;

public:
    MsgSequenceGenerator();
    ~MsgSequenceGenerator();
    bool Init();
    unsigned int GetMsgSequence();
};

class TMsg
{
public:
    TMsg();
    TMsg(int iMsgType,  unsigned int  iMsgID, std::string strSessionID, CThread* pSender);
    virtual ~TMsg();

    int m_iMsgType;
    UInt64 m_iSeq;
    std::string m_strSessionID;
    CThread *m_pSender;
};

class THttpResponseMsg: public TMsg
{
public:
    enum
    {
        HTTP_RESPONSE =0,
        HTTP_TIMEOUT=1,
        HTTP_CLOSED=2,
        HTTP_ERR=3,
    };

    http::HttpResponse m_tResponse;
    UInt32 m_uStatus;
};

class TMsgQueue
{
public:
    TMsgQueue()
    {
        m_iMsgSize = 0;
    };

    ~TMsgQueue() {}

    int GetMsgSize();
    bool AddMsg(TMsg* pMsg);
    TMsg* GetMsg();
    bool empty() {return 0 == m_iMsgSize;}

public:
    int m_iMsgSize;
    std::queue<TMsg*> m_queue;
};

class CThread
{
    typedef boost::function<void (TMsg*)> MessageCallback;
    typedef std::map<int, MessageCallback> MessageCallbackMap;
    typedef std::map<int, MessageCallback>::iterator MessageCallbackMapIter;

public:
    CThread();
    CThread(const char *name);
    virtual ~CThread();

    bool PostMsg(TMsg* pMsg);
    bool Init();
    bool CreateThread();
    bool ThreadStop();

    virtual int GetMsgSize();
    pid_t GetThreadId();
    const string& GetThreadName() {return m_threadName;}
    virtual UInt32 GetMsgCount() {return m_iCount;}

    void setName(cs_t strName) {m_threadName = strName;}
    virtual void SetMsgCount(int count);
    CThreadWheelTimer*  SetTimer(UInt64 iMsgID, const std::string& strSessionID, int iTimeLength);

protected:
    static void * Run(void *arg);

    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);
    virtual void initWorkFuncMap() {};

    void CancelTimer(int iMsgID);

protected:
    pthread_mutex_t m_mutex;

    TMsgQueue m_msgQueue;

    CThreadWheelTimerManager* m_pTimerMng;

    UInt32 m_iCount;

    MessageCallbackMap m_mapMsgCallback;

private:
    bool m_bJoined;

    pthread_t m_thread;

    string m_threadName;

public:
    LinkedBlockPool* m_pLinkedBlockPool;

    UInt32 m_uiIndexInThreadPool;
};

#endif
