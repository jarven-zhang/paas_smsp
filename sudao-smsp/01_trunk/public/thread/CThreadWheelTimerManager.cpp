#include "CThreadWheelTimerManager.h"
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>



void OnAlarm(union sigval sig)
{
    CThreadWheelTimerManager *pTimerMng;
    pTimerMng = (CThreadWheelTimerManager*)sig.sival_ptr;
    pTimerMng->m_iTick += 1;
}

int CThreadWheelTimerManager::CurrentTick()
{
    return m_iTick;
}

int CThreadWheelTimerManager::GetPassedTime()
{
    int time = m_iTick;
    m_iTick = 0;
    return time;
}

CThreadWheelTimerManager::CThreadWheelTimerManager()
{
    m_iCount = 3000;
    m_iTimeUnit = 100;
    m_iTick = 0;
    m_pItem = new CThreadWheelItem();
}

CThreadWheelTimerManager::~CThreadWheelTimerManager()
{
    if(NULL != m_pItem)
    {
        delete m_pItem;
        m_pItem = NULL;
    }
}

void CThreadWheelTimerManager::PlaceTimer(CThreadWheelTimer *timer)
{
    int timeUnit = m_iTimeUnit;
    for (CThreadWheelList::iterator i = m_Wheels.begin(); i != m_Wheels.end(); ++i)
    {
        int remain = timer->GetRemain();
        timeUnit = (*i)->GetMaxTime();
        if (remain < timeUnit)
        {
            (*i)->PlaceTimer(remain / (*i)->GetMinTime(), timer);
            return;
        }
        else
        {
            timer->SetRemain(timer->GetRemain() + (*i)->GetPassedTime());
        }
    }
    while (true)
    {
        CThreadWheel *wheel = new CThreadWheel(m_iCount, timeUnit);
        m_Wheels.push_back(wheel);

        int remain = timer->GetRemain();
        timeUnit = wheel->GetMaxTime();
        if (remain < timeUnit)
        {
            wheel->PlaceTimer(remain / wheel->GetMinTime(), timer);
            return;
        }
        else
        {
            timer->SetRemain(timer->GetRemain() + wheel->GetPassedTime());
        }
    }
}

bool CThreadWheelTimerManager::Init()
{
	struct sigevent evp;
    struct itimerspec ts;
    timer_t timer;
    int ret = 0;

    memset(&evp, 0, sizeof(evp));
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = OnAlarm;
    evp.sigev_value.sival_ptr = this;   //handle() param.

    ret = timer_create(CLOCK_REALTIME, &evp, &timer);
    if( ret)
    {
        printf("timer_create fail");
		return false;
    }

    ts.it_interval.tv_sec  = 0;
    ts.it_interval.tv_nsec = m_iTimeUnit*1000*1000;//100ms
    ts.it_value.tv_sec     = 0;
    ts.it_value.tv_nsec    = m_iTimeUnit*1000*1000;//100ms

    ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL);
    if( ret )
    {
        printf("timer_settime");
		return false;
    }
    return true;
}

bool CThreadWheelTimerManager::Destroy()
{
    return true;
}

void CThreadWheelTimerManager::Click()
{
    int click = GetPassedTime();
    for (int times = 0; times < click; ++times)
    {
        for (CThreadWheelList::iterator i = m_Wheels.begin(); i != m_Wheels.end(); ++i)
        {
            int minTime = (*i)->GetMinTime();
            m_pItem = (*i)->Click(m_pItem);
            while (m_pItem->Begin() != m_pItem->End())
            {
                CThreadWheelTimer *timer = m_pItem->Pop();
                timer->SetRemain(timer->GetRemain() % minTime);
                if (timer->GetRemain() < m_iTimeUnit)
                {
                    timer->OnTimer();
                }
                else
                {
                    PlaceTimer(timer);
                }
            }
            m_pItem->Reset();
            if ((*i)->GetPassedTime() != 0)
            {
                break;
            }
        }
    }
}

CThreadWheelTimer *CThreadWheelTimerManager::CreateTimer()
{
    return new CThreadWheelTimer(this);
}

