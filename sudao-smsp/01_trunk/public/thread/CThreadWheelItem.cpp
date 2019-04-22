#include "CThreadWheelItem.h"

CThreadWheelItem::CThreadWheelItem()
{
    m_pFirstWheelTimer = NULL;
}

CThreadWheelItem::~CThreadWheelItem()
{
}

void CThreadWheelItem::Reset()
{
    m_pFirstWheelTimer = NULL;
}

void CThreadWheelItem::ScheduleTimer(CThreadWheelTimer *timer)
{
    timer->OnSchedule(this);
    if (m_pFirstWheelTimer == NULL)
    {
        timer->SetPrev(NULL);
        timer->SetNext(NULL);
        m_pFirstWheelTimer = timer;
    }
    else
    {
        timer->SetPrev(NULL);
        timer->SetNext(m_pFirstWheelTimer);
        m_pFirstWheelTimer->SetPrev(timer);
        m_pFirstWheelTimer = timer;
    }
}

void CThreadWheelItem::DeleteTimer(CThreadWheelTimer *timer)
{
    if (m_pFirstWheelTimer == timer)
    {
        m_pFirstWheelTimer = timer->GetNext();
        if (m_pFirstWheelTimer != NULL)
        {
            m_pFirstWheelTimer->SetPrev(NULL);
        }
    }
    else
    {
        if (timer->GetPrev() != NULL)
        {
            timer->GetPrev()->SetNext(timer->GetNext());
        }
        if (timer->GetNext() != NULL)
        {
            timer->GetNext()->SetPrev(timer->GetPrev());
        }
    }
}

CThreadWheelTimer *CThreadWheelItem::Begin()
{
    return m_pFirstWheelTimer;
}

CThreadWheelTimer *CThreadWheelItem::End()
{
    return NULL;
}

CThreadWheelTimer *CThreadWheelItem::Pop()
{
    CThreadWheelTimer *timer = m_pFirstWheelTimer;
    if (m_pFirstWheelTimer != NULL)
    {
        m_pFirstWheelTimer = m_pFirstWheelTimer->GetNext();
        if (m_pFirstWheelTimer != NULL)
        {
            m_pFirstWheelTimer->SetPrev(NULL);
        }
    }
    timer->SetPrev(NULL);
    timer->SetNext(NULL);
    return timer;
}

