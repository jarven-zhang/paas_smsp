#ifndef __CMQCHANNELUNUSUALCONSUMERTHREAD__
#define __CMQCHANNELUNUSUALCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "Channel.h"
#include "CMQConsumerThread.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

class CMQChannelUnusualConsumerThread:public CMQConsumerThread
{
public:
    CMQChannelUnusualConsumerThread(const char *name);
    ~CMQChannelUnusualConsumerThread();
    bool Init(string strIp, unsigned int uPort, string strUserName, string strPassWord);

private:
	void HandleMsg(TMsg* pMsg);
	void HandleChannelUpdateRep(TMsg* pMsg);
	bool InitChannels();
	void processMessage(string& strData);
	int  GetMemoryMqMessageSize();
private:
	map<int, models::Channel>  m_mapChannelInfo;	//SELECT * FROM t_sms_channel WHERE state='0';
	map<string,ComponentRefMq> m_componentRefMqInfo;//select * from t_sms_component_ref_mq;
};

#endif /////__CMQCHANNELUNUSUALCONSUMERTHREAD__