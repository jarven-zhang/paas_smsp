#ifndef __CMQIOCONSUMERTHREAD__
#define __CMQIOCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "ComponentConfig.h"
#include "CMQConsumerThread.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

#define SMSP_C2S_CHANNELID		1

class CMQIOConsumerThread:public CMQConsumerThread
{

public:
    CMQIOConsumerThread(const char *name);
    ~CMQIOConsumerThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

private:
	bool InitChannels();
	void HandleMsg(TMsg* pMsg);
	void HandleQueueUpdateRep(TMsg* pMsg);
	void HandleComponentRefMqRep(TMsg * pMsg);
	void processMessage(string& strData);
	bool UpdateChannelSpeedCtrl(UInt64 uMqId);
	int  GetMemoryMqMessageSize();
private:
	ComponentConfig 						m_componentConfigInfo;	
	ComponentRefMq 							m_componentRefMqInfo;
	UInt64 									m_uMqId;
};

#endif /////__CMQCONSUMERTHREAD__