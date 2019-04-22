#ifndef __H_CCHANNLE_TX_THREADS__
#define __H_CCHANNLE_TX_THREADS__

#include "Thread.h"
#include "SnManager.h"
#include <map>
#include "SmsContext.h"
#include "channelErrorDesc.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "SmsAccount.h"
#include "Channel.h"
#include "SignExtnoGw.h"
#include "StateResponse.h"
#include "IChannel.h"
#include "CRedisThread.h"
#include "CHTTPSendThread.h"
#include "VerifySMS.h"




/* 返回值定义*/
enum{
    CC_TX_RET_SUCCESS = 0,
    CC_TX_RET_PARAM_ERROR,       /*参数错误*/
    CC_TX_RET_PHONE_EXSIT,       /*号码重复*/
    CC_TX_RET_NO_CLIENT,         /*账号不存在*/
    CC_TX_RET_CHANNEL_NOT_EXSIT = 20, /*通道不存在*/
    CC_TX_RET_CHANNEL_CLOSED,    /*通道关闭*/
    CC_TX_RET_CHANNEL_FLOW_CTRL, /*通道流速控制*/
    CC_TX_RET_SESSION_NOT_FIND = 30,
    CC_TX_RET_SESSION_INIT_ERR,
    CC_TX_RET_SESSION_STAT_ERROR,
    CC_TX_RET_SESSION_RECORD_NULL,
    CC_TX_RET_SESSION_TIMEOUT,
    CC_TX_RET_CLUSTER_SEND = 40 ,
    CC_TX_RET_CLUSTER_REBACK,
    CC_TX_RET_CLUSTER_RECOVE,
    CC_TX_RET_MQ_CONFIG_ERROR = 50,
    CC_TX_RET_SMSID_NOT_MATCH,
    CC_TX_RET_SEND_MSG_ERROR,
    CC_TX_RET_REDIS_ERROR,
    CC_TX_RET_DB_ERROR,
    CC_TX_RET_SESSION_ACCESS_MAX,
    CC_TX_RET_OTHER = 99,
};

enum SendAuditInterceptType
{
    HTTP = 0,
    DIRECT = 1,
};

/*组包内部状态*/
enum
{
    CC_TX_STATUS_INIT = 0,      /* 初始化状态*/
    CC_TX_STATUS_SEND = 1,      /* 已经发送没有响应回来*/
    CC_TX_STATUS_SUCCESS =2 ,   /* 发送成功*/
    CC_TX_STATUS_FAIL = 3,      /* 发送失败，状态报告*/
    CC_TX_STATUS_REBACK = 4,    /* 发送失败，reback处理*/
    CC_TX_STATUS_DISCARD = 5,   /* 丢弃*/
    CC_TX_STATUS_TIMEOUT = 6,   /* 发送超时*/
    CC_TX_STATUS_SOCKET_ERR_TO_REBACK = 7,/*协议层准备发给通道发现socket error，这种情况需要调整滑窗*/
};

enum
{
    CHANNEL_UN_LOGIN = 0,
    CHANNEL_LOGIN_SUCCESS,
    CHANNEL_LOGIN_FAIL,
};

/*通道发送状态上报*/
class CSMSTxStatusReport
{
public:
    CSMSTxStatusReport()
    {
        m_status = 0;
        m_lSendDate= 0;
        m_lSubmitDate = 0;
        m_lSubretDate = 0;
        m_iIndexNum = 0;
        m_uTotalCount = 0;
        m_iSmsCnt = 0;
    };

public:
    UInt32             m_status;          /*状态*/
    string             m_strSubmit;       /*提交状态描述*/
    string             m_strYZXErrCode;   /*错误码描述*/
    string             m_strSmsid;        /*提交时的平台消息ID*/
    string             m_strChannelSmsid; /*返回的消息ID */
    string             m_strChannelId;    /*通道处理smsId  */
    string             m_strSmsUuid;      /*发送ID */
    Int64              m_lSendDate;       /*发送日期*/
    Int64              m_lSubmitDate;     /*提交日期*/
    Int64              m_lSubretDate;     /*提交响应时间*/
    string             m_strContent;      /*消息内容*/
    string             m_strSign;         /*消息签名*/
    string             m_strSubRet;       /*提交后响应回来的内容*/
    UInt32             m_iIndexNum;       /*分段索引*/
    UInt32             m_uTotalCount;     /*返回的分段数量*/
    UInt32             m_iSmsCnt;         /*通道计费条数*/
    UInt32             m_uChanCnt;        /*通道条数*/

    smsp::mapStateResponse m_mapResponses; /*多值状态处理*/
    string             m_strChannelLibName;    /*libname  */
    string              m_strErrorCode;

};

class CSMSTxStatusReportMsg : public TMsg {
public:
    CSMSTxStatusReport  report;
};

/* 通道会话*/
class channelTxSessions {
public:
    channelTxSessions()
    {
        m_context    = NULL;
        m_pWheelTime = NULL;
        m_threadId   = 0;
        m_pSender    = NULL;
        m_reqMsg     = NULL;
        m_iSeq       = 0;
        m_context    = new smsp::SmsContext();
        m_uStatus    = CC_TX_STATUS_INIT;
        m_uRecvCnt   = 0;
        m_uErrCnt    = 0;
        m_uTotalCnt  = 0;
        m_bDirectProt= false;
    };
    ~channelTxSessions();
public:
    smsp::SmsContext  *m_context;     /* 保存的会话信息*/
    void              *m_reqMsg;      /* 保存的请求信息*/
    CThreadWheelTimer *m_pWheelTime;  /* 会话超时控制*/
    UInt32             m_threadId;    /* 下发的线程ID  */
    string             m_strClusterType;/* 是否为组包会话*/;
    CThread           *m_pSender;     /* 会话建立对象*/
    UInt64             m_iSeq;        /* 会话管理的iSeq */
    UInt64             m_uClusterSession;
    UInt32             m_uPhoneCnt;
    UInt32             m_uRecvCnt;    /* 已经收到的分段*/
    UInt32             m_uTotalCnt;   /* 短信分段数*/
    UInt32             m_uStatus;     /* 会话状态*/
    UInt32             m_uErrCnt;     /* 错误分段数*/
    bool               m_bDirectProt;/*标识是否是直连协议，true:是直连协议*/
};


typedef class IChannelFlowCtr channelTxFlowCtr;

#if 0
/*通道发送流控*/
class channelTxFlowCtr
{
public:

    channelTxFlowCtr()
    {
        iWindowSize = 0;
        uResetCount = 0;
        lLastRspTime = 0;
        lLastReqTime = 0;
        uTotalReqCnt = 0;
        uTotalRspCnt = 0;
        uSubmitFailedCnt = 0;
        uSendFailedCnt = 0;
    };

public:

    int     iWindowSize;      /*当前滑窗大小*/
    UInt32  uResetCount;      /*重置次数*/
    Int64   lLastRspTime;     /*最后一次响应回来*/
    Int64   lLastReqTime;     /*最后一次请求时间*/
    UInt64  uTotalReqCnt;     /*总的请求次数*/
    UInt64  uTotalRspCnt;     /*总的响应次数*/
    UInt64  uSubmitFailedCnt; /*失败错误计数*/
    UInt64  uSendFailedCnt;   /*发送失败计数*/
};
#endif

/* 通道处理线程池*/
class CChannelThreadPool: public CThread
{
    #define CC_TX_CHANNEL_FLOWCTR_RESET_COUNT       3
    #define CC_TX_CHANNEL_THREAD_NUM_MAX            32
    #define CC_TX_CHANNEL_THREAD_NUM_DEF            10

    #define CC_TX_CHANNEL_SESSION_TIMEOUT_90S  (90*1000)         /* 会话总超时时间1  分钟*/
    #define CC_TX_CHANNEL_SESSION_TIMEOUT_30S  (30*1000)         /* 分段超时时间*/

    #define CC_TX_CHANNEL_WINDOWSIZE_HTTP_RESET_TIME  (120)      /* http滑窗重置时间*/
    #define CC_TX_CHANNEL_WINDOWSIZE_TCP_REST_TIME    (60)       /* TCP滑窗重置时间*/
    #define CC_TX_CHANNEL_WINDOWSIZE_CHECK_INTERVAL   (2*1000)   /* 滑窗检测周期*/

    #define CC_TX_CHANNEL_SYSN_SLIDEWND_TIME           (30*1000)//30s
    #define CC_TX_CHANNEL_SLIDEWND_0_TIME_GAP          (5*60)//5分钟

    #define CC_TX_CHANNEL_SESSION_TIMER_ID     (0xff9922)
    #define CC_TX_CHANNLE_SESSION_TIMER_SEG_ID (0xff9923)
    #define CC_TX_CHANNEL_FLOW_RESET_TIMER_ID  (0xff9924)

    #define CC_TX_CHANNEL_SYSN_SLIDEWND_TIMER_ID  (0xff9925)

    typedef std::vector< CThread* > channeThreads_t;
    typedef std::map<string, channelTxSessions* > channeSessionMap_t;
    typedef std::map<UInt32, channelTxFlowCtr*> channelTxFlowCtr_t;
    typedef std::map<std::string,vector<models::SignExtnoGw> > SignExtnoGwMap;
    typedef std::map<int, models::Channel> channelMap_t;

    typedef map<int, int> ChannelLoginStateValue_t;
    typedef map<int, ChannelLoginStateValue_t> ChannelLoginState_t;

    typedef struct {
        UInt32          m_uPoolIdx;
        channeThreads_t m_vecThreads;
    }channelThreadsData_t;

    typedef std::map< int, channelThreadsData_t >  channeThreadMap_t;

public:

    CChannelThreadPool(const char *name ):CThread(name)
    {
        m_uPollIdx = 0;

        // CHANNEL_LINK_FAIL default: 10
        m_uSubmitFailedValue = 10;

        // CHANNEL_RES_FAIL default: 10
        m_uSendFailedValue = 10;

        // CONTINUE_LOGIN_FAILED_NUM default: 10
        m_uContinueLoginFailedValue = 10;

        //CHANNEL_CLOSE_WAIT_RESP_TIME = 15
        m_uChannelCloseWaitTime = 15;

        m_pFlowWheelTime = NULL;

        m_timerSynSlideWnd = NULL;

        //60 minutes expire for gw channel
        m_uSignMoPortExpire = 60*60;
    };

    ~CChannelThreadPool()
    {
    };

    bool   Init( UInt32 maxThreadCount );
    void   MainLoop();
    void   HandleMsg( TMsg* pMsg );
    void   HandleTimeOut( TMsg* pMsg );
    UInt32 smsTxUpdateChannel( TMsg *pMsg );
    UInt32 smsTxChannelReq( TMsg* pMsg );
    UInt32 smsTxGetErrorCode( string strChannelID,string strIn,string& strDesc,string& strReportDesc );
    UInt32 smsTxGetSystemErrorCode( string strError, string strSubmit,string& strErrorOut,string& strSubmitOut );
    void smsTxDirectGetInnerError(string strDircet, UInt32 uChannelId,string strType,string strIn, string& strInnerError);
    UInt32 smsTxCountSubmitFailed(UInt32 uChannelId, bool bResult);
    UInt32 smsTxCountSendFailed(UInt32 uChannelId, bool bResult);
    UInt32 smsTxSendToReport( smsp::SmsContext* pSms, CSMSTxStatusReport * reportStatus );
    UInt32 smsTxSetMoRedisInfo(channelTxSessions * Session);
    UInt32 smsTxRecordInsertDb( smsp::SmsContext* pSms, CSMSTxStatusReport *reportStatus );
    string smsTxGeneratRecordSqlCmd( smsp::SmsContext * pSms,CSMSTxStatusReport * reportStatus);
    UInt32 smsTxSetLongMsgRedis( string strSmsId, string strPhone );
    UInt32 smsTxSendSuccReport(channelTxSessions * ss,CSMSTxStatusReport * reportStatus);
    UInt32 smsTxSendToReBack( channelTxSessions* pSession, CSMSTxStatusReport *reportStatus );
    UInt32 smsTxSendFailReport( channelTxSessions *ss, CSMSTxStatusReport *reportStatus );
    UInt32 smsTxStausToCluster( UInt32 ClusterStatus, channelTxSessions *session,
                              CSMSTxStatusReport *statusInfo, string strExchange, string strRoutingKey );
    UInt32 smsTxChannelSendStatusReport(TMsg * pMsg);

    UInt32 smsTxCheckReback( channelTxSessions *session, CSMSTxStatusReport *reportStatus );

    UInt32 smsTxParseMsgContent(string& content, vector<string>& contents, string& sign, UInt32 uLongSms, UInt32 uSmslength );
    UInt32 GetSessionMapSize();

    UInt32 smsTxFlowCtrSendReq( UInt32 uChannelId, UInt32 uLinkSize = 1, Int32 iCount = 1);//uLinkSize表示直连协议的连接数，对http协议没有作用
    UInt32 smsTxFlowCtrSendRsp( UInt32 uChannelId, UInt32 uStatus, bool bFirstIndex, Int32 iCount );


    UInt32 smsTxFlowCtrCheckAndReset();
    void  SysnSlideWndFromChannel();

    UInt32 smsTxHttpChannelReq( TMsg* pMsg );
    void smsTxDirectChannelReq( TMsg* pMsg );
    bool  smsTxGetSignExtGw( smsDirectInfo& smsParam );
    channelTxSessions* smsTxSaveReqMsg( TMsg* pMsg );
    void  smsTxDirectGetErrorCode(string strDircet, UInt32 uChannelId,
                                   string strType,string strIn, string& strDesc, string& strReportDesc );
    bool  ChannelTxPostMsg( TMsg *pMsg );
    bool  smsTxProcessStatusQueue();
    bool  smsTxPostMsg( TMsg *pMsg );

    void  HandleDirectToDeliveryReqMsg( TMsg *pMsg );

private:
    bool  InitChannelThreadInfo(int ChannelId);
    void  InitDirectChannelThreads(UInt32 maxConnect, int channelId);
    bool  AdaptChannelNodeNumChanged(int ChannelId, int newNodes, int oldWnd, int newWnd);
    int   GetWorkChannelNodeNum(int ChannelId);
    void  HandleChannelLoginStateMsg(TMsg* pMsg);
    void  GetFinalChannelLoginState(ChannelLoginStateValue_t mapLoginValue, int& finalState, char* desc);
    void  PrintLoginState(int ChannelId);

    //void  RefreshChannelSlideWindow(int ChannelId);
    void  UpdateChannelReq(int ChannelId);
    void smsTxUpdateSgipChannel ( TMsg* pMsg );
    void  CheckContinueLoginFailedValueChanged(UInt32 oldValue, UInt32 newValue);
    void  CheckChannelCloseWaitTimeChanged(UInt32 oldValue, UInt32 newValue);
    void  HandleTimeOutResetSlideWindowMsg(TMsg * pMsg);
    void  GetSysPara(const std::map<std::string, std::string>& mapSysPara, bool bInit = false);
    bool  IsChannelUpdate(models::Channel oldChannel, models::Channel newChannel);

    void  PoolChannelFlowCtrDelete(UInt32 uChannelId);
    void  PoolChannelFlowCtrNew(UInt32 uChannelId, UInt32 uWindowSize, bool bCluster = false );
    void  PoolChannelFlowCtrReset(UInt32 uChannelId, int uWindowSize, bool bCluster = false );
    void  PoolChannelFlowCtrModify(UInt32 uChannelId, int iWnd, int iMaxWnd);
    void  PoolChannelFlowCtrModify(UInt32 uChannelId, int iWnd);
    UInt32 PoolChannelFlowCtrSSRecover( UInt32 uChannelId, Int64 timeNow, UInt32 uMaxWindowSize );

    /***************************************/
    void GetSendAuditInterceptRedis(TMsg* pMsg, int type);
    void HandleDirectSendMsg(TMsg* pMsg);
    UInt32 HandleHttpSendMsg(TMsg* pMsg);
    void processChannelReq( TMsg* pMsg );
    void InsertDBHttpInsertDBHttp(TDispatchToHttpSendReqMsg*    pMsg, string strError);
    void sendAuditInterceptHttpRedisResp( TRedisResp* pRedisRes);
    void sendAuditInterceptDirectRedisResp( TRedisResp* pRedisRes);
    void HandleRedisRespMsg(TMsg* pMsg);
    template<typename T>
    void InsertDB(T& smsInfo, string strError);
    UInt32 smsTxFlowCtrAddOne( UInt32 uChannelId);

    /*****************************************/
    UInt32 GetLinkSize(UInt32 uChannelId);
    void  smsTxSetRedisResendInfo( channelTxSessions* pSession, string& strRedisData );
    template<typename T>
    bool  smsTxCheckFailedResendCfg(T& smsInfo);
    UInt32 GetChannelIndustrytype(int iChannelId);

private:
    channeThreadMap_t   m_mapChanneThread;    /*通道与线程对应关系*/
    channeThreads_t     m_poolTxThreads;      /*线程信息*/
    channeThreads_t     m_poolTxDirectThreads;/*直连线程数量*/
    channeSessionMap_t  m_mapSessions;        /*会话信息*/
    UInt32              m_uPollIdx;           /*通道线程轮训ID */
    SnManager*          m_pSnMng;             /*通道SN 管理*/
    UInt32              m_uSubmitFailedValue; ///// submit failed alarm value
    UInt32              m_uContinueLoginFailedValue;
    UInt32              m_uSendFailedValue;   ////  send failed alarm value
    channelTxFlowCtr_t  m_mapChannelFlowCtr; /* 通道流控*/
    SignExtnoGwMap      m_mapSignExtGw;
    map<string, channelErrorDesc> m_mapChannelErrorCode;
    map<string, string> m_mapSystemErrorCode;
    map<UInt64, MqConfig > m_mqConfigInfo;
    map<string, ComponentRefMq> m_componentRefMqInfo;
    map<string, SmsAccount> m_accountMap;
    channelMap_t         m_ChannelMap;     /*通道map*/
    channelMap_t         m_ChannelSgipMap;     /*通道map*/
    CThreadWheelTimer   *m_pFlowWheelTime; /*流速控制定时器*/

    TMsgQueue            m_msgStatusQueue; /*消息状态队列优先处理*/
    pthread_mutex_t      m_statusMutex;
    pthread_rwlock_t     m_rwlock; /*读写锁*/

    ChannelLoginState_t m_mapChannelLoginState;

    UInt32               m_uMaxSendTimes;
    UInt32               m_uChannelCloseWaitTime;
    CThreadWheelTimer   *m_timerSynSlideWnd;//定时器同步滑窗
    /*failed_resend 2018-01-22 add by ljl*/
    bool                 m_bResendSysSwitch;
    map<UInt32,FailedResendSysCfg> m_FailedResendSysCfgMap;
    UInt32               m_uSendAuditInterceptSwitch;
    UInt32               m_uSignMoPortExpire;//minutes
    VerifySMS*  m_pVerify;
};

#endif
