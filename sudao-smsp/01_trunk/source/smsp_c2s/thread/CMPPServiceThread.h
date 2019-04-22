#ifndef C_CMPPSERVERTHREAD_H_
#define C_CMPPSERVERTHREAD_H_

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "CMPPSocketHandler.h"
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
#define CMPP_HEART_BEAT_TIMER_ID		4


//enum
//{
//	SMS_FROM_ACCESS_CMPP3 = 1,
//	SMS_FROM_ACCESS_SMPP = 2,
//	SMS_FROM_ACCESS_CMPP = 3,
//	SMS_FROM_ACCESS_SGIP = 4,
//	SMS_FROM_ACCESS_SMGP = 5,
//	SMS_FROM_ACCESS_HTTP = 6,
//	SMS_FROM_ACCESS_HTTP_2 = 7,
//	SMS_FROM_ACCESS_HTTP_3 = 8,
//	SMS_FROM_ACCESS_MAX,
//};

class TCMPPHeartBeatReq: public TMsg
{
public:
    smsp::CMPPHeartbeatReq* m_reqHeartBeat;
    CMPPSocketHandler* m_socketHandle;
};

class TCMPPHeartBeatResp: public TMsg
{
public:
    smsp::CMPPHeartbeatResp* m_respHeartBeat;
    CMPPSocketHandler* m_socketHandle;
};

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
		m_uIdentify = 0;
        m_iRedisIndex = -1;
		m_uChannelCount = 1;
    }
	string m_strSmsId;
	string m_strDesc;
	string m_strReportDesc;
	string m_strInnerErrcode;
	string m_strPhone;
	string m_strContent;
	string m_strUpstreamTime;
	string m_strClientId;
	string m_strSrcId;
	string m_strChannelId;
	int    m_iLinkId;
	int    m_iType;
	int	   m_iStatus;
	Int64  m_lReportTime;
	UInt32 m_uIdentify;		//index of t_sms_access_*
	UInt32 m_uUpdateFlag;  //// 0 update, 1 no update
    Int32 m_iRedisIndex;
	UInt32 m_uChannelCount;

	CThreadWheelTimer* m_pRedisRespWaitTimer;
};

class CMPPLinkManager
{
public:
    CMPPLinkManager()
    {
        m_CMPPHandler    = NULL;
        m_HeartBeatTimer = NULL;
		m_uNoHBRespCount = 0;
    }
    CMPPSocketHandler* m_CMPPHandler;
	CThreadWheelTimer* m_HeartBeatTimer;
	string	m_strAccount;
	UInt8   m_uNoHBRespCount;
};

class  CMPPServiceThread:public CThread
{
	typedef std::map<string, CMPPLinkManager*> CMPPLinkMap;
	typedef std::map<string, TReportReceiveToSMSReq*> ReportMsgMap;
	typedef std::map<string, SmsAccount> AccountMap;

public:
    CMPPServiceThread(const char *name);
    virtual ~CMPPServiceThread();
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
	void sendCmppLongMo(string strContent,string strDestId,string strPhone,CMPPSocketHandler* CMPPHandler);
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
    CMPPLinkMap m_LinkMap;
	ReportMsgMap	m_ReportMsgMap;
    SnManager* m_pSnManager;
	AccountMap m_AccountMap;
	UInt32     m_uRandom;
};


#endif /* C_SMSERVERTHREAD_H_ */

