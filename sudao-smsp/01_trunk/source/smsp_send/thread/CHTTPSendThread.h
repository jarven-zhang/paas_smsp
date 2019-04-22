#ifndef __C_HTTP_SEND_THREAD_H__
#define __C_HTTP_SEND_THREAD_H__

#include "Thread.h"
#include "SnManager.h"
#include "ChannelMng.h"
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
#include <map>
#include "SmsContext.h"
#include "HttpSender.h"
#include "channelErrorDesc.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "SmsAccount.h"
#include "Comm.h"
#include "StateResponse.h"
#include "CHostIpThread.h"

using namespace std;

class SmsHttpInfo
{
public:
	SmsHttpInfo()
	{
    	m_uSmsType = 0;
		m_uSmsFrom = 0;
		m_uPayType = 0;
		m_uShowSignType = 0;
		m_uAccessId = 0;
		m_uArea = 0;
		m_iChannelId = 0;
		m_lSaleFee = 0;
		m_fCostFee = 0;
		m_uPhoneType = 0;
		m_uClientCnt = 0;
		m_uProcessTimes = 0;
		m_uC2sId = 0;

		////http protocal must
		m_uSendType = 0;
		m_uExtendSize = 0;
		m_uChannelType = 0;
		m_uChannelOperatorsType = 0;
		m_uChannelIdentify = 0;
		m_uLongSms = 0;
		m_uSendDate = 0;
		m_uHttpmode = 0;
		m_uResendTimes = 0;
		m_strFailedChannel = "";
		m_bResendCfgFlag = false;
		m_strShowSignType_c2s = "";
		m_strUserExtendPort_c2s = "";
		m_strSignExtendPort_c2s = "";
		m_uSignMoPortExpire = 60 * 60;
	}

	SmsHttpInfo(const SmsHttpInfo& other)
	{
		this->m_strClientId = other.m_strClientId;
		this->m_strUserName = other.m_strUserName;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strPhone = other.m_strPhone;
		this->m_strSign = other.m_strSign;
		this->m_strContent = other.m_strContent;
		this->m_uSmsType   = other.m_uSmsType;
		this->m_uSmsFrom = other.m_uSmsFrom;
		this->m_uPayType = other.m_uPayType;
		this->m_uShowSignType = other.m_uShowSignType;
		this->m_strUcpaasPort = other.m_strUcpaasPort;
		this->m_strSignPort = other.m_strSignPort;
		this->m_strDisplayNum= other.m_strDisplayNum;
		this->m_uAccessId= other.m_uAccessId;
		this->m_uArea= other.m_uArea;
		this->m_iChannelId= other.m_iChannelId;
		this->m_lSaleFee= other.m_lSaleFee;
		this->m_fCostFee= other.m_fCostFee;
		this->m_uPhoneType= other.m_uPhoneType;
		this->m_strAccessids= other.m_strAccessids;
		this->m_uClientCnt= other.m_uClientCnt;
		this->m_uProcessTimes= other.m_uProcessTimes;
		this->m_uC2sId= other.m_uC2sId;
		this->m_strC2sTime= other.m_strC2sTime;
		this->m_uSendDate= other.m_uSendDate;
		this->m_strErrorDate= other.m_strErrorDate;

		////http protocal must
		this->m_strCodeType= other.m_strCodeType; ////coding type
		this->m_strChannelName= other.m_strChannelName; ////channel name
		this->m_strSendUrl= other.m_strSendUrl;  ////channel url
		this->m_strSendData= other.m_strSendData; ////channel post data
		this->m_uSendType= other.m_uSendType; ////get,post,cmpp,smgp,sgip,smpp
		this->m_uExtendSize= other.m_uExtendSize;////channel extend port length
		this->m_uChannelType= other.m_uChannelType;////support auto sign,not support sign
		this->m_uChannelOperatorsType= other.m_uChannelOperatorsType; ////quan wang,yi dong,lian tong,dian xin,guo ji
		this->m_strChannelRemark= other.m_strChannelRemark; ////channel remark
		this->m_uChannelIdentify= other.m_uChannelIdentify;////channel in database mark
		this->m_uLongSms= other.m_uLongSms;  ////channel is support long sms

		this->m_strOauthUrl= other.m_strOauthUrl;
		this->m_strOauthData= other.m_strOauthData;
		this->m_strTemplateId= other.m_strTemplateId;
		this->m_strTemplateParam= other.m_strTemplateParam;
		this->m_strChannelTemplateId= other.m_strChannelTemplateId;

		/////yunmas channel add 
		this->m_strMasUserId= other.m_strMasUserId;
		this->m_strMasAccessToken= other.m_strMasAccessToken;		
		this->m_uBelong_salse   = other.m_uBelong_salse;
		this->m_uBelong_business= other.m_uBelong_business;
		this->m_uClusterType = other.m_uClusterType;    
		this->m_uclusterMaxNumber = other.m_uclusterMaxNumber; 
		this->m_uClusterMaxTime = other.m_uClusterMaxTime;   
        this->m_strChannelLibName = other.m_strChannelLibName;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_bResendCfgFlag = other.m_bResendCfgFlag;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_strOriChannelId = other.m_strOriChannelId;
		this->m_uSignMoPortExpire = other.m_uSignMoPortExpire;
		this->m_Channel = other.m_Channel;
		
	};

	SmsHttpInfo& operator=(const SmsHttpInfo& other)
	{
		this->m_strClientId = other.m_strClientId;
		this->m_strUserName = other.m_strUserName;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strPhone = other.m_strPhone;
		this->m_strSign = other.m_strSign;
		this->m_strContent = other.m_strContent;
		this->m_uSmsType   = other.m_uSmsType;
		this->m_uSmsFrom = other.m_uSmsFrom;
		this->m_uPayType = other.m_uPayType;
		this->m_uShowSignType = other.m_uShowSignType;
		this->m_strUcpaasPort = other.m_strUcpaasPort;
		this->m_strSignPort = other.m_strSignPort;
		this->m_strDisplayNum= other.m_strDisplayNum;
		this->m_uAccessId= other.m_uAccessId;
		this->m_uArea= other.m_uArea;
		this->m_iChannelId= other.m_iChannelId;
		this->m_lSaleFee= other.m_lSaleFee;
		this->m_fCostFee= other.m_fCostFee;
		this->m_uPhoneType= other.m_uPhoneType;
		this->m_strAccessids= other.m_strAccessids;
		this->m_uClientCnt= other.m_uClientCnt;
		this->m_uProcessTimes= other.m_uProcessTimes;
		this->m_uC2sId= other.m_uC2sId;
		this->m_strC2sTime= other.m_strC2sTime;
		this->m_uSendDate= other.m_uSendDate;
		this->m_strErrorDate= other.m_strErrorDate;

		////http protocal must
		this->m_strCodeType= other.m_strCodeType; ////coding type
		this->m_strChannelName= other.m_strChannelName; ////channel name
		this->m_strSendUrl= other.m_strSendUrl;  ////channel url
		this->m_strSendData= other.m_strSendData; ////channel post data
		this->m_uSendType= other.m_uSendType; ////get,post,cmpp,smgp,sgip,smpp
		this->m_uExtendSize= other.m_uExtendSize;////channel extend port length
		this->m_uChannelType= other.m_uChannelType;////support auto sign,not support sign
		this->m_uChannelOperatorsType= other.m_uChannelOperatorsType; ////quan wang,yi dong,lian tong,dian xin,guo ji
		this->m_strChannelRemark= other.m_strChannelRemark; ////channel remark
		this->m_uChannelIdentify= other.m_uChannelIdentify;////channel in database mark
		this->m_uLongSms= other.m_uLongSms;  ////channel is support long sms

		this->m_strOauthUrl= other.m_strOauthUrl;
		this->m_strOauthData= other.m_strOauthData;
		this->m_strTemplateId= other.m_strTemplateId;
		this->m_strTemplateParam= other.m_strTemplateParam;
		this->m_strChannelTemplateId= other.m_strChannelTemplateId;

		/////yunmas channel add 
		this->m_strMasUserId= other.m_strMasUserId;
		this->m_strMasAccessToken= other.m_strMasAccessToken;

		this->m_uBelong_salse   = other.m_uBelong_salse;
		this->m_uBelong_business= other.m_uBelong_business;

		this->m_uClusterType = other.m_uClusterType;    
		this->m_uclusterMaxNumber = other.m_uclusterMaxNumber; 
		this->m_uClusterMaxTime = other.m_uClusterMaxTime;   
		this->m_strChannelLibName = other.m_strChannelLibName;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_bResendCfgFlag = other.m_bResendCfgFlag;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_strOriChannelId = other.m_strOriChannelId;
		this->m_uSignMoPortExpire = other.m_uSignMoPortExpire;
		this->m_Channel = other.m_Channel;
		return *this;
	};
	
public:
	
    string  m_strClientId;
	string	m_strUserName;
	string  m_strSmsId;
	string  m_strPhone;
	string  m_strSign;
	string  m_strContent;
    UInt32  m_uSmsType;
	UInt32	m_uSmsFrom;
    UInt32  m_uPayType;
    UInt32  m_uShowSignType;
	string  m_strUcpaasPort;
	string  m_strSignPort;
	string  m_strDisplayNum;
	UInt64  m_uAccessId;
	UInt32  m_uArea;
	Int32   m_iChannelId;
	double  m_lSaleFee;
	float	m_fCostFee;
    UInt32  m_uPhoneType;
    string  m_strAccessids;
	UInt32  m_uClientCnt;
	UInt32  m_uProcessTimes;
	UInt64  m_uC2sId;
    string  m_strC2sTime;
	UInt64	m_uSendDate;
	string 	m_strErrorDate;

	////http protocal must
	string  m_strCodeType; ////coding type
	string  m_strChannelName; ////channel name
	string  m_strSendUrl;  ////channel url
	string  m_strSendData; ////channel post data
	UInt32  m_uSendType; ////get,post,cmpp,smgp,sgip,smpp
	UInt32	m_uExtendSize;////channel extend port length
	UInt32	m_uChannelType;////support auto sign,not support sign
	UInt32	m_uChannelOperatorsType; ////quan wang,yi dong,lian tong,dian xin,guo ji
	string	m_strChannelRemark; ////channel remark
	UInt32	m_uChannelIdentify;////channel in database mark
	UInt32	m_uLongSms;  ////channel is support long sms

    string m_strOauthUrl;
    string m_strOauthData;
    string m_strTemplateId;
    string m_strTemplateParam;
    string m_strChannelTemplateId;

	/////yunmas channel add 
	string m_strMasUserId;
	string m_strMasAccessToken;
	
	UInt64 m_uBelong_salse;
	UInt64 m_uBelong_business;
	UInt32 m_uClusterType;      /*为1时候组包*/
	UInt32 m_uclusterMaxNumber; /*最大组包个数*/
	UInt32 m_uClusterMaxTime;   /*最大组包时间*/
	
    std::string m_strChannelLibName; /* 通道使用的库名 */
	UInt32 m_uHttpmode;
	UInt32 m_uResendTimes;//failed resend times
	string m_strFailedChannel;//failed resend channel
	bool   m_bResendCfgFlag;
	string m_strShowSignType_c2s;
	string m_strUserExtendPort_c2s;
	string m_strSignExtendPort_c2s;
	string m_strChannelOverrate_access;
	string m_strOriChannelId;
	UInt32 m_uSignMoPortExpire;
	models::Channel m_Channel;
};


class HttpSendSession
{
public:
	HttpSendSession()
	{
		m_pSmsContext = NULL;
		m_pWheelTime = NULL;
		m_pHttpSender = NULL;
		m_uSendType = 0;
		m_pSmsContext = new smsp::SmsContext();
	}

	~HttpSendSession()
	{
		if (NULL != m_pSmsContext)
	    {
	        delete m_pSmsContext;
	        m_pSmsContext = NULL;
	    }
		
	    if (NULL != m_pWheelTime)
	    {
	        delete m_pWheelTime;
	        m_pWheelTime = NULL;
	    }
		
	    if (NULL != m_pHttpSender)
	    {
	        m_pHttpSender->Destroy();
	        SAFE_DELETE( m_pHttpSender );
	        m_pHttpSender = NULL;
	    }
		
	};

public:
	smsp::SmsContext* m_pSmsContext;
	CThreadWheelTimer* m_pWheelTime;
	http::HttpSender* m_pHttpSender;

	////add by host ip
	string m_strHttpUrl;
	string m_strHttpData;
	UInt32 m_uSendType; ///get or post
	string m_strSendContent;

    //通道认证
    string m_strOauthUrl;
    string m_strOauthData;
	
	//add by reback
	SmsHttpInfo m_SmsHttpInfo;
	string m_strSessionId;

	smsp::mapStateResponse m_mapResponses;/*手机号码对应的状态*/
	
};

class TDispatchToHttpSendReqMsg:public TMsg
{
public:
	SmsHttpInfo m_smsParam;	
	UInt32      m_uPhonesCount;
	UInt64      m_uSessionId;
};

class TSendToDeliveryReportReqMsg:public TMsg
{
public:
	TSendToDeliveryReportReqMsg()
	{
		m_uChannelId = 0;
		m_iStatus = 0;
		m_uChannelCnt = 0;
		m_uReportTime = 0;
		m_uSmsFrom = 0;
		m_uLongSms = 0;
		m_uC2sId = 0;
	}

	string m_strClientId;
	UInt32 m_uChannelId;
	string m_strSmsId;
	string m_strPhone;
	UInt32 m_iStatus;
	string m_strDesc;
	string m_strReportDesc;
	UInt32 m_uChannelCnt;
	UInt64 m_uReportTime;
	UInt32 m_uSmsFrom;

	UInt32 m_uLongSms;
	string m_strType;
	UInt64 m_uC2sId;
};

class yunMasInfo
{
public:
	yunMasInfo()
	{
		m_uAccessTokenExpireSeconds = 0;
	}
	string m_strMasUserId;
	string m_strMasAccessToken;
	UInt64 m_uAccessTokenExpireSeconds;
};

class CHTTPSendThread : public CThread
{	
    typedef std::map<UInt32, models::AccessToken> AccessTokenMap;
	#define HTTP_SEND_SESSION_TIMEOUT  20000
	#define HTTPS_SEND_SESSION_TIMEOUT  20000

public:
	CHTTPSendThread(const char *name);
	~CHTTPSendThread();
	bool Init(UInt32 ThreadId);
	UInt32 GetSessionMapSize();

private:
	void MainLoop();
	void HandleMsg(TMsg* pMsg);

	bool yunMasGetToken( TDispatchToHttpSendReqMsg* pSendReq );
	bool yumMasGetTokenAccess( HttpSendSession *httpSession, smsp::Channellib *mylib,
		        UInt32 uIpAddr, std::vector<std::string> &mv );
	
	void HandleDispatchToHttpSendReqMsg(TMsg* pMsg);
	void HandleHostIpResp(TMsg* pMsg);
	void HandleHttpResponse(TMsg* pMsg);
	void HandleHttpsResponse( TMsg* pMsg );
	void HandleTimeOut(TMsg* pMsg);
	void BindParser(string& strData, HttpSendSession  *pSession );
	UInt32 ParseMsgContent(string& content, vector<string>& contents, string& sign, UInt32 uLongSms, UInt32 uSmslength );
    void DisposeHttpsResponse(std::string strResponse, HttpSendSession* pSession);
	void HandleChannelUpdate ( TMsg* pMsg );
	void smsSendMsgToHttpChan ( HttpSendSession *httpSession, std::string encoded_msg, UInt64 uSeq, UInt64 uIpAddr );
	
	void HttpSendStatusReport( HttpSendSession* pSession );

	void ConvertMutiResponse( HttpSendSession* pSession, smsp::mapStateResponse &reponses );
	void smsSendLongMsg(HttpSendSession *httpSession, string& szSign, smsp::SmsContext *pSmsContext, THostIpResp* pResp , vector<string>& contents, bool isIncludeChinese);
	void smsSendShortMsg(HttpSendSession  * httpSession, string& szSign, smsp::SmsContext *pSmsContext, THostIpResp* pResp , vector<string>& contents);
	void smsSendSplitContent(HttpSendSession  * httpSession, string& szSign, smsp::SmsContext *pSmsContext, THostIpResp* pResp, bool isIncludeChinese);
	UInt32 SplitMsgContent(HttpSendSession  * httpSession, vector<string>& contents, string& sign, bool isIncludeChinese, smsp::SmsContext *pSmsContext);

private:
	InternalService* m_pInternalService;
	SnManager*   m_pSnMng;

	typedef std::map<UInt64, HttpSendSession*> HttpSendSessionMap;

	HttpSendSessionMap m_mapSession;
	ChannelMng* m_pLibMng;

	UInt32 m_ThreadId;
    AccessTokenMap m_mapAccessToken;

	map<UInt32,yunMasInfo> m_mapYunMasInfo;
	std::map<std::string,SmsAccount> m_accountMap;
};

#endif



