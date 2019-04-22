#ifndef __ICHANNEL__
#define __ICHANNEL__
#include "CThreadWheelTimer.h"
#include "Channel.h"
#include "SmsContext.h"
#include "Callback.h"
#include <map>
#include <queue>
#include <vector>
#include "SnManager.h"
#include "writerEx.h"
#include "readerEx.h"
#include "ErrorCode.h"
#include "UrlCode.h"

using namespace std;

enum ChannelState
{
    CHAN_INIT = 0,
    CHAN_DATATRAN,         /*数据传输*/
    CHAN_UNLOGIN,          /*未登陆*/
    CHAN_CLOSE,  		   /*通道关闭*/ 
    CHAN_MAXSLIDINGWINDOW, /*超过最大滑窗*/
    CHAN_EXCEPT,           /*通道异常*/
    CHAN_RESET,            /*重置*/
};

typedef enum
{
    CMPP_SUCCESS					= 0,	
    CMPP_ERROR_MSG_FORMAT			= 1,	
    CMPP_ERROR_COMMAND				= 2,	
    CMPP_ERROR_REPEAT_MSGID			= 3,	
    CMPP_ERROR_MSG_LENGTH			= 4,	
    CMPP_ERROR_FEECODE				= 5,	
    CMPP_ERROR_OVER_MAX_MSGLEN		= 6,	
    CMPP_ERROR_SERVICEID			= 7,	
    CMPP_ERROR_FLOW_CONCTROL		= 8,	
    CMPP_ERROR_SYS					= 9,	
}CMPP_DELIVER_ERROR_CODE;
	
enum ChannelType
{
    CHAN_SELF_SIGN_UCPAAS_TYPE = 0,//自签平台用户端口
    CHAN_GEGULAR_SIGN_NO_EXTEND_PORT_TYPE = 1,//固签无自扩展
    CHAN_GEGULAR_SIGN_EXTEND_PORT_TYPE = 2,//固签有自扩展
    CHAN_SELF_SIGN_CHAN_TYPE = 3,        //自签通道用户端口
};

#define HEARTBEATTIME 30000
#define STATISTICTIME 180000
#define CHECKPHONETIME 3600000
#define RECONNTIME 3000
#define MAXSLIDINGWINDOW 50

#define CMPPMO_MAX_NUM   10

class TUpdateChannelLoginStateReq:public TMsg
{
public:
	TUpdateChannelLoginStateReq()
	{
		m_iChannelId   = 0;
		m_uLoginStatus = 0;
		m_uOldLoginStatus = 0;
		m_strDesc 	   = "init";
		m_iWnd		   = 0;
		m_iMaxWnd = 0;
		m_NodeId = 0;
		m_bDelAll = false;
	}

	int 	m_iChannelId;
	UInt32 	m_uLoginStatus;
	UInt32  m_uOldLoginStatus;
	string 	m_strDesc;
	int 	m_iWnd;//当前滑窗数
	int     m_iMaxWnd;
	int     m_NodeId;
	bool 	m_bDelAll;
};

class TResetChannelSlideWindowMsg:public TMsg
{
public:
	TResetChannelSlideWindowMsg()
	{
		m_iChannelId = 0;
		m_iChannelWnd = 0;
	}
	
	int m_iChannelId;
	int m_iChannelWnd;
};

class smsDirectInfo
{
public:
    smsDirectInfo()
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
		m_uSendDate = 0;
		m_uChannelIdentify = 0;
		m_uLongSms = 0;
		m_uChannelCnt = 0;
		m_uBelong_sale = 0;
		m_uBelong_business= 0;
		m_uSendTimes = 0;
		m_uHttpmode = 0;
		m_uResendTimes = 0;
		m_strFailedChannel = "";
		m_strShowSignType_c2s = "";
		m_strUserExtendPort_c2s = "";
		m_strSignExtendPort_c2s = "";
		m_strChannelOverrate_access = "";
		m_uSignMoPortExpire = 60 * 60;

    }

	smsDirectInfo(const smsDirectInfo& other)
    {
		this->m_strClientId = other.m_strClientId;
		this->m_strUserName = other.m_strUserName;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strPhone = other.m_strPhone;
		this->m_strSign = other.m_strSign;
		this->m_strContent = other.m_strContent;
		this->m_uSmsType = other.m_uSmsType;
		this->m_uSmsFrom = other.m_uSmsFrom;
		this->m_uPayType = other.m_uPayType;
		this->m_uShowSignType = other.m_uShowSignType;
		this->m_strUcpaasPort = other.m_strUcpaasPort;
		this->m_strSignPort = other.m_strSignPort;
		this->m_strDisplayNum = other.m_strDisplayNum;
		this->m_uAccessId = other.m_uAccessId;
		this->m_uArea = other.m_uArea;
		this->m_iChannelId = other.m_iChannelId;
		this->m_lSaleFee = other.m_lSaleFee;
		this->m_fCostFee = other.m_fCostFee;
		this->m_uPhoneType = other.m_uPhoneType;
		this->m_strAccessids = other.m_strAccessids;
		this->m_uClientCnt = other.m_uClientCnt;
		this->m_uProcessTimes = other.m_uProcessTimes;
		this->m_uC2sId = other.m_uC2sId;
		this->m_strC2sTime = other.m_strC2sTime;
		this->m_uSendDate = other.m_uSendDate;
		this->m_uChannelIdentify = other.m_uChannelIdentify;
		this->m_strErrorDate = other.m_strErrorDate;
		this->m_uLongSms = other.m_uLongSms;
		this->m_uChannelCnt = other.m_uChannelCnt;
		this->m_strShowPhone = other.m_strShowPhone;
		this->m_strTemplateId = other.m_strTemplateId;
		this->m_strTemplateParam = other.m_strTemplateParam;
		this->m_strChannelTemplateId = other.m_strChannelTemplateId;
		this->m_uBelong_sale = other.m_uBelong_sale;
		this->m_uBelong_business = other.m_uBelong_business;
		this->m_strFee_Terminal_Id = other.m_strFee_Terminal_Id;
		this->m_strSessionId = other.m_strSessionId;
		this->m_uSendTimes = other.m_uSendTimes;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_strOriChannelId = other.m_strOriChannelId;
		this->m_uSignMoPortExpire = other.m_uSignMoPortExpire;
				
	}

	smsDirectInfo& operator=(const smsDirectInfo& other)
	{
		this->m_strClientId = other.m_strClientId;
		this->m_strUserName = other.m_strUserName;
		this->m_strSmsId = other.m_strSmsId;
		this->m_strPhone = other.m_strPhone;
		this->m_strSign = other.m_strSign;
		this->m_strContent = other.m_strContent;
		this->m_uSmsType = other.m_uSmsType;
		this->m_uSmsFrom = other.m_uSmsFrom;
		this->m_uPayType = other.m_uPayType;
		this->m_uShowSignType = other.m_uShowSignType;
		this->m_strUcpaasPort = other.m_strUcpaasPort;
		this->m_strSignPort = other.m_strSignPort;
		this->m_strDisplayNum = other.m_strDisplayNum;
		this->m_uAccessId = other.m_uAccessId;
		this->m_uArea = other.m_uArea;
		this->m_iChannelId = other.m_iChannelId;
		this->m_lSaleFee = other.m_lSaleFee;
		this->m_fCostFee = other.m_fCostFee;
		this->m_uPhoneType = other.m_uPhoneType;
		this->m_strAccessids = other.m_strAccessids;
		this->m_uClientCnt = other.m_uClientCnt;
		this->m_uProcessTimes = other.m_uProcessTimes;
		this->m_uC2sId = other.m_uC2sId;
		this->m_strC2sTime = other.m_strC2sTime;
		this->m_uSendDate = other.m_uSendDate;
		this->m_uChannelIdentify = other.m_uChannelIdentify;
		this->m_strErrorDate = other.m_strErrorDate;
		this->m_uLongSms = other.m_uLongSms;
		this->m_uChannelCnt = other.m_uChannelCnt;
		this->m_strShowPhone = other.m_strShowPhone;
		this->m_strTemplateId = other.m_strTemplateId;
		this->m_strTemplateParam = other.m_strTemplateParam;
		this->m_strChannelTemplateId = other.m_strChannelTemplateId;
		this->m_uBelong_sale = other.m_uBelong_sale;
		this->m_uBelong_business = other.m_uBelong_business;
		this->m_strFee_Terminal_Id = other.m_strFee_Terminal_Id;
		this->m_strSessionId = other.m_strSessionId;
		this->m_uSendTimes = other.m_uSendTimes;
		this->m_uHttpmode = other.m_uHttpmode;
		this->m_uResendTimes = other.m_uResendTimes;
		this->m_strFailedChannel = other.m_strFailedChannel;
		this->m_strShowSignType_c2s = other.m_strShowSignType_c2s;
		this->m_strUserExtendPort_c2s = other.m_strUserExtendPort_c2s;
		this->m_strSignExtendPort_c2s = other.m_strSignExtendPort_c2s;
		this->m_strChannelOverrate_access = other.m_strChannelOverrate_access;
		this->m_strOriChannelId = other.m_strOriChannelId;
		this->m_uSignMoPortExpire = other.m_uSignMoPortExpire;
		return *this;
	}

	string  m_strClientId;
	string  m_strUserName;
	string  m_strSmsId;
	string  m_strPhone;
	string  m_strSign;
	string  m_strContent;
    UInt32  m_uSmsType;
	UInt32  m_uSmsFrom;
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
	UInt32	m_uChannelIdentify;
	string  m_strErrorDate;

	////dirct use
	UInt32 m_uLongSms;
	UInt32 m_uChannelCnt;
	string m_strShowPhone;

    //模板短信
    std::string m_strTemplateId;
    std::string m_strTemplateParam;
    std::string m_strChannelTemplateId;
	UInt64 m_uBelong_sale;	
	UInt64 m_uBelong_business;
	string m_strFee_Terminal_Id;
	string m_strSessionId;

	UInt32 m_uSendTimes;
	UInt32 m_uHttpmode;
	UInt32 m_uResendTimes;//failed resend times
	string m_strFailedChannel;//failed resend channel
	string m_strShowSignType_c2s;
	string m_strUserExtendPort_c2s;
	string m_strSignExtendPort_c2s;
	string m_strChannelOverrate_access; 
	string m_strOriChannelId;
	UInt32 m_uSignMoPortExpire;
	
};


/*直连会话*/
class directSendSesssion 
{

public:
	directSendSesssion();
	~directSendSesssion();
public:	
	string 			  m_strSessionId;
	smsp::SmsContext* m_pSmsContext;
};

/*通道发送流控*/
class IChannelFlowCtr
{
public:
	IChannelFlowCtr()
	{
		iWindowSize = 0;
		uResetCount = 0;
		lLastRspTime = 0; 
		lLastReqTime = 0;
		uTotalReqCnt = 0;
		uTotalRspCnt = 0;
		uSubmitFailedCnt = 0;
		uSendFailedCnt = 0;
		uLastWndZeroTime = 0;
	};

public:
	int		iWindowSize;      /*当前滑窗大小*/
	UInt32  uResetCount;      /*重置次数*/
	Int64   lLastRspTime;     /*最后一次响应回来*/
	Int64   lLastReqTime;     /*最后一次请求时间*/
	Int64   lLastErrTime;     /*最后一次错误时间*/
	UInt64  uTotalReqCnt;     /*总的请求次数*/
	UInt64  uTotalRspCnt;     /*总的响应次数*/
	UInt64  uSubmitFailedCnt; /*失败错误计数*/
	UInt64  uSendFailedCnt;   /*发送失败计数*/
	UInt64  uLastWndZeroTime; /*最后一次滑窗为0的时间*/
	Int64   uSsthresh;        /*慢启动阀值*/
	UInt64  uSsErrorNum;      /*错误计数*/

};

typedef std::map<UInt32, directSendSesssion*>  channelSessionMap;

enum _loginState{
	UN_LOGIN = 0,
	LOGIN_SUCCESS,
	LOGIN_FAIL,
};

class IChannel
{
	#define ICHANNEL_SESSION_TIMEOUT_SECS  300
	#define ICHANNEL_SLIDER_RESET_TIME     60
	#define ICHANNEL_SLIDER_RESET_COUNT    3

public:
    IChannel();
    virtual ~IChannel();
public:
    virtual void init(models::Channel& chan);
    virtual UInt32 sendSms(smsDirectInfo &smsInfo );
    virtual void destroy();
    virtual void OnTimer();
	virtual void reConn();
	void   continueLoginFailed(UInt32& uFailedNum);
	void   continueNoSubmitResp(UInt32 uChannelId);
	void   updateLoginState(UInt32 oldState, UInt32 newState, string strDesc);	
	UInt32 GetChannelFlowCtr( int& flowRemain );		
    UInt32 parseMsgContent( string& content, vector<string>& contents, string& sign, bool isEnglishSms );
	UInt32 parseSplitContent(string& content, vector<string>& contents, string& sign, bool isIncludeChinese);
	UInt32 getRandom();
	Int32  FlowSendRequst();
	Int32  FlowSendResponse();

	void   UpdateChannelInfo(models::Channel& chan);
	void   UpdateContinueLoginFailedValue(UInt32 newValue);

protected:
	void channelStatusReport( UInt64 uSeq, bool bDestory = true );
	virtual bool ContentCoding(string& strSrc,UInt8 uCodeType,string& strDst );
	UInt32 saveSmsContent( smsDirectInfo *smsDirect, string& content, UInt32 total, UInt32 index, UInt32 seq, UInt32 msgTotal );
	UInt32 saveSplitContent( smsDirectInfo *smsDirect, string& content, UInt32 total, UInt32 index, UInt32 seq, UInt32 msgTotal, bool isIncludeChinese );
	void FlowSliderResetCheck();
	void ResetChannelSlideWindowReq(int Channelid);
	void InitFlowCtr();
	
	bool isAlive()
    {
        if((m_iState == CHAN_CLOSE) || (m_iState == CHAN_INIT) || (m_iState == CHAN_EXCEPT))
        {
            return false;
        }
        return true;
    }

public:
	CThread* 		   m_pProcessThread; /*处理线程*/
    models::Channel    m_ChannelInfo; //通道信息    
    CThreadWheelTimer* m_wheelTime;
	UInt32			   m_uChannelLoginState;
	UInt32 			   m_uNodeId;//此节点的线程id，同一种协议这个id唯一

	bool 			   m_bCloseChannel;//主动断开连接时设为true, 默认false

	CThreadWheelTimer* m_timerCloseChannel;
	bool 			   m_bDelAll;//true:关闭所有节点
	
protected:	
	
    UInt32             m_iState;	//通道状态
	UInt32             m_uRandom;	//长短信random值
    bool               m_bHasReconnTask;
    bool 	           m_bHasConn;    //是否会话存在
    SnManager          m_SnMng;
	UInt32             m_uContinueLoginFailedNum;
	IChannelFlowCtr    m_flowCtr; /*通道流控*/
    channelSessionMap  _packetDB;	
	UInt32			   m_uContinueLoginFailedValue;
};

#endif ///__ICHANNEL__
