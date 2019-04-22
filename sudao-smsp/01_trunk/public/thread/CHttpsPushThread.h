#ifndef __H_HTTPS_PUSH_THEARD_
#define __H_HTTPS_PUSH_THEARD_

#include "platform.h"
#include <map>
#include <vector>
#include "HttpsSender.h"
#include "Thread.h"
#include "HttpSender.h"
#include "LogMacro.h"


class THttpsRequestMsg : public TMsg
{
public:
	std::vector < std::string > vecHeader;
	UInt32      uIP;
	std::string strUrl;
	std::string strBody;
};

class THttpsResponseMsg : public TMsg
{
public:
	UInt32 m_uStatus;
	std::string m_strResponse;
};

/* http会话*/
class CHttpsConn
{
public:
	UInt64 connLastSendTime;
	http::HttpsSender httpsSender;
};
class CHttpConn
{
public:
	CHttpConn()
	{
		pHttpSender = NULL;
	}
	http::HttpSender *pHttpSender;
	CThreadWheelTimer* m_httpTimer;
};

enum
{
	CHTTPS_SEND_STATUS_NONE,
	CHTTPS_SEND_STATUS_SUCC,
	CHTTPS_SEND_STATUS_CONN_ERR,
	CHTTPS_SEND_STATUS_FAIL
};


class CHttpsSendThread : public CThread
{
	#define CONN_CHECK_TIMER_ID (0x123456789)
	#define CONN_MAX_IDLE_TIME  (2*60) /*最大空闲时间*/
	#define CONN_CHECK_TIMER_INTERVAL (30*1000)

	/*https保存的会话*/
	typedef std::map< std::string, CHttpsConn *> httpsConnMap;
	typedef std::map< UInt64, CHttpConn *> httpConnMap;

public:
	CHttpsSendThread(const char *strName);
	~CHttpsSendThread();
    virtual void HandleMsg(TMsg* pMsg);
	virtual void MainLoop();
	bool Init();

public:
	static bool InitHttpsSendPool( UInt32 uThreadCount );
	static bool sendHttpsMsg ( TMsg* pMsg );
	static void sendHttpMsg ( TMsg* pMsg);

private:

	UInt32 HandleHttpsSendReq( TMsg* pMsg, int mode);
	void  HandleTimeOut(TMsg* pMsg);
	void HandleHttpSendReq( TMsg* pMsg);
	void HandleHttpResponseMsg(TMsg* pMsg);

private:
	InternalService* m_pInternalService;
	httpsConnMap   m_mapHttpsConns;
	httpConnMap	   m_mapHttpConns;
    CThreadWheelTimer* m_wheelTimer;

	static UInt32  m_uThreadCount;
	static std::vector< CThread * > m_uThreads;

};

#endif
