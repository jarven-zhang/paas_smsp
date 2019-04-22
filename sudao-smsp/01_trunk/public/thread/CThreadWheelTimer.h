#ifndef _CTHREAD_WHEEL_WHEELTIMER_H_
#define _CTHREAD_WHEEL_WHEELTIMER_H_

#include <stdlib.h>
#include <stdio.h>
#include "CThreadWheelItem.h"
#include "CThreadWheelTimerManager.h"

#include "Thread.h"

class TMsg;
class CThreadWheelItem;
class CThreadWheelTimerManager;

class CThreadWheelTimer
{
public:
    CThreadWheelTimer(CThreadWheelTimerManager *pwheelMng);
    virtual ~CThreadWheelTimer();

public:
    CThreadWheelTimer *GetPrev();
    CThreadWheelTimer *GetNext();

    void SetPrev(CThreadWheelTimer *prev);
    void SetNext(CThreadWheelTimer *next);
    void OnTimer();
    int GetRemain() const;
    void SetRemain(int remain);
    void OnSchedule(CThreadWheelItem *item);
    virtual bool Schedule(TMsg *msg, int milisec);
    virtual void Destroy();

private:
    CThreadWheelTimerManager *m_pWheelManager;
    CThreadWheelItem *m_pItem;
    int m_iRemain;
    TMsg *m_pMsg;

    CThreadWheelTimer *m_pPrevTimer;
    CThreadWheelTimer *m_pNextTimer;
};

#endif /* _CTHREAD_WHEEL_WHEELTIMER_H_ */

