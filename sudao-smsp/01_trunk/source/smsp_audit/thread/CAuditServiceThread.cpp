#include "CAuditServiceThread.h"
#include <stdlib.h>
#include <iostream>
#include <mysql.h>
#include "Comm.h"
#include "main.h"
#include "CThreadSQLHelper.h"
#include "CEmojiString.h"
#include "CMQProducerThread.h"
#include "Uuid.h"
#include "boost/foreach.hpp"

using namespace std;

const UInt64 AUDIT_EXPIRE_CHECK_TIMER_ID = 11;
const UInt64 ERROR_SQL_HANDLE_TIMER_ID = 12;
const UInt64 GROUPSENDLIM_AUDIT_EXPIRE_CHECK_TIMER_ID = 13;
const UInt64 GROUPSENDLIM_RELEASE_TIMER_ID = 14;

#define foreach BOOST_FOREACH

#define CHK_INT_BRK(var, min, max)                                          \
        if (((var) < (min)) || ((var) > (max)))                             \
        {                                                                   \
            LogError("Invalid system parameter(%s) value(%s), %s(%d).",     \
                strSysPara.c_str(),                                         \
                iter->second.c_str(),                                       \
                VNAME(var),                                                 \
                var);                                                       \
            break;                                                          \
        }

void MqC2sDbAuditContentReqQueMsg::print()
{
    LogDebug("strClientId_(%s)", strClientId_.data());
    LogDebug("strUserName_(%s)", strUserName_.data());
    LogDebug("strSmsId_(%s)", strSmsId_.data());
    LogDebug("strPhone_(%s)", strPhone_.data());
    LogDebug("strSign_(%s)", strSign_.data());
    LogDebug("strContent_(%s)", strContent_.data());
    LogDebug("strAuditContent_(%s)", strAuditContent_.data());
    LogDebug("ucSmsType_(%u)", ucSmsType_);
    LogDebug("ucPayType_(%u)", ucPayType_);
    LogDebug("ucOperatorType_(%u)", ucOperatorType_);
    LogDebug("strCsDate_(%s)", strCsDate_.data());
    LogDebug("uiGroupSendLimUserFlag_(%u)", uiGroupSendLimUserFlag_);
    LogDebug("uiGroupSendLimTime_(%u)", uiGroupSendLimTime_);
    LogDebug("uiGroupSendLimNum_(%u)", uiGroupSendLimNum_);
    LogDebug("eMsgType_(%u)", eMsgType_);
    LogDebug("uiCsDataTimestamp_(%u)", uiCsDataTimestamp_);
}


CAuditServiceThread::CAuditServiceThread(const char *name      ):
    CThread(name)
{
    m_uiAuditExpireTime = 60 * 60;
    m_uiAuditCheckTimeInterval = 5 * 60;

    m_uiGroupSendLimAuditExpireTime = 60 * 60; // minute to second
    m_uiGroupsendLimReleaseFrequency = 3; // second

    m_mapWorkFunc[MSGTYPE_AUDIT_SERVICE_REQ] = &CAuditServiceThread::handleAuditServiceReqMsg;
    m_mapWorkFunc[MSGTYPE_TIMEOUT] = &CAuditServiceThread::handleTimerOutMsg;
    m_mapWorkFunc[MSGTYPE_DB_QUERY_RESP] = &CAuditServiceThread::handleQueryDbRespMsg;
    m_mapWorkFunc[MSGTYPE_REDIS_RESP] = &CAuditServiceThread::handleRedisResp;
    m_mapWorkFunc[MSGTYPE_DB_NOTQUERY_RESP] = &CAuditServiceThread::handleDBResponseMsg;
    m_mapWorkFunc[MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ] = &CAuditServiceThread::handleUpdateSystemParaMsg;
    m_mapWorkFunc[MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ] = &CAuditServiceThread::handleUpdateMqConfigMsg;
    m_mapWorkFunc[MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ] = &CAuditServiceThread::handleUpdateComponentRefMqMsg;
}

CAuditServiceThread::~CAuditServiceThread()
{
}

bool CAuditServiceThread::Init()
{
    if (!CThread::Init())
    {
        LogNoticeP("CThread::Init is failed.");
        return false;
    }

    CHK_NULL_RETURN_FALSE(g_pRuleLoadThread);
    g_pRuleLoadThread->getNeedAuditMap(m_mapNeedAudit);

    map<string, string> mapSysParam;
    g_pRuleLoadThread->getSysParamMap(mapSysParam);
    getSysPara(mapSysParam);

    g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
    g_pRuleLoadThread->getComponentRefMq(m_mapComponentRefMq);

    if (!updateMqConfig())
    {
        LogNoticeP("Call updateMqConfig failed. You need config producer for MQ(c2s_db).");
        return false;
    }

    m_pAuditExpireCheckTimer = SetTimer(AUDIT_EXPIRE_CHECK_TIMER_ID, "", 5 * 1000); // check once in 5 seconds after startup.
    m_pGroupSendLimAuditExpireTimer = SetTimer(GROUPSENDLIM_AUDIT_EXPIRE_CHECK_TIMER_ID, "", 5 * 1000);
    m_pFailedSqlHandleTimer = SetTimer(ERROR_SQL_HANDLE_TIMER_ID, "", 10 * 1000);
    m_pGroupSendLimReleaseTimer = SetTimer(GROUPSENDLIM_RELEASE_TIMER_ID, "", m_uiGroupsendLimReleaseFrequency * 1000);

    return true;
}

void CAuditServiceThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            usleep(g_uSecSleep);
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}

void CAuditServiceThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    WorkFuncMapIter iter;
    CHK_MAP_FIND_INT_RET(m_mapWorkFunc, iter, pMsg->m_iMsgType);

    WorkFunc func = iter->second;
    CHK_NULL_RETURN(func);

    (this->*func)(pMsg);
}

void CAuditServiceThread::handleUpdateSystemParaMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
    getSysPara(pSysParamReq->m_SysParamMap);
}

void CAuditServiceThread::getSysPara(std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::iterator iter;

    do
    {
        strSysPara = "AUDIT";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        Comm comm;
        std::vector<std::string> paramslist;
        comm.splitExVector(iter->second, ";", paramslist);
        if (paramslist.size() != 5)
        {
            LogError("Invalid system parameter(%s) value(%s).", strSysPara.data(), iter->second.data());
            break;
        }

        Int32 iAuditExpireTime = atoi(paramslist[1].data());
        Int32 iAuditCheckTimeInterval = atoi(paramslist[2].data());
        Int32 iGroupSendLimAuditExpireTime = atoi(paramslist[3].data());
        Int32 iGroupsendReleaseLimitFrequency = atoi(paramslist[4].data());

        CHK_INT_BRK(iAuditExpireTime, 30, 5000);
        CHK_INT_BRK(iAuditCheckTimeInterval, 5, 300);
        CHK_INT_BRK(iGroupSendLimAuditExpireTime, 30, 5000);
        CHK_INT_BRK(iGroupsendReleaseLimitFrequency, 1, 5);

        m_uiAuditExpireTime = iAuditExpireTime * 60;    // minute to second
        m_uiAuditCheckTimeInterval = iAuditCheckTimeInterval * 60;
        m_uiGroupSendLimAuditExpireTime = iGroupSendLimAuditExpireTime * 60;
        m_uiGroupsendLimReleaseFrequency = iGroupsendReleaseLimitFrequency; // second
    }
    while (0);

    LogNotice("System parameter(%s) value(m_uAuditExpire[%u]sec, m_uiAuditCheckTimeInterval[%u]sec, "
        "m_uiGroupSendLimAuditExpireTime[%u]sec, m_uiGroupsendLimReleaseFrequency[%u]sec).",
        strSysPara.c_str(),
        m_uiAuditExpireTime,
        m_uiAuditCheckTimeInterval,
        m_uiGroupSendLimAuditExpireTime,
        m_uiGroupsendLimReleaseFrequency);
}

void CAuditServiceThread::handleUpdateMqConfigMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    TUpdateMqConfigReq* pMqConfig = (TUpdateMqConfigReq*)pMsg;
    m_mapMqConfig = pMqConfig->m_mapMqConfig;

    updateMqConfig();
}

void CAuditServiceThread::handleUpdateComponentRefMqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    TUpdateComponentRefMqReq* pComponentRefMq = (TUpdateComponentRefMqReq*)pMsg;
    m_mapComponentRefMq = pComponentRefMq->m_componentRefMqInfo;

    updateMqConfig();
}

bool CAuditServiceThread::updateMqConfig()
{
    string strKey;
    strKey.append(Comm::int2str(g_uComponentId));
    strKey.append("_");
    strKey.append(MESSAGE_CONTENT_AUDIT_RESULT);
    strKey.append("_");
    strKey.append(Comm::int2str(PRODUCT_MODE));

    map<string, ComponentRefMq>::iterator iterComponentRefMq;
    CHK_MAP_FIND_STR_RET_FALSE(m_mapComponentRefMq, iterComponentRefMq, strKey);

    map<UInt64, MqConfig>::iterator iterMqConfig;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapMqConfig, iterMqConfig, iterComponentRefMq->second.m_uMqId);

    if (m_strMQDBExchange != iterMqConfig->second.m_strExchange)
    {
        m_strMQDBExchange = iterMqConfig->second.m_strExchange;
    }

    if (m_strMQDBRoutingKey != iterMqConfig->second.m_strRoutingKey)
    {
        m_strMQDBRoutingKey = iterMqConfig->second.m_strRoutingKey;
    }

    if (m_strMQDBExchange.empty() || m_strMQDBRoutingKey.empty())
    {
        LogError("MQDBExchange(%s) or MQDBRoutingKey(%s) is empty()",
            m_strMQDBExchange.data(),
            m_strMQDBRoutingKey.data());

        return false;
    }

    LogNotice("MQDBExchange(%s), MQDBRoutingKey(%s).", m_strMQDBExchange.data(), m_strMQDBRoutingKey.data());
    return true;
}

void CAuditServiceThread::handleAuditServiceReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    MqC2sDbAuditContentReqQueMsg* pQueMsg = static_cast<MqC2sDbAuditContentReqQueMsg*>(pMsg);
    SessionInfo* pSession = new SessionInfo();
    CHK_NULL_RETURN(pSession);

    pSession->mqMsg_.m_iSeq = m_SnManager.getSn();
    pSession->setMqMsg(*pQueMsg);
    pSession->getMode();
    m_mapSession[pSession->mqMsg_.m_iSeq] = pSession;

    string strCmd;
    strCmd.append("HGETALL ");
    strCmd.append(pSession->strRedisEndKey_);

    string strSession;
    strSession.append("check ");
    strSession.append(pSession->strRedisEndNameSpace_);

    if (redisProcess(pSession->mqMsg_.m_iSeq, strSession, strCmd))
    {
//        pSession->m_pWheelTime_ = SetTimer(pSession->mqMsg_.m_iSeq, "", 15 * 1000);
    }
}

void CAuditServiceThread::handleRedisResp(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    LogDebug("redis session(%s).", pMsg->m_strSessionID.data());

    if (("check endauditsms" == pMsg->m_strSessionID)
      ||("check endgroupsendauditsms" == pMsg->m_strSessionID))
    {
        handleRedisGetEndAuditSmsResp(pMsg);
    }
    else if (("check needauditsms" == pMsg->m_strSessionID)
          || ("check needgroupsendauditsms" == pMsg->m_strSessionID))
    {
        handleRedisGetNeedAuditSmsResp(pMsg);
    }
    else if (("set needauditsms" == pMsg->m_strSessionID)
          || ("set needgroupsendauditsms" == pMsg->m_strSessionID))
    {
        handleRedisSetNeedAuditSmsResp(pMsg);
    }
    else
    {
        LogError("Invalid SessionID(%s).", pMsg->m_strSessionID.data());
    }

    TRedisResp* pResp = static_cast<TRedisResp*>(pMsg);
    freeReply(pResp->m_RedisReply);
}

void CAuditServiceThread::handleRedisGetEndAuditSmsResp(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    std::map<UInt64, SessionInfo*>::iterator sessionIter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, sessionIter, pMsg->m_iSeq);

    SessionInfo* pSession = sessionIter->second;
    CHK_NULL_RETURN(pSession);

//    pSession->destoryTimer();

    TRedisResp* pResp = static_cast<TRedisResp*>(pMsg);
    redisReply* pRedisReply = pResp->m_RedisReply;

    if (NULL != pRedisReply) //found
    {
        //get endaudit redis.
        if ((pRedisReply->type == REDIS_REPLY_ARRAY) && (pRedisReply->elements > 0))
        {
            //have already audited, do nothing
            LogDebug("===Already audited===, do nothing, md5[%s], smsid[%s], phone[%s], ",
                pSession->mqMsg_.strAuditContent_.data(),
                pSession->mqMsg_.strSmsId_.data(),
                pSession->mqMsg_.strPhone_.data());
			m_mapSession.erase( sessionIter );
			SAFE_DELETE( pSession );
            return;
        }
        else
        {
            LogNotice("have not set redis maybe,md5[%s], smsid[%s], phone[%s]",
                pSession->mqMsg_.strAuditContent_.data(),
                pSession->mqMsg_.strSmsId_.data(),
                pSession->mqMsg_.strPhone_.data());
        }
    }
    else
    {
        LogError("query redis fail md5[%s], smsid[%s], phone[%s]",
            pSession->mqMsg_.strAuditContent_.data(),
            pSession->mqMsg_.strSmsId_.data(),
            pSession->mqMsg_.strPhone_.data());
    }

    string strCmd;
    strCmd.append("HGETALL ");
    strCmd.append(pSession->strRedisNeedKey_);

    string strSession;
    strSession.append("check ");
    strSession.append(pSession->strRedisNeedNameSpace_);

    if (redisProcess(pSession->mqMsg_.m_iSeq, strSession, strCmd))
    {
//        pSession->m_pWheelTime_ = SetTimer(pSession->mqMsg_.m_iSeq, "", 15 * 1000);
    }
}

void CAuditServiceThread::handleRedisGetNeedAuditSmsResp(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    SessionInfoMapIter sessionIter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, sessionIter, pMsg->m_iSeq);

    SessionInfo* pSession = sessionIter->second;
    CHK_NULL_RETURN(pSession);

//    pSession->destoryTimer();


    UInt8 ucOperation = 0; //default
    TRedisResp* pResp = static_cast<TRedisResp*>(pMsg);
    redisReply* pRedisReply = pResp->m_RedisReply;

     if((NULL != pRedisReply) && (pRedisReply->type == REDIS_REPLY_ARRAY && pRedisReply->elements > 0) )
     {
         LogDebug("have set needauditsms redis, update table. md5[%s], smsid[%s], phone[%s].",
             pSession->mqMsg_.strAuditContent_.data(),
             pSession->mqMsg_.strSmsId_.data(),
             pSession->mqMsg_.strPhone_.data());

         ucOperation = 2; // update
         paserReply("auditid", pRedisReply, pSession->strAuditId_);
     }
     else
     {
         //find needaudit map.
         NeedAuditMap::iterator it = m_mapNeedAudit.find(pSession->mqMsg_.strAuditContent_);
         if (it != m_mapNeedAudit.end())
         {
             LogDebug("in the memory, update table. md5[%s], smsid[%s], phone[%s].",
                 pSession->mqMsg_.strAuditContent_.data(),
                 pSession->mqMsg_.strSmsId_.data(),
                 pSession->mqMsg_.strPhone_.data());

             ucOperation = 2; // update
             pSession->strAuditId_ = it->second.strAuditId_;
         }
         else
         {
             LogDebug("not in the memory, insert table. md5[%s], smsid[%s], phone[%s].",
                 pSession->mqMsg_.strAuditContent_.data(),
                 pSession->mqMsg_.strSmsId_.data(),
                 pSession->mqMsg_.strPhone_.data());

            ucOperation = 1; // insert
         }
    }

    if (1 == ucOperation)
    {
        dbInsertProcess(pSession);
    }
    else if (2 == ucOperation)
    {
        dbUpdateProcess(pSession);

        m_mapSession.erase(pMsg->m_iSeq);
        SAFE_DELETE(pSession);
    }
}

void CAuditServiceThread::handleRedisSetNeedAuditSmsResp(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    SessionInfoMapIter sessionIter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, sessionIter, pMsg->m_iSeq);

    SessionInfo* pSession = sessionIter->second;
    CHK_NULL_RETURN(pSession);

    NeedAuditMapIter it = m_mapNeedAudit.find(pSession->mqMsg_.strAuditContent_);
    if (it != m_mapNeedAudit.end())
    {
        m_mapNeedAudit.erase(it);
        LogDebug("del m_mapNeedAudit. md5(%s).", pSession->mqMsg_.strAuditContent_.data());
    }

    TRedisResp* pResp = (TRedisResp*)pMsg;

    if (NULL != pResp->m_RedisReply)
    {
        if ((pResp->m_RedisReply->type == REDIS_REPLY_STATUS) && (strcmp(pResp->m_RedisReply->str, "OK") == 0))
        {
            LogDebug("set needauditsms:%s success.", pSession->mqMsg_.strAuditContent_.data());
        }
        else
        {
            LogError("set needauditsms:%s failed. but make no difference.", pSession->mqMsg_.strAuditContent_.data());
        }
    }
    else
    {
        LogError("set needauditsms:%s failed. redis return null.", pSession->mqMsg_.strAuditContent_.data());
    }

    if (0 == --pSession->uiRefCount_)
    {
        SAFE_DELETE(pSession);
        m_mapSession.erase(pMsg->m_iSeq);
        LogDebug("delete session(%lu).", pMsg->m_iSeq);
    }
}


void CAuditServiceThread::handleDBResponseMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    LogDebug("sequence(%lu).", pMsg->m_iSeq);

    bool bResult = true;
    TDBQueryResp* pAuditResult = (TDBQueryResp*)pMsg;
    LogDebug("affectRow(%d), result(%d).", pAuditResult->m_iAffectRow, pAuditResult->m_iResult);

    if ((-1 == pAuditResult->m_iAffectRow) || (0 == pAuditResult->m_iAffectRow))
    {
        if ((2006 == pAuditResult->m_iResult) || (2013 == pAuditResult->m_iResult) || (2000 == pAuditResult->m_iResult))
        {
            bResult = false;
        }
    }


    if (ERROR_SQL_HANDLE_TIMER_ID == pMsg->m_iSeq)
    {
        if (bResult)
        {
            LogDebug("timer retry insert success.");

            AuditFailedSqlMapIter iter = m_mapAuditFailedSql.find(pMsg->m_strSessionID);
            if (m_mapAuditFailedSql.end() != iter)
            {
                m_mapAuditFailedSql.erase(iter);
            }
        }

        if (m_pFailedSqlHandleTimer == NULL)
        {
            int sqlCnt = m_mapAuditFailedSql.size();
            int timerlen = (sqlCnt < 100) ? (10*1000) : ((sqlCnt/10 + sqlCnt%10) * 1000);

            m_pFailedSqlHandleTimer = SetTimer(ERROR_SQL_HANDLE_TIMER_ID, "", timerlen);

            LogDebug("set %d second timer.", timerlen/1000);
        }
    }
    else
    {
        SessionInfoMapIter iter;
        CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);

        SessionInfo* pSession = iter->second;
        CHK_NULL_RETURN(pSession);

        if (bResult)
        {
            LogDebug("insert success.");
        }
        else
        {
            m_mapAuditFailedSql[pSession->strAuditId_] = pSession->strSql_;

            LogDebug("insert failed, save failed sql, auditid(%s), sql(%s).",
                pSession->strAuditId_.data(),
                pSession->strSql_.data());
        }

        if (0 == --pSession->uiRefCount_)
        {
            SAFE_DELETE(pSession);
            m_mapSession.erase(pMsg->m_iSeq);
            LogDebug("delete session, sequence(%lu).", pMsg->m_iSeq);
        }
    }
}

void CAuditServiceThread::handleTimerOutMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    LogDebug("Sequence(%lu), SessionID(%s).", pMsg->m_iSeq, pMsg->m_strSessionID.data());

    if (AUDIT_EXPIRE_CHECK_TIMER_ID == pMsg->m_iSeq)
    {
        SAFE_DELETE(m_pAuditExpireCheckTimer);

        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "select * from t_sms_audit where smsp_audit_id = %lu and removeflag = 0 and "
            "((status = 0 and TIMESTAMPDIFF(HOUR,createtime,NOW()) > 48) "
            "or (status in(1,2) and TIMESTAMPDIFF(SECOND,audittime,NOW()) >= %u)) limit 1000;",
            g_uComponentId,
            m_uiAuditExpireTime);

        LogDebug("sql(%s).", sql);

        TDBQueryReq* pDBQueryReq = new TDBQueryReq();
        pDBQueryReq->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
        pDBQueryReq->m_iSeq = pMsg->m_iSeq;
        pDBQueryReq->m_pSender = this;
        pDBQueryReq->m_SQL.assign(sql);

        if (!g_pDisPathDBThreadPool->PostMsg(pDBQueryReq))
        {
            LogDebug("Call PostMsg failed.");
            m_pAuditExpireCheckTimer = SetTimer(pMsg->m_iSeq, "", m_uiAuditCheckTimeInterval * 1000);
        }
    }
    else if (GROUPSENDLIM_AUDIT_EXPIRE_CHECK_TIMER_ID == pMsg->m_iSeq)
    {
        SAFE_DELETE(m_pGroupSendLimAuditExpireTimer);

        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "select * from t_sms_groupsend_audit where smsp_audit_id = %lu and removeflag = 0 and "
            "((status = 0 and TIMESTAMPDIFF(HOUR,createtime,NOW()) > 48) "
            "or (status in(1,2) and TIMESTAMPDIFF(SECOND,audittime,NOW()) >= %u)) limit 1000;",
            g_uComponentId,
            m_uiGroupSendLimAuditExpireTime);

        LogDebug("sql(%s).", sql);

        TDBQueryReq* pDBQueryReq = new TDBQueryReq();
        pDBQueryReq->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
        pDBQueryReq->m_iSeq = pMsg->m_iSeq;
        pDBQueryReq->m_pSender = this;
        pDBQueryReq->m_SQL.assign(sql);

        if (!g_pDisPathDBThreadPool->PostMsg(pDBQueryReq))
        {
            LogDebug("Call PostMsg failed.");
            m_pGroupSendLimAuditExpireTimer = SetTimer(pMsg->m_iSeq, "", m_uiAuditCheckTimeInterval * 1000);
        }
    }
    else if (GROUPSENDLIM_RELEASE_TIMER_ID == pMsg->m_iSeq)
    {
        SAFE_DELETE(m_pGroupSendLimReleaseTimer);

        char sql[1024] = {0};
        snprintf(sql, sizeof(sql), "select * from t_sms_groupsend_audit where smsp_audit_id = %lu and "
            "removeflag = 0 and status = 0 and unlimit_state = 0 and ((sendnum >= groupsendlim_num) or "
            "((sendnum < groupsendlim_num) and (groupsendlim_userflag = 0) and "
            "(%u - groupsendlim_stime > groupsendlim_time))) limit 1000;",
            g_uComponentId,
            (UInt32)time(NULL));

        LogDebug("sql(%s).", sql);

        TDBQueryReq* pDBQueryReq = new TDBQueryReq();
        pDBQueryReq->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
        pDBQueryReq->m_iSeq = pMsg->m_iSeq;
        pDBQueryReq->m_pSender = this;
        pDBQueryReq->m_SQL.assign(sql);

        if (!g_pDisPathDBThreadPool->PostMsg(pDBQueryReq))
        {
            LogDebug("Call PostMsg failed.");
            m_pGroupSendLimReleaseTimer = SetTimer(pMsg->m_iSeq, "", m_uiGroupsendLimReleaseFrequency * 1000);
        }
    }
    else if (ERROR_SQL_HANDLE_TIMER_ID == pMsg->m_iSeq)
    {
        SAFE_DELETE(m_pFailedSqlHandleTimer);

        bool bFlag = false;
        typedef const AuditFailedSqlMap::value_type const_pair;

        foreach(const_pair& node, m_mapAuditFailedSql)
        {
            TDBQueryReq* pAddAudit = new TDBQueryReq();
            pAddAudit->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
            pAddAudit->m_iSeq = ERROR_SQL_HANDLE_TIMER_ID;
            pAddAudit->m_pSender = this;
            pAddAudit->m_strSessionID = node.first;
            pAddAudit->m_SQL = node.second;

            LogDebug("===failed sql retry===Sql(%s).", pAddAudit->m_SQL.data());

            if (!g_pDisPathDBThreadPool->PostMsg(pAddAudit))
            {
                LogError("Call PostMsg failed.");
            }
            else
            {
                bFlag = true;
            }
        }

        m_pFailedSqlHandleTimer = bFlag ? NULL : SetTimer(ERROR_SQL_HANDLE_TIMER_ID, "", 10 * 1000);
    }
    else
    {
        LogWarn("no this timerId[%ld].", pMsg->m_iSeq);
    }

    return;
}

void CAuditServiceThread::handleQueryDbRespMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    LogDebug("sequence(%lu).", pMsg->m_iSeq);

    if (AUDIT_EXPIRE_CHECK_TIMER_ID == pMsg->m_iSeq)
    {
        handleAuditExpireTimerMsg(pMsg, m_pAuditExpireCheckTimer);
    }
    else if (GROUPSENDLIM_AUDIT_EXPIRE_CHECK_TIMER_ID == pMsg->m_iSeq)
    {
        handleAuditExpireTimerMsg(pMsg, m_pGroupSendLimAuditExpireTimer);
    }
    else if (GROUPSENDLIM_RELEASE_TIMER_ID == pMsg->m_iSeq)
    {
       handleGroupSendLimReleaseMsg(pMsg);
    }
    else
    {
        LogWarn("Invalid sequence(%lu).", pMsg->m_iSeq);
    }
}

void CAuditServiceThread::handleAuditExpireTimerMsg(TMsg* pMsg, CThreadWheelTimer*& pTimer)
{
    CHK_NULL_RETURN(pMsg);

    TDBQueryResp* pAuditResult = (TDBQueryResp*)pMsg;

    if ((0 != pAuditResult->m_iResult) || (NULL == pAuditResult->m_recordset))
    {
        LogError("sql query error!");
        SAFE_DELETE(pAuditResult->m_recordset);
        pTimer = SetTimer(pMsg->m_iSeq, "", m_uiAuditCheckTimeInterval * 1000);
        return;
    }

    RecordSet* rs = pAuditResult->m_recordset;
    int iCount = rs->GetRecordCount();

    if (NULL == pTimer)
    {
        if (iCount >= 1000)
        {
            pTimer = SetTimer(pMsg->m_iSeq, "", 15 * 1000);
            LogDebug("count(%d), more than 1000, set timer 15 second", iCount);
        }
        else
        {
            pTimer = SetTimer(pMsg->m_iSeq, "", m_uiAuditCheckTimeInterval * 1000);
            LogDebug("count(%d), little than 1000, set timer %d minutes", iCount, m_uiAuditCheckTimeInterval/60);
        }
    }

    TblSmsGroupSendAudit obj;

    for (int i = 0; i < iCount; ++i)
    {
        obj.strAuditId_ = (*rs)[i]["auditid"];
        obj.strCreateTime_ = (*rs)[i]["createtime"];
        obj.strMd5_ = (*rs)[i]["md5"];
        obj.uiGroupSendLimUserFlag_ = atoi((*rs)[i]["groupsendlim_userflag"].data()); // for t_sms_audit, it is ""
        obj.uiGroupSendLimFlag_ = (GROUPSENDLIM_AUDIT_EXPIRE_CHECK_TIMER_ID == pMsg->m_iSeq) ? 1 : 0;
        int iStatus = atoi((*rs)[i]["status"].data());

        if (0 == iStatus)
        {
            obj.eStatus_ = TblSmsGroupSendAudit::AuditResult_NotAudit_To_SystemExpire;
        }
        else if (1 == iStatus)
        {
            obj.eStatus_ = TblSmsGroupSendAudit::AuditResult_AuditPass_To_SystemExpire;
        }
        else if (2 == iStatus)
        {
            obj.eStatus_ = TblSmsGroupSendAudit::AuditResult_AuditNotPass_To_SystemExpire;
        }
        else
        {
            LogError("Invalid status(%d), auditid(%s).", iStatus, obj.strAuditId_.data());
            continue;
        }

        NeedAuditMapIter iter = m_mapNeedAudit.find(obj.strMd5_);
        if (m_mapNeedAudit.end() != iter)
        {
            m_mapNeedAudit.erase(iter);
        }

        dbUpdateProcess(obj);
        mqProduceProcess(obj);
    }

    SAFE_DELETE(rs);
}

void CAuditServiceThread::handleGroupSendLimReleaseMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    TDBQueryResp* pAuditResult = (TDBQueryResp*)pMsg;

    if ((0 != pAuditResult->m_iResult) || (NULL == pAuditResult->m_recordset))
    {
        LogError("sql query error!");
        SAFE_DELETE(pAuditResult->m_recordset);
        m_pGroupSendLimReleaseTimer = SetTimer(pMsg->m_iSeq, "", m_uiGroupsendLimReleaseFrequency * 1000);
        return;
    }

    RecordSet* rs = pAuditResult->m_recordset;
    int iCount = rs->GetRecordCount();

    if (m_pGroupSendLimReleaseTimer == NULL)
    {
        if (iCount >= 1000)
        {
            m_pGroupSendLimReleaseTimer = SetTimer(pMsg->m_iSeq, "", 100);
            LogNotice("count(%d), more than 1000, set timer 1 second", iCount);
        }
        else
        {
            m_pGroupSendLimReleaseTimer = SetTimer(pMsg->m_iSeq, "", m_uiGroupsendLimReleaseFrequency * 1000);
            LogDebug("count(%d), little than 1000, set timer %u second", iCount, m_uiGroupsendLimReleaseFrequency);
        }
    }

    TblSmsGroupSendAudit obj;

    for (int i = 0; i < iCount; ++i)
    {
        obj.strAuditId_ = (*rs)[i]["auditid"];
        obj.strCreateTime_ = (*rs)[i]["createtime"];
        obj.strMd5_ = (*rs)[i]["md5"];
        obj.uiStatus_ = atoi((*rs)[i]["status"].data());
        obj.strAuditTime_ = (*rs)[i]["audittime"];
        obj.uiSendNum_ = atoi((*rs)[i]["sendnum"].data());
        obj.uiGroupSendLimStartTime_ = atoi((*rs)[i]["groupsendlim_stime"].data());
        obj.uiGroupSendLimEndTime_ = atoi((*rs)[i]["groupsendlim_etime"].data());
        obj.uiGroupSendLimUserFlag_ = atoi((*rs)[i]["groupsendlim_userflag"].data());
        obj.uiGroupSendLimTime_ = atoi((*rs)[i]["groupsendlim_time"].data());
        obj.uiGroupSendLimNum_ = atoi((*rs)[i]["groupsendlim_num"].data());

        LogDebug("SendNum(%u), GroupSendLimStartTime(%u), GroupSendLimEndTime(%u), "
            "GroupSendLimUserFlag(%u), GroupSendLimTime(%u), uiGroupSendLimNum(%u)",
            obj.uiSendNum_,
            obj.uiGroupSendLimStartTime_,
            obj.uiGroupSendLimEndTime_,
            obj.uiGroupSendLimUserFlag_,
            obj.uiGroupSendLimTime_,
            obj.uiGroupSendLimNum_);

        if (0 == obj.uiGroupSendLimUserFlag_)
        {
            if (obj.uiSendNum_ >= obj.uiGroupSendLimNum_)
            {
                obj.uiUnlimitState_ = 1;

                dbUpdateProcess(obj);
                LogDebug("ShengChan system user, display on the web front end interface.");
            }
            else
            {
                obj.uiUnlimitState_ = 2;
                obj.uiStatus_ = 3;
                obj.strAuditTime_ = Comm::getCurrentTime(); // 这里更新audittime,但实际上state是3未审核
                obj.eStatus_ = TblSmsGroupSendAudit::AuditResult_GroupSendLim_Release_To_Audit;
                obj.uiGroupSendLimFlag_ = 1;

                dbUpdateProcess(obj);
                mqProduceProcess(obj);
                LogDebug("ShengChan system user, release group send limit.");
            }
        }
        else if (1 == obj.uiGroupSendLimUserFlag_)
        {
            obj.uiUnlimitState_ = 2;
            obj.uiStatus_ = 1;
            obj.strAuditTime_ = Comm::getCurrentTime();
            obj.eStatus_ = TblSmsGroupSendAudit::AuditResult_GroupSendLim_Release_To_Send;
            obj.uiGroupSendLimFlag_ = 1;

            dbUpdateProcess(obj);
            mqProduceProcess(obj);
            LogDebug("TeDing system user, release group send limit.");
        }
        else
        {
            LogError("Invalid groupsendlim_userflag(%u).", obj.uiGroupSendLimUserFlag_)
        }
    }

    SAFE_DELETE(rs);
}

void CAuditServiceThread::dbInsertProcess(SessionInfo* pSession)
{
    CHK_NULL_RETURN(pSession);
    CHK_NULL_RETURN(g_pDisPathDBThreadPool);

    const string& strUserName = pSession->mqMsg_.strUserName_;
    const string& strSign = pSession->mqMsg_.strSign_;
    const string& strContent = pSession->mqMsg_.strContent_;
    const string strCurrentDate = Comm::getCurrentTime();
    const string strAuditId = pSession->strAuditIdPrefix_ + "_" + getUUID();

    {
        string strCmd;
        strCmd.append("HMSET ");
        strCmd.append(pSession->strRedisNeedKey_);
        strCmd.append(" clientid ");
        strCmd.append(pSession->mqMsg_.strClientId_);
        strCmd.append(" auditid ");
        strCmd.append(strAuditId);

        string strSession;
        strSession.append("set ");
        strSession.append(pSession->strRedisNeedNameSpace_);

        if (redisProcess(pSession->mqMsg_.m_iSeq, strSession, strCmd))
        {
            redisExpireProcess(pSession->strRedisNeedKey_, pSession->strExpireTime_);
            pSession->uiRefCount_++;
        }
    }

    char sql[8192]  = {0};
    char srcContent[4196] = {0}, dstContent[4196] = {0};
    char srcSign[1024] = {0}, dstSign[1024] = {0};
    char srcUserName[1024] = {0}, dstUserName[1024] = {0};

    {
        UInt32 position = strSign.length();

        if (strSign.length() > 100)
            position = Comm::getSubStr(strSign, 100);

        MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

        if (MysqlConn != NULL)
        {
            mysql_real_escape_string(MysqlConn, srcContent, strContent.data(), strContent.length());
            mysql_real_escape_string(MysqlConn, srcSign, strSign.substr(0, position).data(), strSign.substr(0, position).length());
            mysql_real_escape_string(MysqlConn, srcUserName, strUserName.data(), strUserName.length());
        }
        else
        {
            Comm::escape_string_for_mysql(srcContent, 4196, strContent.data(), strContent.length());
            Comm::escape_string_for_mysql(srcSign, 1024, strSign.substr(0, position).data(), strSign.substr(0, position).length());
            Comm::escape_string_for_mysql(srcUserName, 1024, strUserName.data(), strUserName.length());
        }

        CEmojiString::fillEmojiRealString(srcContent, dstContent);
        CEmojiString::fillEmojiRealString(srcSign, dstSign);
        CEmojiString::fillEmojiRealString(srcUserName, dstUserName);
    }

    if (MqC2sDbAuditContentReqQueMsg::GroupSendLimMsg == pSession->mqMsg_.eMsgType_)
    {
        snprintf(sql, sizeof(sql), "insert into t_sms_groupsend_audit"
            "(auditid,createtime,clientid,username,content,sign,smstype,operatortype,md5,sendnum,last_sendtime,paytype,"
            "groupsendlim_stime,groupsendlim_etime,groupsendlim_userflag,groupsendlim_time,groupsendlim_num,smsp_audit_id) values "
            "('%s', '%s', '%s', '%s', '%s', '%s', %d , %d, '%s', %u, '%s', %d, %u, %u, %u, %u, %u, %lu);",
            strAuditId.data(),
            strCurrentDate.data(),
            pSession->mqMsg_.strClientId_.data(),
            dstUserName,
            dstContent,
            dstSign,
            pSession->mqMsg_.ucSmsType_,
            pSession->mqMsg_.ucOperatorType_,
            pSession->mqMsg_.strAuditContent_.data(),
            pSession->mqMsg_.uiPhoneCount_,
            strCurrentDate.data(),
            pSession->mqMsg_.ucPayType_,
            pSession->mqMsg_.uiCsDataTimestamp_,
            pSession->mqMsg_.uiCsDataTimestamp_,
            pSession->mqMsg_.uiGroupSendLimUserFlag_,
            pSession->mqMsg_.uiGroupSendLimTime_,
            pSession->mqMsg_.uiGroupSendLimNum_,
            g_uComponentId);
    }
    else
    {
        snprintf(sql, sizeof(sql), "insert into t_sms_audit"
            "(auditid,createtime,clientid,content,sendnum,last_sendtime,md5,username,paytype,sign,smstype,smsp_audit_id) values "
            "('%s', '%s', '%s', '%s', %u, '%s', '%s', '%s', %d, '%s', %d, %lu);",
            strAuditId.data(),
            strCurrentDate.data(),
            pSession->mqMsg_.strClientId_.data(),
            dstContent,
            pSession->mqMsg_.uiPhoneCount_,
            strCurrentDate.data(),
            pSession->mqMsg_.strAuditContent_.data(),
            dstUserName,
            pSession->mqMsg_.ucPayType_,
            dstSign,
            pSession->mqMsg_.ucSmsType_,
            g_uComponentId);
    }

    LogNotice("[%s:%s] sql[%s].", pSession->mqMsg_.strSmsId_.data(), pSession->mqMsg_.strPhone_.data(), sql);

    TDBQueryReq* pAddAudit = new TDBQueryReq();
    CHK_NULL_RETURN(pAddAudit);
    pAddAudit->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pAddAudit->m_iSeq = pSession->mqMsg_.m_iSeq;
    pAddAudit->m_SQL.assign(sql);
    pAddAudit->m_pSender = this;

    pSession->strAuditId_ = strAuditId;
    pSession->strSql_ = sql;

    if (!g_pDisPathDBThreadPool->PostMsg(pAddAudit)) //link err
    {
        // There is a problem here.
        // We just saved the records of insert failure.
        // The records of update failure don't need saved? yes

        m_mapAuditFailedSql[strAuditId] = sql;
        LogError("Call PostMsg failed, save failed sql.");
    }
    else
    {
        // need wait db resopnse
        pSession->uiRefCount_++;
    }

    m_mapNeedAudit[pSession->mqMsg_.strAuditContent_] = NeedAudit(pSession->mqMsg_.strClientId_, strAuditId);
}

void CAuditServiceThread::dbUpdateProcess(SessionInfo* pSession)
{
    CHK_NULL_RETURN(pSession);

    char sql[8192]  = {0};
    string strCurrentDate = Comm::getCurrentTime();

    if (MqC2sDbAuditContentReqQueMsg::GroupSendLimMsg == pSession->mqMsg_.eMsgType_)
    {
        snprintf(sql, sizeof(sql), "update t_sms_groupsend_audit set sendnum = sendnum + %u, last_sendtime = '%s', "
            "groupsendlim_etime = %u where auditid = '%s';",
            pSession->mqMsg_.uiPhoneCount_,
            strCurrentDate.data(),
            pSession->mqMsg_.uiCsDataTimestamp_,
            pSession->strAuditId_.data());
    }
    else
    {
        snprintf(sql, sizeof(sql), "update t_sms_audit set sendnum = sendnum + %u, last_sendtime = '%s' "
            "where auditid = '%s';",
            pSession->mqMsg_.uiPhoneCount_,
            strCurrentDate.data(),
            pSession->strAuditId_.data());
    }

    TDBQueryReq* pMsg = new TDBQueryReq();
    CHK_NULL_RETURN(pMsg);
    pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pMsg->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pMsg);

    LogNotice("[%s:%s] sql[%s].", pSession->mqMsg_.strSmsId_.data(), pSession->mqMsg_.strPhone_.data(), sql);
}

void CAuditServiceThread::dbUpdateProcess(const TblSmsGroupSendAudit& obj)
{
    char sql[1024] = {0};

    if ((1 == obj.uiUnlimitState_) || (2 == obj.uiUnlimitState_))
    {
        if (obj.strAuditTime_.empty())
        {
            snprintf(sql, sizeof(sql), "update t_sms_groupsend_audit set unlimit_state = %u, status = %u where auditid = '%s' and createtime = '%s';",
                obj.uiUnlimitState_,
                obj.uiStatus_,
                obj.strAuditId_.data(),
                obj.strCreateTime_.data());
        }
        else
        {
            snprintf(sql, sizeof(sql), "update t_sms_groupsend_audit set unlimit_state = %u, status = %u, audittime = '%s' where auditid = '%s' and createtime = '%s';",
                obj.uiUnlimitState_,
                obj.uiStatus_,
                obj.strAuditTime_.data(),
                obj.strAuditId_.data(),
                obj.strCreateTime_.data());
        }
    }
    else
    {
        string strTable = (1 == obj.uiGroupSendLimFlag_) ? "t_sms_groupsend_audit" : "t_sms_audit";

        snprintf(sql, sizeof(sql), "update %s set removeflag = 1 where auditid = '%s' and createtime = '%s';",
            strTable.data(),
            obj.strAuditId_.data(),
            obj.strCreateTime_.data());
    }

    TDBQueryReq* pMsg = new TDBQueryReq();
    CHK_NULL_RETURN(pMsg);
    CHK_NULL_RETURN(g_pDisPathDBThreadPool);

    pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pMsg->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pMsg);

    LogNotice("sql(%s).", pMsg->m_SQL.data());
}

void CAuditServiceThread::mqProduceProcess(const TblSmsGroupSendAudit& obj)
{
    string strMsg;
    strMsg.append("auditcontent=");
    strMsg.append(obj.strMd5_);
    strMsg.append("&auditid=");
    strMsg.append(obj.strAuditId_);
    strMsg.append("&status=");
    strMsg.append(Comm::int2str(obj.eStatus_));
    strMsg.append("&groupsendlim_flag=");
    strMsg.append(Comm::int2str(obj.uiGroupSendLimFlag_));
    strMsg.append("&groupsendlim_userflag=");
    strMsg.append(Comm::int2str(obj.uiGroupSendLimUserFlag_));

    TMQPublishReqMsg* pMQPublishReqMsg = new TMQPublishReqMsg;
    CHK_NULL_RETURN(pMQPublishReqMsg);
    CHK_NULL_RETURN(g_pMqC2sDbProducerThread);

    pMQPublishReqMsg->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQPublishReqMsg->m_strExchange = m_strMQDBExchange;
    pMQPublishReqMsg->m_strRoutingKey = m_strMQDBRoutingKey;
    pMQPublishReqMsg->m_strData = strMsg;

    g_pMqC2sDbProducerThread->PostMsg(pMQPublishReqMsg);

    LogNotice("publish msg(%s), MQDBExchange(%s), MQDBRoutingKey(%s).",
        strMsg.data(),
        m_strMQDBExchange.data(),
        m_strMQDBRoutingKey.data());
}

bool CAuditServiceThread::redisProcess(UInt64 ulSeq, const string& strSession, const string& strCmd)
{
    TRedisReq* pRedisReq = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisReq);

    pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedisReq->m_pSender = this;
    pRedisReq->m_iSeq = ulSeq;
    pRedisReq->m_strSessionID = strSession;
    pRedisReq->m_strKey = strSession;
    pRedisReq->m_RedisCmd = strCmd;

    LogDebug("sequence(%lu), Session(%s), Cmd(%s).", ulSeq, strSession.data(), strCmd.data());

    return SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);
}

bool CAuditServiceThread::redisExpireProcess(const string& strKey, const string& strExpireTime)
{
    TRedisReq* pRedisExpire = new TRedisReq();
    CHK_NULL_RETURN_FALSE(pRedisExpire);

    pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedisExpire->m_strKey = strKey;
    pRedisExpire->m_RedisCmd.assign("EXPIRE ");
    pRedisExpire->m_RedisCmd.append(strKey);
    pRedisExpire->m_RedisCmd.append(" ");
    pRedisExpire->m_RedisCmd.append(strExpireTime);

    LogDebug("Key(%s), ExpireTime(%s)", strKey.data(), strExpireTime.data());

    return SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisExpire);
}


