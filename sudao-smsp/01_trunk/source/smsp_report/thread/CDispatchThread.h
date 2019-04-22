#ifndef _CDISPATCH_Thread_h_
#define _CDISPATCH_Thread_h_
#include <set>
#include <string.h>
#include "Thread.h"
#include "SnManager.h"
#include "SmspUtils.h"
#include "SignExtnoGw.h"
#include "channelErrorDesc.h"
#include "MqConfig.h"
#include "ComponentConfig.h"
#include "SmsAccount.h"
#include "Channel.h"
#include "ComponentRefMq.h"
#include "Channel.h"
#include "CRedisThread.h"
#include "VerifySMS.h"

using namespace std;
using namespace models;
class ChannelExtendPort;

enum
{
	UPSTM_STUS_SENDED=100,		//already sended
	UPSTM_STUS_URLBLANK=104,	//cb url is blank

};



/*短信类型定义*/

enum channelType
{
	CHANGWCLIENTNOTFOUND,
	CHANAUTOORCLIENTFOUNDORERROR,
};



typedef set<UInt64> UInt64Set;

class TDispatchToReportPushReqMsg:public TMsg
{
public:
	UInt32 m_uIp;
	string m_strData;
	string m_strDeliveryUrl;
};


class TReportToDispatchReqMsg:public TMsg
{
public:
	TReportToDispatchReqMsg()
	{
		m_iStatus = 0;
		m_lReportTime =0;
		m_uChannelId =0;
		m_uChannelIdentify =0;
		m_bSaveReport = false;
		m_strReportStat = "";
	}

	string  m_strChannelSmsId;
	Int32   m_iStatus;
	Int64   m_lReportTime;
	string  m_strDesc;
	UInt32  m_uChannelId;
	string  m_strChannelLibName;
	string 	m_strPhone;
	UInt32 	m_uChannelIdentify;
	UInt32  m_uClusterType; /*组包类型*/
	bool    m_bSaveReport;//是否缓存状态报告
	string  m_strReportStat;
	string  m_strInnerErrorcode;

};

class TUpStreamToDispatchReqMsg:public TMsg
{
public:
	string  m_strPhone;
	string	m_strAppendId;
	string 	m_strUpTime;
	UInt32  m_iChannelId;
    string m_strChannelLibName;
	UInt64 	m_lUpTime;
	string 	m_strContent;
	UInt32  m_uChannelType;
	string  m_strMoPrefix;
};

class SessionStruct
{
public:
	SessionStruct()
	{
		m_iStatus = 0;
		m_lReportTime = 0;
		m_uChannelId = 0;
		m_lUpTime = 0;
		m_uSmsfrom = 0;
		m_uChannelType = 0;
		m_uChannelIdentify =0;
		m_uChannelCnt = 0;
		m_uC2sId = 0;
		m_uClusterType = 0;
		m_strReportStat = "";
		m_strResendMsg = "";
		m_uFailedResendFlag = 0;
		m_uFailedResendTimes = 0;
	}
	string  m_strClientId;
	UInt32  m_uChannelId;
	string  m_strSmsId;
	string  m_strPhone;
	Int32   m_iStatus;
	string  m_strDesc;
	string  m_strReportDesc;
	string  m_strInnerErrorcode;
	UInt32  m_uChannelCnt;
	Int64   m_lReportTime;
	UInt32  m_uSmsfrom;

	////use
	string m_strType;
	string  m_strChannelSmsId;
	UInt32 m_uChannelIdentify;
	UInt64 m_uC2sId;

	////mo
	string	m_strAppendId;
	string 	m_strUpTime;

	UInt64 	m_lUpTime;
	string 	m_strContent;
	UInt32  m_uChannelType;
	string  m_strSmsUuid;
	string  m_strUserPort;
	string  m_strSignPort;
	string  m_strSign;
	string  m_strMoId;

	UInt32  m_uClusterType;
	// for failed resend ,add by ljl 2018-01-22
	string m_strReportStat;//channel report desc
	UInt32 m_uLongSms; // 0:short sms ,1:long sms
	UInt32 m_uOperater;
	UInt32 m_uSmsType;
	string m_strResendMsg;
	UInt32 m_uFailedResendFlag;// t_sms_record_XXX  failed_resend_flag
	UInt32 m_uFailedResendTimes;// t_sms_record_XXX failed_resend_times
	UInt32 m_uClusterGetType;
	string m_strGwExtend;

};

struct ReportFailedTime
{
	string m_strToDay;
	UInt32 m_uTime;
	string m_strReportTimeList;
};

class CDispatchThread:public CThread
{
	typedef std::map<UInt64, SessionStruct*> SessionMap;
	typedef std::map<std::string,vector<SignExtnoGw> > SignExtnoGwMap;
	typedef std::map<std::string, SmsAccount> AccountMap;
	typedef std::map<int, models::Channel> ChannelMap_t;
public:
	CDispatchThread(const char *name);
	virtual ~CDispatchThread();
	bool Init();
	UInt32 GetSessionMapSize();

private:
	void HandleMsg(TMsg* pMsg);
	void HandleReportToDispatchReqMsg(TMsg* pMsg);
	void HandleUpStreamToDisPatchReqMsg(TMsg* pMsg);
	void QuerySignMoPort(SessionStruct* pSession, UInt64 iSeq);
	void HandleRedisResp(TMsg* pMsg);
	void getErrorCode(string &strChannelLibName,UInt32 uChannelId,string strIn,string& strDesc,string& strReportDesc);
	void getErrorByChannelId(UInt32 uChannelId,string strIn,string& strDesc,string& strReportDesc);
	void smsGetInnerError(string strDircet, UInt32 uChannelId,string strType,string strIn, string& strInnerError);

	void HandleRedisGetMoResp(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisGetSmspCbResp(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisGetKeyResp(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisGetReportInfoResp( TRedisResp *pRedisRsp );
	void HandleRedisGetStatusResp(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisComplete(UInt64 iSeq);

	void UpdateRecord(string& strSmsUuid,string& strSendTime,UInt64 uSubmitDate,SessionStruct* psession);
	void InsertMoRecord(UInt64 uSeq, int type);
	void InsertToBlankPhone(std::string& strResp);
	void updateBlankPhone(std::string& strResp);
	void PushMoToRandC2s(UInt64 uSeq);
	void countReportFailed(UInt32 uChannelId, bool bResult,string toDay, UInt64 uNowTime);
	void HandleTimeOutMsg(TMsg* pMsg);
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);
	bool getUserBySp(UInt64 uSeq);
	bool FailedReSendToReBack( SessionStruct* pReportInfo );
	bool CheckFailedResend(redisReply* pRedisReply,SessionStruct* pReportInfo);
	UInt32 GetChannelIndustrytype(int ChannelId);
	void HandleGWMORedisResp(redisReply* pRedisReply,UInt64 uSeq);
	void GetReportInfoRedis(SessionStruct* rtSession, UInt64 seq);
	void handleDBResponseMsg(TMsg* pMsg);
	int GetAcctStatus(const string& strClientId);

public:

private:
	SnManager m_SnMng;
	SessionMap m_mapSession;
	SmspUtils m_SmspUtils;
	SignExtnoGwMap m_mapSignExtGw;
	map<UInt32, UInt64Set> m_ChannelBlackListMap;

	////5.0
	map<string,channelErrorDesc> m_mapChannelErrorCode;
	map<UInt64,MqConfig> m_mapMqConfig;
	map<UInt64,ComponentConfig> m_mapComponentConfig;

	map<UInt32,ReportFailedTime> m_mapReportFailedAlarm;
	UInt32 m_uReportFailedAlarmValue;
	UInt32 m_uReportFailedSendTimeInterval;
	UInt32 m_uReportFailedAlarmSwitch;
	std::map<string, SmsAccount> m_SmsAccountMap;
	std::multimap<UInt32,ChannelExtendPort> m_mapChannelExt;
	/*failed_resend 2018-01-22 add by ljl*/
	map<string, ComponentRefMq> m_componentRefMqInfo;
	bool                 m_bResendSysSwitch;
	map<UInt32,FailedResendSysCfg> m_FailedResendSysCfgMap;
	ChannelMap_t m_ChannelMap;
	VerifySMS*  m_pVerify;
	/*blank phone begin*/
	bool                 m_bBlankPhoneSwitch;
	string 				 m_strBlankPhoneErr;
	/*blank phone end*/
};


#endif

