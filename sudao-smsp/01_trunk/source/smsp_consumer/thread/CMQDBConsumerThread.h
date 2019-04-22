#ifndef __CMQDBCONSUMERTHREAD__
#define __CMQDBCONSUMERTHREAD__

#include <string.h>
#include "Thread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "CMQAckConsumerThread.h"
#include "CDBThread.h"

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

class CMQDBConsumerThread:public CMQAckConsumerThread
{
public:
    CMQDBConsumerThread(const char *name, UInt8 uConsumerdatabase);
    ~CMQDBConsumerThread();
    bool 	Init(string strIp, unsigned int uPort, string strUserName, string strPassWord);
	
private:
	bool InitChannels();
	void HandleMsg(TMsg* pMsg);
	void HandleComponentRefMqRep(TMsg* pMsg);
	void processMessage(string& strData);
	int GetMemoryMqMessageSize();
	bool UpdateMqConsumer(map <string,ComponentRefMq>& mapComponentRefMq);
	bool AllChannelConsumerPause();
	bool AllChannelConsumerResume();
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);

    bool parseMsg(const string& strData, SessionInfo& sessionInfo);
    bool parseMsgForOperFlag(SessionInfo& sessionInfo);
    void parseMsgForTableName(SessionInfo& sessionInfo);
    void parseMsgForState(SessionInfo& sessionInfo);
    int parseMsgForStateEx(const string& strSql, string::size_type& posStart);
    void modifySql(SessionInfo& sessionInfo);

    void forwardMsg(const SessionInfo& sessionInfo);
    void forwardMsgEx(const SessionInfo& sessionInfo);

    void fillEmojiString(string& strSql);

private:
	UInt32 						m_uDBSwitch;
	//UInt64                      m_uMqId;
	map <string,ComponentRefMq> m_componentRefMqInfo;

    UInt8   m_ucConsumerFlag;
    UInt8   m_ucReQueueTimes;
    UInt32  m_uiReQueueTime;
	UInt32  m_uifWriteBigData;
};

#endif /////__CMQDBCONSUMERTHREAD__
