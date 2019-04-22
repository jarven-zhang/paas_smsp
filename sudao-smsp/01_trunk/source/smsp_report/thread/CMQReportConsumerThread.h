#ifndef __CMQIOCONSUMERTHREAD__
#define __CMQIOCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "ComponentConfig.h"
#include "CMQConsumerThread.h"
#include "CRuleLoadThread.h"
#include "base64.h"
#include "Comm.h"
extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

using namespace std;
using namespace models;
class CMQReportConsumerThread:public CMQConsumerThread
{
public:
    CMQReportConsumerThread(const char *name);
    ~CMQReportConsumerThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

private:
	bool InitChannels();
	void HandleMsg(TMsg* pMsg);
	void HandleComponentRefMqRep(TMsg * pMsg);
	void processMessage(string& strData);
	int  GetMemoryMqMessageSize();
private:
	ComponentConfig 						m_componentConfigInfo;	
	ComponentRefMq 							m_componentRefMqInfo;
	UInt64 									m_uMqId;
};

#endif /////__CMQCONSUMERTHREAD__