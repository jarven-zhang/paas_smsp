#ifndef __H_CTHREAD_MONITOR__
#define __H_CTHREAD_MONITOR__

#include "Thread.h"
#include "Comm.h"
#include "alarm.h"
#include <unistd.h>

class monitorObj
{
public: 
	monitorObj( CThread* pThread, string loginfo )
	{
		m_pThread = pThread;
		m_strLoginfo = loginfo;
		m_uQueueSize = 0;
		m_uQueueCount = 0;
	};

public:
	CThread* m_pThread;   //待监控指针的地址
    UInt32   m_uQueueSize;
    UInt32   m_uQueueCount;
    string   m_strLoginfo;
};

typedef list <monitorObj*> monitorobjList;

class CThreadMonitor
{
	/* 内存队列消息数量*/
	#define MSG_QUEUE_ALARM_THRESHOLD	500000
public:
	 CThreadMonitor();
	~CThreadMonitor();
	static bool Init();
	/*添加监控线程*/
	static bool ThreadMonitorAdd( CThread* pThread, string loginfo );
	static void ThreadMonitorRun();
};

#endif
