#ifndef C_HTTPSERVER_H_
#define C_HTTPSERVER_H_

#include <list>
#include <map>

#include "Thread.h"
#include "AccessHttpSocketHandler.h"
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
#include "PhoneDao.h"
#include "SmsAccount.h"
#include "AgentInfo.h"
#include "SignExtnoGw.h"
#include "SmsTemplate.h"
#include "SMSSmsInfo.h"
#include "json/json.h"


using namespace models;
using namespace std;

#define MAX_MOBILE_COUNTS										100
#define RESP_CODE_JD_INVALID_PARAMS								"-1"		//参数为空。信息、电话号码等有空指针，登陆失败
#define RESP_CODE_JD_MOBILE_COUNTS_OVERSIZE						"-2"		//电话号码个数超过100
#define RESP_CODE_JD_MEMORY_ALLOC_FAILED						"-10"		//申请缓存空间失败
#define RESP_CODE_JD_ILLEGAL_MOBILE								"-11"		//电话号码中有非数字字符
//#define RESP_CODE_JD_TIMEOUT									"-12"		//有异常电话号码
#define RESP_CODE_JD_MOBILE_COUNTS_MATCHING_ERR					"-13"		//电话号码个数与实际个数不相等
#define RESP_CODE_JD_MOBILE_COUNTS_OVERSIZE2					"-14"		//实际号码个数超过100
#define RESP_CODE_JD_SND_SMS_TIMEOUT							"-101"		//发送消息等待超时
#define RESP_CODE_JD_SND_RECV_SMS_ERR							"-102"		//发送或接收消息失败
#define RESP_CODE_JD_RECV_SMS_TIMEOUT							"-103"		//接收消息超时
#define RESP_CODE_JD_GENERAL_ERR								"-200"		//其他错误
#define RESP_CODE_JD_INTERNAL_ERR 								"-999"	   	//服务器内部错误
#define RESP_CODE_JD_AUTHENTICATION_ERR							"-10001"	//用户登陆不成功(帐号密码错误)
#define RESP_CODE_JD_FORMAT_ERR									"-10002"	//提交格式不正确
#define RESP_CODE_JD_INSUFFICIENT_BALANCE						"-10003"	//用户余额不足
#define RESP_CODE_JD_INVALID_MOBILE								"-10004"	//手机号码不正确
#define RESP_CODE_JD_INVALID_PREPAY_ACCOUNT						"-10005"	//计费用户帐号错误
#define RESP_CODE_JD_INVALID_PREPAY_PASSWD						"-10006"	//计费用户密码错
#define RESP_CODE_JD_EXPIRED_ACCOUNT							"-10007"	//账号已经被停用
#define RESP_CODE_JD_ACCOUNT_UNSUPPORT_FEATURE					"-10008"	//账号类型不支持该功能
#define RESP_CODE_JD_GENERAL_ERR2								"-10009"	//其它错误
#define RESP_CODE_JD_INVALID_ENTERPRISE_CODE					"-10010"	//企业代码不正确
#define RESP_CODE_JD_MSG_OVERSIZE								"-10011"	//信息内容超长
#define RESP_CODE_JD_UNICOM_UNSUPPORT							"-10012"	//不能发送联通号码
#define RESP_CODE_JD_PERMISSION_DENY							"-10013"	//操作员权限不够
#define RESP_CODE_JD_INVALID_PRICE_CODE							"-10014"	//费率代码不正确
#define RESP_CODE_JD_SERVER_BUSY								"-10015"	//服务器繁忙
#define RESP_CODE_JD_ENTERPRISE_PERMISSION_DENY					"-10016"	//企业权限不够
#define RESP_CODE_JD_DISABLE_PERIOD								"-10017"	//此时间段不允许发送
#define RESP_CODE_JD_INVALID_AGENT								"-10018"	//经销商用户名或密码错
#define RESP_CODE_JD_INVALID_MOBILELIST							"-10019"	//手机列表或规则错误
#define RESP_CODE_JD_PERMISSION_DENY_ENABLE_DISABLE_ACCOUNT		"-10021"	//没有开停户权限
#define RESP_CODE_JD_PERMISSION_DENY_SWITCH_ACCOUNT_TYPE		"-10022"	//没有转换用户类型的权限
//#define RESP_CODE_JD_INVALID_MOBILE								"-10023"	//没有修改用户所属经销商的权限
//#define RESP_CODE_JD_INVALID_MOBILE						-10024	//经销商用户名或密码错
//#define RESP_CODE_JD_INVALID_MOBILE						-10025	//操作员登陆名或密码错误
//#define RESP_CODE_JD_INVALID_MOBILE						-10026	//操作员所充值的用户不存在
//#define RESP_CODE_JD_INVALID_MOBILE						-10027	//操作员没有充值商务版的权限
//#define RESP_CODE_JD_INVALID_MOBILE						-10028	//该用户没有转正不能充值
//#define RESP_CODE_JD_INVALID_MOBILE						-10029	//此用户没有权限从此通道发送信息(用户没有绑定该性质的通道，比如：用户发了小灵通的号码)
#define RESP_CODE_JD_CHINAMOBILE_UNSUPPORT 						"-10030"	//不能发送移动号码
#define RESP_CODE_JD_INVALID_MOBILE2							"-10031"	//手机号码(段)非法
//#define RESP_CODE_JD_INVALID_MOBILE-10032	//用户使用的费率代码错误
#define RESP_CODE_JD_ILLEGAL_KEY_WORD							"-10033"	//非法关键词


#define JD_SMS_FIELD_USERID				"userId"
#define JD_SMS_FIELD_PASSWD				"password"
#define JD_SMS_FIELD_MOBILENUMS			"pszMobis"
#define JD_SMS_FIELD_MSG				"pszMsg"
#define JD_SMS_FIELD_MOBILECOUNT		"iMobiCount"
#define JD_SMS_FIELD_SUBPORT			"pszSubPort"

#define JD_SENDSMS_RESP_MSG			"<?xml version=\"1.0\" encoding=\"utf-8\"?>"	\
									"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"	\
									  "<soap:Body>"		\
									    "<MongateCsSpSendSmsNewResponse xmlns=\"http://tempuri.org/\">"		\
									     "<MongateCsSpSendSmsNewResult>%s</MongateCsSpSendSmsNewResult>"	\
									    "</MongateCsSpSendSmsNewResponse>"		\
									  "</soap:Body>"		\
									"</soap:Envelope>"

class THTTPRequest: public TMsg
{
public:
    THTTPRequest()
    {
        m_ucHttpProtocolType = INVALID_UINT8;
    }

public:
    string m_strRequest;
    AccessHttpSocketHandler *m_socketHandle;
    UInt8 m_ucHttpProtocolType;
};

enum MLERROR
{
    MLERR 									= 1,
    MLERR_MISSING_USERNAME 					= 2,
    MLERR_MISSING_PASSWORD					= 3,
    MLERR_MISSING_APIKEY					= 4,
    MLERR_MISSING_RECIPIENT					= 5,//phone empty
    MLERR_MISSING_MESSAGE_CONTENT			= 6,
    MLERR_ACCOUNTBLOCK						= 7,
    MLERR_UNRECOGNIZED_ENCODING				= 8,
    MLERR_APIKEY_ORPASSWORD_ERR				= 9,
    MLERR_UNANTHORIZED_IP					= 10,
    MLERR_ACCOUNT_BALANCE_INSUFFICIENT		= 11,
    MLERR_THROUGHPUT_RATE_EXCEEDED,
    MLERR_INVALID_MD5_PASSWORD_LENGTH,
    MLERR_SMSTYPE_EMPTY,
    MLERR_ACCOUNTFROZEN,
    MLERR_ACCOUNTCANCLE,
    MLERR_PROTOCAL_ERR						= 17,
    MLERR_CONTENT_OVER_LONG,
    MLERR_MISSING_SIGN,
    MLERR_SIGN_SHORT,
    MLERR_SIGN_LONG,
    MLERR_SIGN_UNKNOWN,
    MLERR_PHONE_TOO_MANY,
    MLERR_PHONE_FORMATE_ERR,
    MLERR_PHONE_FOREIGN_NOT,
    MLERR_PHONE_BLACK,
    MLERR_PHONE_REPEAT,
};

const UInt32 TimerSendSubmitType_SubClient = 0;
const UInt32 TimerSendSubmitType_Agent = 1;


class TAcessHTTPPhoneState
{
public:
    string m_strPhone;
    string m_strSmsid;
};

class CHttpServiceSession
{
public:
    CHttpServiceSession()
    {
        m_webHandler = NULL;
        m_wheelTimer = NULL;
        m_uFeeNum = 0;
        m_uSmsfrom = 0;
        m_strUid = "";
        m_ucHttpProtocolType = INVALID_UINT8;
    }
    AccessHttpSocketHandler *m_webHandler;
    string m_strGustIP;
    CThreadWheelTimer *m_wheelTimer;
    UInt32 m_uFeeNum;

    UInt32 m_uSmsfrom;
    string m_strVer;
    string m_strUid;
    UInt8 m_ucHttpProtocolType;

    bool m_bTimerSend;
};


class  CHttpServiceThread: public CThread
{
    typedef std::map<UInt64, CHttpServiceSession *> SessionMap;
    typedef std::map<std::string, models::SmsTemplate> SmsTemplateMap;
    typedef std::map<std::string, SmsAccount> AccountMap;
public:
    CHttpServiceThread(const char *name);
    virtual ~CHttpServiceThread();
    bool Init(const std::string ip, unsigned int port);
    UInt32 GetSessionMapSize();

private:
    void MainLoop();
    InternalService *m_pInternalService;
    virtual void HandleMsg(TMsg *pMsg);
    void HandleHTTPServiceAcceptSocketMsg(TMsg *pMsg);
    void HandleHTTPServerReqMsg(TMsg *pMsg);

    void HandleHTTPServerReqMsg_2(TMsg *pMsg);
    void HandleHTTPServerReqMsg_JD(TMsg *pMsg);

    void HandleSMServiceReDirectReqMsg(TMsg *pMsg);

    void HandleHTTPServiceReturnOverMsg(TMsg *pMsg);
    std::string &trim(std::string &s);

    void HandleHTTPResponse(UInt64 ullSeq, string &response);

    void UpdateBySysPara(const std::map<std::string, std::string> &mapSysPara);
    /**
        process unity send to response msg
    */
    void HandleUnityRespMsg(TMsg *pMsg);

    /**
     * Valid related params of timer send request, and do some prehandle.
     */
    bool validAndPreHandleTimerSendReq(const std::string &strAccount,
                                       const Json::Value &root,
                                       UInt32 &uTimerSendSubmittype,
                                       std::string &strTimerSendTime,
                                       std::string &strPhone,
                                       int &iRespCode);

    /**
     *
     */
    void packetNormalJsonReply(TMsg *pMsg,
                               const CHttpServiceSession *const pSession,
                               const SessionMap::iterator iter,
                               Json::Value &Jsondata);

    /**
     *
     */
    void packetTimerSendJsonReply(TMsg *pMsg,
                                  const CHttpServiceSession *const pSession,
                                  const SessionMap::iterator iter,
                                  Json::Value &Jsondata);


    /**
        process https execpt
        bNormal:true is http1 or template,false is http2
    */
    void proHttpsExecpt(int uErrorCode, UInt64 uSessionId, bool bNormal = true);
    void proHttpsExecpt_JD(const char *jd_resp_code, int uErrorCode, UInt64 uSessionId);
    void proHttpsExecpt_ML(int uErrorCode, UInt64 uSessionId, bool bNormal = true);
    int transferToMLErr(int err);
    void HandleHTTPServerReqMsg_ML(TMsg *pMsg);

    void HandleHTTPServerTemplateReqMsg(TMsg *pMsg);

    void InsertAccessDb(SMSSmsInfo smsInfo);

    void HandleTimerSendPhoneList(TMsg *pMsg,
                                  const SessionMap::iterator iter,
                                  const vector<string> &vecPhoneList,
                                  Json::Value &Jsondata,
                                  int iErrCode);

    void HandlePhoneList(TMsg *pMsg,
                         const SessionMap::iterator iter,
                         const vector<string> &vecPhoneList,
                         Json::Value &Jsondata,
                         int iErrCode);


private:
    InternalServerSocket 		*m_pServerSocekt;
    SessionMap 					m_SessionMap;
    SnManager 					*m_pSnManager;

    SmsTemplateMap 				m_mapSmsTemplate;
    AccountMap 					m_mapAccount;
    std::map<UInt64, AgentInfo> m_AgentInfoMap;
    map<string, string> 		m_mapSystemErrorCode;
    map<int, string> 			m_mapHttpErrCodeDes;

    UInt32 						m_uTimersendReqMinInterval;
};

#endif  ////C_HTTPSERVER_H_

