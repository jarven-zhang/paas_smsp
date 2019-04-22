#ifndef __CGETCHANNELMQSIZETHREAD__
#define __CGETCHANNELMQSIZETHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "Channel.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

class CGetChannelMqSizeThread:public CThread
{

public:
    CGetChannelMqSizeThread(const char *name);
    ~CGetChannelMqSizeThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

	bool RabbitMQConnect();
	bool close();

	bool m_linkState;

private:
	virtual void MainLoop(void);
    virtual void HandleMsg(TMsg* pMsg);

	void getChannelMqSize();

	void HandleTimeOut(TMsg* pMsg);

public:

private:
	string m_strIp;
	UInt32 m_uPort;
	string m_strUserName;
	string m_strPassWord;

	amqp_connection_state_t m_MQconn;
	amqp_basic_properties_t m_Props;

	map<UInt64,MqConfig> m_mapMqConfig;
	map<int, models::Channel> m_mapChannelInfo;

	CThreadWheelTimer* m_pGetChannelMqSizeTimer;

};

#endif /////__CGETCHANNELMQSIZETHREAD__

