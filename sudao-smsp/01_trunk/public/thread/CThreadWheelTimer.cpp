#include "CThreadWheelTimer.h"


CThreadWheelTimer::CThreadWheelTimer(CThreadWheelTimerManager *pwheelMng)
{
    m_pItem = NULL;
    m_iRemain = 0;
    m_pPrevTimer = NULL;
    m_pNextTimer = NULL;
    m_pMsg = NULL;
    m_pWheelManager = pwheelMng;
}

CThreadWheelTimer::~CThreadWheelTimer()
{
    if(m_pItem != NULL)
    {
        m_pItem->DeleteTimer(this);
    }
    if(m_pMsg != NULL)
    {
        delete m_pMsg;
        m_pMsg = NULL;
    }
}

CThreadWheelTimer *CThreadWheelTimer::GetPrev()
{
    return m_pPrevTimer;
}

CThreadWheelTimer *CThreadWheelTimer::GetNext()
{
    return m_pNextTimer;
}

void CThreadWheelTimer::SetPrev(CThreadWheelTimer *prev)
{
    m_pPrevTimer = prev;
}

void CThreadWheelTimer::SetNext(CThreadWheelTimer *next)
{
    m_pNextTimer = next;
}

void CThreadWheelTimer::OnTimer()
{
    m_pMsg->m_pSender->PostMsg(m_pMsg);
    m_pMsg = NULL;
}

int CThreadWheelTimer::GetRemain() const
{
    return m_iRemain;
}

void CThreadWheelTimer::SetRemain(int remain)
{
    m_iRemain = remain;
}

void CThreadWheelTimer::OnSchedule(CThreadWheelItem *item)
{
    m_pItem = item;
}

bool CThreadWheelTimer::Schedule(TMsg *msg, int milisec)
{
    m_pMsg = msg;
    m_iRemain = milisec;
    m_pWheelManager->PlaceTimer(this);
    return true;
}

void CThreadWheelTimer::Destroy()
{
    delete this;
}

