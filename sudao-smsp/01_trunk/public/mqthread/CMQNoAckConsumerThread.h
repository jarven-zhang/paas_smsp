#ifndef __CMQNOACKCONSUMERTHREAD__
#define __CMQNOACKCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "Channel.h"
#include "CMQConsumerThread.h"
extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}
#define   MQNOACK_CONSUMER_SPEED_MIN  400

class CMQNoAckConsumerThread:public CThread
{

public:
    CMQNoAckConsumerThread(const char *name);
    ~CMQNoAckConsumerThread();
	bool 		 close();
    virtual bool Init(string strIp, unsigned int uPort, string strUserName, string strPassWord);
	bool 		 RabbitMQConnect();

protected:
	bool 		 HandleCommonMsg(TMsg* pMsg);
	int 		 amqpConsumeMessage(string& strData);
	bool 	     amqpBasicChannelOpen(UInt32 uChannelId, UInt64 uMqId);
	bool 		 amqpBasicConsume(UInt32 uChannelId, UInt64 uMqId);
	void 		 SetChannelSpeedCtrl(int iChannelId, ChannelSpeedCtrl stuChannelSpeedCtrl);
	bool         ConsumerSpeedCheck();
	void 		 MainLoop();
	virtual int  GetMemoryMqMessageSize() = 0;
	virtual bool InitChannels() = 0;
	virtual void processMessage(string& strData) = 0;
	virtual bool AllChannelConsumerPause() = 0;
    virtual bool AllChannelConsumerResume() = 0;
protected:
	string 			m_strIp;
	unsigned int 	m_uPort;
	string 			m_strUserName;
	string 			m_strPassWord;

	amqp_connection_state_t 		m_mqConnectionState;
	map<UInt64, MqConfig> 			m_mapMqConfig;
	map<int, ChannelSpeedCtrl> 		m_mapChannelSpeedCtrl;	//���ٺͼ���
	bool 							m_bConsumerSwitch;		//true:��ʼ����  false:ֹͣ����
	//map<UInt32, blockedChannelInfo>	m_mapBlockedChannel;	//�����ص�ͨ��
	int								m_iSumChannelSpeed;		//���ٶ�
	int                             m_iCurrentSpeed;        //��ǰ�ٶ�
	struct timeval                  m_tvStartTime;
};

#endif /////__CMQCONSUMERTHREAD__
