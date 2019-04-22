#include "RuleLoadThread.h"
#include "LogMacro.h"
#include "ThreadMacro.h"
#include "AppData.h"
#include "AppDataFactory.h"

#include "boost/foreach.hpp"


#define foreach BOOST_FOREACH


RuleLoadThread* g_pRuleLoadThreadV2 = NULL;

const UInt64 TIMER_ID_CHECK_UPDATA = 1;
const UInt64 TIMER_ID_CHECK_DB_CONN = 2;


RuleLoadThread::RuleLoadThread(cs_t strDbHost,
                                      UInt16 usDbPort,
                                      cs_t strDbUserName,
                                      cs_t strDbPassword,
                                      cs_t strDatabaseName)
  : m_strDbHost(strDbHost),
    m_usDbPort(usDbPort),
    m_strDbUserName(strDbUserName),
    m_strDbPassword(strDbPassword),
    m_strDatabaseName(strDatabaseName)
{
    CThread::setName("RuleLoadThdV2");
}

RuleLoadThread::~RuleLoadThread()
{
    SAFE_DELETE(m_pDbConn);

    foreach (TblMgrMapValueType iter, m_mapTblMgr)
    {
        TableInfo* pTbl = iter.second;
        SAFE_DELETE(pTbl);
    }

    m_mapTblMgr.clear();
}

bool RuleLoadThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());
    INIT_CHK_FUNC_RET_FALSE(connect());

    regTbl(NULL, t_sms_table_update_time, "t_sms_table_update_time");

    //////////////////////////////////////////////////////////////////////////////////////////

    m_pTimerDbUpdate = SetTimer(TIMER_ID_CHECK_UPDATA, "", 1*1000);
    INIT_CHK_NULL_RET_FALSE(m_pTimerDbUpdate);

    m_pTimerDbConn = SetTimer(TIMER_ID_CHECK_DB_CONN, "", 3*1000);
    INIT_CHK_NULL_RET_FALSE(m_pTimerDbConn);

    return true;
}

bool RuleLoadThread::regTbl(CThread* pThread, UInt32 uiTblId, cs_t strTblName)
{
    TblMgrMapCIter iter = m_mapTblMgr.find(uiTblId);

    if (m_mapTblMgr.end() == iter)
    {
        LogDebug("[%s:%u] register table.", strTblName.data(), uiTblId);

        TableInfo* pTbl = new TableInfo();
        INIT_CHK_NULL_RET_FALSE(pTbl);

        pTbl->m_uiTableId = uiTblId;
        pTbl->m_strTableName = strTblName;
        pTbl->m_uiUpdateReqMsgId = uiTblId;
        pTbl->m_vecThread.push_back(pThread);
        pTbl->m_pFunc = NULL;
        pTbl->init();
        m_mapTblMgr[uiTblId] = pTbl;
        m_mapTblName2Id[strTblName] = uiTblId;
    }
    else
    {
        LogDebug("[%s:%u] already register table.", strTblName.data(), uiTblId);

        TableInfo* pTbl = iter->second;
        INIT_CHK_NULL_RET_FALSE(pTbl);

        pTbl->m_vecThread.push_back(pThread);
    }

    return true;
}

bool RuleLoadThread::checkDbUpdate()
{
    CHK_FUNC_RET_FALSE(getTblDataFromDb(t_sms_table_update_time));
    CHK_FUNC_RET_FALSE(saveUpdateTime());
    CHK_FUNC_RET_FALSE(send2Thread());
    return true;
}

bool RuleLoadThread::getTblDataFromDb(UInt32 uiTblId)
{
    CHK_NULL_RETURN_FALSE(m_pDbConn);

    TblMgrMapCIter iter;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapTblMgr, iter, uiTblId);

    TableInfo* pTbl = iter->second;
    CHK_NULL_RETURN_FALSE(pTbl);

    ////////////////////////////////////////////////////////////

    pTbl->clear();

    RecordSet rs(m_pDbConn);

    if (-1 == rs.ExecuteSQL(pTbl->m_strSql.data()))
    {
        LogError("Call ExecuteSQL failed. sql:%s.", pTbl->m_strSql.data());
        return false;
    }

    foreach (Record& r, rs.getRecordSet())
    {
        AppData* pAppData = AppDataFactory::getInstance().newAppData(pTbl->m_uiTableId);
        CHK_NULL_RETURN_FALSE(pAppData);

        pAppData->setData(r);
        pAppData->specialHandle();

        pTbl->insert(pAppData);
    }

    if (NULL != pTbl->m_pFunc)
    {
        if (!(this->*(pTbl->m_pFunc))())
        {
            LogError("Call func failed. tableid:%u.", pTbl->m_uiTableId);
        }
    }

    return true;
}

bool RuleLoadThread::saveUpdateTime()
{
    TblMgrMapCIter iter;
    CHK_MAP_FIND_UINT_RET_FALSE(m_mapTblMgr, iter, t_sms_table_update_time);

    TableInfo* pTblUpdateTime = iter->second;
    CHK_NULL_RETURN_FALSE(pTblUpdateTime);

    foreach (AppDataMapValueType it, pTblUpdateTime->m_mapAppData)
    {
        const string& strTblName = it.first;
        UInt32 uiTblIdTmp = m_mapTblName2Id[strTblName];

        TSmsTblUpdateTime* pTblUpdateTime = (TSmsTblUpdateTime*)(it.second);
        CHK_NULL_RETURN_FALSE(pTblUpdateTime);

        TblMgrMapCIter iterTbl = m_mapTblMgr.find(uiTblIdTmp);

        if (m_mapTblMgr.end() != iterTbl)
        {
            TableInfo* pTbl = iterTbl->second;
            CHK_NULL_RETURN_FALSE(pTbl);

            pTbl->saveUpdateTime(pTblUpdateTime->m_update_time);

//            LogDebug("Table[%s:%u] UpdateTime[%s], UpdateTimestamp[%u].",
//                strTblName.data(),
//                uiTblIdTmp,
//                pTblUpdateTime->m_update_time.data(),
//                pTbl->m_uiUpdateTimestamp);
        }
    }

    return true;
}

bool RuleLoadThread::send2Thread()
{
    foreach (TblMgrMapValueType& iter, m_mapTblMgr)
    {
        UInt32 uiTblId = iter.first;
        TableInfo* pTbl = iter.second;
        CHK_NULL_RETURN_FALSE(pTbl);

//        LogDebug("TableId[%u], TableName[%s], update_status[%d].",
//            uiTblId,
//            pTbl->m_strTableName.data(),
//            pTbl->m_bUpdate);

        if (!pTbl->m_bUpdate) continue;
        if (!getTblDataFromDb(uiTblId)) continue;
        if (!pTbl->send2Thread()) continue;
    }

    return true;
}

bool RuleLoadThread::connect()
{
    LogNotice("dbhost:%s, dbport:%u, dbuser:%s, dbpwd:%s, dbname:%s.",
        m_strDbHost.data(),
        m_usDbPort,
        m_strDbUserName.data(),
        m_strDbPassword.data(),
        m_strDatabaseName.data());

    MYSQL* pConn = mysql_init(NULL);
    CHK_NULL_RETURN_FALSE(pConn);

    if (NULL == mysql_real_connect(pConn,
                                    m_strDbHost.data(),
                                    m_strDbUserName.data(),
                                    m_strDbPassword.data(),
                                    m_strDatabaseName.data(),
                                    m_usDbPort,
                                    NULL,
                                    0))
    {
        LogError("connect to database fail error: %s", mysql_error(pConn));

        mysql_close(pConn);
        pConn = NULL;
        return false;
    }

    static const string strSql = "set names utf8";
    if (!mysql_real_query(pConn, strSql.c_str(), (unsigned long)strSql.length()))
    {
        //mysql_affected_rows(pConn);
    }
    else
    {
        LogError("set names utf8 is failed.");
        return false;
    }

    m_pDbConn = pConn;
    return true;
}

bool RuleLoadThread::pingDb()
{
    bool ret = false;

    if (NULL == m_pDbConn)
    {
        LogError("m_pDbConn is NULL, reconnect DB.");

        ret =  connect();
    }

    if ((NULL != m_pDbConn) && (0 != mysql_ping(m_pDbConn)))
    {
        LogError("ping fail, reconnect DB.");
        mysql_close(m_pDbConn);
        m_pDbConn = NULL;

        ret =  connect();
    }
    else
    {
        ret = true;
    }

    return ret;
}

void RuleLoadThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_TIMEOUT:
        {
            if (TIMER_ID_CHECK_UPDATA == pMsg->m_iSeq)
            {
                checkDbUpdate();

                SAFE_DELETE(m_pTimerDbUpdate);
                m_pTimerDbUpdate = SetTimer(TIMER_ID_CHECK_UPDATA, "", 6*1000);
            }
            else if (TIMER_ID_CHECK_DB_CONN == pMsg->m_iSeq)
            {
                pingDb();

                SAFE_DELETE(m_pTimerDbConn);
                m_pTimerDbConn = SetTimer(TIMER_ID_CHECK_DB_CONN, "", 3*1000);
            }
            else
            {
                LogError("timeout type is invalid.");
            }

            break;
        }

        default:
        {
            break;
        }
    }
}

