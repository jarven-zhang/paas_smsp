#ifndef C_SMSERVERTHREAD_H_
#define C_SMSERVERTHREAD_H_

#include <list>
#include <map>
#include <time.h>
#include "Thread.h"
#include "internalserversocket.h"
#include "SnManager.h"
#include "PhoneDao.h"
#include "CRedisThread.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "Comm.h"
#include <boost/noncopyable.hpp>
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/date_time/posix_time/time_parsers.hpp"
#include "boost/date_time/posix_time/conversion.hpp"


class MqC2sDbAuditContentReqQueMsg : public TMsg
{
public:
    typedef enum MsgType
    {
        CommonMsg = 0,
        GroupSendLimMsg = 1,
    }MsgType;

public:
    MqC2sDbAuditContentReqQueMsg()
    {
        ucSmsType_ = INVALID_UINT8;
        ucPayType_ = INVALID_UINT8;
        ucOperatorType_ = INVALID_UINT8;

        uiGroupSendLimUserFlag_ = 0;
        uiGroupSendLimTime_ = 0;
        uiGroupSendLimNum_ = 0;
        eMsgType_ = CommonMsg;
        uiCsDataTimestamp_ = 0;
        uiPhoneCount_ = 0;
    }

    MqC2sDbAuditContentReqQueMsg& operator=(const MqC2sDbAuditContentReqQueMsg& mqMsg)
    {
        strClientId_= mqMsg.strClientId_;
        strUserName_= mqMsg.strUserName_;
        strSmsId_= mqMsg.strSmsId_;
        strPhone_= mqMsg.strPhone_;
        strSign_= mqMsg.strSign_;
        strContent_= mqMsg.strContent_;
        strAuditContent_= mqMsg.strAuditContent_;
        ucSmsType_= mqMsg.ucSmsType_;
        ucPayType_= mqMsg.ucPayType_;
        ucOperatorType_ = mqMsg.ucOperatorType_;
        strCsDate_= mqMsg.strCsDate_;
        uiGroupSendLimUserFlag_= mqMsg.uiGroupSendLimUserFlag_;
        uiGroupSendLimTime_= mqMsg.uiGroupSendLimTime_;
        uiGroupSendLimNum_= mqMsg.uiGroupSendLimNum_;
        eMsgType_= mqMsg.eMsgType_;
        uiCsDataTimestamp_ = mqMsg.uiCsDataTimestamp_;
        uiPhoneCount_ = mqMsg.uiPhoneCount_;

        return *this;
    }

    void converCsDate2Timesatmp()
    {
        // csdate=20180201171601
        if (14 != strCsDate_.size())
            return;

        using namespace boost::posix_time;

        std::string str(strCsDate_);
        str.insert(8, 1, 'T');

        ptime pt(from_iso_string(str));
        struct tm tm1 = to_tm(pt);
        time_t t = mktime(&tm1);
        uiCsDataTimestamp_ = (-1 == t) ? 0 : t;
    }

    inline void getPhoneCount()
    {
        if (strPhone_.empty())
            return;

        vector<string> vecPhones;
        Comm::splitExVector(strPhone_, ",", vecPhones);
        uiPhoneCount_ = vecPhones.size();
    }

    void print();

public:
    // mq message field
    std::string strClientId_;
    std::string strUserName_;
    std::string strSmsId_;
    std::string strPhone_;
    std::string strSign_;
    std::string strContent_;
    std::string strAuditContent_;
    UInt8 ucSmsType_;
    UInt8 ucPayType_;
    UInt8 ucOperatorType_;
    std::string strCsDate_;
    UInt32 uiGroupSendLimUserFlag_;
    UInt32 uiGroupSendLimTime_;
    UInt32 uiGroupSendLimNum_;

    // other field
    MsgType eMsgType_; // 0:common message 1:groupsendlimit message
    UInt32 uiCsDataTimestamp_;   // the original time of smsp_c2s or smsp_http
    UInt32 uiPhoneCount_;

private:
    MqC2sDbAuditContentReqQueMsg(const MqC2sDbAuditContentReqQueMsg&);
};


class TblSmsGroupSendAudit : public boost::noncopyable
{
public:
    typedef enum AuditResult
    {
        AuditResult_Pass                            = 1,
        AuditResult_Not_Pass                        = 2,
        AuditResult_AuditPass_To_ManualExpire       = 3,
        AuditResult_AuditNotPass_To_ManualExpire    = 4,
        AuditResult_NotAudit_To_SystemExpire        = 5,
        AuditResult_GroupSendLim_Release_To_Send    = 6,
        AuditResult_GroupSendLim_Release_To_Audit   = 7,
        AuditResult_AuditPass_To_SystemExpire       = 8,
        AuditResult_AuditNotPass_To_SystemExpire    = 9
    }AuditResult;

public:
    TblSmsGroupSendAudit()
    {
        uiGroupSendLimUserFlag_ = 0;
        uiGroupSendLimFlag_ = 0;
    }

public:
    std::string strAuditId_;
    std::string strCreateTime_;
    std::string strMd5_;
    std::string strAuditTime_;
    UInt32 uiStatus_;
    UInt32 uiSendNum_;
    UInt32 uiGroupSendLimStartTime_;
    UInt32 uiGroupSendLimEndTime_;
    UInt32 uiUnlimitState_;
    UInt32 uiGroupSendLimUserFlag_;
    UInt32 uiGroupSendLimTime_;
    UInt32 uiGroupSendLimNum_;

    AuditResult eStatus_; // for mq
    UInt32 uiGroupSendLimFlag_; // for mq
};

class NeedAudit
{
public:
    NeedAudit(){}
    NeedAudit(const std::string& strClientId, const std::string& strAuditId)
    : strClientId_(strClientId), strAuditId_(strAuditId){}

public:
    std::string strClientId_;
    std::string strAuditId_;
};

class AuditFailedSql
{
public:
    std::string m_strSql;
    std::string m_strMd5;
};

class  CAuditServiceThread : public CThread
{
    class SessionInfo : public boost::noncopyable
    {
    public:
        SessionInfo()
        {
            m_pWheelTime_ = NULL;
            uiRefCount_ = 0;
        }

        ~SessionInfo()
        {
            destoryTimer();
        }

        inline void setMqMsg(const MqC2sDbAuditContentReqQueMsg& mqMsg)
        {
            mqMsg_ = mqMsg;
        }

        inline void getMode()
        {
            if (MqC2sDbAuditContentReqQueMsg::CommonMsg == mqMsg_.eMsgType_)
            {
                strRedisEndNameSpace_ = "endauditsms";
                strRedisNeedNameSpace_ = "needauditsms";
                strRedisEndKey_ = strRedisEndNameSpace_ + ":" + mqMsg_.strAuditContent_;
                strRedisNeedKey_ = strRedisNeedNameSpace_ + ":" + mqMsg_.strAuditContent_;
                strExpireTime_ = "172800";
                strAuditIdPrefix_ = "0";

            }
            else if (MqC2sDbAuditContentReqQueMsg::GroupSendLimMsg == mqMsg_.eMsgType_)
            {
                strRedisEndNameSpace_ = "endgroupsendauditsms";
                strRedisNeedNameSpace_ = "needgroupsendauditsms";
                strRedisEndKey_ = strRedisEndNameSpace_ + ":" + mqMsg_.strAuditContent_;
                strRedisNeedKey_ = strRedisNeedNameSpace_ + ":" + mqMsg_.strAuditContent_;
                strExpireTime_ = "86400"; // because the key(0208_md5(32)) has the date and time, so only save 24 hours
                strAuditIdPrefix_ = "1";
            }
        }

        inline void destoryTimer()
        {
            SAFE_DELETE(m_pWheelTime_);
        }

    public:
        MqC2sDbAuditContentReqQueMsg mqMsg_;
        CThreadWheelTimer* m_pWheelTime_;

        // other variate
        string strSql_;
        string strAuditId_;
        UInt32 uiRefCount_;

        string strRedisEndNameSpace_;
        string strRedisNeedNameSpace_;
        string strRedisEndKey_;
        string strRedisNeedKey_;
        string strAuditIdPrefix_;
        string strExpireTime_;
    };

    typedef std::map<std::string, NeedAudit> NeedAuditMap;
    typedef std::map<std::string, NeedAudit>::iterator NeedAuditMapIter;

    typedef std::map<std::string, std::string> AuditFailedSqlMap;
    typedef std::map<std::string, std::string>::iterator AuditFailedSqlMapIter;

    typedef void (CAuditServiceThread::*WorkFunc)(TMsg* pMsg);

    typedef std::map<int, WorkFunc> WorkFuncMap;
    typedef std::map<int, WorkFunc>::const_iterator WorkFuncMapIter;

    typedef std::map<UInt64, SessionInfo*> SessionInfoMap;
    typedef std::map<UInt64, SessionInfo*>::const_iterator SessionInfoMapIter;


public:
    CAuditServiceThread(const char *name);
    virtual ~CAuditServiceThread();
    bool Init();
    UInt32 GetSessionMapSize();

private:
    void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);
    void HandleAuditServiceAcceptSocketMsg(TMsg* pMsg);
    void handleAuditServiceReqMsg(TMsg* pMsg);
    void HandleAuditServiceReturnOverMsg(TMsg* pMsg);
    void handleQueryDbRespMsg(TMsg* pMsg);

    void handleRedisResp(TMsg* pMsg);
    void handleRedisGetEndAuditSmsResp(TMsg* pMsg);
    void handleRedisGetNeedAuditSmsResp(TMsg* pMsg);
    void handleRedisSetNeedAuditSmsResp(TMsg* pMsg);

    void handleDBResponseMsg(TMsg* pMsg);

    void handleUpdateSystemParaMsg(TMsg* pMsg);
    void getSysPara(std::map<std::string, std::string>& mapSysPara);

    void handleUpdateMqConfigMsg(TMsg* pMsg);
    void handleUpdateComponentRefMqMsg(TMsg* pMsg);
    bool updateMqConfig();

    void handleTimerOutMsg(TMsg* pMsg);
    void handleAuditExpireTimerMsg(TMsg* pMsg, CThreadWheelTimer*& pTimer);
    void handleGroupSendLimReleaseMsg(TMsg* pMsg);

    void dbInsertProcess(SessionInfo* pSession);
    void dbUpdateProcess(SessionInfo* pSession);
    void dbUpdateProcess(const TblSmsGroupSendAudit& obj);
    void mqProduceProcess(const TblSmsGroupSendAudit& obj);
    bool redisProcess(UInt64 ulSeq, const string& strSession, const string& strCmd);
    bool redisExpireProcess(const string& strKey, const string& strExpireTime);

private:
    WorkFuncMap m_mapWorkFunc;
    SessionInfoMap m_mapSession;

    NeedAuditMap m_mapNeedAudit;
    SnManager m_SnManager;

    CThreadWheelTimer* m_pAuditExpireCheckTimer;
    CThreadWheelTimer* m_pFailedSqlHandleTimer;
    CThreadWheelTimer* m_pGroupSendLimAuditExpireTimer;
    CThreadWheelTimer* m_pGroupSendLimReleaseTimer;

    UInt32 m_uiAuditExpireTime;
    UInt32 m_uiAuditCheckTimeInterval;
    UInt32 m_uiGroupSendLimAuditExpireTime;
    UInt32 m_uiGroupsendLimReleaseFrequency;

//    std::list<AuditFailedSql> m_listSql;
    AuditFailedSqlMap m_mapAuditFailedSql;

    std::string m_strMQDBExchange;
    std::string m_strMQDBRoutingKey;

    std::map<UInt64, MqConfig> m_mapMqConfig;
    std::map<std::string, ComponentRefMq> m_mapComponentRefMq;
};




#endif /* C_SMSERVERTHREAD_H_ */

