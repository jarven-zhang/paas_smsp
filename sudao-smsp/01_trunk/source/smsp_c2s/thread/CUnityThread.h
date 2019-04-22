#ifndef __CUNITYTHREAD__
#define __CUNITYTHREAD__

#include "Thread.h"
#include "boost/regex.hpp"
#include "PhoneDao.h"
#include "MQWeightGroup.h"
#include "SnManager.h"
#include "CMPPSocketHandler.h"


using namespace boost;

extern string HTTPS_RESPONSE_MSG_0;
extern string HTTPS_RESPONSE_MSG_1;
extern string HTTPS_RESPONSE_MSG_6;
extern string HTTPS_RESPONSE_MSG_7;
extern string HTTPS_RESPONSE_MSG_15;
extern string HTTPS_RESPONSE_MSG_16;

extern int ACCESS_HTTPSERVER_RETURNCODE_ACCOUNTNotEXIST;
extern int ACCESS_HTTPSERVER_RETURNCODE_PARAMERR;
extern int ACCESS_HTTPSERVER_RETURNCODE_PHONEERR;
extern int ACCESS_HTTPSERVER_RETURNCODE_ERRORTEMPLATEID;
extern int ACCESS_HTTPSERVER_RETURNCODE_ERRORTEMPLATEPARAM;

#define INVISIBLE_NUMBER_STRING	"00000000000"

enum LoginAndSubmitResult
{
    TCP_RESULT_OK = 0,                // 正确
    TCP_RESULT_MSG_FORMAT_ERROR = 1,  // 消息结构错误
    TCP_RESULT_INVALID_SRC = 2,       // 非法源地址
    TCP_RESULT_INVALID_DEST = 3,      // 非法目标地址
    TCP_RESULT_AUTH_ERROR = 4,        // 认证错
    TCP_RESULT_VERSION = 5,           // 版本错误
    TCP_RESULT_PARAM_ERROR = 6,       // 参数错误
    TCP_RESULT_MSG_LENGTH_ERROR = 7,  // 消息长度错误
    TCP_RESULT_OTHER_ERROR = 8,       // 其他错误
    TCP_RESULT_FLOWCTRL = 9,          // 流控
    TCP_RESULT_ERROR_MAX = 10,
};

class CTcpLoginReqMsg: public TMsg
{
public:
    string m_strLinkId;
    UInt32 m_uSequenceNum;
    string m_strAccount;
    string m_strPwd;
    UInt32 m_uSmsFrom;
    string m_strIp;

    UInt8 m_loginMode; ///only smgp
    UInt32 m_uTimeStamp;     ///// only cmpp
    string m_strAuthSource;  ////  only cmpp

    //// under only sgip
    string m_strSgipPassWd;
    UInt32 m_uSequenceIdNode;
    UInt32 m_uSequenceIdTime;
    UInt32 m_uSequenceId;

};

class CTcpLoginRespMsg: public TMsg
{
public:
    string m_strLinkId;
    UInt32 m_uSequenceNum;
    UInt32 m_uResult;
    string m_strAccount;

    ////under sgip only
    UInt32 m_uSequenceIdNode;
    UInt32 m_uSequenceIdTime;
    UInt32 m_uSequenceId;
};

class CTcpDisConnectLinkReqMsg: public TMsg
{
public:
    string m_strAccount;
    string m_strLinkId;
};

class CTcpCloseLinkReqMsg: public TMsg
{
public:
    string m_strAccount;
    string m_strLinkId;
};

class CLinkedClientListRespMsg: public TMsg
{
public:
    list<string> m_list_Client;
};


class CTcpSubmitReqMsg: public TMsg
{
public:
    CTcpSubmitReqMsg()
    {
        m_uSequenceNum = 0;
        m_uSmsFrom = 0;
        m_uPkNumber = 0;
        m_uPkTotal = 0;
        m_uMsgLength = 0;
        m_uMsgFmt = 0;
        m_vecPhone.clear();
        m_uRandCode = 0;
        m_uPhoneNum = 0;

        m_uSequenceIdNode = 0;
        m_uSequenceIdTime = 0;
        m_uSequenceId = 0;

        m_uDestAddrTon = 0;
        m_uDestAddrNpi = 0;
        m_uSourceAddrTon = 0;
        m_uSourceAddrNpi = 0;
    }

    ~CTcpSubmitReqMsg()
    {
    }
    string m_strLinkId;
    UInt32 m_uSequenceNum;
    UInt32 m_uSmsFrom;
    string m_strAccount;
    UInt32 m_uPkNumber;
    UInt32 m_uPkTotal;
    UInt32 m_uMsgLength;
    string m_strMsgContent;
    UInt32 m_uMsgFmt;
    string m_strSrcId;
    ////string m_strPhone;
    vector<string> m_vecPhone;
    UInt32 m_uPhoneNum;
    UInt32 m_uRandCode;

    ////under sgip only
    UInt32 m_uSequenceIdNode;
    UInt32 m_uSequenceIdTime;
    UInt32 m_uSequenceId;

    ////under smpp only
    UInt8 m_uDestAddrTon;
    UInt8 m_uDestAddrNpi;
    UInt8 m_uSourceAddrTon;
    UInt8 m_uSourceAddrNpi;
};

class CTcpSubmitRespMsg: public TMsg
{
public:
    CTcpSubmitRespMsg()
    {
        m_uSequenceNum = 0;
        m_uResult = 0;
        m_uSubmitId = 0;
    }

    ~CTcpSubmitRespMsg()
    {
        ;
    }
    string m_strLinkId;
    UInt32 m_uSequenceNum;
    UInt8  m_uResult;
    UInt64 m_uSubmitId;

    ////under sgip only
    UInt32 m_uSequenceIdNode;
    UInt32 m_uSequenceIdTime;
    UInt32 m_uSequenceId;
};


class CTcpDeliverReqMsg: public TMsg
{
public:
    UInt64 m_uSubmitId;
    string m_strLinkId;
    UInt32 m_uResult;
    string m_strPhone;
    string m_strYZXErrCode;    ///added by weilu for tou chuan report

    ////under sgip only
    UInt32 m_uSequenceIdNode;
    UInt32 m_uSequenceIdTime;
    UInt32 m_uSequenceId;
};

class ShortSmsInfo
{
public:
    ShortSmsInfo()
    {
        m_uPkNumber  = 0;
        m_uPkTotal   = 0;
        m_uSubmitId = 0;
        m_uSequenceNum = 0;
        m_uState = 0;
        m_ulC2sId = 0;
    }

    string m_strShortContent;
    UInt8  m_uPkNumber;
    UInt8  m_uPkTotal;
    UInt64 m_uSubmitId;
    UInt32 m_uSequenceNum;
    string m_strLinkId;
    string m_strSgipNodeAndTimeAndId;////sgip only
    map<string, string> m_mapPhoneRefIDs;
    string m_strSmsid;
    UInt32 m_uState;
    string m_strErrCode;
    UInt64 m_ulC2sId;
};

class SMSSmsInfo
{
public:
    SMSSmsInfo()
    {
        m_lchargeTime = 0;
        m_uSmsFrom = 0;
        ////m_uSmsType = 6;
        m_uPayType = 0;
        m_uState   = 0;
        m_uNeedExtend = 0;
        ////m_uNeedSign = 0;
        m_uNeedSignExtend = 0;
        m_uNeedAudit  = 0;
        m_uShowSignType = 1;
        m_uiSignManualFlag = 0;
        /////m_uSequenceNum = 0;
        m_uPkNumber = 0;
        m_uPkTotal = 0;
        m_uRandCode = 0;
        m_uShortSmsCount = 0;
        m_pLongSmsTimer = NULL;
        m_pLongSmsRedisTimer = NULL;
        m_pLongSmsGetAllTimer = NULL;
        m_uRecvCount = 0;
        m_iErrorCode = 0;
        m_uSMSSessionId = 0;
        m_uSendHttpType = 0;
        m_uSmsNum = 1;
        m_uIdentify = 0;
        m_uChannleId = 0;
        m_strSmsType = "6";   /////default value
        m_uOperater = 0;
        m_uLongSms = 0;
        m_uChannelSendMode = 0;
        m_fCostFee = 0;
        m_uSaleFee = 0;
        m_uExtendSize = 0;
        m_uChannelType = 0;
        m_bInOverRateWhiteList = false;
        m_bOverRateSessionState = false;
        m_bHostIPSessionState = false;
        m_bOverRatePlanFlag = false;
        m_uSendNodeId = 0;
        m_uArea = 0;
        m_uProductType = 99; ////product_type default 99
        m_uProductCost = 0;
        m_uSubId = 0;
        m_uAgentId = 0;
        m_uOverRateCharge = 0;
        m_uChannelOperatorsType = 0;
        m_uIdentify = 0;
        m_uChannelIdentify = 0;
        m_pWheelTime = NULL;

        /* BEGIN: Added by fxh, 2016/10/15   PN:RabbitMQ */
        m_uSelectChannelNum = 0;
        /* END:   Added by fxh, 2016/10/15   PN:RabbitMQ */

        m_iRedisIndex = -1;
        m_uAgentType = 0;
        m_bIsNeedSendReport = false;
        m_uProcessTimes = 1;

        m_strHttpOauthUrl = "";
        m_strHttpOauthData = "";

        m_strTemplateId = "";
        m_strChannelTemplateId = "";
        m_strTemplateParam = "";
        m_strTemplateType = "-1";    //模板类型初值赋值为-1
        m_uProcessTimes = 0;
        m_uBelong_sale = 0;

        m_uDestAddrTon = 0;
        m_uDestAddrNpi = 0;
        m_uSourceAddrTon = 0;
        m_uSourceAddrNpi = 0;
        m_uAccountExtAttr = 0x0;

        m_uiLongSmsRecvCountRedis = 0;
        m_ulSequence = 0;
    }

    void Destory()
    {
        if (NULL != m_pWheelTime)
        {
            delete m_pWheelTime;
            m_pWheelTime = NULL;
        }
    }

    UInt64  m_lchargeTime;
    string  m_strPhone;
    string  m_strSign;
    string  m_strSmsId;
    UInt64  m_uSubmitId;
    string 	m_strSgipNodeAndTimeAndId;////sgip only
    string  m_strClientId;
    string  m_strSid;
    string  m_strUserName;
    string  m_strContent;
    string  m_strContentWithSign;
    string  m_strChargeFeeUrl;
    string  m_strDeliveryUrl;
    string  m_strErrorCode;		//入DB errorcode 描述
    string	m_strDate;
    string	m_strSubmit;		//应答描述
    string	m_strSubmitDate;
    string	m_strReport;
    string	m_strReportDate;
    string	m_strSrcPhone;
    string  m_strExtpendPort; ///// user extend port
    string	m_strUid;		//access httpserver需要。 批次号。
    string  m_strIsChina;
    string  m_strSignPort;	////sign port
    string  m_strYZXErrCode; //added by weilu for tou chuan report

    UInt32  m_uPkNumber;
    UInt32  m_uPkTotal;
    UInt32  m_uRandCode;

    UInt32  m_uShortSmsCount;
    /////UInt32  m_uSmsType;
    string m_strSmsType;
    UInt32  m_uSmsFrom;
    UInt32  m_uPayType;
    int  m_uState;		//入access的DB状态
    UInt8   m_uNeedAudit;
    ////UInt32  m_uNeedSign;
    UInt32  m_uNeedExtend;
    UInt32  m_uNeedSignExtend;
    UInt32  m_uShowSignType;
    UInt32  m_uiSignManualFlag;
    UInt32  m_uSmsNum;   ////短信计费条数 ，sms fee number

    UInt32	m_uIdentify;	//t_sms_account.identify
    UInt32	m_uChannelIdentify;
    std::vector<ShortSmsInfo> m_vecShortInfo;
    std::vector<std::string> m_Phonelist;
    std::vector<std::string> m_RepeatPhonelist;
    std::vector<std::string> m_ErrorPhoneList;

    //短信拼接成功之后，记录他是由那几个id的短信拼接起来的
    vector<string> m_vecLongMsgContainIDs;

    /////专门为拼接长短信而设定的
    CThreadWheelTimer *m_pLongSmsTimer;
    CThreadWheelTimer *m_pLongSmsRedisTimer;
    CThreadWheelTimer *m_pLongSmsGetAllTimer;

    UInt32 m_uRecvCount;

    ///////专门为https用的
    Int32 m_iErrorCode;
    string m_strError;

    ////专门为httpsend线程用的
    UInt64 m_uSMSSessionId;
    UInt32 m_uSendHttpType; /////SMS_TO_SEND 1,SMS_TO_AUDIT 2,SMS_TO_CHARGE 3
    string m_strSendUrl;    //// send to audit or send
    UInt32 m_uChannleId;

    UInt32 m_uOperater;
    UInt32 m_uHttpIp;  ////http channel channel ip
    string m_strHttpChannelUrl;////http channel url
    string m_strHttpChannelData; ////http channel data
    UInt32 m_uLongSms;   ///is long messege? 0 is short,1 is long
    string m_strCodeType;
    string m_strChannelName;
    UInt32 m_uChannelSendMode;   /////1 get,2 post,3 cmpp,4 smgp,5 sgip,6 smpp
    float  m_fCostFee;   /////成本价
    UInt64 m_uSaleFee;   ////销售价

    UInt32 m_uExtendSize;
    UInt32 m_uChannelType;
    string m_strUcpaasPort;
    UInt32 m_uArea;
    UInt32 m_uSendNodeId;

    UInt32 m_uProductType;
    UInt64 m_uProductCost;
    UInt64 m_uSubId;/////订单id

    UInt64 m_uAgentId;
    UInt32 m_uAgentType;
    UInt32 m_uOverRateCharge;
    UInt32 m_uChannelOperatorsType;
    string m_strChannelRemark;

    bool   m_bInOverRateWhiteList;
    bool m_bOverRateSessionState;
    bool m_bHostIPSessionState;
    bool m_bOverRatePlanFlag;  /////true 表示超频计费
    UInt64 m_uBelong_sale;

    CThreadWheelTimer *m_pWheelTime;

    /* BEGIN: Added by fxh, 2016/10/15   PN:RabbitMQ */
    UInt32 m_uSelectChannelNum;   /////select channel num
    /* END:   Added by fxh, 2016/10/15   PN:RabbitMQ */

    Int32 m_iRedisIndex;
    UInt32 m_uProcessTimes;  //how many times did the messege be dealed with?
    bool m_bIsNeedSendReport;
    UInt8 m_uFirstShortSmsResp;

    string m_strHttpOauthUrl;   ////http channel oauth url
    string m_strHttpOauthData; ////http channel oauth data

    string m_strTemplateId;           // 平台内部模板ID
    string m_strChannelTemplateId;    // 通道模板ID
    string m_strTemplateParam;        // 模板参数(用户传入)
    string m_strTemplateType;         // 模板类型

    UInt8 m_uDestAddrTon; //smpp专用
    UInt8 m_uDestAddrNpi;
    UInt8 m_uSourceAddrTon;
    UInt8 m_uSourceAddrNpi;
    UInt32 m_uAccountExtAttr;

    string m_strMergeLongKey;
    UInt32 m_uiLongSmsRecvCountRedis;
    UInt64 m_ulSequence;
};

class CHttpServerToUnityReqMsg: public TMsg
{
public:
    string m_strPhone;
    string m_strContent;
    string m_strAccount;
    string m_strPwdMd5;
    string m_strUid;
    string m_strExtendPort;
    string m_strIp;
    UInt32 m_uSmsFrom;
    UInt64 m_uSessionId;
    string m_strSmsType;

    //模板短信所需参数
    string m_strTemplateId;
    string m_strTemplateParam;
    string m_strTemplateType;
};

class CUnityToHttpServerRespMsg: public TMsg
{
public:
    string m_strSmsid;
    UInt64 m_uSessionId;
    vector<string> m_vecCorrectPhone;
    vector<string> m_vecFarmatErrorPhone;
    vector<string> m_vecParamErrorPhone;
    Int32 m_iErrorCode;  //// this is m_vecParamErrorPhone
    string m_strError;    //// this is m_vecParamErrorPhone
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

///store one long messege's all responses
struct LongSmsResp
{
    LongSmsResp()
    {
        m_bIsAllRespOK = true;
        m_uCount = 0;
    }
    bool m_bIsAllRespOK;	///is all short messege's response ok?
    UInt8 m_uFirstShortSmsResp;	////this long messege's first short messege's response
    UInt32 m_uCount;		///if one short messege coming,u_count ++
};

typedef struct SpeedCtrl
{
    UInt32 m_uiRecvCount;
    UInt64 m_ulCheckStart;
    set<string> m_setLongSmsKey;
} SpeedCtrl;

class LinkManager
{
public:
    LinkManager():m_iLinkState(LINK_INIT){};
    UInt32 m_iLinkState;
    string m_strAccount;
};

class CUnityThread: public CThread
{
    typedef std::map<string, SmsAccount> AccountMap;
    typedef std::map<std::string, ClientIdSignPort> ClientIdAndSignMap;
    typedef map<string, SMSSmsInfo *> LongSmsMap;
    typedef map<UInt64, SMSSmsInfo *> LongSmsRedisMap;
    typedef map<string, LongSmsResp *> LongSmsRespMap;
    typedef list<regex> RegList;
    typedef map<string, SpeedCtrl> SpeedCtrlMap;
    typedef SpeedCtrlMap::iterator SpeedCtrlMapIter;

public:
    CUnityThread(const char *name);
    virtual ~CUnityThread();
    bool Init(UInt64 SwitchNumber);
    UInt32 GetSessionMapSize();

private:
    void HandleMsg(TMsg *pMsg);

    void MainLoop();

    /**
    	process table t_sms_account update
    */
    void HandleAccountUpdateReq(TMsg *pMsg);
    void AccountConnectClose(SmsAccount &oldAccount);
    /**
    	process cmpp or smpp login req msg
    */
    void HandleTcpLoginReq(TMsg *pMsg);

    /**
    	process tcp login resp
    */
    void SendLoginResp(CTcpLoginReqMsg *pLogMsg , UInt32 uResult);

    /**
    	process account 5 failed locked
    */
    void AccountLock(SmsAccount &accountInfo, const char *accountLockReason);

    /**
    	process cmpp or smpp submit req msg
    */
    void HandleSubmitReq(TMsg *pMsg);

    /**
    	process cmpp/smspp submit resp

    */
    void ConstructResp(SMSSmsInfo *pSmsInfo, UInt8 uResult);

    void SendSubmitResp(SMSSmsInfo *pSmsInfo, UInt8 uResult);

    /**
    	process parse content from codetype
    */
    bool ContentCoding(string &strSrc, UInt8 uCodeType, string &strDst);

    /**
    	process smpp or cmpp insert access db
    */
    void InsertAccessDb(SMSSmsInfo *pSmsInfo);

    /**
    	return: true 里面有电话号码是要发送的,false 里面没有要发送的号码

    */
    bool CheckPhoneFormatAndBlackList(SMSSmsInfo *pSmsInfo, vector<string> &formatErrorVec, vector<string> &blackListVec);

    /**
    	return: true 检查成功可以下发，false检查失败
    */
    bool CheckSignExtendAndKeyWord(SMSSmsInfo *pSmsInfo);

    /**
    	return: true 检查成功，false检查失败
    */
    bool CheckSignExtend(SMSSmsInfo *pSmsInfo);

    /**
    	from content extract sign and content,showsigntype
    	0:ok
    	-6:缺少参数
    	-8:签名过长或者过短
    	-9:签名未报备
    */
    int ExtractSignAndContent(string &strContent, string &strOut, string &strSign, UInt32 &uChina, UInt32 &uShowSignType);

    /**
    	return: true表示获取签名端口成功，false获取签名端口失败
    */
    bool getSignPort(string &strClientId, string &strSign, string &strOut);


    bool getPortSign( string strClientId, string strSignPort, string &strSignOut );

    /**
    	return: true 表示检查成功，false表示检查失败
    */
    bool checkSignAndContent(const string &strSign, const string &strContent);
    /*
    	check msg param
    */
    bool CheckMsgParam(CTcpSubmitReqMsg *pReq, SMSSmsInfo *pSmsInfo, string &utf8Content, SmsAccount &AccountInfo);
    /**
    	return: true检查成功，false 检查失败
    */
    bool CheckProcess(SMSSmsInfo *pSmsInfo);

    /**
    	send failed report to cmpp or smpp or https
    */
    void SendFailedDeliver(SMSSmsInfo *pSmsInfo);

    /**
    	merge long sms
    	return: 还回NULL 表示没有要处理的长短信，还回非空 表示合并后的长短信消息
    */
    SMSSmsInfo *mergeLongSms(SMSSmsInfo *pSmsInfo);

    /**
    	process timeout msg
    */
    void HandleTimeOut(TMsg *pMsg);

    /**
    	process cmpp or smpp disconnect link msg
    */
    void HandleDisConnectLinkReq(TMsg *pMsg);

    /**
    	process httpserver send to msg
    */

    /**
    	process set redis accesssms:smsid_phone
    */
    void proSetInfoToRedis(SMSSmsInfo *pSmsInfo);

    //get MQ config
    void getMQconfig(UInt32 ioperator, UInt32 smstype, string &strExchange, string &strRoutingKey);

    /*
    	IOMQ link err, and g_pIOMQPublishThread.queuesize > m_uIOProduceMaxChcheSize.
    	then will update db and send report.
    */
    void UpdateRecord(SMSSmsInfo *pInfo);
    bool signalErrSmsPushRtAndDb(SMSSmsInfo *pSmsInfo, UInt32 i);
    bool signalErrSmsPushRtAndDb2(SMSSmsInfo *pSmsInfo, UInt32 i);

    bool updateMQWeightGroupInfo(const map<string, ComponentRefMq> &componentRefMqInfo);

    void HandleGetLinkedClientReq(TMsg *pMsg);
    void getSysPara(const map<string, string> &mapSysPara);
    bool getContentWithSign(SMSSmsInfo *pSmsInfo, string &strSign, UInt32 &uiChina);
    bool isSignExist(const string &strSrcContent, string &strOut);

    bool checkSpeed(const SmsAccount &smsAccount, SMSSmsInfo *pSmsInfo);
    void HandleClientForceExit(TMsg* pMsg);


private:
    bool mergeLong(CTcpSubmitReqMsg *pReq, SMSSmsInfo *pSmsInfo, const SmsAccount &accountInfo);
    bool mergeLongGetKey(SMSSmsInfo *pSmsInfo);
    bool mergeLongGetRedisCount(SMSSmsInfo *pSmsInfo);
    bool handleRedisRsp(TMsg *pMsg);
    bool mergeLongRedisRspCount(SMSSmsInfo *pSmsInfo, redisReply *pRedisReply);
    bool mergeLongSetSmsDetail(SMSSmsInfo *pSmsInfo);
    bool mergeLongSetTimeout(SMSSmsInfo *pSmsInfo);
    bool mergeLongTimeout(SMSSmsInfo *pSmsInfo, redisReply *pRedisReply);
    bool mergeLongRedisRspGetShorts(UInt64 ulSession, SMSSmsInfo *pSmsInfo, redisReply *pRedisReply);
    bool mergeLongGetAllMsg(SMSSmsInfo *pSmsInfo, UInt32 uiSessionId);
    bool mergeLongParseMsgDetail(SMSSmsInfo *pSmsInfo, string &strValKey);
    bool mergeLongSmsEx(SMSSmsInfo *pSmsInfo);
    bool mergeLongDelRedisCache(SMSSmsInfo *pSmsInfo);
    bool mergeLongReleaseResource(SMSSmsInfo *pSmsInfo);
    UInt32 mergeLongCheckMsgParam(CTcpSubmitReqMsg *pReq, SMSSmsInfo *pSmsInfo, const string &utf8Content, const SmsAccount &AccountInfo);
    bool mergeLongDbInsertProcess(SMSSmsInfo *pSmsInfo);
    void mergeLongSendFailedDeliver(SMSSmsInfo *pSmsInfo, const vector<string> &phoneList, const vector<ShortSmsInfo> &vecShortInfo);
    bool mergeLongMsgRealProc( SMSSmsInfo *pSmsInfo );
    void mergeLongInsertAccessDb(SMSSmsInfo *pSmsInfo, const vector<string> &phonelist);


private:
    AccountMap m_AccountMap;
    SnManager m_SnManager;
    PhoneDao m_honeDao;
    UInt64 m_uSwitchNumber;
    ClientIdAndSignMap m_mapClientSignPort;

    LongSmsMap m_mapLongSmsMng;
    LongSmsRedisMap m_mapLongSmsMngRedis;
    CThreadWheelTimer *m_UnlockTimer;

    CThreadWheelTimer *m_LinkNumTimer;//for account's LinkNum to db

    std::map<std::string, models::UserGw> m_userGwMap; //KEY: userid_smstype

    UInt32 m_uIsSendForStat;    //是否需要发送数据供统计模块使用

    //map<UInt32, string> m_mapMQExchange;		//smstype<==>Exchange     type=0,4,5,6
    //map<UInt32, string> m_mapMQRoutingKey;		//smstype<==>RoutingKey     type=0,4,5,6

    //MQ config
    UInt32 m_uSelectChannelMaxNum;


    UInt32 m_uIOProduceMaxChcheSize;	//IF IOMQ error, g_pMQIOProducerThread will cache m_uIOProduceMaxChcheSize at most

    string m_strYDyingxiaoExchange ;
    string m_strLTyingxiaoExchange;
    string m_strDXyingxiaoExchange;
    string m_strYDhangyeExchange;
    string m_strLThangyeExchange;
    string m_strDXhangyeExchange;
    string m_strYDyingxiaoRoutingKey;
    string m_strLTyingxiaoRoutingKey;
    string m_strDXyingxiaoRoutingKey;
    string m_strYDhangyeRoutingKey;
    string m_strLThangyeRoutingKey;
    string m_strDXhangyeRoutingKey;

    UInt32 m_uC2sId;                    // c2s组件ID
    LongSmsRespMap m_mapLongSmsResp;    // store all long messege's all responses

    int m_arrErrorCodeMap[SMS_FROM_ACCESS_MAX][TCP_RESULT_ERROR_MAX];  //保存内部错误码和三大运营商标准错误码对应关系（例：a[x][y],x表示运营商编号，y表示内部错误码编号）
    int m_arrLoginErrorCodeMap[SMS_FROM_ACCESS_MAX][TCP_RESULT_ERROR_MAX];//保存登陆错误码对应关系
    ///smsp_v5.0
    map<UInt32, MiddleWareConfig> m_middleWareConfigInfo;
    ComponentConfig m_componentConfigInfo;
    map<string, ListenPort> m_listenPortInfo;
    map<UInt64, MqConfig> m_mqConfigInfo;
    map<string, ComponentRefMq> m_componentRefMqInfo;
    set<UInt64> m_mqId;		///all of this c2s's mq

    map<string, string> m_mapSystemErrorCode;

    mq_weightgroups_map_t m_mapMQWeightGroups;

    UInt32 m_uiLongSmsMergeTimeOut;

    SpeedCtrlMap m_mapSpeedCtrl;
    std::map<string, LinkManager*> m_LinkMap;
    std::map<string, int> m_clientForceExitMap;
};

#endif ////__CUNITYTHREAD__
