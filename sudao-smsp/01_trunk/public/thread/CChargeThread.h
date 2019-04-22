#ifndef __C_CHARGE_THREAD_H__
#define __C_CHARGE_THREAD_H__

#include "Thread.h"
#include "sockethandler.h"
#include "internalservice.h"
#include "SnManager.h"
#include "HttpSender.h"
#include <time.h>
#include <map>
#include "SmspUtils.h"

using namespace std;

class TSendToChargeReqMsg:public TMsg
{
public:
	std::string m_strCallbackUrl;
	UInt32 m_iPayType; //0:immediately paying bill(interface) ;1:after paying bill
};

class ChargeSession
{
public:
	ChargeSession()
	{
		m_strUrl = "";
		m_pTimer = NULL;
		m_pHttpSender = NULL;
	}
	std::string m_strUrl;
	CThreadWheelTimer* m_pTimer;
	http::HttpSender* m_pHttpSender;
};

class CChargeThread : public CThread
{
	typedef std::map<UInt64, ChargeSession*> SessionMap;
public:
	CChargeThread(const char *name);
	~CChargeThread();
	bool Init();
	UInt32 GetSessionMapSize();

private:
	virtual void MainLoop();
	virtual void HandleMsg(TMsg* pMsg);
	void HandleSendToChargeReqMsg(TMsg* pMsg);
	void HandleHostIpResp(TMsg* pMsg);
	void HandleHttpResponseMsg(TMsg* pMsg);
	void HandleTimeOutMsg(TMsg* pMsg);
	bool OpenFile(UInt32 iPayType);
	void WriteFile(const std::string strData, UInt32 iPayType = 0);

private:
	InternalService* m_pInternalService;
	SnManager m_SnMng;

	FILE *m_WriteUpBillFile;
	FILE *m_AfterPayBillFile;
	std::string m_sLogfileDir;
    std::string m_sPrefixName;
    tm m_CurrentTime;

	SmspUtils m_SmspUtils;
	SessionMap m_mapSession;
};

#endif ///__C_CHARGE_THREAD_H__

