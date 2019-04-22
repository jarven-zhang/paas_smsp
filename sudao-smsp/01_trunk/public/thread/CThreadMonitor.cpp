#include "CThreadMonitor.h"
#include "CAlarmThread.h"

static monitorobjList m_monitorObjList;
static const UInt32   m_monitorTime = 60;

bool CThreadMonitor::ThreadMonitorAdd( CThread *pThread, string LogInfo )
{
	if( pThread == NULL ){
		return false;
	}
	monitorObj * pmonitorObj = new monitorObj( pThread, LogInfo );
	m_monitorObjList.push_back( pmonitorObj );	
	LogNotice("Monitor Add Thread %s, LogInfo:%s ", pThread->GetThreadName().data(), LogInfo.data());
	return true;
}

void CThreadMonitor::ThreadMonitorRun()
{
	while( true )
	{
		sleep( m_monitorTime );
		string strInfo = "QueueSize ";
		for ( list<monitorObj*>::iterator iter = m_monitorObjList.begin(); iter != m_monitorObjList.end(); ++iter )
		{
			(*iter)->m_uQueueSize = (*iter)->m_pThread->GetMsgSize();
			if( (*iter)->m_uQueueSize > MSG_QUEUE_ALARM_THRESHOLD )
			{
				LogWarn("[AC16]%s[%d] is alarmed!", (*iter)->m_strLoginfo.c_str(), (*iter)->m_uQueueSize);
				string strChinese = "";
				strChinese = DAlarm::getAlarmStr((*iter)->m_strLoginfo.c_str(), (*iter)->m_uQueueSize, MSG_QUEUE_ALARM_THRESHOLD);
				Alarm(SYS_QUEUE_SUPERTHRESHOLD_ALARM, (*iter)->m_strLoginfo.c_str(), strChinese.data());
			}
			
			(*iter)->m_uQueueCount = (*iter)->m_pThread->GetMsgCount();
			(*iter)->m_pThread->SetMsgCount(0);

			strInfo.append(" [" + (*iter)->m_pThread->GetThreadName() + ":");			
			strInfo.append("(" + Comm::int2str((*iter)->m_uQueueSize)  + ")");  /* 队列里面等待处理的消息个数*/
			strInfo.append("(" + Comm::int2str((*iter)->m_uQueueCount) + ")]"); /* 队列里上一分钟处理的消息数量*/
		}
		LogFatal("%s", strInfo.c_str());
	}
	
}

