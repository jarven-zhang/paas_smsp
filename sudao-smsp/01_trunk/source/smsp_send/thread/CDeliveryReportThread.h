#ifndef __C_DELIVERY_REPORT_THREAD_H__
#define __C_DELIVERY_REPORT_THREAD_H__

#include "Thread.h"
#include "sockethandler.h"
#include "internalservice.h"
#include "SnManager.h"
#include <map>
#include "SmspUtils.h"
#include "HttpSender.h"
#include "MqConfig.h"
#include "ComponentConfig.h"
#include "SmsAccount.h"
#include "Channel.h"
#include "ComponentRefMq.h"
class TDirectToDeliveryReportReqMsg:public TMsg
{
public:
	TDirectToDeliveryReportReqMsg()
	{	
		m_iStatus = 0;
		m_lReportTime = 0;
		m_uChannelId = 0;
		m_uChannelIdentify = 0;
		m_bSaveReport = false;
		m_industrytype = 99;
	}
	
	string  m_strChannelSmsId;
	Int32   m_iStatus;
	Int64   m_lReportTime;
	string  m_strDesc;
	string  m_strErrorCode;
	string  m_strReportStat;
	string  m_strInnerErrorcode;
	UInt32  m_uChannelId;

	string m_strPhone;
	UInt32 m_uChannelIdentify;
	bool   m_bSaveReport;//是否缓存状态报告
	int 	m_iHttpMode;
	UInt32 m_industrytype;
	
};


class DeliveryReportSession
{
public:
	DeliveryReportSession()
	{	
		m_uChannelId = 0;
		m_iStatus = 0;
		m_uChannelCnt = 0;
		m_uReportTime = 0;
		m_uSmsFrom = 0;
		m_uChannelIdentify =0;
		m_uC2sId = 0;
		m_pWheelTime = NULL;
		m_strReportStat = "";
		m_strResendMsg = "";
		m_uLongSms = 0;
		m_uFailedResendFlag = 0;
		m_uFailedResendTimes = 0;
		m_industrytype = 99;
	}

	string m_strClientId;
	UInt32  m_uChannelId;
	string m_strSmsId;
	string m_strPhone;
	Int32  m_iStatus;
	string m_strDesc;	
	string m_strReportDesc;  ///zhuang tai bao gap tou chuan
	UInt32 m_uChannelCnt;
	UInt64 m_uReportTime;
	UInt32 m_uSmsFrom;

	////use
	string m_strType;	
	string m_strChannelSmsId;
	UInt32 m_uChannelIdentify;
	UInt64 m_uC2sId;
	// for failed resend ,add by ljl 2018-01-22
	string m_strReportStat;//channel report desc
	
	string m_strInnerErrorcode;
	UInt32 m_uLongSms; // 0:short sms ,1:long sms
	UInt32 m_uOperater;
	UInt32 m_uSmsType;	
	string m_strResendMsg;
	UInt32 m_uFailedResendFlag;// t_sms_record_XXX  failed_resend_flag
	UInt32 m_uFailedResendTimes;// t_sms_record_XXX failed_resend_times
	UInt32 m_industrytype;//通道类型，告警用
	
	CThreadWheelTimer* m_pWheelTime;
};

struct ReportFailedTime
{
	string m_strToDay;
	UInt32 m_uTime;
	string m_strReportTimeList;
};

class CDeliveryReportThread : public CThread
{
	typedef std::map<UInt64,DeliveryReportSession*> DeliveryReportSessionMap; 
	typedef std::map<int, models::Channel> channelMap_t;
public:
	CDeliveryReportThread(const char *name);
	~CDeliveryReportThread();
	bool Init();	
	UInt32 GetSessionMapSize();
	
private:
	void MainLoop();
	void HandleMsg(TMsg* pMsg);
	void HandleSendToDeliveryReqMsg(TMsg* pMsg);
	void HandleDirectToDeliveryReqMsg(TMsg* pMsg);
	void HandleRedisResp(TMsg* pMsg);

	void HandleRedisRespGetKey(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisRespGetReportInfo(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisRespGetStatus(redisReply* pRedisReply,UInt64 iSeq);
	void HandleRedisComplete(UInt64 iSeq);
	
	void HandleTimeOutMsg(UInt64 iSeq);
	void UpdateRecord(string& strSmsUuid,string& strSendTime,UInt64 uSubmitDate,DeliveryReportSession* psession);
	
	void countReportFailed(UInt32 uChannelId, UInt32 industrytype, bool bResult,string toDay, UInt64 uNowTime);	
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);
	bool FailedReSendToReBack( DeliveryReportSession* pReportInfo );
	bool CheckFailedResend(redisReply* pRedisReply,DeliveryReportSession* pReportInfo);
	void InsertToBlankPhone(std::string& strResp);
	void updateBlankPhone(std::string& strResp);
	void GetReportInfoRedis(DeliveryReportSession* rtSession, UInt64 seq);
	void handleDBResponseMsg(TMsg* pMsg);
private:
	InternalService* m_pInternalService;
	SnManager m_SnMng;
	SmspUtils m_SmspUtils;

	DeliveryReportSessionMap m_mapSession;

	////5.0
	map<UInt64,MqConfig> m_mapMqConfig;
	map<UInt64,ComponentConfig> m_mapComponentConfig;	
	map<UInt32,ReportFailedTime> m_mapReportFailedAlarm;
	UInt32 m_uReportFailedAlarmValue;
	UInt32 m_uReportFailedSendTimeInterval;
	/*failed_resend 2018-01-22 add by ljl*/
	map<string, ComponentRefMq> m_componentRefMqInfo;
	map<string, SmsAccount> m_accountMap;
	channelMap_t            m_ChannelMap;	
	bool                 m_bResendSysSwitch;
	map<UInt32,FailedResendSysCfg> m_FailedResendSysCfgMap;
	
	UInt32 m_uReportFailedAlarmSwitch;
	/*blank phone begin*/
	bool                 m_bBlankPhoneSwitch;
	string 				 m_strBlankPhoneErr;
	/*blank phone end*/
};

#endif ///__C_DELIVERY_REPORT_THREAD_H__

