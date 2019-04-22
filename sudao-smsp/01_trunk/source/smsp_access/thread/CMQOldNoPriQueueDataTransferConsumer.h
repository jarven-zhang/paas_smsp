#ifndef __CMQOLDNOPRIQUEUEDATATRANSFERTHREAD__
#define __CMQOLDNOPRIQUEUEDATATRANSFERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "Channel.h"
#include "CMQConsumerThread.h"
#include "ClientPriority.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}
class smsSessionInfo;

class CMQOldNoPriQueueDataTransferConsumer:public CMQConsumerThread
{
public:
    CMQOldNoPriQueueDataTransferConsumer(const char *name);
    ~CMQOldNoPriQueueDataTransferConsumer();
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
    client_priorities_t m_clientPriorities;

    map<int, UInt64> m_transferedChannels;
};

#endif /////__CMQOLDNOPRIQUEUEDATATRANSFERTHREAD__
