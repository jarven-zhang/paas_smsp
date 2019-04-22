#ifndef __CMQAUDITCONSUMERTHREAD__
#define __CMQAUDITCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "CMQAckConsumerThread.h"


extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

#define SMSP_CONSUMER_CHANNELID 	1

class TMQPublishDBReqMsg:public TMsg
{
public:
	string m_strData;
};

class CMQAuditConsumerThread:public CMQAckConsumerThread
{
public:
    CMQAuditConsumerThread(const char *name);
    ~CMQAuditConsumerThread();
    bool 	Init(string strIp, unsigned int uPort, string strUserName, string strPassWord);
	
private:
	bool 	InitChannels();
	void 	HandleMsg(TMsg* pMsg);
	void 	HandleComponentRefMqRep(TMsg* pMsg);
	void 	processMessage(string& strData);
	int 	GetMemoryMqMessageSize();
	bool    UpdateMqConsumer(map <string,ComponentRefMq>& mapComponentRefMq);
	bool    AllChannelConsumerPause();
	bool    AllChannelConsumerResume();
private:
	UInt32 						m_uDBSwitch;
	//UInt64                      m_uMqId;
	map <string,ComponentRefMq> m_componentRefMqInfo;
};

#endif /////__CMQAUDITCONSUMERTHREAD__
