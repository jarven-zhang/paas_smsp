#ifndef __CMQCHANNELCONSUMERTHREAD__
#define __CMQCHANNELCONSUMERTHREAD__

#include <string.h>
#include "CMQConsumerThread.h"
#include "MqConfig.h"
#include "Channel.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

class CMQChannelConsumerThread:public CMQConsumerThread
{

public:
    CMQChannelConsumerThread(const char *name);
    ~CMQChannelConsumerThread();
	bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

private:
	void HandleMsg(TMsg* pMsg);
	void HandleChannelUpdateReq(TMsg * pMsg);
	bool InitChannels();
	void processMessage(string& strData);
	int  GetMemoryMqMessageSize();
private:
	//当前组件的所有通道
	map<int, models::Channel> m_mapChannelInfo;
};

#endif /////__CMQCHANNELCONSUMERTHREAD__