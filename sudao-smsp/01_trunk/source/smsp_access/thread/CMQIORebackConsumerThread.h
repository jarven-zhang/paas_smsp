#ifndef __CMQIOREBACKCONSUMERTHREAD__
#define __CMQIOREBACKCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "CMQConsumerThread.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

class CMQIORebackConsumerThread:public CMQConsumerThread
{

public:
    CMQIORebackConsumerThread(const char *name);
    ~CMQIORebackConsumerThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

private:
	void HandleMsg(TMsg* pMsg);
	void HandleQueueUpdateRep(TMsg* pMsg);
	bool InitChannels();
	void processMessage(string& strData);
	int  GetMemoryMqMessageSize();
private:
	map<string,ComponentRefMq> 		m_componentRefMqInfo;			
};

#endif /////__CMQCONSUMERTHREAD__