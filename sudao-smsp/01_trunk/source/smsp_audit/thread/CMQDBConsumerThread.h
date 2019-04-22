#ifndef __CMQDBCONSUMERTHREAD__
#define __CMQDBCONSUMERTHREAD__

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


class CMQDBConsumerThread:public CMQConsumerThread
{
public:
    typedef map<string, ComponentRefMq> ComponentRefMqMap;

public:
    CMQDBConsumerThread(const char *name);
    ~CMQDBConsumerThread();
    bool Init(string strIp, unsigned int uPort,string strUserName,string strPassWord);

private:
    bool InitChannels();
    void HandleMsg(TMsg* pMsg);
//    void HandleQueueUpdateRep(TMsg* pMsg);
    void handleComponentRefMqRep(TMsg * pMsg);
    void processMessage(string& strData);
//    bool UpdateChannelSpeedCtrl(UInt64 uMqId);
    int  GetMemoryMqMessageSize();
private:
    ComponentRefMqMap m_mapComponentRefMq;
    UInt64 m_uMqId;
};

#endif /////__CMQCONSUMERTHREAD__