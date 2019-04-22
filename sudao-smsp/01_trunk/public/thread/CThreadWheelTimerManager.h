#ifndef _WHEELTIMER_MANAGER_H_
#define _WHEELTIMER_MANAGER_H_

#include <stdlib.h>
#include <list>
#include <signal.h>
#include "CThreadWheel.h"
#include "CThreadWheelItem.h"
#include "CThreadWheelTimer.h"


class CThreadWheel;
class CThreadWheelItem;
class CThreadWheelTimer;

class CThreadWheelTimerManager
{

public:
    CThreadWheelTimerManager();
    virtual ~CThreadWheelTimerManager();

public:
    void PlaceTimer(CThreadWheelTimer *timer);

    virtual bool Init();
    virtual bool Destroy();

    virtual void Click();

    virtual CThreadWheelTimer *CreateTimer();

    int m_iTick;

private:
    typedef std::list<CThreadWheel *> CThreadWheelList;

    int CurrentTick();
    int GetPassedTime();

    CThreadWheelList m_Wheels;
    int m_iTimeUnit;
    int m_iCount;
    CThreadWheelItem *m_pItem;
};

#endif /* _WHEELTIMER_MANAGER_H_ */

