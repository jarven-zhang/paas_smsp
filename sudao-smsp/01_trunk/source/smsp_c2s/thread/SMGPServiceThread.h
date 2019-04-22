#ifndef C_SMGPSERVERTHREAD_H_
#define C_SMGPSERVERTHREAD_H_

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "SMGPSocketHandler.h"
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

////#include "PhoneDao.h"




using namespace std;


#define	HEART_BEAT_MAX_COUNT			3
#define	HEART_BEAT_TIMEOUT				10*1000
#define	LONG_MSG_WAIT_TIMEOUT 			10*1000
#define	REDIS_RESP_WAIT_TIMEOUT 		30*1000

#define ACCOUNT_UNLOCK_TIMER_ID     	1   
#define LONG_MSG_WAIT_TIMER_ID     		2
#define REDIS_RESP_WAIT_TIMER_ID		3

#define SMGP_HEART_BEAT_TIMER_ID		4

/*
enum
{
	SMS_FROM_ACCESS_SMPP = 2,
	SMS_FROM_ACCESS_CMPP = 3,
	SMS_FROM_ACCESS_SGIP = 4,
	SMS_FROM_ACCESS_SMGP = 5,
	SMS_FROM_ACCESS_HTTP = 6,
};
*/
class TSMGPHeartBeatReq: public TMsg
{
public:
    smsp::SMGPHeartbeatReq* m_reqHeartBeat;
    SMGPSocketHandler* m_socketHandle;
};

class TSMGPHeartBeatResp: public TMsg
{
public:
    smsp::SMGPHeartbeatResp* m_respHeartBeat;
    SMGPSocketHandler* m_socketHandle;
};

class TReportReceiverMoMsg;
class TReportReceiveToSMSReq;


/*
class TReportReceiverMoMsg:public TMsg
{
public:
	TReportReceiverMoMsg()
	{
		m_uChannelId = 0;
	}

public:
	string m_strClientId;
	string m_strSign;
	string m_strMoId;
	string m_strSignPort;
	string m_strUserPort;
	string m_strPhone;
	string m_strContent;
	string m_strTime;
	string m_strMoUrl;
	string m_strShowPhone;
	UInt32 m_uChannelId;
};


class TReportReceiveToSMSReq: public TMsg
{
public:
	TReportReceiveToSMSReq()
    {
        m_pRedisRespWaitTimer = NULL;
		m_iLinkId = 0;
		m_iType = 0;
		m_iStatus = 0;
		m_lReportTime = 0;
		m_uUpdateFlag = 0;
    }
	string m_strSmsId;
	string m_strDesc;
	string m_strPhone;
	string m_strContent;
	string m_strUpstreamTime;
	string m_strClientId;
	string m_strSrcId;
	int    m_iLinkId;
	int    m_iType;
	int	   m_iStatus;
	Int64  m_lReportTime;

	UInt32 m_uUpdateFlag;  //// 0 update, 1 no update

	CThreadWheelTimer* m_pRedisRespWaitTimer;
};
*/
class SMGPLinkManager
{
public:
    SMGPLinkManager()
    {
        m_SMGPHandler    = NULL;
        m_HeartBeatTimer = NULL;
		m_uNoHBRespCount = 0;
    }
    SMGPSocketHandler* m_SMGPHandler;
	CThreadWheelTimer* m_HeartBeatTimer;
	string	m_strAccount;
	UInt8   m_uNoHBRespCount;
};

class  SMGPServiceThread:public CThread
{
	typedef std::map<string, SMGPLinkManager*> SMGPLinkMap;
	typedef std::map<string, TReportReceiveToSMSReq*> ReportMsgMap;
	typedef std::map<string, SmsAccount> AccountMap;
	
public:
    SMGPServiceThread(const char *name);
    virtual ~SMGPServiceThread();
    bool Init(const std::string ip, unsigned int port); 
	UInt32 GetSessionMapSize();

private:
	void MainLoop();
    InternalService* m_pInternalService;
    virtual void HandleMsg(TMsg* pMsg);
	void HandleSMGPAcceptSocketMsg(TMsg* pMsg);
	void HandleTimerOutMsg(TMsg* pMsg);
	void HandleSMGPHeartBeatReq(TMsg* pMsg);
	void HandleSMGPHeartBeatResp(TMsg* pMsg);
	void HandleSMGPLinkCloseReq(TMsg* pMsg);
	void HandleReportReceiveMsg(TMsg* pMsg);
	void HandleMoMsg(TMsg* pMsg);
	void HandleRedisRspSendReportToSMGP(TMsg* pMsg);
	void UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv);

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


	void HandleReportGetAgain(TMsg* pMsg);

	void HandleMOGetAgain(TMsg* pMsg);
private:
    InternalServerSocket* m_pServerSocekt;
    SMGPLinkMap m_LinkMap;
	ReportMsgMap	m_ReportMsgMap;
    SnManager* m_pSnManager;
	AccountMap m_AccountMap;
};


#endif /* C_SMSERVERTHREAD_H_ */

