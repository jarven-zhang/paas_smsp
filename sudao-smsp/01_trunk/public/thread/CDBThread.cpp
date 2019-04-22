#include "CDBThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include "Comm.h"
#include "alarm.h"


#define etime(a,b,c,d) ((long)(c-a)*1000000+d-b)


CDBThread::CDBThread(const char *name):CThread(name)
{
    m_uDBExecultErrCunts = 0;
    m_DBConn = NULL;

    // DB_REQUEUE_CFG default: 3;21600
    m_usReQueueTimes = 3;
    m_uiReQueueTime = 21600;
}

CDBThread::~CDBThread()
{
    if(NULL != m_DBConn)
    {
        mysql_close(m_DBConn);
        m_DBConn = NULL;
    }
}

bool CDBThread::Available()
{
    if(m_DBConn == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }

}

bool CDBThread::Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
{
    if(false == CThread::Init())
        return false;
    m_DBHost = dbhost;
    m_DBUser = dbuser;
    m_DBPwd  = dbpwd;
    m_DBName = dbname;
    m_DBPort = dbport;
    if(!DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName))
    {
        printf( "DBConnect() fail.\n");
        return false;
    }
    m_pTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "", CHECK_DB_CONN_TIMEOUT);
    if(NULL == m_pTimer)
    {
        printf( "SetTimer() fail.\n");
        return false;
    }

    PropertyUtils::GetValue("db_execult_err_cunts", m_uDBExecultErrCunts);
    LogDebug("db_execult_err_cunts[%d]", m_uDBExecultErrCunts);
    m_componentId =  g_uComponentId;
    LogDebug("CDBThread.cpp:m_componentId:%lu\n", m_componentId);

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;
}

void CDBThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "DB_REQUEUE_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;
        std::size_t pos = strTmp.find_last_of(";");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                strSysPara.c_str(), strTmp.c_str());

            break;
        }

        int iReQueueTimes = atoi(strTmp.substr(0, pos).data());
        int iReQueueTime = atoi(strTmp.substr(pos + 1).data());

        if ((0 > iReQueueTimes) || (1000 < iReQueueTimes))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(),
                strTmp.c_str(),
                iReQueueTimes);

            break;
        }

        if ((1 > iReQueueTime) || (86400 < iReQueueTime))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), iReQueueTime);

            break;
        }

        m_usReQueueTimes = iReQueueTimes;
        m_uiReQueueTime = iReQueueTime;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u, %u).", strSysPara.c_str(), m_usReQueueTimes, m_uiReQueueTime);
    do
    {
        strSysPara = "DB_SWITCH";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        size_t pos = strTmp.find("|");
        int dbSwitchTemp = 0;
        int iFWriteBigData = 0;
        if (pos != string::npos)
        {
            dbSwitchTemp = atoi(strTmp.substr(0,pos).data());
            iFWriteBigData = atoi(strTmp.substr(pos + 1).data());
        }

        if ((0 != dbSwitchTemp) && (1 != dbSwitchTemp) && (2 != dbSwitchTemp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), dbSwitchTemp);
            break;
        }
        if ((0 != iFWriteBigData) && (1 != iFWriteBigData))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), iFWriteBigData);
            break;
        }

        m_uifWriteBigData = iFWriteBigData;
        m_uDBSwitch = dbSwitchTemp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u,%u).", strSysPara.c_str(), m_uDBSwitch, m_uifWriteBigData);
}

//connect fail n times or excute command fail n times alarm
void CDBThread::mysqlFailedAlarm(int iCount, int type)
{
    std::string strChinese = "";
    strChinese = DAlarm::getDBContinueFailedAlarmStr(m_DBName, iCount, m_componentId, type);
    Alarm(DB_CONN_FAILED_ALARM,m_DBName.data(),strChinese.data());
}

void CDBThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_DB_UPDATE_QUERY_REQ:
        {
            DBUpdateReq(pMsg);
            break;
        }
        case MSGTYPE_DB_QUERY_REQ:
        {
            DBQueryReq(pMsg);
            break;
        }
        case MSGTYPE_DB_NOTQUERY_REQ:
        {
            DBNotQueryReq(pMsg);
            break;
        }
        case MSGTYPE_DB_IN_REQ:
        {
            handleDbInReq(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            int ret = DBPing();
            if (false == ret)
            {
                //alarm
                mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_CONNECT_FAIL);
            }
            if(NULL != m_pTimer)
            {
                delete m_pTimer;
            }
            m_pTimer = NULL;
            m_pTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "", CHECK_DB_CONN_TIMEOUT);
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == pSysParamReq)
            {
                break;
            }

            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        default:
        {
            break;
        }
    }

}

bool CDBThread::DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
{
    MYSQL* conn;
    conn = mysql_init(NULL);
    if(NULL == conn)
    {
        LogError( "mysql_init fail.\n");
        return false;
    }
    if (NULL == mysql_real_connect(conn, dbhost.data(), dbuser.data(), dbpwd.data(), dbname.data(), dbport, NULL, 0))
    {
        LogError( "connect to database fail error: %s", mysql_error(conn));
        mysql_close(conn);
        conn = NULL;
        return false;
    }

    std::string strSql = "set names utf8";

    if (!mysql_real_query(conn, strSql.c_str(), (unsigned long) strSql.length()))
    {
        //mysql_affected_rows(conn);
    }
    else
    {
        LogError("set names utf8 is failed.");
        return false;
    }

    m_DBConn = conn;
    return true;
}

bool CDBThread::DBPing()
{
    bool ret = false;
    UInt32 conFailedNumber = 0;
    do
    {
        if(m_DBConn == NULL)
        {
            LogError("m_DBConn is NULL, reconnect DB.");
            ret =  DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName);
        }
        else if((m_DBConn != NULL) && (0 != mysql_ping(m_DBConn)))
        {
            LogError("ping fail, reconnect DB.");
            mysql_close(m_DBConn);
            m_DBConn = NULL;
            ret =  DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName);
        }
        else
        {
            ret = true;
        }

        if (false == ret)
        {
            conFailedNumber++;
        }
        else
        {
            break;
        }


    }while (conFailedNumber < m_uDBExecultErrCunts);

    if (conFailedNumber >= m_uDBExecultErrCunts)
    {
        LogError("connect DB %u times fail.", conFailedNumber);
    }

    return ret;
}


/*
    DBUpdateReq() will return update result and affectRow
*/
void CDBThread::DBUpdateReq(TMsg* pMsg)
{
    TDBQueryReq* pDBQueryReq = (TDBQueryReq*)pMsg;
    int iResult = -1;
    UInt32 conFailedNumber = 0;

    do
    {
        if(m_DBConn != NULL)
        {
            iResult = mysql_query(m_DBConn, (char*)pDBQueryReq->m_SQL.data());
        }
        else
        {
            //exculet failed, then reconn and execult again
            if(DBPing())
            {
                iResult = mysql_query(m_DBConn, (char*)pDBQueryReq->m_SQL.data());
            }
        }
        if (iResult != 0)
        {
            conFailedNumber++;
        }
        else
        {
            break;
        }
    }while(conFailedNumber < m_uDBExecultErrCunts);

    if(conFailedNumber >= m_uDBExecultErrCunts)
    {
        //this log used for grep log .
        LogError("db exclute err! sql[%s] error:[%d][%s]", pDBQueryReq->m_SQL.data(), mysql_errno(m_DBConn), mysql_error(m_DBConn));
        mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_EXE_COMMAND_FAIL);
    }

    ////get affrow
    int affrow = 0;
    if(m_DBConn != NULL)
    {
        affrow = mysql_affected_rows(m_DBConn);
    }

    if(pDBQueryReq->m_pSender != NULL)
    {
        TDBQueryResp* pDBQueryResp =  new TDBQueryResp();
        pDBQueryResp->m_iMsgType= MSGTYPE_DB_QUERY_RESP;
        pDBQueryResp->m_iSeq = pDBQueryReq->m_iSeq;
        pDBQueryResp->m_strSessionID = pDBQueryReq->m_strSessionID;
        pDBQueryResp->m_iResult   = iResult;
        pDBQueryResp->m_iAffectRow   = affrow;
        pDBQueryResp->m_recordset = NULL;
        pDBQueryReq->m_pSender->PostMsg(pDBQueryResp);
    }
}


void CDBThread::DBQueryReq(TMsg* pMsg)
{
    TDBQueryReq* pDBQueryReq = (TDBQueryReq*)pMsg;
    RecordSet* rs = new RecordSet(m_DBConn);
    int iResult = -1;
    int result = 0;
    UInt32 conFailedNumber = 0;

    do
    {
        if(m_DBConn != NULL)
        {
            iResult = rs->ExecuteSQL((char*)pDBQueryReq->m_SQL.data());
        }
        else
        {
            if(DBPing())
            {
                iResult = rs->ExecuteSQL((char*)pDBQueryReq->m_SQL.data());
            }
        }
        if (iResult == -1)
        {
            conFailedNumber++;
            result = 1;
        }
        else
        {
            result = 0;
            break;
        }

    }while (conFailedNumber < m_uDBExecultErrCunts);

    if (conFailedNumber >= m_uDBExecultErrCunts)
    {
        LogError("==db== continue failed number:%d,is over warn number.",conFailedNumber);

        mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_EXE_COMMAND_FAIL);
    }

    if(pDBQueryReq->m_pSender != NULL)
    {
        TDBQueryResp* pDBQueryResp =  new TDBQueryResp();
        pDBQueryResp->m_iMsgType= MSGTYPE_DB_QUERY_RESP;
        pDBQueryResp->m_iSeq = pDBQueryReq->m_iSeq;
        pDBQueryResp->m_strSessionID = pDBQueryReq->m_strSessionID;
        pDBQueryResp->m_iResult   = result;
        pDBQueryResp->m_recordset = rs;
        pDBQueryReq->m_pSender->PostMsg(pDBQueryResp);
    }
}

void CDBThread::DBNotQueryReq(TMsg* pMsg)
{
    TDBQueryReq* pDBNotQueryReq = (TDBQueryReq*)pMsg;
    int iResult = -1;
    UInt32 conFailedNumber = 0;
    int err = 0;

    do
    {
        if(NULL != m_DBConn)
        {
            iResult = mysql_query(m_DBConn, (char*)pDBNotQueryReq->m_SQL.data());
        }
        else
        {
            if(DBPing())
            {
                iResult = mysql_query(m_DBConn, (char*)pDBNotQueryReq->m_SQL.data());
            }
        }

        if(0 != iResult)
        {
            err = mysql_errno(m_DBConn);
            LogError("execult sql error. sql[%s], errno[%d], err[%s]", pDBNotQueryReq->m_SQL.data(), err, mysql_error(m_DBConn));
            conFailedNumber++;
            if (err == 1306 || err == 1064 || err==1062)
            {
                conFailedNumber = m_uDBExecultErrCunts;
                break;
            }
        }
        else
        {
            break;
        }
    }while (conFailedNumber < m_uDBExecultErrCunts);

    if (conFailedNumber >= m_uDBExecultErrCunts)
    {
        LogError("==db== continue failed number:%d,is over warn number.",conFailedNumber);
        mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_EXE_COMMAND_FAIL);
    }

        ////get affrow
    int affrow = 0;
    if(m_DBConn != NULL)
    {
        affrow = mysql_affected_rows(m_DBConn);
    }

    if(pDBNotQueryReq->m_pSender != NULL)
    {
        TDBQueryResp* pDBQueryResp =  new TDBQueryResp();
        pDBQueryResp->m_iMsgType= MSGTYPE_DB_NOTQUERY_RESP;
        pDBQueryResp->m_iSeq = pDBNotQueryReq->m_iSeq;
        pDBQueryResp->m_strSessionID = pDBNotQueryReq->m_strSessionID;
        pDBQueryResp->m_strKey = pDBNotQueryReq->m_strKey;
        pDBQueryResp->m_iResult   = err;
        pDBQueryResp->m_iAffectRow   = affrow;
        pDBQueryResp->m_recordset = NULL;
        pDBNotQueryReq->m_pSender->PostMsg(pDBQueryResp);
    }
#ifdef SMSP_CONSUMER
    if (iResult == 0 && affrow > 0 && m_uifWriteBigData == 1)
    {
        TSqlContentReq* pSql = new TSqlContentReq();
        pSql->m_iMsgType = MSGTYPE_LOG_REQ;
        pSql->m_strSqlContent.assign(pDBNotQueryReq->m_SQL);
        g_pBigDataThread->PostMsg(pSql);
    }
#endif
}

void CDBThread::handleDbInReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    DBInReq* pDBInReq = (DBInReq*)pMsg;
    const SessionInfo& s = pDBInReq->sessionInfo_;

    if (TableFlag_Access == s.eTableFlag_)
    {
        handleDbInReqForAccess(s);
    }
    else if (TableFlag_Record == s.eTableFlag_)
    {
        handleDbInReqForRecord(s);
    }
    else
    {
        LogError("Internal Error. eTableFlag(%d), sql(%s).",
            s.eTableFlag_,
            s.strSrcSql_.data());
    }
}

void CDBThread::handleDbInReqForAccess(const SessionInfo& s)
{
    int iResult = -1;
    UInt32 conFailedNumber = 0;
    int err = 0;

    do
    {
        if (s.bModify_)
        {
            iResult = excuteSql(s.strSqlModifyWhere_, err);

            if (iResult > 0)
            {
#ifdef SMSP_CONSUMER
                if (m_uifWriteBigData == 1)
                {
                    TSqlContentReq* pSql = new TSqlContentReq();
                    pSql->m_iMsgType = MSGTYPE_LOG_REQ;
                    pSql->m_strSqlContent.assign(s.strSqlModifyWhere_);
                    g_pBigDataThread->PostMsg(pSql);
                }
#endif
                break;
            }
            else if (0 == iResult)
            {
                LogWarn("===state have been covered=== ModifyWhere(%s).",
                    s.strSqlModifyWhere_.data());

                iResult = excuteSql(s.strSqlModifySet_, err);

                if (iResult > 0)
                {
#ifdef SMSP_CONSUMER
                    if (m_uifWriteBigData == 1)
                    {
                        TSqlContentReq* pSql = new TSqlContentReq();
                        pSql->m_iMsgType = MSGTYPE_LOG_REQ;
                        pSql->m_strSqlContent.assign(s.strSqlModifySet_);
                        g_pBigDataThread->PostMsg(pSql);
                    }
#endif
                    break;
                }
                else
                {
                    LogError("Call excuteSql failed for ModifySet Sql(%s).",
                        s.strSqlModifySet_.data());
                }
            }
            else
            {
                LogError("Call excuteSql failed for ModifyWhere Sql(%s).",
                    s.strSqlModifyWhere_.data());
            }

            LogWarn("===fault-tolerant===, excute source sql(%s).",
                s.strSrcSql_.data());

            iResult = excuteSql(s.strSrcSql_, err);
        }
        else
        {
            iResult = excuteSql(s.strSrcSql_, err);
        }

        if (iResult > 0)
        {
#ifdef SMSP_CONSUMER
            if (m_uifWriteBigData == 1)
            {
                TSqlContentReq* pSql = new TSqlContentReq();
                pSql->m_iMsgType = MSGTYPE_LOG_REQ;
                pSql->m_strSqlContent.assign(s.strSrcSql_);
                g_pBigDataThread->PostMsg(pSql);
            }
#endif
            break;
        }
        else if (0 == iResult)
        {
            LogWarn("Call excuteSql return 0. no row changed.reque");
            reQueue(s);
            break;
        }
        else
        {
            conFailedNumber++;
            if (err == 1306 || err == 1064 || err==1062)
            {
                conFailedNumber = m_uDBExecultErrCunts;
                break;
            }
        }
    }while (conFailedNumber < m_uDBExecultErrCunts);

    if (conFailedNumber >= m_uDBExecultErrCunts)
    {
        LogError("==db== continue failed number:%d,is over warn number.",conFailedNumber);
        mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_EXE_COMMAND_FAIL);
    }
}

void CDBThread::handleDbInReqForRecord(const SessionInfo& s)
{
    int iResult = -1;
    UInt32 conFailedNumber = 0;
    int err = 0;

    do
    {
        iResult = excuteSql(s.strSrcSql_, err);

        if (iResult > 0)
        {
#ifdef SMSP_CONSUMER
            if (m_uifWriteBigData == 1)
            {
                TSqlContentReq* pSql = new TSqlContentReq();
                pSql->m_iMsgType = MSGTYPE_LOG_REQ;
                pSql->m_strSqlContent.assign(s.strSrcSql_);
                g_pBigDataThread->PostMsg(pSql);
            }
#endif

            break;
        }
        else if (0 == iResult)
        {
            LogDebug("Call excuteSql return 0. reQueue.");
            reQueue(s);
            break;
        }
        else
        {
            conFailedNumber++;
            if (err == 1306 || err == 1064 || err==1062)
            {
                conFailedNumber = m_uDBExecultErrCunts;
                break;
            }
        }
    }while (conFailedNumber < m_uDBExecultErrCunts);

    if (conFailedNumber >= m_uDBExecultErrCunts)
    {
        LogError("==db== continue failed number:%d,is over warn number.",conFailedNumber);
        mysqlFailedAlarm(m_uDBExecultErrCunts, MYSQL_EXE_COMMAND_FAIL);
    }
}

int CDBThread::excuteSql(const string& strSql, int& err)
{
    int ret = -1;

    if(NULL != m_DBConn)
    {
        ret = mysql_query(m_DBConn, strSql.data());
    }
    else
    {
        if(DBPing())
        {
            ret = mysql_query(m_DBConn, strSql.data());
        }
    }

    if (0 == ret)
    {
        my_ulonglong affected = mysql_affected_rows(m_DBConn);

        if ((my_ulonglong)-1 == affected)
        {
            LogError("Call mysql_affected_rows failed. strSql[%s].",
                strSql.data());

            return -1;  // if here, excute source sql
        }

        return (int)affected;
    }
    else
    {
        err = mysql_errno(m_DBConn);

        LogError("Call mysql_query failed. sql[%s], errno[%d], err[%s]",
            strSql.data(), err, mysql_error(m_DBConn));
    }

    return -1;
}

void CDBThread::reQueue(const SessionInfo& s)
{
    if ((0 == m_usReQueueTimes)
      || (0 == m_uiReQueueTime))
    {
        return;
    }

    if (OperFlag_Update != s.eOperFlag_)
    {
        return;
    }

    time_t now = time(NULL);
    UInt32 uiFirstTime = 0;
    bool bFirstTime = ((0 == s.usReQueueTimes_) && (0 == s.uiReQueueTime_))
                        ? true : false;

    if (bFirstTime)
    {
        uiFirstTime = now;
    }
    else
    {
        uiFirstTime = s.uiReQueueTime_;
        UInt32 uiExpireTime = s.uiReQueueTime_ + m_uiReQueueTime;

        if ((m_usReQueueTimes <= s.usReQueueTimes_) || (uiExpireTime <= now))
        {
            LogWarn("===requeue discard msg=== "
                "m_usReQueueTimes(%u), s.usReQueueTimes_(%u), "
                "uiExpireTime(%u), now(%u), Sql(%s).",
                m_usReQueueTimes,
                s.usReQueueTimes_,
                uiExpireTime,
                now,
                s.strSrcSql_.data());

            return;
        }
    }

    string strMsg;
    strMsg.append(s.strSrcSql_);
    strMsg.append("RabbitMQFlag=");
    strMsg.append(s.strId_);
    strMsg.append(";ReQueueTimes=");
    strMsg.append(Comm::int2str(s.usReQueueTimes_ + 1));
    strMsg.append(";ReQueueTime=");
    strMsg.append(Comm::int2str(uiFirstTime));

#ifdef SMSP_CONSUMER
    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    CHK_NULL_RETURN(pMQ);
    CHK_NULL_RETURN(g_pMqSendDbProducerThread);
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strData.assign(strMsg);
    g_pMqSendDbProducerThread->PostMsg(pMQ);
#endif

    LogWarn("===requeue=== sql[%s].", strMsg.data());
}

