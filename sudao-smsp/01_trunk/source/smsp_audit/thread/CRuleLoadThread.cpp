#include "CRuleLoadThread.h"
#include "Comm.h"
#include "main.h"

#include "boost/foreach.hpp"

using namespace std;
using namespace models;

#define foreach BOOST_FOREACH


CRuleLoadThread::CRuleLoadThread(const char *name):
    CThread(name)
{
}

CRuleLoadThread::~CRuleLoadThread()
{
    if(NULL != m_DBConn)
    {
        mysql_close(m_DBConn);
        m_DBConn = NULL;
    }
}

std::string& CRuleLoadThread::trim(std::string& s)
{
    if (s.empty())
    {
        return s;
    }

    std::string::iterator c;
    // Erase whitespace before the string
    for (c = s.begin(); c != s.end() && iswspace(*c++);)
        ;
    s.erase(s.begin(), --c);

    // Erase whitespace after the string
    for (c = s.end(); c != s.begin() && iswspace(*--c);)
        ;
    s.erase(++c, s.end());

    return s;
}

bool CRuleLoadThread::Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
{
    m_DBHost = dbhost;
    m_DBUser = dbuser;
    m_DBPwd  = dbpwd;
    m_DBName = dbname;
    m_DBPort = dbport;

    if(false == CThread::Init())
    {
        return false;
    }

    if(!DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName))
    {
        LogError( "DBConnect() fail.\n");
        return false;
    }

    m_pDbTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "db_connect_ping", CHECK_DB_CONN_TIMEOUT);
    if(NULL == m_pDbTimer)
    {
        LogError( "SetTimer() fail.\n");
        return false;
    }

    getAllParamFromDB();
    getAllTableUpdateTime();

    /* send msg to LogThread, change default log level. */
    TUpdateSysParamRuleReq* pMsg2Log = new TUpdateSysParamRuleReq();
    pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
    pMsg2Log->m_SysParamMap = m_SysParamMap;
    g_pLogThread->PostMsg(pMsg2Log);

    m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "sessionid-check-db-update", 5*CHECK_TABLE_UPDATE_TIME_OUT);

    return true;
}

bool CRuleLoadThread::DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
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

bool CRuleLoadThread::DBPing()
{
    bool ret = false;
    if(m_DBConn == NULL)
    {
        LogError("m_DBConn is NULL, reconnect DB.");
        ret =  DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName);
    }

    //m_DBConn != NULL
    if((m_DBConn != NULL) && (0 != mysql_ping(m_DBConn)))
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

    return ret;
}

void CRuleLoadThread::getAllTableUpdateTime()
{
    m_SysParamLastUpdateTime    = getTableLastUpdateTime("t_sms_param");
    m_SysNoticeLastUpdateTime        = getTableLastUpdateTime("t_sms_sys_notice");
    m_SysNoticeUserLastUpdateTime    = getTableLastUpdateTime("t_sms_sys_notice_user");
    m_SysUserLastUpdateTime          = getTableLastUpdateTime("t_sms_user");
    m_strmiddleWareUpdateTime  = getTableLastUpdateTime("t_sms_middleware_configure");
    m_mqConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_mq_configure");
    m_componentRefMqUpdateTime = getTableLastUpdateTime("t_sms_component_ref_mq");
    m_componentConfigUpdateTime = getTableLastUpdateTime("t_sms_component_configure");
}

std::string CRuleLoadThread::getTableLastUpdateTime(std::string tableName)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    std::string updateTime;
    char sql[2048] = { 0 };

    snprintf(sql, 2048,"SELECT tablename, updatetime FROM t_sms_table_update_time where tablename='%s';", tableName.data());

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql))
    {
        if(0 < rs.GetRecordCount())
        {
            updateTime = rs[0]["updatetime"];
            return updateTime;
        }
    }
    else
    {
        LogError("sql execute err!");
    }

    return updateTime;
}

void CRuleLoadThread::HandleMsg(TMsg* pMsg)
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
        case  MSGTYPE_TIMEOUT:
        {
            if (CHECK_UPDATA_TIMER_MSGID == pMsg->m_iSeq)
            {
                checkDbUpdate();
                if(NULL != m_pTimer)
                {
                    delete m_pTimer;
                    m_pTimer = NULL;
                }
                m_pTimer = NULL;
                m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "", CHECK_TABLE_UPDATE_TIME_OUT);
            }
            else if (CHECK_DB_CONN_TIMER_MSGID == pMsg->m_iSeq)
            {
                DBPing();
                if(NULL != m_pDbTimer)
                {
                    delete m_pDbTimer;
                }
                m_pDbTimer = NULL;
                m_pDbTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "", CHECK_DB_CONN_TIMEOUT);
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

bool CRuleLoadThread::isTableUpdate(std::string tableName, std::string& lastUpdateTime)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    char sql[2048] = { 0 };

    snprintf(sql, 2048,"SELECT tablename, updatetime FROM t_sms_table_update_time \
            where tablename='%s' and updatetime>'%s';", tableName.data(), lastUpdateTime.data());

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql))
    {
        if(0 < rs.GetRecordCount())
        {
            lastUpdateTime = rs[0]["updatetime"];
            return true;
        }
    }
    else
    {
        LogError("sql execute err!");
    }

    return false;
}

void CRuleLoadThread::checkDbUpdate()
{
    if(isTableUpdate("t_sms_param", m_SysParamLastUpdateTime))
    {
        m_SysParamMap.clear();
        getDBSysParamMap(m_SysParamMap);

        TUpdateSysParamRuleReq* pMsg2Log = new TUpdateSysParamRuleReq();
        pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2Log->m_SysParamMap = m_SysParamMap;
        g_pLogThread->PostMsg(pMsg2Log);

        TUpdateSysParamRuleReq* pMsg2Audit = new TUpdateSysParamRuleReq();
        pMsg2Audit->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2Audit->m_SysParamMap = m_SysParamMap;
        g_pAuditServiceThread->PostMsg(pMsg2Audit);

        if (NULL != g_pAlarmThread)
        {
            TUpdateSysParamRuleReq* pMsg2Alarm = new TUpdateSysParamRuleReq();
            pMsg2Alarm->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsg2Alarm->m_SysParamMap = m_SysParamMap;
            g_pAlarmThread->PostMsg(pMsg2Alarm);
        }
        TUpdateSysParamRuleReq* pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );
    }
    if(isTableUpdate("t_sms_sys_notice", m_SysNoticeLastUpdateTime))
    {
        m_SysNoticeMap.clear();
        getDBSysNoticeMap(m_SysNoticeMap);
        TUpdateSysNoticeReq* pMsg = new TUpdateSysNoticeReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_UPDATE_REQ;
        pMsg->m_SysNoticeMap = m_SysNoticeMap;
        g_pAlarmThread->PostMsg(pMsg);
    }
    if(isTableUpdate("t_sms_sys_notice_user", m_SysNoticeUserLastUpdateTime))
    {
        m_SysNoticeUserList.clear();
        getDBSysNoticeUserList(m_SysNoticeUserList);
        TUpdateSysNoticeUserReq* pMsg = new TUpdateSysNoticeUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_USER_UPDATE_REQ;
        pMsg->m_SysNoticeUserList = m_SysNoticeUserList;
        g_pAlarmThread->PostMsg(pMsg);
    }
    if(isTableUpdate("t_sms_user", m_SysUserLastUpdateTime))
    {
        m_SysUserMap.clear();
        getDBSysUserMap(m_SysUserMap);
        TUpdateSysUserReq* pMsg = new TUpdateSysUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_USER_UPDATE_REQ;
        pMsg->m_SysUserMap = m_SysUserMap;
        //g_pCMPPServiceThread->PostMsg(pMsg);
    }

        /*中间件变更*/
    if( isTableUpdate("t_sms_middleware_configure", m_strmiddleWareUpdateTime ))
    {
        m_middleWareConfigInfo.clear();
        getDBMiddleWareConfig(m_middleWareConfigInfo);
        map <UInt32,MiddleWareConfig>::iterator itr = m_middleWareConfigInfo.find( MIDDLEWARE_TYPE_REDIS );
        if (itr != m_middleWareConfigInfo.end())
        {
            TRedisUpdateInfo* pReq = new TRedisUpdateInfo();
            pReq->m_strIP = itr->second.m_strHostIp;
            pReq->m_uPort = itr->second.m_uPort;
            pReq->m_strPwd = itr->second.m_strPassWord;
            pReq->m_iMsgType = MSGTYPE_RULELOAD_REDIS_ADDR_UPDATE_REQ;
            postRedisMngMsg(g_pRedisThreadPool, pReq );
        }
    }

    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        {
            TUpdateMqConfigReq* pMqCon = new TUpdateMqConfigReq();
            pMqCon->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
            pMqCon->m_mapMqConfig = m_mqConfigInfo;
            g_pMqdbConsumerThread->PostMsg(pMqCon);
        }

        {
            TUpdateMqConfigReq* pMqCon = new TUpdateMqConfigReq();
            pMqCon->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
            pMqCon->m_mapMqConfig = m_mqConfigInfo;
            g_pAuditServiceThread->PostMsg(pMqCon);
        }

    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        {
            TUpdateComponentRefMqReq * pMsgToConsumer = new TUpdateComponentRefMqReq();
            pMsgToConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
            pMsgToConsumer->m_componentRefMqInfo = m_componentRefMqInfo;
            g_pMqdbConsumerThread->PostMsg(pMsgToConsumer);
        }

        {
            TUpdateComponentRefMqReq * pMsgToConsumer = new TUpdateComponentRefMqReq();
            pMsgToConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
            pMsgToConsumer->m_componentRefMqInfo = m_componentRefMqInfo;
            g_pAuditServiceThread->PostMsg(pMsgToConsumer);
        }
    }

    if (isTableUpdate("t_sms_component_configure", m_componentConfigUpdateTime))
    {
        m_mapComponentConfigInfo.clear();
        getDBComponentConfig(m_mapComponentConfigInfo);
        TUpdateComponentConfigReq* pComponentConfig = new TUpdateComponentConfigReq();
        pComponentConfig->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        pComponentConfig->m_mapComponentConfigInfo= m_mapComponentConfigInfo;
        g_pMqdbConsumerThread->PostMsg(pComponentConfig);

    }
}

void CRuleLoadThread::getSysParamMap(std::map<std::string, std::string>& sysParamMap)
{
    pthread_mutex_lock(&m_mutex);
    sysParamMap = m_SysParamMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysUserMap(std::map<int, SysUser>& sysUserMap)
{
    pthread_mutex_lock(&m_mutex);
    sysUserMap = m_SysUserMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap)
{
    pthread_mutex_lock(&m_mutex);
    sysNoticeMap = m_SysNoticeMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList)
{
    pthread_mutex_lock(&m_mutex);
    sysNoticeUserList = m_SysNoticeUserList;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getNeedAuditMap(std::map<std::string, NeedAudit>& NeedAuditMap)
{
    pthread_mutex_lock(&m_mutex);
    NeedAuditMap = m_NeedAuditMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    middleWareConfigInfo = m_middleWareConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getComponentConfig(map<UInt64, ComponentConfig>& componentConfigInfoMap)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfoMap = m_mapComponentConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMqConfig(map<UInt64,MqConfig>& mqConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    mqConfigInfo = m_mqConfigInfo;
    pthread_mutex_unlock(&m_mutex);

}

void CRuleLoadThread::getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentRefMqInfo = m_componentRefMqInfo;
    pthread_mutex_unlock(&m_mutex);

}

bool CRuleLoadThread::getComponentSwitch(UInt32 uComponentId)
{
    pthread_mutex_lock(&m_mutex);
    map<UInt64,ComponentConfig>::iterator itor =  m_mapComponentConfigInfo.find(uComponentId);
    if(itor != m_mapComponentConfigInfo.end())
    {
        pthread_mutex_unlock(&m_mutex);
        return itor->second.m_uComponentSwitch;
    }
    else
    {
        pthread_mutex_unlock(&m_mutex);
        return false;
    }
}

bool CRuleLoadThread::getAllParamFromDB()
{
    bool ret = true;
    ret = ret && getDBSysParamMap(m_SysParamMap);
    ret = ret && getDBSysUserMap(m_SysUserMap);
    ret = ret && getDBSysNoticeMap(m_SysNoticeMap);
    ret = ret && getDBSysNoticeUserList(m_SysNoticeUserList);
    ret = ret && getDBNeedAuditMap(m_NeedAuditMap);
    ret = ret && getDBMiddleWareConfig(m_middleWareConfigInfo);
    ret = ret && getDBComponentConfig(m_mapComponentConfigInfo);
    ret = ret && getDBMqConfig(m_mqConfigInfo);
    ret = ret && getDBComponentRefMq(m_componentRefMqInfo);

    return ret;
}

bool CRuleLoadThread::getDBSysParamMap(std::map<std::string, std::string>& sysParamMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    std::string paramValue;
    std::string paramKey;
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_param where param_status = 1");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            paramValue = rs[i]["param_value"].data();
            paramKey   = rs[i]["param_key"].data();
            sysParamMap[paramKey] = paramValue;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSysUserMap(std::map<int, SysUser>& sysUserMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select id, email, mobile from t_sms_user");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SysUser sysUser;
            sysUser.m_iUserId   = atoi(rs[i]["id"].data());
            sysUser.m_Mobile = rs[i]["mobile"];
            sysUser.m_Email = rs[i]["email"];

            sysUserMap[sysUser.m_iUserId] = sysUser;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_sys_notice where status = '1'");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SysNotice sysNotice;
            sysNotice.m_iNoticeId = atoi(rs[i]["notice_id"].data());
            sysNotice.m_iStatus = atoi(rs[i]["status"].data());
            sysNotice.m_StartTime = rs[i]["start_date"];
            sysNotice.m_EndTime = rs[i]["end_date"];
			sysNotice.m_ucNoticeType = atoi(rs[i]["notice_type"].data());

            sysNoticeMap[sysNotice.m_iNoticeId] = sysNotice;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_sys_notice_user");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SysNoticeUser sysNoticeUser;
            sysNoticeUser.m_iNoticeId  = atoi(rs[i]["notice_id"].data());
            sysNoticeUser.m_iUserId    = atoi(rs[i]["user_id"].data());
            sysNoticeUser.m_iAlarmType = atoi(rs[i]["alarm_type"].data());
            sysNoticeUserList.push_back(sysNoticeUser);
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBNeedAuditMap(std::map<string, NeedAudit>& mapNeedAudit)
{
    CHK_NULL_RETURN_FALSE(m_DBConn);

    vector<string> v;
    v.push_back("t_sms_audit");
    v.push_back("t_sms_groupsend_audit");

    foreach(const string& s, v)
    {
        char sql[500] = { 0 };
        snprintf(sql, sizeof(sql), "select * from %s where removeflag = 0", s.data());

        RecordSet rs(m_DBConn);

        if (-1 == rs.ExecuteSQL(sql))
        {
            LogError("sql execute err!");
            return false;
        }

        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strMd5 = rs[i]["md5"];

            if (strMd5.empty())
            {
                LogWarn("t_sms_audit md5 is empty");
                continue;
            }

            NeedAudit needAudit;
            needAudit.strAuditId_ = rs[i]["auditid"];
            needAudit.strClientId_ = rs[i]["clientid"];
            mapNeedAudit[strMd5] = needAudit;
        }
    }

    return true;
}

bool CRuleLoadThread::getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_middleware_configure");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            MiddleWareConfig middleWare;
            middleWare.m_strHostIp = rs[i]["host_ip"];
            middleWare.m_strMiddleWareName = rs[i]["middleware_name"];
            middleWare.m_strPassWord = rs[i]["pass_word"];
            middleWare.m_strUserName = rs[i]["user_name"];
            middleWare.m_uMiddleWareId = atol(rs[i]["middleware_id"].data());
            middleWare.m_uMiddleWareType = atoi(rs[i]["middleware_type"].data());
            middleWare.m_uNodeId = atoi(rs[i]["node_id"].data());
            middleWare.m_uPort = atoi(rs[i]["port"].data());
            ////middleWareConfigInfo[middleWare.m_uMiddleWareType] = middleWare;
            middleWareConfigInfo.insert(make_pair(middleWare.m_uMiddleWareType,middleWare));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_component_configure");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ComponentConfig comInfo;
            comInfo.m_strComponentName = rs[i]["component_name"];
            comInfo.m_strComponentType = rs[i]["component_type"];
            comInfo.m_strHostIp = rs[i]["host_ip"];
            comInfo.m_uComponentId = atol(rs[i]["component_id"].data());
            comInfo.m_uNodeId = atoi(rs[i]["node_id"].data());
            comInfo.m_uRedisThreadNum = atoi(rs[i]["redis_thread_num"].data());
            comInfo.m_uSgipReportSwitch = atoi(rs[i]["sgip_report_switch"].data());
            comInfo.m_uMqId = atoi(rs[i]["mq_id"].data());
            comInfo.m_uComponentSwitch = atoi(rs[i]["component_switch"].data());
            componentConfigInfo.insert(make_pair(comInfo.m_uComponentId,comInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_component_configure execute err!");
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_mq_configure");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            MqConfig mqInfo;
            mqInfo.m_strExchange = rs[i]["mq_exchange"];
            mqInfo.m_strQueue = rs[i]["mq_queue"];
            mqInfo.m_strRemark = rs[i]["remark"];
            mqInfo.m_strRoutingKey = rs[i]["mq_routingkey"];
            mqInfo.m_uMiddleWareId = atol(rs[i]["middleware_id"].data());
            mqInfo.m_uMqId = atol(rs[i]["mq_id"].data());

            mqConfigInfo.insert(make_pair(mqInfo.m_uMqId,mqInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo)
{
    CHK_NULL_RETURN_FALSE(m_DBConn);

    RecordSet rs(m_DBConn);

    char sql[500] = {0};
    snprintf(sql, sizeof(sql), "select * from t_sms_component_ref_mq");

    if (-1 == rs.ExecuteSQL(sql))
    {
        LogError("sql table t_sms_component_ref_mq execute err!");
        return false;
    }

    for (int i = 0; i < rs.GetRecordCount(); ++i)
    {
        ComponentRefMq refInfo;
        refInfo.m_strMessageType = rs[i]["message_type"];
        refInfo.m_strRemark = rs[i]["remark"];
        refInfo.m_uComponentId = atol(rs[i]["component_id"].data());
        refInfo.m_uGetRate = atoi(rs[i]["get_rate"].data());
        refInfo.m_uMode = atoi(rs[i]["mode"].data());
        refInfo.m_uMqId = atol(rs[i]["mq_id"].data());
        refInfo.m_uId = atol(rs[i]["id"].data());
        refInfo.m_uWeight = atoi(rs[i]["weight"].data());

        string strKey;
        strKey.append(Comm::int2str(refInfo.m_uComponentId));
        strKey.append("_");
        strKey.append(refInfo.m_strMessageType);
        strKey.append("_");
        strKey.append(Comm::int2str(refInfo.m_uMode));

        // MQ(c2s_db)-Audit-Content-Request-Queue need multi-queue.
        // And smsp_audit need multi-component.
        // So there could be more than two records of same component_id,message_type.
        if (MESSAGE_AUDIT_CONTENT_REQ == refInfo.m_strMessageType)
        {
            strKey.append("_");
            strKey.append(Comm::int2str(refInfo.m_uMqId));
        }

        componentRefMqInfo.insert(make_pair(strKey, refInfo));
    }

    return true;
}




