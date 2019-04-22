#ifndef __CACCESSREPORTPUSHTHTRAD__
#define __CACCESSREPORTPUSHTHTRAD__
#include "Thread.h"
#include "sockethandler.h"
#include "internalservice.h"
#include "SnManager.h"
#include <map>
#include "HttpSender.h"
#include "SmspUtils.h"
#include "curl/curl.h"


class ReportRePushReqMsg : public TMsg
{
public:
    ReportRePushReqMsg()
    {
        m_uiIp = 0;
        m_uiRetryCount = 0;
    }

    string m_strClientId;
    string m_strData;
    string m_strUrl;
    UInt32 m_uiIp;
    UInt32 m_uiRetryCount;
};

class RedisListReqSession
{
public:
    RedisListReqSession()
    {
        m_pTimer = NULL;
    }

    string m_strKey;
    CThreadWheelTimer* m_pTimer;
};

class ReportPushSession;

class CReportPushThread : public CThread
{
    friend class ReportPushSession;

    typedef std::map<UInt64, ReportPushSession*> SessionMap;
    typedef std::map<string, SmsAccount> AccountMap;
public:
    CReportPushThread(const char *name);
    ~CReportPushThread();
    bool Init();
    UInt32 GetSessionMapSize();

private:
    void initSysPara(const map<string, string>& mapSysPara);

    void HandleMsg(TMsg* pMsg);
    void MainLoop();
    void HandleReportReciveTOReportPushReq(TMsg* pMsg);
    void HandleHttpResponseMsg(TMsg* pMsg);
    void HandleAccountUpdateReq(TMsg* pMsg);
    void HandleHostIpResp(int iResult,UInt32 uIp,UInt64 iSeq);
    void HandleTimeOutMsg(TMsg* pMsg);
    void HandleRedisResp(TMsg* pMsg);
    void HandleMoMsg(TMsg* pMsg);
    void UpdateRecord(string strIDs, UInt32 uIdentify, string strDate, ReportPushSession* rpRecv);

    void CheckState(SessionMap::iterator it);

    void CheckState_2(SessionMap::iterator itr);
    void TransformDescForTX(string& strDesc);
    void HandleHttpsResponseMsg( TMsg* pMsg );

    void checkRePush(ReportPushSession* pSession);

    void handleReportRePushReq(TMsg* pMsg);

private:
    SnManager m_SnMng;
    SessionMap m_mapSession;

    InternalService* m_pInternalService;

    map<UInt64, RedisListReqSession*> m_redisListReqMap;            //useq ---RedisKey
    AccountMap m_AccountMap;
    SmspUtils m_SmspUtils;

    map<string, string> m_mapReportErrCode_Desc;
    UInt32 m_uiHttpsReportRetry;

    ComponentConfig m_componentCfg;
    MqConfigMap m_mapMqCfg;
};

class ReportPushSession
{
public:
    ReportPushSession()
    {
        init();
    }

    ReportPushSession(CReportPushThread* pThread)
    {
        init();
        m_pThread = pThread;
        m_ulSeq = m_pThread->m_SnMng.getSn();
    }

    ~ReportPushSession()
    {
        SAFE_DELETE(m_pTimer);

        if (m_pHttpSender != NULL)
        {
            m_pHttpSender->Destroy();
            SAFE_DELETE(m_pHttpSender);
        }
    }

    void init()
    {
        m_pHttpSender = NULL;
        m_pTimer = NULL;
        m_iLinkId = 0;
        m_iType = 0;
        m_iStatus = 0;
        m_lReportTime = 0;
        m_uIp =0;
        m_uSmsfrom = 0;
        //m_bHostIPstate = false;
        //m_bRedisRetrunFlag = false;
        m_uNeedReport = 0;
        m_uNeedMo = 0;
        m_uUpdateFlag = 0;
        m_uIdentify =0;
        m_uChannelCount = 1;
        m_pThread = NULL;
        m_ulSeq = 0;
        m_uiRetryCount = 0;
    }

    http::HttpSender* m_pHttpSender;
    CThreadWheelTimer* m_pTimer;

    // 0: no, do not need report
    // 1: yes, need simple report
    // 2: yes, need source report
    // 3: yes, https users get report by itself
    UInt32 m_uNeedReport;

    string m_strDeliveryUrl;
    string m_strSmsId;  //report , 对于upstream 是moid
    string m_strDesc;   //
    string m_strReportDesc;
    string m_strInnerErrcode;
    string m_strPhone;  //
    string m_strContent;    //UPSTREAM
    string m_strUpstreamTime;
    string m_strClientId;   //report
    string m_strSrcId;
    int    m_iLinkId;

    // 1: report push
    // 2: upstream message
    int    m_iType;

    int    m_iStatus;
    Int64  m_lReportTime;
    UInt32 m_uIp;
    UInt32 m_uSmsfrom;
    string m_strUid;
    UInt32 m_uNeedMo;     //upstream    0：no，1：yes       3:user get mo
    string m_strMourl;
    string m_strSign;       //upstream
    string m_strSignPort;       //upstream 暂时没用
    string m_strUserPort;       ////upstream up for user
    string m_strTime;           //upstream time
    UInt32 m_uUpdateFlag;       //// 0 update, 1 no update
    UInt32 m_uIdentify;
    UInt32 m_uChannelCount;
    string m_strSourceExtend;     // add for tengxun cloud "extend"
    //bool m_bHostIPstate;      //hostip is ok
    //bool m_bRedisRetrunFlag;  //redis return ok

    CReportPushThread* m_pThread;
    UInt64 m_ulSeq;
    string m_strData;
    string m_strUrl;
    UInt32 m_uiRetryCount;
};

#endif ///__CACCESSREPORTPUSHTHTRAD__
