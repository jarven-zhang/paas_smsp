#ifndef _CTHREAD_WHEEL_WHEELITEM_H_
#define _CTHREAD_WHEEL_WHEELITEM_H_

#include <stdlib.h>
#include "CThreadWheelTimer.h"


class CThreadWheelTimer;

class CThreadWheelItem
{
public:
    CThreadWheelItem();
    virtual ~CThreadWheelItem();

public:
    void Reset();
    void ScheduleTimer(CThreadWheelTimer *timer);
    void DeleteTimer(CThreadWheelTimer *timer);

    CThreadWheelTimer *Begin();
    CThreadWheelTimer *End();
    CThreadWheelTimer *Pop();

private:
    CThreadWheelTimer *m_pFirstWheelTimer;
};

#endif /* _WHEEL_WHEELITEM_H_ */
