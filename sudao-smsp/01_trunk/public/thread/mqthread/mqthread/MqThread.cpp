#include "MqThread.h"
#include "LogMacro.h"

MqThread::MqThread()
{
    m_pMqThreadPool = NULL;
    m_bStop = false;
}

MqThread::~MqThread()
{
}

bool MqThread::init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());
    return true;
}

bool MqThread::getLinkStatus()
{
    return true;
}

bool MqThread::PostMngMsg(TMsg* pMsg)
{
    pthread_mutex_lock(&m_mutex);
    m_msgMngQueue.AddMsg(pMsg);
    pthread_mutex_unlock(&m_mutex);
    return true;
}

TMsg* MqThread::GetMngMsg()
{
    pthread_mutex_lock(&m_mutex);
    TMsg* pMsg = m_msgMngQueue.GetMsg();
    pthread_mutex_unlock(&m_mutex);
    return pMsg;
}

