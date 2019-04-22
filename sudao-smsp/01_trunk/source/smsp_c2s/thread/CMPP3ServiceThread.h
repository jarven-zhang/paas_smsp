#ifndef C_CMPP3SERVERTHREAD_H_
#define C_CMPP3SERVERTHREAD_H_

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "CMPP3SocketHandler.h"
#include "internalservice.h"
#include "internalsocket.h"
#include "eventtype.h"
#include "address.h"
#include "sockethandler.h"
#include "inputbuffer.h"
#include "outputbuffer.h"
#include "internalserversocket.h"
#include "ibuffermanager.h"
#include "epollselector.h"
#include "internalserversocket.h"
#include "SnManager.h"
#include "SmsAccount.h"
#include "Comm.h"
#include "CMPPServiceThread.h"
////#include "PhoneDao.h"




using namespace std;

#if 0
#define	HEART_BEAT_MAX_COUNT			3
#define	HEART_BEAT_TIMEOUT				10*1000
#define	LONG_MSG_WAIT_TIMEOUT 			10*1000
#define	REDIS_RESP_WAIT_TIMEOUT 		30*1000

#define ACCOUNT_UNLOCK_TIMER_ID     	1   
#define LONG_MSG_WAIT_TIMER_ID     		2
#define REDIS_RESP_WAIT_TIMER_ID		3
#define CMPP_HEART_BEAT_TIMER_ID		4
#endif
class TCMPP3HeartBeatReq: public TMsg
{
public:
    cmpp3::CMPPHeartbeatReq* m_reqHeartBeat;
    CMPP3SocketHandler* m_socketHandle;
};

class TCMPP3HeartBeatResp: public TMsg
{
public:
    cmpp3::CMPPHeartbeatResp* m_respHeartBeat;
    CMPP3SocketHandler* m_socketHandle;
};

class CMPP3LinkManager
{
public:
    CMPP3LinkManager()
    {
        m_CMPPHandler    = NULL;
        m_HeartBeatTimer = NULL;
		m_uNoHBRespCount = 0;
    }
    CMPP3SocketHandler* m_CMPPHandler;
	CThreadWheelTimer* m_HeartBeatTimer;
	string	m_strAccount;
	UInt8   m_uNoHBRespCount;
};

class  CMPP3ServiceThread:public CThread
{
	typedef std::map<string, CMPP3LinkManager*> CMPP3LinkMap;
	typedef std::map<string, TReportReceiveToSMSReq*> ReportMsgMap;
	typedef std::map<string, SmsAccount> AccountMap;
	
public:
    CMPP3ServiceThread(const char *name);
    virtual ~CMPP3ServiceThread();
    bool Init(const std::string ip, unsigned int port); 
	UInt32 GetSessionMapSize();

private:
	void MainLoop();
    InternalService* m_pInternalService;
    virtual void HandleMsg(TMsg* pMsg);
	void HandleCMPPAcceptSocketMsg(TMsg* pMsg);
	void HandleTimerOutMsg(TMsg* pMsg);
	void HandleCMPPHeartBeatReq(TMsg* pMsg);
	void HandleCMPPHeartBeatResp(TMsg* pMsg);
	void HandleCMPPLinkCloseReq(TMsg* pMsg);
	void HandleReportReceiveMsg(TMsg* pMsg);
	void HandleMoMsg(TMsg* pMsg);
	void HandleRedisRspSendReportToCMPP(TMsg* pMsg);
	void UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv);
	void sendCmppLongMo(string strContent,string strDestId,string strPhone,CMPP3SocketHandler* CMPPHandler);
	UInt32 getRandom();
	/**
		process cunitythread send loginresp msg
	*/
	void HandleLoginResp(TMsg* pMsg);

	/**
		process submit resp msg
	*/
	void HandleSubmitResp(TMsg* pMsg);

	/**
		process unitythread send deliverreq msg
	*/
	void HandleDeliverReq(TMsg* pMsg);

	/*
		get Rpt from redis again.
	*/
	void HandleReportGetAgain(TMsg* pMsg);

	/*
		get mo from redis again.
	*/
	void HandleMOGetAgain(TMsg* pMsg);
private:
    InternalServerSocket* m_pServerSocekt;
    CMPP3LinkMap m_LinkMap;
	ReportMsgMap	m_ReportMsgMap;
    SnManager* m_pSnManager;
	AccountMap m_AccountMap;
	UInt32     m_uRandom;
};


#endif /* C_SMSERVERTHREAD_H_ */

