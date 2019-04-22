#ifndef __SYS_INFO_H__
#define __SYS_INFO_H__
#include <string>
#include <iostream>
#include <set>
#include "platform.h"
#include "Thread.h"
#include "Channel.h"
#include "HttpSender.h"
using namespace std;

class ShortSmsInfo
{
public:
    ShortSmsInfo()
    {
        m_uPkNumber	 = 0;
        m_uPkTotal   = 0;
        m_uSubmitId = 0;
        m_uSequenceNum = 0;
    }

    string m_strShortContent;
    UInt8  m_uPkNumber;
    UInt8  m_uPkTotal;
    UInt64 m_uSubmitId;
    UInt32 m_uSequenceNum;
    string m_strLinkId;
    string m_strSgipNodeAndTimeAndId;////sgip only
    map<string, string> m_mapPhoneRefIDs;
};

class SMSSmsInfo

{

public:
    SMSSmsInfo()
    {
        m_lchargeTime		= 0;
        m_uSmsFrom			= 0;

        ////m_uSmsType = 6;
        m_uPayType			= 0;
        m_uState			= 0;
        m_uNeedExtend		= 0;

        ////m_uNeedSign = 0;
        m_uNeedSignExtend	= 0;
        m_uNeedAudit		= 0;
        m_uShowSignType 	= 0;

        /////m_uSequenceNum = 0;
        m_uPkNumber 		= 0;
        m_uPkTotal			= 0;
        m_uRandCode 		= 0;
        m_uShortSmsCount	= 0;
        m_pLongSmsTimer 	= NULL;
        m_uRecvCount		= 0;
        m_iErrorCode		= 0;
        m_uSMSSessionId 	= 0;
        m_uSendHttpType 	= 0;
        m_uSmsNum			= 1;
        m_uIdentify 		= 0;
        m_uChannleId		= 0;
        m_uChannelExValue   = CHANNEL_CHINESE_SUPORT_FALG | CHANNEL_ENGLISH_SUPORT_FALG;
        m_strSmsType		= "5";					/////default value
        m_uOperater 		= 0;
        m_uPhoneOperator	= 0;
        m_uLongSms			= 0;
        m_uChannelSendMode	= 0;
        m_fCostFee			= 0;
        m_uSaleFee			= 0;
        m_uExtendSize		= 0;
        m_uChannelType		= 0;
        m_bInOverRateWhiteList = false;
        m_bOverRateSessionState = false;
        m_bHostIPSessionState = false;
        m_bOverRatePlanFlag = false;
        m_uSendNodeId		= 0;
        m_uArea 			= 0;
        m_uProductType		= 99;					////product_type default 99
        m_uProductCost		= 0;
        m_strSubId			= "0";
        m_uAgentId			= 0;
        m_uOverRateCharge	= 0;
        m_uChannelOperatorsType = 0;
        m_uIdentify 		= 0;
        m_uChannelIdentify	= 0;
        m_pHttpSender		= NULL;
        m_pWheelTime		= NULL;

        /* BEGIN: Added by fxh, 2016/10/15	 PN:RabbitMQ */
        m_uSelectChannelNum = 0;

        /* END:   Added by fxh, 2016/10/15	 PN:RabbitMQ */
        m_iRedisIndex		= -1;
        m_uAgentType		= 0;
        m_uChannelMQID		= 0;
        m_bIsNeedSendReport = false;
        m_uProcessTimes 	= 0;
        m_uFirstShortSmsResp = 0;

        m_strHttpOauthUrl	= "";
        m_strHttpOauthData	= "";

        m_strTemplateId 	= "";
        m_strChannelTemplateId = "";
        m_strTemplateParam	= "";
        m_strTemplateType	= "-1"; 				// 模板类型初值赋值为-1
        m_bReadyAudit		= false;

        m_uOverTime 		= 0;
        m_iOverCount		= 0;
        m_uPorcessTime		= 0;
        m_uClientAscription = 0;
        m_uBelong_sale		= 0;
        m_uBelong_business	= 0;
        m_uFreeChannelKeyword = 0;
        m_uSendType 		= 0;
        m_bIsKeyWordOverRate = false;				//ture:key word over rate  false: message type over rate
        m_uChargeFlage		= false;

        m_bMatchAutoWhiteTemplate = false;
        m_strTestChannelId = "";

        m_bIncludeChinese = false;
        m_strKeyWordOverRateKey = "";
        m_strChannelOverRateKey = "";
        m_bSelectChannelAgain = false;
        m_strAuditContent = "";
        m_uAuditState = 0;
        m_strAuditDate = "";
        m_strExtraErrorDesc = "";
        m_ucHttpProtocolType = INVALID_UINT8;

        m_iFailedResendTimes = 0;
        m_strFailedResendChannels = "";
        m_strSendUrl = "";

        m_uGroupsendLimUserflag = 0;
        m_uGroupsendLimNum = 200;
        m_uGroupsendLimTime = 30;
        m_bWaitAuditByGroupsend = false;

        m_bCheckChannelOverRate = false;
        m_uOriginChannleId		= 0;

        m_strChannelOverrate_access = "";

        m_strContentWithSign = "";
        m_uAccountExtAttr = 0;
    }

    void Destory()
    {
        if (NULL != m_pWheelTime)
        {
            delete m_pWheelTime;
            m_pWheelTime		= NULL;
        }

        if (NULL != m_pHttpSender)
        {
            m_pHttpSender->Destroy();
            delete m_pHttpSender;
            m_pHttpSender		= NULL;
        }
    }

    UInt64			m_lchargeTime;
    string			m_strPhone;
    string			m_strSmsId;
    UInt64			m_uSubmitId;
    string			m_strSgipNodeAndTimeAndId;		////sgip only
    string			m_strClientId;
    string			m_strSid;
    string			m_strUserName;

    string			m_strSign;						// 原签名
    string			m_strSignSimple;				// 转换为简体中文的签名, 后续所有检查都用简体中文

    string			m_strContent;				    //原短信内容
    string          m_strContentWithSign;
    string			m_strContentSimple; 			//转换为简体中文的短信内容, 后续所有检查都用简体中文

    string			m_strChargeFeeUrl;
    string			m_strDeliveryUrl;
    string			m_strErrorCode; 				//入DB errorcode 描述
    string 			m_strInnerErrorcode;
    string			m_strDate;
    string			m_strSubmit;					//应答描述
    string			m_strSubmitDate;
    string			m_strReport;
    string			m_strReportDate;
    string			m_strSrcPhone;
    string			m_strExtpendPort;				// user extend port
    string			m_strUid;						//access httpserver需要批次号
    string			m_strIsChina;
    string			m_strSignPort;					////sign port
    string			m_strYZXErrCode;				//added by weilu for tou chuan report

    UInt32			m_uPkNumber;
    UInt32			m_uPkTotal;
    UInt32			m_uRandCode;

    UInt32			m_uShortSmsCount;

    /////UInt32  m_uSmsType;
    string			m_strSmsType;
    UInt32			m_uSmsFrom;
    UInt32			m_uPayType;
    UInt32 			m_uState;						//入access的DB状态
    UInt8			m_uNeedAudit;

    bool			m_bReadyAudit;

    ////UInt32	m_uNeedSign;
    UInt32			m_uNeedExtend;
    UInt32			m_uNeedSignExtend;
    UInt32			m_uShowSignType;
    UInt32			m_uSmsNum;						////短信计费条数 ，sms fee number

    UInt32			m_uIdentify;					//t_sms_account.identify
    UInt32			m_uChannelIdentify;

    std::vector <ShortSmsInfo> m_vecShortInfo;
    std::vector <std::string> m_Phonelist;
    std::map<std::string, std::string> m_mapPhoneSmsId;
    std::vector <std::string> m_RepeatPhonelist;

    //短信拼接成功之后，记录他是由那几个id的短信拼接起来的
    vector <string> m_vecLongMsgContainIDs;

    /////专门为拼接长短信而设定的
    CThreadWheelTimer *m_pLongSmsTimer;
    UInt32			m_uRecvCount;

    ///////专门为https用的
    Int32			m_iErrorCode;
    string			m_strError;
    string			m_strUserResp;

    ////专门为httpsend线程用的
    UInt64			m_uSMSSessionId;
    UInt32			m_uSendHttpType;				/////SMS_TO_SEND 1,SMS_TO_AUDIT 2,SMS_TO_CHARGE 3
    string			m_strSendUrl;					//// send to audit or send
    UInt32			m_uChannleId;
    UInt64			m_uChannelMQID;
    UInt64          m_uChannelExValue;

    UInt32			m_uOperater;
    UInt32			m_uPhoneOperator;				//号码所属运营商
    UInt32			m_uHttpIp;						////http channel channel ip
    string			m_strHttpChannelUrl;			////http channel url
    string			m_strHttpChannelData;			////http channel data
    UInt32			m_uLongSms;
    string			m_strCodeType;
    string			m_strChannelName;
    UInt32			m_uChannelSendMode; 			/////1 get,2 post,3 cmpp,4 smgp,5 sgip,6 smpp
    float			m_fCostFee; 					/////成本价
    double			m_uSaleFee; 					////销售价

    UInt32			m_uExtendSize;
    UInt32			m_uChannelType;
    string			m_strUcpaasPort;
    UInt32			m_uArea;						//地区id(省)
    UInt32			m_uSendNodeId;

    UInt32			m_uProductType;
    double			m_uProductCost;
    string			m_strSubId; 					/////订单id

    UInt64			m_uAgentId;
    UInt32			m_uAgentType;
    UInt32			m_uOverRateCharge;
    UInt32			m_uChannelOperatorsType;
    string			m_strChannelRemark;

    bool			m_bInOverRateWhiteList;
    bool			m_bOverRateSessionState;
    bool			m_bHostIPSessionState;
    bool			m_bOverRatePlanFlag;			/////true 表示超频计费

    http::HttpSender *m_pHttpSender;
    CThreadWheelTimer *m_pWheelTime;

    /* BEGIN: Added by fxh, 2016/10/15	 PN:RabbitMQ */
    UInt32			m_uSelectChannelNum;			/////select channel num

    /* END:   Added by fxh, 2016/10/15	 PN:RabbitMQ */
    Int32			m_iRedisIndex;

    UInt32			m_uProcessTimes;				//how many times did the messege be dealed with?
    bool			m_bIsNeedSendReport;
    UInt8			m_uFirstShortSmsResp;

    //add by fangjinxiong 20170105.
    string			m_strCSid;						//C2S id
    string			m_strCSdate;					//C2S date

    UInt32			m_uAuditFlag;

    string			m_strHttpOauthUrl;				////http channel oauth url
    string			m_strHttpOauthData; 			////http channel oauth data

    string			m_strTemplateId;				//平台内部模板ID
    string			m_strChannelTemplateId;         //通道模板ID
    string			m_strTemplateParam; 			//模板参数(用户传入)
    string			m_strTemplateType;				//模板类型

    /////for reback
    string			m_strIds;
    UInt64			m_uOverTime;
    int 			m_iOverCount;
    UInt32			m_uPorcessTime;
    UInt32			m_uClientAscription;			///add by weilu for cloud charge
    UInt64			m_uBelong_sale;
    UInt64			m_uBelong_business;

    set<string> 	m_phoneBlackListInfo;				//eg: value:0:3 the phone is in sysblacklist ,value:2154 the phone is in channel 2154 blacklist
    UInt32			m_uFreeChannelKeyword;			//0:check channel keyword, 1:not check channel keyword;
    UInt32			m_uSendType;					//0:hangye 1:yingxiao
    bool			m_bIsKeyWordOverRate;
    bool			m_uChargeFlage;

    bool			m_bMatchGlobalNoSignAutoTemplate;		// 是否匹配上的是全局空签名智能模板
    bool			m_bMatchAutoWhiteTemplate;		//智能模板匹配是否通过，true:是

    vector <string> m_vectorContentAfterCheckAutoTemplate;  //智能模板匹配过剩下的短信内容，可能被砍成几段
    string          m_strTestChannelId;
    bool 			m_bIncludeChinese;			//内容或签名是否包含中文
    string          m_strKeyWordOverRateKey;
    string          m_strChannelOverRateKey;
    bool            m_bSelectChannelAgain;
    string          m_strAuditId;
    string          m_strAuditContent; //client|sign|content|smstype  MD5
    UInt32          m_uAuditState;//??????????
    string          m_strAuditDate;
    string          m_strExtraErrorDesc;     // maybe about error description we need more
    UInt8           m_ucHttpProtocolType;

    UInt8           m_uTimerSendSubmittype;
    string          m_strTaskUuid;

    int 			m_iFailedResendTimes;
    string			m_strFailedResendChannels;
    set<UInt32> 	m_FailedResendChannelsSet;
    UInt32          m_uGroupsendLimTime;
    UInt32          m_uGroupsendLimUserflag;
    UInt32          m_uGroupsendLimNum;
    bool            m_bWaitAuditByGroupsend;           // 送到审核是普通送审，还是群发限制解除后送的
    bool			m_bCheckChannelOverRate;
    UInt32			m_uOriginChannleId;
    string 			m_strChannelOverrate_access;
    string			m_strIsReback;
    UInt32 			m_uAccountExtAttr;

};



class CUnityToHttpSendMsg: public TMsg
{
public:
    CUnityToHttpSendMsg()
    {
        m_pSmsInfo = NULL;
    }
    SMSSmsInfo *m_pSmsInfo;
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
        m_uChannelCount = 0;
        m_uSmsfrom = 0;
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

    UInt32 m_uSmsfrom;

    Int32 m_iRedisIndex;
    UInt32 m_uChannelCount;

    CThreadWheelTimer *m_pRedisRespWaitTimer;
};

class TReportReceiverMoMsg: public TMsg
{
public:
    TReportReceiverMoMsg()
    {
        m_uChannelId = 0;
        m_uSmsfrom = 0;
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
    UInt32 m_uSmsfrom;
};

#endif  /*__SYS_INFO_H__*/

