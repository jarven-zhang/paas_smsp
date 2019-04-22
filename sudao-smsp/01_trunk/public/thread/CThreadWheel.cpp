#include "CThreadWheel.h"

CThreadWheel::CThreadWheel(int size, int time)
{
    m_ItemArray.resize(size);
    for (int i = 0; i < size; ++i)
    {
        m_ItemArray[i] = new CThreadWheelItem();
    }
    m_iCursor = 0;
    m_iTimeUnit = time;
}

CThreadWheel::~CThreadWheel()
{
}

int CThreadWheel::GetMinTime() const
{
    return m_iTimeUnit;
}

int CThreadWheel::GetMaxTime() const
{
    return m_iTimeUnit * m_ItemArray.size();
}

int CThreadWheel::GetItemCount() const
{
    return m_ItemArray.size();
}

int CThreadWheel::GetRemainTime() const
{
    return (m_ItemArray.size() - m_iCursor) * m_iTimeUnit;
}

int CThreadWheel::GetPassedTime() const
{
    return m_iCursor * m_iTimeUnit;
}

void CThreadWheel::PlaceTimer(int place, CThreadWheelTimer *timer)
{
    if (place % m_ItemArray.size() == 0)
    {
        place = 1;
    }
    int pos = (m_iCursor + place) % m_ItemArray.size();
    m_ItemArray[pos]->ScheduleTimer(timer);
}

CThreadWheelItem *CThreadWheel::Click(CThreadWheelItem *newItem)
{
    m_iCursor = (m_iCursor + 1) % m_ItemArray.size();
    CThreadWheelItem *item = m_ItemArray[m_iCursor];
    m_ItemArray[m_iCursor] = newItem;
    return item;
}

