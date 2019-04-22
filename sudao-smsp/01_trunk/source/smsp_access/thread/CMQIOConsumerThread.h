#ifndef __CMQIOCONSUMERTHREAD__
#define __CMQIOCONSUMERTHREAD__

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

class CMQIOConsumerThread:public CMQConsumerThread
{

public:
    CMQIOConsumerThread(const char *name);
    ~CMQIOConsumerThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);
private:
	void HandleMsg(TMsg* pMsg);
	void HandleQueueUpdateRep(TMsg* pMsg);
	bool InitChannels();
	void processMessage(string& strData);
	int  GetMemoryMqMessageSize();
private:
	map<string,ComponentRefMq> m_componentRefMqInfo;			//本节点相关的 mq信息。	
};

#endif /////__CMQIOCONSUMERTHREAD__