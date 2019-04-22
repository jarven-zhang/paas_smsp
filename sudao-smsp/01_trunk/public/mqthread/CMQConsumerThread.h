#ifndef __CMQCONSUMERTHREAD__
#define __CMQCONSUMERTHREAD__

#include <string.h>
#include <set>
#include "Thread.h"
#include "MqConfig.h"
#include "Channel.h"

extern "C"
{
	#include "amqp_tcp_socket.h"
	#include "amqp.h"
	#include "amqp_framing.h"
}

#define TIME_ONE_SECOND 			(1000*1000)
#define PRE_FETCH_MESSAGE_SIZE		1
#define RABBITMQ_TIME_OUT			(20*1000)
#define NO_SLIDER_WINDOW			-1

#define MEMORY_MQ_MESSAGE_TIMES_SPEED	5

enum
{
	AMQP_CONSUME_MESSAGE_READY = -1,	//��ʼ��
	AMQP_CONSUME_MESSAGE_FAIL,			//�쳣������ʧ��
	AMQP_CONSUME_MESSAGE_TIMEOUT,		//��ʱ
	AMQP_CHANNEL_SLIDER_WINDOW_ABNOMAL,	//����Ϊ0��ֱ��ͨ���رգ�������ⲻ��
	AMQP_CHANNEL_OVER_SPEED,			//����
	AMQP_BASIC_ACK_FAIL,				//�쳣����ackʧ��
	AMQP_BASIC_ACK_OK,					//��ack�ɹ��������յ�һ����Ϣ
	AMQP_BASIC_REJECT_FAIL,				//�쳣����rejectʧ��
	AMQP_BASIC_REJECT_OK,				//��reject�ɹ�
	AMQP_COMPONENT_MEMORY_QUEUE_FULL,	//���mq��Ϣ�ڴ��������
	AMQP_COMPONENT_SWITCH_OFF,
	AMQP_BASIC_GET_MESSAGE_OK,			//noack�����յ�һ����Ϣ
};

#define IS_RABBITMQ_EXCEPTION(result) ((result == AMQP_CONSUME_MESSAGE_FAIL) || (result == AMQP_BASIC_ACK_FAIL) || (result == AMQP_BASIC_REJECT_FAIL))

typedef struct _ChannelSpeedCtrl
{
	_ChannelSpeedCtrl()
	{
		uLastTime 	  	= 0;
		uCurrentSpeed 	= 0;
		uChannelSpeed 	= 0;
		uChannelState 	= 1;
		strQueue	  	= "";
		uMqId         	= 0;
		uDeliveryTag    = 0;
		uCount          = 0;
		uShowsigntype	= 0;
   		uLongsms		= 1;
		uChannelType	= 0;
		uExValue		= 3;			
		uSplitRule		= 0;
		uCnLen			= 70;
		uEnLen			= 140;
		uCnSplitLen		= 67;
		uEnSplitLen		= 67;
		
	}

	UInt64 		uLastTime;
	UInt32 		uCurrentSpeed;
	UInt32 		uChannelSpeed;
	UInt32 		uChannelState;
	string 		strQueue;
	UInt64 		uMqId;
	UInt64 		uDeliveryTag;
	UInt32 		uCount;
	UInt32      uShowsigntype;
   	UInt32      uLongsms;
	UInt32		uChannelType;
	UInt32		uExValue;			
	UInt32		uSplitRule;
	UInt32		uCnLen;
	UInt32		uEnLen;
	UInt32		uCnSplitLen;
	UInt32		uEnSplitLen;
}ChannelSpeedCtrl;

typedef enum{
	MQ_BLOCK_TYPE_QUEUE_SIZE,
	MQ_BLOCK_TYPE_SPEED,
	MQ_BLOCK_TYPE_SLIDER,
	MQ_BLOCK_TYPE_MAX
}blockType;

typedef struct _blockedChannelInfo
{
	_blockedChannelInfo()
	{
		uDeliveryTag = 0;
		strData = "";
		uBlockedTime = 0;
		uBlockType = MQ_BLOCK_TYPE_SPEED;
	}
	UInt64 uDeliveryTag;
	string strData;
	UInt64 uBlockedTime;	//����ʱ��
	UInt32 uBlockType;     //��������
}blockedChannelInfo;

class CMQConsumerThread:public CThread
{

public:
    CMQConsumerThread(const char *name);
    ~CMQConsumerThread();
	bool 		 close();
    virtual bool Init(string strIp, unsigned int uPort, string strUserName, string strPassWord);
	bool 		 RabbitMQConnect();
    void 		 clearBlockedMsgAndResumeSlideWindowSize();

protected:
	bool 		 HandleCommonMsg(TMsg* pMsg);
	int 		 amqpConsumeMessage(string& strData);
	bool 		 amqpBasicConsume(UInt32 uChannelId, UInt64 uMqId, UInt32 uPreFetchSize = PRE_FETCH_MESSAGE_SIZE);
    int          amqpQueryMsgCnt(UInt32 uChannelId, const std::string& strQueue);
	void 		 amqpCloseAllChannels();
	void 		 SetChannelSpeedCtrl(int iChannelId, ChannelSpeedCtrl stuChannelSpeedCtrl);
	void 		 AddBlockedChannel(UInt32 uChannelId, UInt64 uDeliveryTag, string strData, UInt64 uBlockedTime, UInt32 uBlockType);
	void 		 ReleaseBlockedChannels(UInt64 uCurrentTime, int iCurrentMemoryMqQueueSize);
	void 		 MainLoop();

	virtual int  GetMemoryMqMessageSize() = 0;
	virtual bool InitChannels() = 0;
	virtual void processMessage(string& strData) = 0;

protected:
	string 			m_strIp;
	unsigned int 	m_uPort;
	string 			m_strUserName;
	string 			m_strPassWord;

	amqp_connection_state_t 		m_mqConnectionState;
	map<UInt64, MqConfig> 			m_mapMqConfig;
	map<int, ChannelSpeedCtrl> 		m_mapChannelSpeedCtrl;	//���ٺͼ���
	bool 							m_bConsumerSwitch;		//true:��ʼ����  false:ֹͣ����
	map<UInt32, blockedChannelInfo>	m_mapBlockedChannel;	//�����ص�ͨ��
	int								m_iSumChannelSpeed;		//���ٶ�
};

#endif /////__CMQCONSUMERTHREAD__
