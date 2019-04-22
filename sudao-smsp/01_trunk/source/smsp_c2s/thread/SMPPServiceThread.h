#ifndef C_SMPPSERVERTHREAD_H_
#define C_SMPPSERVERTHREAD_H_

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "SMPPSocketHandler.h"
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
#include "Comm.h"
#include "SmsAccount.h"

using namespace std;



const UInt32 SMPP_ESME_ROK        = 0x00000000;    ////?T�䨪?��
const UInt32 SMPP_ESME_RINVMSGLEN = 0x00000001;    ///???��3��?���䨪
const UInt32 SMPP_ESME_RINVCMDLEN = 0x00000002;    ///?����?3��?���䨪
const UInt32 SMPP_ESME_RINVCMDID  = 0x00000003;    ///?TD���??����?ID
const UInt32 SMPP_ESME_RALYBND    = 0x00000005;    ///ESME ��??-�㨮?��
const UInt32 SMPP_ESME_RINVSRCADR = 0x0000000A;    ///?���??��?TD��
const UInt32 SMPP_ESME_RINVDSTADR = 0x0000000B;    ///??������??���䨪
const UInt32 SMPP_ESME_RINVMSGID  = 0x0000000C;    ///???��ID �䨪
const UInt32 SMPP_ESME_RBINDFAIL  = 0x0000000D;    ///�㨮?������㨹
const UInt32 SMPP_ESME_RINVPASWD  = 0x0000000E;    ///?��??�䨪?��
const UInt32 SMPP_ESME_RINVSYSID  = 0x0000000F;    ///?�̨�3ID �䨪?��
const UInt32 SMPP_ESME_RSYSERR    = 0x00000008;    ///?�̨�3�䨪?��
const UInt32 SMPP_ESME_RINVNUMDESTS = 0x00000033;  ////??����o?�䨪?��
const UInt32 SMPP_ESME_RINVESMCLASS = 0x00000043;  ///esm_class ��???��y?Y��?����
const UInt32 SMPP_ESME_RINVPARLEN = 0x000000C2;    ///2?��y3��?���䨪

class TSMPPHeartBeatReq: public TMsg
{
public:
    smsp::SMPPHeartbeatReq* m_reqHeartBeat;
    SMPPSocketHandler* m_socketHandle;
};

class TSMPPHeartBeatResp: public TMsg
{
public:
    smsp::SMPPHeartbeatResp* m_respHeartBeat;
    SMPPSocketHandler* m_socketHandle;
};

class SMPPLinkManager
{
public:
    SMPPLinkManager()
    {
        m_SMPPHandler    = NULL;
        m_HeartBeatTimer = NULL;
		m_uNoHBRespCount = 0;
    }
    SMPPSocketHandler* m_SMPPHandler;
	CThreadWheelTimer* m_HeartBeatTimer;
	string	m_strAccount;
	UInt8   m_uNoHBRespCount;
};

class  SMPPServiceThread:public CThread
{
	typedef std::map<string, SMPPLinkManager*> SMPPLinkMap;
	typedef std::map<string, TReportReceiveToSMSReq*> ReportMsgMap;
	typedef std::map<string, SmsAccount> AccountMap;
	
public:
    SMPPServiceThread(const char *name);
    virtual ~SMPPServiceThread();
    bool Init(const std::string ip, unsigned int port); 
	UInt32 GetSessionMapSize();

private:
	void MainLoop();
    InternalService* m_pInternalService;
    virtual void HandleMsg(TMsg* pMsg);
	void HandleSMPPAcceptSocketMsg(TMsg* pMsg);
	void HandleTimerOutMsg(TMsg* pMsg);
	void HandleSMPPHeartBeatReq(TMsg* pMsg);
	void HandleSMPPHeartBeatResp(TMsg* pMsg);
	void HandleSMPPLinkCloseReq(TMsg* pMsg);
	void HandleReportReceiveMsg(TMsg* pMsg);
	void HandleRedisRspSendReportToSMPP(TMsg* pMsg);
	void UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv);

	/**
		process unityhread send login resp msg
	*/
	void HandleLoginResp(TMsg* pMsg);

	/**
		process unitythread send submit resp msg
	*/
	void HandleSubmitResp(TMsg* pMsg);

	/**
		process unitythread send deliverreq msg
	*/
	void HandleDeliverReq(TMsg* pMsg);
	
private:
    InternalServerSocket* m_pServerSocekt;
    SMPPLinkMap m_LinkMap;
	ReportMsgMap	m_ReportMsgMap;
    SnManager* m_pSnManager;
	AccountMap m_AccountMap;
};

#endif ////C_SMPPSERVERTHREAD_H_

