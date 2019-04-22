#ifndef DIRECTMOTHREAD__
#define DIRECTMOTHREAD__
#include <string.h>
#include "Thread.h"
#include "SnManager.h"
#include "SmspUtils.h"
#include "SmsAccount.h"
#include "VerifySMS.h"

using namespace std;
typedef set<UInt64> UInt64Set;


class ChannelExtendPort;

enum
{
	UPSTM_STUS_SENDED=100,		//already sended
	UPSTM_STUS_URLBLANK=104,	//cb url is blank
};

enum channelType
{
	CHANGWCLIENTNOTFOUND,
	CHANAUTOORCLIENTFOUNDORERROR,
};

#define LONG_MO_WAIT_TIMER_ID     		1
class CMoParamInfo
{
public:
	CMoParamInfo()
	{
		m_uSmsFrom = 0;
		m_uC2sId = 0;
	}

	CMoParamInfo(const CMoParamInfo& other)
	{
		this->m_strClientId = other.m_strClientId;
		this->m_strSign = other.m_strSign;
		this->m_strSignPort = other.m_strSignPort;
		this->m_strExtendPort = other.m_strExtendPort;
		this->m_strShowPhone = other.m_strShowPhone;
		this->m_uSmsFrom = other.m_uSmsFrom;
		this->m_uC2sId = other.m_uC2sId;
	}
	
	~CMoParamInfo()
	{
	}
	
public:
	string m_strClientId;
	string m_strSign;
	string m_strSignPort;
	string m_strExtendPort;
	UInt64 m_uC2sId;
	string m_strShowPhone;
	UInt32 m_uSmsFrom;
};


class TDirectSendToDriectMoUrlReqMsg:public TMsg
{
public:
	CMoParamInfo m_MoParmInfo;
};

class TDirectMoToDeliveryReqMsg:public TMsg
{
public:
	string m_strData;
    string m_strNotifyUrl;
    UInt32 m_uIp;
};

class TToDirectMoReqMsg:public TMsg
{
public:
	TToDirectMoReqMsg()
	{
		m_uChannelId = 0;
		m_uMoType = 0;
		m_uChannelType = 0;
		m_uChannelMoId = 1;
		m_uRandCode = 0;
		m_uPkTotal = 1;
		m_uPkNumber = 1;
		m_bHead = false;
	}
	string m_strSp;      /////sp
	string m_strPhone;   /// phone
	string m_strDstSp;   /// complete port
	string m_strContent; //// MO content
	UInt32 m_uChannelId;
	UInt32 m_uMoType;
	UInt32 m_uChannelType;
	UInt64 m_uChannelMoId;
	bool   m_bHead;
	UInt16 m_uRandCode;
	UInt8  m_uPkTotal;
	UInt8  m_uPkNumber;
};

class LongMoInfo
{
public:
    LongMoInfo()
    {
		m_uPkNumber	 = 0;
		m_strShortContent = "";
    }
	UInt32  m_uPkNumber;
    string m_strShortContent;
};
class MoSession
{
public:
	MoSession()
	{
		m_uChannelId = 0;
		m_uMoType = 0;
		m_pTimer = NULL;
		m_uSmsFrom = 0;
		m_uChannelType = 0;
		m_uC2sId = 0;
		m_uChannelMoId = 1;
		m_uRevShortMoCount = 0;
	}

	~MoSession()
	{

	}

	CThreadWheelTimer *m_pTimer;
	string m_strSp;         //////sp number,those number the channel give us to use
	string m_strPhone;      
	string m_strDstSp;      //// complete port,those number showed on the phone
	string m_strGwExtend;
	string m_strContent;    
	UInt32 m_uChannelId;
	UInt32 m_uMoType;
	UInt32 m_uChannelType;


	UInt64 m_uC2sId;
	string m_strSignPort;     
	string m_strUserPort;
	string m_strSign;
	string m_strClientId;
	
	string m_strMoId;
	UInt64 m_uChannelMoId;
	UInt32 m_uSmsFrom;
	UInt32 m_uRevShortMoCount;
	UInt32  m_uPkTotal;
	std::vector <LongMoInfo> m_vecLongMoInfo;
};


class CDirectMoThread:public CThread
{
	typedef std::map<UInt64, MoSession*> SessionMap;
	//typedef std::map<std::string, models::UserGw> UserGWMap;
	typedef std::map<std::string, SmsAccount> AccountMap;
	typedef std::map<std::string,vector<models::SignExtnoGw> > SignExtnoGwMap;
	typedef map<string,MoSession*> LongMoMap;
public:
	CDirectMoThread(const char *name);
	virtual ~CDirectMoThread();
	bool Init();
private:	
	void HandleMsg(TMsg* pMsg);
	void HandleTimeOutMsg(TMsg* pMsg);
	void HandleToDirectMoReqMsg(TMsg* pMsg);
	void HandleRedisResp(TMsg* pMsg);

	void HandleSMSPCBRedisResp(redisReply* pRedisReply,UInt64 iSeq);
	void HandleMoPortRedisResp(redisReply* pRedisReply,UInt64 iSeq);

	void HandleDirectSendReqMsg(TMsg* pMsg);

	bool getUserBySp(UInt64 uSeq);
	void InsertMoRecord(UInt64 uSeq, int type);
	void processMoMessage(UInt64 uSeq);
	void PushMoToRandC2s(UInt64 uSeq);
	bool mergeLongMo(MoSession* pSession);
	void QuerySignMoPort(MoSession* pSession, UInt64 iSeq);
	void HandleGWMORedisResp(redisReply* pRedisReply,UInt64 uSeq);
	// ??““?????足?芍??“?? 1?那o?芍?谷2“⊿“a“o3“|?那?5?那o?3?“⊿?那?6?那o?芍?谷?“2?那?7?那o????∫
	int GetAcctStatus(const string& strClientId);
private:
	SnManager m_SnMng;
	SmspUtils m_SmspUtils;
	SessionMap m_mapSession;
	//UserGWMap m_mapUserGw;
	AccountMap m_accountMap;
	SignExtnoGwMap m_mapSignExtGw;
	CThreadWheelTimer* m_pTimer;
	std::multimap<UInt32,ChannelExtendPort> m_mapChannelExt;
	
	map<string,CMoParamInfo*> m_mapMoInfo;
	map<UInt32, UInt64Set> m_ChannelBlackListMap;

	////5.0
	map<UInt64,MqConfig> m_mapMqConfig;
	map<UInt64,ComponentConfig> m_mapComponentConfig;
	LongMoMap m_mapLogMo;
	VerifySMS*  m_pVerify;
};
#endif ////DIRECTMOTHREAD__
