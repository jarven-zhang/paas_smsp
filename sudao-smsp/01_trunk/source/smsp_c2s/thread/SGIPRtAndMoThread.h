#ifndef __SGIPRTANDMOTHREAD__
#define __SGIPRTANDMOTHREAD__

#include <list>
#include <map>
#include "SnManager.h"
#include "Thread.h"
#include "SGIPPush.h"
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
#include "SmsAccount.h"


const UInt32 SGIP_REDIS_TIMEOUT_NUM = 6*1000;
const UInt32 SGIP_CONNECT_TIMEOUT_NUM = 30*1000;

const UInt64 SGIP_REDIS_TIMEOUT_ID = 10;
const UInt64 SGIP_CONNECT_TIMEOUT_ID = 11;

class TReportReceiveToSMSReq;

class CSgipRtReqMsg:public TMsg
{
public:
	string m_strClientId;
	UInt32 m_uSequenceIdNode;
	UInt32 m_uSequenceIdTime;
	UInt32 m_uSequenceId;

	string m_strPhone;
	UInt32 m_uState;
	UInt32 m_uErrorCode;
};

class CSgipFailRtReqMsg:public TMsg
{
public:
	string m_strClientId;
	UInt32 m_uSequenceIdNode;
	UInt32 m_uSequenceIdTime;
	UInt32 m_uSequenceId;

	string m_strPhone;
	int m_uState;
};

class CSgipCloseReqMsg:public TMsg
{
public:
	string m_strClientId;
};

class CSgipMoReqMsg:public TMsg
{
public:
	string m_strClientId;
	string m_strPhone;
	string m_strSpNum;
	string m_strContent;
};

class CSgipRtAndMoThread:public CThread
{
	typedef std::map<string, SmsAccount> AccountMap;
	typedef std::map<string, TReportReceiveToSMSReq*> ReportMsgMap;
	typedef std::map<string, CSgipPush*> RtAndMoLinkMap;

public:
	CSgipRtAndMoThread(const char* name);
	~CSgipRtAndMoThread();
	bool Init(); 

private:
	virtual void HandleMsg(TMsg* pMsg);

	void procRtReqMsg(TMsg* pMsg);

	void procMoReqMsg(TMsg* pMsg);

	void procRtPushReqMsg(TMsg* pMsg);

	void procMoPushReqMsg(TMsg* pMsg);
	
	void MainLoop();

	void procSgipReportRedisResp(TMsg* pMsg);

	void procTimeOut(TMsg* pMsg);

	void procSgipCloseReqMsg(TMsg* pMsg);

	void UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv);

	/**
		process unitythread send failed deliverreq msg
	*/
	void HandleDeliverReq(TMsg* pMsg);

	void HandleGetLinkedClientReq(TMsg* pMsg);
public:
	InternalService* m_pInternalService;
	SnManager m_SnManager;

private:
	ReportMsgMap	m_ReportMsgMap;
	AccountMap m_AccountMap;

	RtAndMoLinkMap m_mapRtAndMoLinkMap;
	CThreadWheelTimer* m_pTimer_CheckLink;
	
};

#endif ////__SGIPRTANDMOTHREAD__

