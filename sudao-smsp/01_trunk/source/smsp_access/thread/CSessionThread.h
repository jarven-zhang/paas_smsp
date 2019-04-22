#ifndef __C_SESSION_THREAD_H__
#define __C_SESSION_THREAD_H__

#include "Thread.h"
#include "SnManager.h"
#include "eventtype.h"
#include "PhoneDao.h"
#include "SmspUtils.h"
#include "CChineseConvert.h"
#include "CommClass.h"
#include "HttpSender.h"
#include "Channel.h"
#include "enum.h"
#include "ClientPriority.h"
#include "ChannelPriorityHelper.h"
#include "CRouterThread.h"
#include "AppData.h"

using namespace std;
using namespace boost;
using namespace models;

typedef set<UInt64> UInt64Set;


bool startSessionThread();
enum SEND_CONTROL_TYPE
{
    NOT_CONTROL,
    RANDOM_CONTROL = 1, 
    BLACKLIST_CONTROL = 2, 
    OVERRATE_CONTROL = 3,
} ;


class apiTrace
{

    #define CHECK_STATE( s ) if( s >= STATE_MAX ) return false;

public:
    class traceInfo
    {
    public:
        traceInfo()
        {
            m_uStartTime = 0;
            m_uEndTime = 0;
            m_uErrorCode = ACCESS_ERROR_NONE;
        }
        ~traceInfo() {}
    public:
        UInt64 m_uStartTime;
        UInt64 m_uEndTime;
        UInt32 m_uErrorCode;
    };

public:
    apiTrace()
    {
    }
    ~apiTrace() {}

    bool Begin(UInt32 uState)
    {
        CHECK_STATE(uState);

        traceInfo& traceSate = m_mapTraceInfo[uState];
        traceSate.m_uStartTime = getCurrentTimeMS();
        traceSate.m_uEndTime = traceSate.m_uStartTime;
        return true;
    }

    bool End(UInt32 uState)
    {
        CHECK_STATE(uState);

        traceInfo& traceSate = m_mapTraceInfo[uState];
        UInt64 uEndTime = getCurrentTimeMS();

        if (traceSate.m_uStartTime != 0 &&
            (uEndTime > traceSate.m_uStartTime))
        {
            traceSate.m_uEndTime = uEndTime ;
            return true;
        }

        return false;
    }

    bool setError(UInt32 uState, UInt32 uErrorCode)
    {
        CHECK_STATE(uState);
        traceInfo& traceSate = m_mapTraceInfo[uState];
        traceSate.m_uErrorCode = uErrorCode;
        return true;
    }

    //get raw info
    bool InfoRawData(traceInfo& info, UInt32 uState = STATE_INIT)
    {
        CHECK_STATE(uState);
        info = m_mapTraceInfo[uState];
        return true;
    }

    //uSTate 没有指定特定状态时候获取所有的状态，指定后打印指定状态
    bool InfoString(string& strOut, UInt32 uState = STATE_INIT)
    {
        CHECK_STATE(uState);

        if (uState != STATE_INIT)
        {
            traceInfo& trace =  m_mapTraceInfo[ uState ];
            strOut = Fmt< 128 >("[ trace %s ] s_time:%lu, e_time:%lu, t_time:%lu ms ",
                               state2String( uState ).data(), trace.m_uStartTime,
                               trace.m_uEndTime, trace.m_uEndTime -  trace.m_uStartTime );
            return true;
        }
        else
        {
            //strOut = "\n";
            strOut = Fmt<128>("\n[ %-16s, %-13s, %-13s, %-12s, %-25s ] \n",
                               "state", "s_time", "e_time", "elapse_time", "    error" );

            for ( ; uState < STATE_DONE ; uState++ )
            {
                traceInfo &trace =  m_mapTraceInfo[ uState ];
                string strErro = "" ;

                if ( trace.m_uStartTime != 0 )
                {
                    strErro = error2Str( trace.m_uErrorCode );
                    if( strErro.length() > 7 )
                    {
                        strErro = strErro.substr(7);
                    }
                }

                strOut.append(Fmt< 128 >("[ %-16s, %13lu, %13lu, %8lu(ms), %-25s ] \n",
                             state2String( uState ).data(), trace.m_uStartTime,
                             trace.m_uEndTime, trace.m_uEndTime -  trace.m_uStartTime,
                             strErro.data()));
            }
            strOut.append(Fmt< 128 >("[ %-16s, %13lu, %13lu, %8lu(ms), %-25s ] \n",
                    state2String(STATE_DONE).data(),
                    m_mapTraceInfo[ STATE_INIT ].m_uStartTime,
                    m_mapTraceInfo[ STATE_DONE ].m_uEndTime,
                    m_mapTraceInfo[ STATE_DONE ].m_uEndTime - m_mapTraceInfo[ STATE_INIT ].m_uStartTime, ""));
            return true;
        }

        return false;

    }

private:

    traceInfo m_mapTraceInfo[ STATE_MAX ];

};


class smsSessionInfo
{
public:
    smsSessionInfo()
    {
        m_lchargeTime       = 0;
        m_uSmsFrom          = 0;
        m_uPayType          = 0;
        m_uState            = 0;
        m_uNeedExtend       = 0;
        m_uNeedSignExtend   = 0;
        m_uNeedAudit        = 0;
        m_uShowSignType     = 0;
        m_uShortSmsCount    = 0;
        m_uSendHttpType     = 0;
        m_uSessionId        = 0;
        m_uSmsNum           = 1;
        m_uIdentify         = 0;
        m_uChannleId        = 0;
        m_strSmsType        = "5";
        m_uOperater         = 0;
        m_uLongSms          = 0;
        m_uChannelSendMode  = 0;
        m_fCostFee          = 0;
        m_uSaleFee          = 0;
        m_uExtendSize       = 0;
        m_uChannelType      = 0;
        m_bInOverRateWhiteList = false;
        m_bSendControlOverRateWhiteList = false;
        m_bOverRateSessionState = false;
        m_bHostIPSessionState = false;
        m_bOverRatePlanFlag = false;
        m_uSendNodeId       = 0;
        m_uArea             = 0;
        m_uProductType      = 99;
        m_uProductCost      = 0;
        m_strSubId          = "0";
        m_uAgentId          = 0;
        m_uOverRateCharge   = 0;
        m_uChannelOperatorsType = 0;
        m_uIdentify         = 0;
        m_uChannelIdentify  = 0;
        m_uSelectChannelNum = 0;
        m_iRedisIndex       = -1;
        m_uAgentType        = 0;
        m_iAgentPayType     = 0;
        m_uChannelMQID      = 0;
        m_bIsNeedSendReport = false;
        m_uProcessTimes     = 0;
        m_uFirstShortSmsResp = 0;

        m_bReadyAudit       = false;

        m_uOverTime         = 0;
        m_iOverCount        = 0;
        m_uPorcessTime      = 0;
        m_uClientAscription = 0;
        m_uBelong_sale      = 0;
        m_uBelong_business  = 0;
        m_uFreeChannelKeyword = 0;
        m_uSendType         = 0;
        m_bIsKeyWordOverRate = false;

        m_bMatchAutoWhiteTemplate = false;

        m_bIncludeChinese = false;

        m_bSelectChannelAgain = false;
        m_uAuditState = INVALID_UINT32;

        m_ucHttpProtocolType = INVALID_UINT8;

        m_iFailedResendTimes = 0;

        m_uGroupsendLimUserflag = 0;
        m_uGroupsendLimNum = 200;
        m_uGroupsendLimTime = 30;
        m_bWaitAuditByGroupsend = false;

        m_bCheckChannelOverRate = false;
        m_uOriginChannleId      = 0;

        m_eMatchSmartTemplateRes = NoMatch;

        m_uSessionState = STATE_INIT;
        m_pHttpSender = NULL;
        m_pSessionTimer = NULL;
        m_apiTrace = new apiTrace();
        m_vecPhoneBlackListInfo.clear();
        m_vecFailedResendChannelsSet.clear();
        m_ulOverRateSessionId = 0;
        m_uRepushPushType  = 0;
        m_uAuditFlag = 0;
        m_uFromSmsReback = 0;
        m_iClientSendLimitCtrlTypeFlag = NOT_CONTROL;
        m_ucChargeRule = 0;
    }

    ~smsSessionInfo()
    {
        if (NULL != m_pHttpSender)
        {
            m_pHttpSender->Destroy();
        }

        SAFE_DELETE(m_pHttpSender);
        SAFE_DELETE(m_pSessionTimer);
        SAFE_DELETE(m_apiTrace);
    }

    UInt64          m_lchargeTime;
    string          m_strPhone;
    string          m_strSmsId;
    UInt64          m_uSubmitId;
    string          m_strSgipNodeAndTimeAndId;
    string          m_strClientId;
    string          m_strSid;
    string          m_strUserName;
    string          m_strSign;
    string          m_strSignSimple;
    string          m_strContent;
    string          m_strContentSimple;
    string          m_strDeliveryUrl;
    string          m_strErrorCode;
    string          m_strDate;
    string          m_strSubmit;
    string          m_strSubmitDate;
    string          m_strSrcPhone;
    string          m_strExtpendPort;
    string          m_strUid;
    string          m_strIsChina;
    string          m_strSignPort;
    string          m_strYZXErrCode;
    UInt32          m_uShortSmsCount;
    string          m_strSmsType;
    UInt32          m_uSmsFrom;
    UInt32          m_uPayType;
    UInt32          m_uState;
    UInt8           m_uNeedAudit;
    bool            m_bReadyAudit;
    UInt32          m_uNeedExtend;
    UInt32          m_uNeedSignExtend;
    UInt32          m_uShowSignType;
    UInt32          m_uSmsNum;
    UInt32          m_uIdentify;
    UInt32          m_uChannelIdentify;
    string          m_strError;
    UInt32          m_uSendHttpType;
    string          m_strSendUrl;
    UInt32          m_uChannleId;
    UInt64          m_uChannelMQID;
    UInt64          m_uChannelExValue;
    UInt32          m_uOperater;
    UInt32          m_uHttpIp;
    string          m_strHttpChannelUrl;
    string          m_strHttpChannelData;
    UInt32          m_uLongSms;
    string          m_strCodeType;
    string          m_strChannelName;
    UInt32          m_uChannelSendMode;
    float           m_fCostFee;
    double          m_uSaleFee;
    UInt32          m_uExtendSize;
    UInt32          m_uChannelType;
    string          m_strUcpaasPort;
    UInt32          m_uArea;
    UInt32          m_uSendNodeId;
    UInt32          m_uProductType;
    double          m_uProductCost;
    string          m_strSubId;
    UInt64          m_uAgentId;
    UInt32          m_uAgentType;
    int             m_iAgentPayType;
    UInt32          m_uOverRateCharge;
    UInt32          m_uChannelOperatorsType;
    string          m_strChannelRemark;
    bool            m_bInOverRateWhiteList;
    bool            m_bSendControlOverRateWhiteList;
    bool            m_bOverRateSessionState;
    bool            m_bHostIPSessionState;
    bool            m_bOverRatePlanFlag;
    UInt32          m_uSelectChannelNum;
    Int32           m_iRedisIndex;
    UInt32          m_uProcessTimes;
    bool            m_bIsNeedSendReport;
    UInt8           m_uFirstShortSmsResp;
    string          m_strCSid;
    string          m_strCSdate;
    string          m_strHttpOauthUrl;
    string          m_strHttpOauthData;
    string          m_strChannelTemplateId;
    string          m_strTemplateParam;
    string          m_strTemplateType;
    string          m_strIds;
    UInt64          m_uOverTime;
    int             m_iOverCount;
    UInt32          m_uPorcessTime;
    UInt32          m_uClientAscription;
    UInt64          m_uBelong_sale;
    UInt64          m_uBelong_business;
    UInt32          m_uFreeChannelKeyword;
    UInt32          m_uSendType;
    bool            m_bIsKeyWordOverRate;
    bool            m_bMatchGlobalNoSignAutoTemplate;
    bool            m_bMatchAutoWhiteTemplate;
    string          m_strTestChannelId;
    bool            m_bIncludeChinese;
    string          m_strKeyWordOverRateKey;
    string          m_strChannelOverRateKey;
    bool            m_bSelectChannelAgain;
    string          m_strAuditId;
    string          m_strAuditContent;
    UInt32          m_uAuditState;
    string          m_strAuditDate;
    string          m_strExtraErrorDesc;
    UInt8           m_ucHttpProtocolType;
    UInt8           m_uTimerSendSubmittype;
    string          m_strTaskUuid;
    int             m_iFailedResendTimes;
    string          m_strFailedResendChannels;
    set<UInt32>     m_FailedResendChannelsSet;
    UInt32          m_uGroupsendLimTime;
    UInt32          m_uGroupsendLimUserflag;
    UInt32          m_uGroupsendLimNum;
    bool            m_bWaitAuditByGroupsend;
    bool            m_bCheckChannelOverRate;
    UInt32          m_uOriginChannleId;
    string          m_strChannelOverrate_access;
    string          m_strIsReback;
    UInt32          m_uAuditFlag;
    UInt32          m_uRepushPushType;
    vector<string>  m_vecLongMsgContainIDs;
    UInt32          m_uSessionState;
    UInt32          m_uSessionPreState;
    UInt64          m_uSessionId;
    string          m_strTemplateId;
    string          m_strInnerErrorcode;

    SmartTemplateMatchResult    m_eMatchSmartTemplateRes;
    http::HttpSender* m_pHttpSender; //计费模块使用
    CThreadWheelTimer* m_pSessionTimer; //会话管理定时器

    set<UInt32>     m_vecFailedResendChannelsSet;      // access组件通道失败重新发送表
    set<string>     m_vecPhoneBlackListInfo;            // eg: value:0:3 the phone is in sysblacklist ,value:2154 the phone is in channel 2154 blacklist
    apiTrace*        m_apiTrace;

    UInt64  m_ulOverRateSessionId;
    string  m_strChannelOverRateKeyall;
    UInt8   m_uFromSmsReback;
    int     m_iClientSendLimitCtrlTypeFlag;
    UInt32 m_ucChargeRule;
};


typedef smsSessionInfo* session_pt;

#define SESSION_NEW( ) ( new smsSessionInfo())

class CSessionThread : public CThread
{
    struct SendLimitCtrl
    {
        SendLimitCtrl(): m_uiCount(0), m_uiThreshold(0) {}

        string m_strDate;
        UInt32 m_uiCount;
        UInt32 m_uiThreshold;
        string m_strUpdateTime;
    };

    #define ACCESS_SESSION_TIMER_ID    "SESSION_TIMER_ID"
    #define ACCESS_STATISTIC_TIMER_ID  "STATISTIC_TIMER_ID"

    #define ACCESS_SESSION_MAX_SIZE  6000
    #define ACCESS_SESSION_TIMEOUT   10000
    #define ACCESS_STATISTIC_TIMEOUT 5000

    typedef map<UInt64, session_pt>  SMSSessionMAp;
    typedef map<string, string> strErrorMap;
    typedef map<int, Channel> channelInfoMap;
    typedef map<string, SendLimitCtrl> SendLimitCtrlMap;

    enum CHARGE_TYPE { CHARGE_PRE = 0, CHARGE_POST = 1, DEDUCT_MASTAR_ACCOUNT = 2, DEDUCT_SUB_ACCOUNT = 3 } ;
    enum OVERRATE_TYPE { OVERRATE_CHECK = 0, OVERRATE_SET = 1 };

public:
    CSessionThread(const char* name);
    ~CSessionThread();
    bool Init();
    UInt32 GetSessionMapSize(){return m_uCurSessionSize;}

private:
    void MainLoop();
    void HandleMsg(TMsg* pMsg);
    UInt32 HandleRedisRespMsg(TMsg* pMsg);

    void HandleTimeOut(TMsg* pMsg);
    void HandleMQReqMsg(TMsg* pMsg);

    void initGetSysPara(constSysParamMap& mapSysPara);

    void DBUpdateRecord(session_pt pSession, const char* pStrMq = NULL);
    void setSessionSendUrl(session_pt pSession, string& strSendUrl);
    void setSendUrlFromReback2Send(session_pt pSession, string& strSendUrl);
    UInt32 MQPushBackToC2sIO(session_pt pSession, int iPushType);
    UInt32 MQC2sioFailedSendReport(session_pt pInfo);

    void sendLimitSuccessReport(session_pt pSession);
    bool   MQChannelpublishMeaasge(session_pt pInfo);
    MqConfig* MQGetC2SIOConfigDown(session_pt pSession);
    MqConfig* MQGetC2SIOConfigUP(string strC2Sid, UInt32 uSmsfrom);
    smsSessionInfo* GetSessionBySeqID(UInt64 uSeq);

    void HandleMQRebackReqMsg(TMsg* pMsg);
    void GenerateMQSendUrl(session_pt pSmsInfo);
    bool CheckRebackSendTimes(session_pt pInfo);

    UInt32 GetSessionTimeoutMS();
    UInt32 UpdateAuditState(session_pt pSession);
    bool SendToReback(session_pt pInfo);
    bool GetRebackMQInfo(session_pt pInfo, string& strExchange, string& strRoutingKey);


    UInt32 nextState(session_pt pSession);
    UInt32 HandleStatusRpt(TMsg* pMsg, UInt32 uState = STATE_INIT);
    UInt32 sessionStateProc(session_pt pSession, UInt32 uState) ;
    UInt32 sessionPreProcess(session_pt pSession);

    UInt32 sessionBlackList(session_pt pSession);
    UInt32 sessionBlackListReport(session_pt pSession, TMsg* pMsg);

    UInt32 sessionTemplate(session_pt pSession);
    UInt32 sessionTemplateReport(session_pt pSession, TMsg* pMsg);

    UInt32 sessionAudit(session_pt pSession);
    UInt32 sessionAuditReport(session_pt pSession, TMsg* pMsg);

    UInt32 sessionRouter(session_pt pSession);
    UInt32 sessionRouterReport(session_pt pSession, TMsg* pMsg);

    UInt32 sessionOverRate(session_pt pSession, OVERRATE_TYPE eOverRateType = OVERRATE_CHECK);
    UInt32 sessionOverRateReport(session_pt pSession, TMsg* pMsg);

    UInt32 sessionCharge(session_pt pSession,  CHARGE_TYPE chargeType = CHARGE_PRE);
    UInt32 sessionChargeReport(session_pt pSession, TMsg* PMsg);

    UInt32 sessionDone(session_pt pSession);

    void handleDbUpdateReq(TMsg* pMsg);

    void checkSendLimit(session_pt pSession, int iControlType);
    void refreshSendLimit(session_pt pSession, int iControlType);
    void clearSendLimit(AppDataMap& mapAppData);
    void getChargeRule(session_pt pSession);
    bool checkOverRateWhiteList(session_pt pSession);
    bool checkSendControlWhiteList(session_pt pSession);

private:

    UInt32           m_uAllowSessionCount;
    volatile UInt32  m_uCurSessionSize;
    SnManager*       m_pSnMng;
    CChineseConvert* m_pCChineseConvert;
    SMSSessionMAp    m_mapSMSSession;
    PhoneDao         m_honeDao;
    UInt64           m_uAccessId;
    accountMap       m_mapAccount;
    strErrorMap      m_mapSystemErrorCode;

    componentRefMQMap m_mqConfigRef;
    mqConfigMap       m_mqConfigInfo;
    compoentMap      m_componentConfigInfo;

    channelInfoMap   m_mapChannelInfo;

    CThread*         m_pBlackListThread;
    CThread*         m_pChargeThread;
    UInt32           m_uSleepTime;
    phoneAreaMap     m_mapPhoneArea; //号码区域对应表

    CThreadWheelTimer* m_pThreadTimer; //会话管理定时器
    client_priorities_t m_clientPriorities;

    UInt64 m_uOverTime;   //reback订单超时时间
    int m_iOverCount;  //reback订单失效次数
    map<UInt64, list<channelPropertyLog_ptr_t> > m_channelPropertyLogMap;
    map<string, list<userPriceLog_ptr_t> > m_userPriceLogMap;
    map<string, list<userPropertyLog_ptr_t> > m_userPropertyLogMap;

    set<string> m_setOverRateWhiteList;

    AppDataMap m_mapClientSendLimitCfg;
    AppDataVecMap m_mapVecUserProperty;

    SendLimitCtrlMap m_mapSendLimitCtrl;
};


extern CSessionThread* g_pSessionThread;


#endif
