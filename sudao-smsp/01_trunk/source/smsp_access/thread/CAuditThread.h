#ifndef SMS_ACCESS_THREAD_CAUDITTHREAD_H
#define SMS_ACCESS_THREAD_CAUDITTHREAD_H

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "SnManager.h"
#include "SmsAccount.h"
#include "Comm.h"
#include "PhoneDao.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "SMSSmsInfo.h"
#include "PhoneSection.h"
#include "json/json.h"
#include "Fmt.h"
#include "KeyWordCheck.h"
#include "base64.h"
#include "HttpParams.h"
#include "enum.h"
#include "CChineseConvert.h"
#include "boost/regex.hpp"

using namespace std;
using namespace boost;
using namespace Json;
using namespace models;

//////////////////////////////////////////////////////////////////

bool startAuditThread();

void debugAuditThread();

//////////////////////////////////////////////////////////////////


enum
{
    GROUPSENDLIM_OFF = 0,
    GROUPSENDLIM_ON = 1,
};

enum AUDIT_STATUS_FROM_MQ
{
    MAN_AUDIT_STATUS_PASS         = 1,
    MAN_AUDIT_STATUS_FAIL         = 2,
    MAN_AUDIT_STATUS_PASS_EXPIRE = 3,
    MAN_AUDIT_STATUS_FAIL_EXPIRE = 4,
    SYS_AUDIT_STATUS_WAIT_EXPIRE = 5,
    SYS_AUDIT_STATUS_GROUPSEND_UNLIM_SEND  = 6,
    SYS_AUDIT_STATUS_GROUPSEND_UNLIM_AUDIT = 7,
    SYS_AUDIT_STATUS_PASS_EXPIRE = 8,
    SYS_AUDIT_STATUS_FAIL_EXPIRE = 9,
};
enum AUTO_AUDIT_STATUS
{
    AUTO_AUDIT_STATUS_INVALID                           = 0,
    AUTO_AUDIT_STATUS_PASS                              = 1,
    AUTO_AUDIT_STATUS_FAIL                              = 2,
    AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_SEND        = 3,
    AUTO_AUDIT_STATUS_GROUPSEND_ULIMITED_TO_AUDIT       = 4,
};

enum AUDIT_SOURCE
{
    NORMAL_AUDIT_RESULT = 0,
    GROUPSEND_AUDIT_RESULT = 1,
};

enum
{
    GROUPSENDLIM_USERFLAG_PRODUCE = 0,
    GROUPSENDLIM_USERFLAG_SPECIFIED = 1,
};

enum
{
    // 运营商
    GROUPSENDLIM_CFG_OPER_YD_IDX = 0,        // 移动
    GROUPSENDLIM_CFG_OPER_LT_IDX = 1,        // 联通
    GROUPSENDLIM_CFG_OPER_DX_IDX = 2,        // 电信
    // 短信类型
    GROUPSENDLIM_CFG_SMS_TZ_IDX = 3,         // 通知
    GROUPSENDLIM_CFG_SMS_YZM_IDX = 4,       // 验证码
    GROUPSENDLIM_CFG_SMS_YX_IDX= 5,         // 营销
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class AuditReqMsg : public TMsg
{
public:
    AuditReqMsg()
    {
        m_uiPayType = 0;
        m_uiOperater = 0;
        m_uiSmsFrom = 0;
        m_uiShowSignType = 0;
        m_uiSmsNum = 0;
        m_bIncludeChinese = false;
        m_eMatchSmartTemplateRes = NoMatch;
        m_iClientSendLimitCtrlTypeFlag = 0;
    }

    ~AuditReqMsg() {}

private:
    AuditReqMsg(const AuditReqMsg&);

public:
    string m_strClientId;
    string m_strUserName;
    string m_strSmsId;
    string m_strPhone;
    string m_strSign;
    string m_strSignSimple;
    string m_strContent;
    string m_strContentSimple;
    string m_strSmsType;
    UInt32 m_uiPayType;
    UInt32 m_uiOperater;
    string m_strC2sDate;

    UInt32 m_uiSmsFrom;
    UInt32 m_uiShowSignType;
    string m_strSignPort;
    string m_strExtpendPort;
    string m_strIds;
    UInt32 m_uiSmsNum;
    string m_strCSid;
    string m_strUid;

    bool m_bIncludeChinese;

    SmartTemplateMatchResult m_eMatchSmartTemplateRes;
    int     m_iClientSendLimitCtrlTypeFlag;
};

// AuditThread发送审核响应消息给SessionThread
// 1.m_uiAuditState为INVALID_UINT32时，代表没有审核结果，SessionThread需要继续后续发送流程
// 2.m_uiAuditState为非INVALID_UINT32时，代表存在审核结果，SessionThread需要根据审核状态做相应处理
class AuditRspMsg : public TMsg
{
public:
    AuditRspMsg()
    {
        m_uiAuditState = INVALID_UINT32;
    }
    ~AuditRspMsg() {}

private:
    AuditRspMsg(const AuditRspMsg&);
    AuditRspMsg& operator=(const AuditRspMsg&);

public:
    string m_strAuditContent;
    string m_strAuditId;
    string m_strAuditDate;
    UInt32 m_uiAuditState;
    string m_strErrCode;
};


class TRedisResp;

class AuditThread : public CThread
{

typedef enum Branchflow
{
    Audit,              // 走审核流程
    GroupSendLimit,     // 走群发限制流程
    ContinueToSend,     // 走继续发送流程
    End
}Branchflow;

class Session
{
public:
    Session();
    Session(AuditThread* pThread);
    ~Session();

    void init();
    void setAuditReqMsg(AuditReqMsg& reqMsg) {m_auditReqMsg = reqMsg;}
    void getAuditContent();
    void getGroupSendLimitAuditContent();
    void packMsg2Audit();
    void praseAuditResMsg(cs_t strData);

    void packMsg2MqC2sIoDown();
    void packMsg2MqC2sIoUp();

private:
    Session(const Session&);
    Session& operator=(const Session&);

public:
    AuditThread* m_pThread;
    UInt64 m_ulSequence;

    AuditReqMsg m_auditReqMsg;

    string m_strAuditContent;
    string m_strAuditId;
    string m_strAuditDate;
    UInt32 m_uiAuditState;

    // 内容审核结果队列status字段
    UInt32 m_uiAuditResult;

    UInt32 m_uiState;
    string m_strErrCode;

    UInt32 m_uiGroupSendLimitFlag;
    UInt32 m_uiGroupsendLimUserFlag;

    string m_strSendMsg;
};

typedef map<UInt64, Session*> SessionMap;
typedef SessionMap::iterator SessionMapIter;


public:
    AuditThread(const char *name);
    virtual ~AuditThread();
    bool Init();

private:
    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);

    bool initAuditReqMqQueue();
    void initAuditOverKeyWordSearchTree(map<string, list<string> >& mapSetIn, map<string, searchTree*>& mapSetOut);
    void initSysPara(const std::map<std::string, std::string>& mapSysPara);

    bool getMqCfgByC2sId(cs_t strC2Sid, MqConfig& mqConfig);
    bool getMQConfig(cs_t strPhone, cs_t strSmsType, cs_t strCsid, MqConfig& mqCfg);
    UInt32 getPhoneArea(cs_t strPhone);

    bool TimeRegCheck(string strTime);
    bool praseJsonAuditDetail(cs_t strData, Session* pSession);

    void sendAuditReqMsg2Audit(Session* pSession);
    bool pickOneAuditReqMqQueue(Session* pSession, MqConfig& config);

    void handleTimeoutMsg(TMsg* pMsg);

    void handleAuditReqMsg(TMsg* pMsg);
    Branchflow checkBranchflow(AuditReqMsg* pReq);
    bool matchGroupsendLimitCfgMode(AuditReqMsg* pReq, const SmsAccount& smsAccount);
    void processAudit(AuditReqMsg* pReq, Session* pSession = NULL);
    void processAuditReal(AuditReqMsg* pReq);
    void processGroupSendLimit(AuditReqMsg* pReq);
    void processContinueToSend(AuditReqMsg* pReq);
    int checkAuditAttributeAndKeyword(AuditReqMsg* pReq);
    bool checkfreeAuditKeyword(const AuditReqMsg* pReq, string& strOut);


    void handleRedisRspMsg(TMsg* pMsg);
    void processCheckEndAuditSms(TRedisResp* pRedisRsp);
    void processCheckGroupSendEndAuditSms(TRedisResp* pRedisRsp);
    bool checkMarketingTimeArea(Session* pSession);
    void setAuditDetail(cs_t strRedisCmdKey, Session* pSession);
    void sendAuditRspMsg(Session* pSession);


    void handleMqAuditResReqMsg(TMsg* pMsg);
    void processAuditResultPassOrFailed(Session* pSession);
    void processAuditResultExpire(Session* pSession);
    void processAuditResultGroupSendUnLimit(Session* pSession);


    void handleRedisListRspMsg(TMsg* pMsg);
    void processGetAuditDetailRspMsg(TMsg* pMsg);
    void processGetCacheDetailRspMsg(TMsg* pMsg);
    void processAuditDetail(Session* pSession);
    void sendMsg2MqC2sIo(Session* pSession);
    void updateDbAuditState(Session* pSession);

private:
    SnManager m_snManager;

    SessionMap m_mapSession;

    MqConfigMap m_mapMqCfg;

    ComponentConfigMap m_mapComponentCfg;

    ComponentRefMqMap m_mapComponentRefMq;

    SmsAccountMap m_mapAccount;

    map<string, string> m_mapSysErrCode;

    PhoneSectionMap m_mapPhoneArea;

    PhoneDao m_phoneDao;

    CChineseConvert m_chineseConvert;

    // 审核关键字检查
    KeyWordCheck m_keyWordCheck;
    int m_iAuditKeywordCovRegular;
    map<string, searchTree*> m_NoAuditKeyWordMap;

    // 系统参数:MQ(c2s_db)审核内容请求队列数量
    UInt32 m_uiSysParaAuditReqMqNum;
    vector<MqConfig> m_vecAuditReqMqCfg;


    // 系统参数:营销短信发送时间区间
    string m_strSysParaAuditTimeAreaBegin;
    string m_strSysParaAuditTimeAreaEnd;

    // 系统参数:群发限制系统级开关
    UInt32 m_uiSysParaGroupSendLimCfg;

    // 系统参数:smsp_access组件从审核明细表中单次读取短信条数
    UInt32 m_uiSysParaAuditOutNumber;

    int m_iAuditTimeOut;

    CThreadWheelTimer* m_pTimerGetCacheDetail;

    regex m_regTime;        //eg 21:53

};

extern AuditThread* g_pAuditThread;

#endif // SMS_ACCESS_THREAD_CAUDITTHREAD_H