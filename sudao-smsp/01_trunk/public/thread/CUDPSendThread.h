#ifndef __C_UDPSENDTHREAD_H__
#define __C_UDPSENDTHREAD_H__

#include <string.h>
#include <time.h>
#include "Thread.h"

class TUDPSendMsg : public TMsg
{
public:
    std::string m_strSendData;
};

class CUDPSendThread : public CThread
{

public:
    CUDPSendThread(const char *name);
    ~CUDPSendThread();	
    bool Init( string strIp, UInt32 uport );

public:	
    static bool gInit( string strAddrIp, UInt32 uAddrPort, UInt32 uThreadCnt );
	static bool gdeInit();
	static bool postSendMsg( TMsg* pMsg );	

private:
    virtual void HandleMsg( TMsg* pMsg );
    bool udpSend( string &data );
	bool openSocket();
	bool HandleUDPSend( TMsg* pMsg );

private:
	Int32  m_isockfd;
	string  m_strIp;
	UInt32  m_uPort;

};

#endif  //__C_SQLFILETHREAD_H__

