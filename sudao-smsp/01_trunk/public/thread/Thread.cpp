#include "Thread.h"
#include "LogMacro.h"

#include <iostream>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

MsgSequenceGenerator::MsgSequenceGenerator()
{
    m_iSeq = 0;
}

MsgSequenceGenerator::~MsgSequenceGenerator()
{
    pthread_mutex_destroy(&m_mutex);
}

bool MsgSequenceGenerator::Init()
{
    pthread_mutex_init(&m_mutex,NULL);
    return true;
}

unsigned int MsgSequenceGenerator::GetMsgSequence()
{
    pthread_mutex_lock(&m_mutex);
    unsigned int iSeqID = m_iSeq++;
    pthread_mutex_unlock(&m_mutex);
    return iSeqID;
}


TMsg::TMsg(int msgType,  unsigned int  iSeqID, std::string strSessionID, CThread* pSender):
    m_iMsgType(msgType),
    m_iSeq(iSeqID),
    m_strSessionID(strSessionID),
    m_pSender(pSender)
{
}

TMsg::TMsg()
{
    m_iMsgType = 0;
    m_iSeq = 0;
    m_pSender = NULL;
}

TMsg::~TMsg()
{
}

int TMsgQueue::GetMsgSize()
{
    return m_iMsgSize;
}

bool TMsgQueue::AddMsg(TMsg* pMsg)
{
    m_queue.push(pMsg);
    m_iMsgSize++;
    return true;
}

TMsg* TMsgQueue::GetMsg()
{
    TMsg* pMsg = NULL;
    if (m_iMsgSize == 0 || m_queue.size() == 0)
    {
        return NULL;
    }
    pMsg = m_queue.front();
    m_queue.pop();
    m_iMsgSize--;
    return pMsg;
}

CThread::CThread()
{
    m_pLinkedBlockPool = NULL;
    m_uiIndexInThreadPool = 0;
}

CThread::CThread(const char *name)
{
    m_threadName = name;
    m_pLinkedBlockPool = NULL;
    m_uiIndexInThreadPool = 0;
}

CThread::~CThread()
{
    pthread_mutex_destroy(&m_mutex);

    LogWarnT("==> exit <==");
}

bool CThread::Init()
{
    m_bJoined = false;
    pthread_mutex_init(&m_mutex,NULL);

    memset(&m_thread, 0, sizeof(m_thread));

    m_pTimerMng = new CThreadWheelTimerManager();

    if (NULL == m_pTimerMng)
    {
        printf("m_pTimerMng is NULL. \n");
        return false;
    }

    if (!m_pTimerMng->Init())
    {
        printf("m_pTimerMng->Init() fail. \n");
        return false;
    }

    initWorkFuncMap();

    m_iCount = 0;
    return true;
}

bool CThread::PostMsg(TMsg* pMsg)
{
    pthread_mutex_lock(&m_mutex);
    m_msgQueue.AddMsg(pMsg);
    pthread_mutex_unlock(&m_mutex);
    return true;
}

int CThread::GetMsgSize()
{
    pthread_mutex_lock(&m_mutex);
    int iMsgNum = m_msgQueue.GetMsgSize();
    pthread_mutex_unlock(&m_mutex);
    return iMsgNum;
}

pid_t CThread::GetThreadId()
{
    return syscall(SYS_gettid);
}

void CThread::SetMsgCount(int count)
{
    pthread_mutex_lock(&m_mutex);
    m_iCount = count;
    pthread_mutex_unlock(&m_mutex);
}


void* CThread::Run(void *arg)
{
    CThread *ptr = (CThread *)arg;
    ptr->MainLoop();
    return NULL;
}

bool CThread::ThreadStop()
{
    TMsg* pMsg = NULL;
    /** 结束线程的时候释
      ** 放线程内部堆积的消息*/
    do
    {
        pMsg = m_msgQueue.GetMsg();

        if (pMsg)
        {
            delete(pMsg);
        }
    } while (pMsg != NULL);

    pthread_cancel( m_thread );
    return true;
}

bool CThread::CreateThread()
{
    pthread_attr_t attr;

    memset(&attr,0,sizeof(pthread_attr_t));
    if (0 != pthread_attr_init(&attr) )
    {
        std::cout << " pthread_attr_init failed" << std::endl;
        return false;
    }

    pthread_attr_setstacksize(&attr, 64 * 1024);

    if (m_bJoined == false)
    {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    if (pthread_create(&m_thread, &attr, Run, this) != 0)
    {
        std::cout << "create thread" << "failed." << std::endl;
        return false;
    }
    else
    {
        string name = m_threadName.substr(0, 14);

        int rc = pthread_setname_np(m_thread, name.data());

        if (0 != rc)
        {
            std::cout<<"set thread name["<<name<<"] failed."<<std::endl;
        }

//        char threadname0[16] = {0};
//        pthread_getname_np(m_thread, threadname0, 16);
//        std::cout<<"rc["<<rc<<"], create thread name["<<GetThreadName()<<"] successfully."<< std::endl;
    }

    if (0 != pthread_attr_destroy(&attr))
    {
        std::cout << "set attr destroy failed." << std::endl;
    }

    return true;
}

void CThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(1)
    {
        m_pTimerMng->Click();

        for (int i = 0; i < 10; i++)
        {
            pthread_mutex_lock(&m_mutex);
            TMsg* pMsg = m_msgQueue.GetMsg();
            pthread_mutex_unlock(&m_mutex);

            if (NULL == pMsg)
            {
                usleep(1000*10);
            }
            else
            {
                HandleMsg(pMsg);
                delete pMsg;
            }
        }
    }
}

void CThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    //    Duration duration;
    //    duration.click();

    MessageCallbackMapIter iter = m_mapMsgCallback.find(pMsg->m_iMsgType);

    if (iter == m_mapMsgCallback.end())
    {
        LogError("Can not find key[%x] in %s::m_mapMsgCallback.",
            pMsg->m_iMsgType, m_threadName.data());
        return;
    }

    (iter->second)(pMsg);
}

CThreadWheelTimer* CThread::SetTimer(UInt64 iSeq, const std::string& strSessionID, int iTimeLength)
{
    TMsg* pMsg= new TMsg();
    pMsg->m_iMsgType = MSGTYPE_TIMEOUT;
    pMsg->m_iSeq = iSeq;
    pMsg->m_strSessionID = strSessionID;
    pMsg->m_pSender = this;

    CThreadWheelTimer *pTimer = m_pTimerMng->CreateTimer();
    pTimer->Schedule(pMsg, iTimeLength);

    return pTimer;
}


