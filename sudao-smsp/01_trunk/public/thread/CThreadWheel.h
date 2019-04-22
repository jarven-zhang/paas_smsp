#ifndef _CTHREAD_WHEEL_H_
#define _CTHREAD_WHEEL_H_

#include <vector>
#include "CThreadWheelItem.h"


class CThreadWheelTimer;
class CThreadWheelItem;

class CThreadWheel
{
public:
    CThreadWheel(int size, int time);
    virtual ~CThreadWheel();

public:
    int GetMinTime() const;
    int GetMaxTime() const;

    int GetItemCount() const;

    int GetRemainTime() const;
    int GetPassedTime() const;

    void PlaceTimer(int place, CThreadWheelTimer *timer);

    CThreadWheelItem *Click(CThreadWheelItem *newItem);

private:
    typedef std::vector<CThreadWheelItem *> CThreadWheelItemArray;

private:
    CThreadWheelItemArray m_ItemArray;
    int m_iCursor;
    int m_iTimeUnit;
};

#endif /* _CTHREAD_WHEEL_H_ */
