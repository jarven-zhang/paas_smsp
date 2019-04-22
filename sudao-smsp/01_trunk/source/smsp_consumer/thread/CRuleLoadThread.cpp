#include "CRuleLoadThread.h"
#include "Comm.h"
#include "main.h"

using namespace models;
using namespace std;


void postSysParaMsg2DbThreadPool(map<string, string> mapTmp)
{
    CHK_NULL_RETURN(g_pDBThreadPool);

    for(UInt32 i = 0; i < g_pDBThreadPool->getThreadNum(); ++i)
    {
        CDBThread *pThread = g_pDBThreadPool->getThread(i);
        CHK_NULL_CONTINUE(pThread);

        TUpdateSysParamRuleReq *pMsg = new TUpdateSysParamRuleReq();
        CHK_NULL_CONTINUE(pMsg);
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg->m_SysParamMap = mapTmp;
        pThread->PostMsg(pMsg);
    }
}


CRuleLoadThread::CRuleLoadThread(const char *name):
    CThread(name)
{
    m_uBlackListSwitch = COMPONENT_BLACKLIST_CLOSE;
    m_uBlackListIndex = 0;
    m_uBlistTotalNum = 0;
    m_uBlackListCount = 0;
    m_uBlistDBIndex = 1;
}

CRuleLoadThread::~CRuleLoadThread()
{
    if(NULL != m_DBConn)
    {
        mysql_close(m_DBConn);
        m_DBConn = NULL;
    }
}

std::string &CRuleLoadThread::trim(std::string &s)
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

bool CRuleLoadThread::Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname, UInt32 uRedisSwitch)
{
    m_DBHost = dbhost;
    m_DBUser = dbuser;
    m_DBPwd  = dbpwd;
    m_DBName = dbname;
    m_DBPort = dbport;
    m_uRedisSwitch = uRedisSwitch;
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

    getAllParamFromDB();
    getAllTableUpdateTime();

    /* send msg to LogThread, change default log level. */
    TUpdateSysParamRuleReq *pMsg2Log = new TUpdateSysParamRuleReq();
    pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
    pMsg2Log->m_SysParamMap = m_SysParamMap;
    g_pLogThread->PostMsg(pMsg2Log);

    m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "sessionid-check-db-update", 5 * CHECK_TABLE_UPDATE_TIME_OUT);
    return true;
}

bool CRuleLoadThread::DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
{
    MYSQL *conn;
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
    m_SysParamLastUpdateTime         = getTableLastUpdateTime("t_sms_param");
    m_SysNoticeLastUpdateTime        = getTableLastUpdateTime("t_sms_sys_notice");
    m_SysNoticeUserLastUpdateTime    = getTableLastUpdateTime("t_sms_sys_notice_user");
    m_SysUserLastUpdateTime          = getTableLastUpdateTime("t_sms_user");

    m_mqConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_mq_configure");
    m_componentRefMqUpdateTime = getTableLastUpdateTime("t_sms_component_ref_mq");
    m_componentConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_component_configure");
    m_strmiddleWareUpdateTime  = getTableLastUpdateTime("t_sms_middleware_configure");
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

    snprintf(sql, 2048, "SELECT tablename, updatetime FROM t_sms_table_update_time where tablename='%s';", tableName.data());
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

void CRuleLoadThread::HandleMsg(TMsg *pMsg)
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
    case MSGTYPE_REDIS_RESP:
    {
        HandleRedisRespMsg(pMsg);
        break;
    }
    default:
    {
        break;
    }
    }

}

bool CRuleLoadThread::isTableUpdate(std::string tableName, std::string &lastUpdateTime)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    char sql[2048] = { 0 };

    snprintf(sql, 2048, "SELECT tablename, updatetime FROM t_sms_table_update_time "
             "where tablename='%s' and updatetime>'%s';", tableName.data(), lastUpdateTime.data());
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

        TUpdateSysParamRuleReq *pMsg2Log = new TUpdateSysParamRuleReq();
        pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2Log->m_SysParamMap = m_SysParamMap;
        g_pLogThread->PostMsg(pMsg2Log);

        TUpdateSysParamRuleReq *pMsg2DB = new TUpdateSysParamRuleReq();
        pMsg2DB->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2DB->m_SysParamMap = m_SysParamMap;
        g_pMQDBConsumerThread->PostMsg(pMsg2DB);

        if (NULL != g_pAlarmThread)
        {
            TUpdateSysParamRuleReq *pMsg2Alarm = new TUpdateSysParamRuleReq();
            pMsg2Alarm->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsg2Alarm->m_SysParamMap = m_SysParamMap;
            g_pAlarmThread->PostMsg(pMsg2Alarm);
        }

        TUpdateSysParamRuleReq *pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );

        postSysParaMsg2DbThreadPool(m_SysParamMap);
    }
    if(isTableUpdate("t_sms_sys_notice", m_SysNoticeLastUpdateTime))
    {
        m_SysNoticeMap.clear();
        getDBSysNoticeMap(m_SysNoticeMap);
        TUpdateSysNoticeReq *pMsg = new TUpdateSysNoticeReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_UPDATE_REQ;
        pMsg->m_SysNoticeMap = m_SysNoticeMap;
        g_pAlarmThread->PostMsg(pMsg);
    }
    if(isTableUpdate("t_sms_sys_notice_user", m_SysNoticeUserLastUpdateTime))
    {
        m_SysNoticeUserList.clear();
        getDBSysNoticeUserList(m_SysNoticeUserList);
        TUpdateSysNoticeUserReq *pMsg = new TUpdateSysNoticeUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_USER_UPDATE_REQ;
        pMsg->m_SysNoticeUserList = m_SysNoticeUserList;
        g_pAlarmThread->PostMsg(pMsg);
    }
    if(isTableUpdate("t_sms_user", m_SysUserLastUpdateTime))
    {
        m_SysUserMap.clear();
        getDBSysUserMap(m_SysUserMap);
        TUpdateSysUserReq *pMsg = new TUpdateSysUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_USER_UPDATE_REQ;
        pMsg->m_SysUserMap = m_SysUserMap;
        g_pAlarmThread->PostMsg(pMsg);
    }

    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        TUpdateMqConfigReq *pMq = new TUpdateMqConfigReq();
        pMq->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMq->m_mapMqConfig = m_mqConfigInfo;

        g_pMQDBConsumerThread->PostMsg(pMq);
    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        TUpdateComponentRefMqReq *pCom = new TUpdateComponentRefMqReq();
        pCom->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pCom->m_mapComponentRefMq = m_componentRefMqInfo;
        g_pMQDBConsumerThread->PostMsg(pCom);
    }
    if (isTableUpdate("t_sms_component_configure", m_componentConfigInfoUpdateTime))
    {
        m_mapComponentConfigInfo.clear();
        getDBComponentConfig(m_mapComponentConfigInfo);
        LogDebug("t_sms_component_configure update")
        if(CONSUMER_RELOAD_BLACKLIST != m_uRedisSwitch)
        {
            LogDebug("this component will not reload blacklist!")
        }
        else
        {
            map<UInt64, ComponentConfig>::iterator itrCom = m_mapComponentConfigInfo.find(g_uComponentId);
            if (itrCom == m_mapComponentConfigInfo.end())
            {
                LogError("componentid:%lu is not find in t_sms_component_configure.", g_uComponentId);
            }
            else
            {
                ComponentConfig componentInfo = itrCom->second;
                if((m_uBlackListSwitch != componentInfo.m_uBlacklistSwitch) && COMPONENT_BLACKLIST_OPEN == componentInfo.m_uBlacklistSwitch)
                {
                    FlushReidsBlacklist();
                }
                m_uBlackListSwitch = componentInfo.m_uBlacklistSwitch;
            }
        }

        TMsg *pMsgToMqConsumerThread = new TMsg();
        pMsgToMqConsumerThread->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMQDBConsumerThread->PostMsg(pMsgToMqConsumerThread);
    }

    /*中间件变更*/
    if( isTableUpdate("t_sms_middleware_configure", m_strmiddleWareUpdateTime ))
    {
        m_middleWareConfigInfo.clear();
        getDBMiddleWareConfig(m_middleWareConfigInfo);
        map <UInt32, MiddleWareConfig>::iterator itr = m_middleWareConfigInfo.find( MIDDLEWARE_TYPE_REDIS );
        if (itr != m_middleWareConfigInfo.end())
        {
            TRedisUpdateInfo *pReq = new TRedisUpdateInfo();
            pReq->m_strIP = itr->second.m_strHostIp;
            pReq->m_uPort = itr->second.m_uPort;
            pReq->m_strPwd = itr->second.m_strPassWord;
            pReq->m_iMsgType = MSGTYPE_RULELOAD_REDIS_ADDR_UPDATE_REQ;
            postRedisMngMsg(g_pRedisThreadPool, pReq );
        }
    }
}

void CRuleLoadThread::getSysParamMap(std::map<std::string, std::string> &sysParamMap)
{
    pthread_mutex_lock(&m_mutex);
    sysParamMap = m_SysParamMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysUserMap(std::map<int, SysUser> &sysUserMap)
{
    pthread_mutex_lock(&m_mutex);
    sysUserMap = m_SysUserMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysNoticeMap(std::map<int, SysNotice> &sysNoticeMap)
{
    pthread_mutex_lock(&m_mutex);
    sysNoticeMap = m_SysNoticeMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysNoticeUserList(std::list<SysNoticeUser> &sysNoticeUserList)
{
    pthread_mutex_lock(&m_mutex);
    sysNoticeUserList = m_SysNoticeUserList;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    middleWareConfigInfo = m_middleWareConfigInfo;
    pthread_mutex_unlock(&m_mutex);

}

void CRuleLoadThread::getComponentConfig(map<UInt64, ComponentConfig> &componentConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfo = m_mapComponentConfigInfo;
    pthread_mutex_unlock(&m_mutex);

}

void CRuleLoadThread::getMqConfig(map<UInt64, MqConfig> &mqConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    mqConfigInfo = m_mqConfigInfo;
    pthread_mutex_unlock(&m_mutex);

}

void CRuleLoadThread::getComponentRefMq(map<string, ComponentRefMq> &componentRefMqInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentRefMqInfo = m_componentRefMqInfo;
    pthread_mutex_unlock(&m_mutex);

}

bool CRuleLoadThread::getComponentSwitch(UInt32 uComponentId)
{
    pthread_mutex_lock(&m_mutex);
    map<UInt64, ComponentConfig>::iterator itor =  m_mapComponentConfigInfo.find(uComponentId);
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
    getDBSysParamMap(m_SysParamMap);
    getDBSysUserMap(m_SysUserMap);
    getDBSysNoticeMap(m_SysNoticeMap);
    getDBSysNoticeUserList(m_SysNoticeUserList);

    ////5.0
    getDBMiddleWareConfig(m_middleWareConfigInfo);
    getDBComponentConfig(m_mapComponentConfigInfo);
    getDBMqConfig(m_mqConfigInfo);
    getDBComponentRefMq(m_componentRefMqInfo);
    return true;
}

bool CRuleLoadThread::getDBSysParamMap(std::map<std::string, std::string> &sysParamMap)
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
    snprintf(sql, 500, "select * from t_sms_param where param_status = 1");
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

bool CRuleLoadThread::getDBSysUserMap(std::map<int, SysUser> &sysUserMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select id, email, mobile from t_sms_user");
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

bool CRuleLoadThread::getDBSysNoticeMap(std::map<int, SysNotice> &sysNoticeMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_sys_notice where status = '1'");
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

bool CRuleLoadThread::getDBSysNoticeUserList(std::list<SysNoticeUser> &sysNoticeUserList)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_sys_notice_user");
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

bool CRuleLoadThread::getDBMiddleWareConfig(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_middleware_configure");
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

            middleWareConfigInfo.insert(make_pair(middleWare.m_uMiddleWareType, middleWare));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBComponentConfig(map<UInt64, ComponentConfig> &componentConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_component_configure");
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
            comInfo.m_uBlacklistSwitch = atoi(rs[i]["black_list_switch"].data());
            comInfo.m_uComponentSwitch = atoi(rs[i]["component_switch"].data());
            componentConfigInfo.insert(make_pair(comInfo.m_uComponentId, comInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBMqConfig(map<UInt64, MqConfig> &mqConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_mq_configure");
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

            mqConfigInfo.insert(make_pair(mqInfo.m_uMqId, mqInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBComponentRefMq(map<string, ComponentRefMq> &componentRefMqInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_component_ref_mq");
    if (-1 != rs.ExecuteSQL(sql))
    {
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
            char temp[250] = {0};
            snprintf(temp, 250, "%lu_%s_%u_%lu", refInfo.m_uComponentId, refInfo.m_strMessageType.data(), refInfo.m_uMode, refInfo.m_uMqId);
            string strKey;
            strKey.assign(temp);

            componentRefMqInfo.insert(make_pair(strKey, refInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }

    return true;
}

Int32 CRuleLoadThread::getDBChannelBlackListMap(map<std::string, std::string> &ChannelBlackListMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return -1;
    }
    ChannelBlackListMap.clear();
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select id,cid,phone from t_sms_channel_black_list where status = '1' and id>'%lu' order by id asc limit %d;", m_uBlackListIndex, GET_DB_BLACK_LIST_NUM);
    if (-1 != rs.ExecuteSQL(sql))
    {
        m_uBlackListCount += rs.GetRecordCount();
        if(rs.GetRecordCount() > 0)
        {
            m_uBlackListIndex = Comm::strToUint64(rs[rs.GetRecordCount() - 1]["id"]);
        }

        //get db info into map string
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {

            string strChannelID = rs[i]["cid"];
            string strPhone = rs[i]["phone"];
            if(strPhone.empty() || strChannelID.empty())
            {
                LogWarn("phone[%s] channelid[%s] is not legal!!!", strPhone.data(), strChannelID.data());
                continue;
            }
            map<string, string>::iterator it = ChannelBlackListMap.find(strPhone);
            if(it == ChannelBlackListMap.end())
            {
                //find no uPhoneNum, then new it
                ChannelBlackListMap[strPhone] = strChannelID + "&";	//map add set
            }
            else
            {
                string strBlacklistInfo = it->second;
                it->second.append(strChannelID);
                it->second.append("&");
            }
        }
        return rs.GetRecordCount();
    }
    else
    {
        LogError("sql execute err!");
        return -1;
    }
}

Int32 CRuleLoadThread::getDBUnsubBlackList(map<string, string> &blackLists)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return -1;
    }
    blackLists.clear();
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "SELECT id,phone,clientid,smstypes from t_sms_blacklist_unsubscribe where id>'%lu' and state=1 order by id asc limit %d;"
             , m_uBlackListIndex , GET_DB_BLACK_LIST_NUM);

    if (-1 != rs.ExecuteSQL(sql))
    {
        m_uBlackListCount += rs.GetRecordCount();
        if(rs.GetRecordCount() > 0)
        {
            m_uBlackListIndex = Comm::strToUint64(rs[rs.GetRecordCount() - 1]["id"]);
        }
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strPhone = rs[i]["phone"];
            string strSmsTypes = rs[i]["smstypes"];
            string clientid = rs[i]["clientid"];

            if(strPhone.empty() || strSmsTypes.empty() || clientid.empty())
            {
                LogWarn("t_sms_black_list phone[%s] or smstypes[%s] or clientid[%s] is null!!"
                        , strPhone.c_str(), strSmsTypes.c_str(), clientid.c_str());
                continue;
            }
            map<string, string>::iterator it = blackLists.find(strPhone);
            if(it == blackLists.end())
            {
                blackLists[strPhone] = clientid + ":" + strSmsTypes + "&";
            }
            else
            {
                it->second.append(clientid);
                it->second.append(":");
                it->second.append(strSmsTypes);
                it->second.append("&");
            }
        }
        return rs.GetRecordCount();
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return -1;
    }

}

Int32 CRuleLoadThread::getDBBlackList(map<string, string> &blackLists)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return -1;
    }
    blackLists.clear();
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "SELECT id,phone,category_id FROM t_sms_blacklist_%d where id>'%lu' and state=1 order by id asc limit %d;"
             , m_uBlistDBIndex , m_uBlackListIndex , GET_DB_BLACK_LIST_NUM);

    if (-1 != rs.ExecuteSQL(sql))
    {
        m_uBlackListCount += rs.GetRecordCount();
        if(rs.GetRecordCount() > 0)
        {
            m_uBlackListIndex = Comm::strToUint64(rs[rs.GetRecordCount() - 1]["id"]);
        }
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strPhone = rs[i]["phone"];
            string strCat = rs[i]["category_id"];

            if(strPhone.empty() || strCat.empty())
            {
                LogWarn("t_sms_black_list phone[%s] or category_id[%lu] is null!!"
                        , strPhone.c_str(), strCat.c_str());
                continue;
            }
            map<string, string>::iterator it = blackLists.find(strPhone);
            if(it != blackLists.end())
            {
                LogWarn("phone[%s] config more blacklist", strPhone.c_str());
            }
            else
            {
                blackLists[strPhone] = "c:" + strCat + "&";
            }

        }
        return rs.GetRecordCount();
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return -1;
    }

}

Int32 CRuleLoadThread::getDBBlankPhone(set<string> &blankPhone)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return -1;
    }
    blankPhone.clear();
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "SELECT id,blank_phone from t_sms_blank_phone where id>'%lu' order by id asc limit %d;"
             , m_uBlackListIndex , GET_DB_BLACK_LIST_NUM);

    if (-1 != rs.ExecuteSQL(sql))
    {
        m_uBlackListCount += rs.GetRecordCount();
        if(rs.GetRecordCount() > 0)
        {
            m_uBlackListIndex = Comm::strToUint64(rs[rs.GetRecordCount() - 1]["id"]);
        }
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strPhone = rs[i]["blank_phone"];

            if(strPhone.empty())
            {
                LogWarn("t_sms_blank_phone phone[%s] is null!!" , strPhone.c_str());
                continue;
            }
            blankPhone.insert(strPhone);

        }
        return rs.GetRecordCount();
    }
    else
    {
        LogError("sql execute err!");
        return -1;
    }

}

void CRuleLoadThread::FlushReidsBlacklist()
{
    //now clear the redis
    LogNotice("attention please, start FLUSHDB 5!!");
    TRedisReq *pRedisClear = new TRedisReq();
    pRedisClear->m_RedisCmd = "FLUSHDB ";
    pRedisClear->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedisClear->m_strSessionID = "flushdb 5";
    pRedisClear->m_pSender = this;
    g_pRedisThreadPool[REDIS_COMMON_USE_CLEAN_5]->PostMsg(pRedisClear);
    return;
}
void CRuleLoadThread::HandleRedisRespMsg(TMsg *pMsg)
{
    TRedisResp *pRedisResp = (TRedisResp *)pMsg;
    if(NULL == pRedisResp)
    {
        LogError("pRedisResp is null!!");
        return;
    }
    LogDebug("==debug== redis SessionID[%s]!", pRedisResp->m_strSessionID.data());

    if ("flushdb 5" == pMsg->m_strSessionID)
    {
        if(pRedisResp->m_RedisReply && (0 == strncasecmp(pRedisResp->m_RedisReply->str, "OK", 2)))
        {
            //set black_list_switch close
            char strSql[256] = { 0 };
            RecordSet rs(m_DBConn);
            LogNotice("set black_list_switch close componetid[%lu]!!", g_uComponentId);
            snprintf(strSql, sizeof(strSql), "UPDATE t_sms_component_configure SET black_list_switch='%d' where component_id='%lu';",
                     COMPONENT_BLACKLIST_CLOSE, g_uComponentId);
            rs.ExecuteSQL(strSql);
            ReloadRedisBlacklistFromDb("sysblacklist");
        }
        else
        {
            LogWarn("flushdb 5 fail,please check!!");
        }
    }
    else if ("sysblacklist" == pMsg->m_strSessionID ||
             "unsubscribeblacklist" == pMsg->m_strSessionID ||
             "channleblacklist" == pMsg->m_strSessionID ||
             "blankphonelist" == pMsg->m_strSessionID)
    {
        ReloadRedisBlacklistFromDb(pMsg->m_strSessionID);
    }
    else if ("reload_over" == pMsg->m_strSessionID)
    {
        if(pRedisResp->m_RedisReply &&
                REDIS_REPLY_INTEGER == pRedisResp->m_RedisReply->type)
        {
            LogNotice("reload blacklist over!!");
        }
        else
        {
            LogWarn("reload fail,please check");
        }
    }
    else
    {
        LogWarn("unknow session[%s]", pMsg->m_strSessionID.data());
    }
    return;
}

bool CRuleLoadThread::ReloadDbBlackList2Redis(map<string, string> &BlackListMap, const string &strSessionID)
{
    UInt32 uBlacklistNum = 0;
    map<string, string>::iterator itr = BlackListMap.begin();
    for (; itr != BlackListMap.end(); ++itr)
    {
        m_uBlistTotalNum++;
        uBlacklistNum++;
        TRedisReq *pRedis = new TRedisReq();
        string strkey = "";
        if(uBlacklistNum >= BlackListMap.size())
        {
            //发送最后一条要求返回值
            pRedis->m_strSessionID = strSessionID;
            pRedis->m_pSender = this;
        }
        pRedis->m_strKey.append("black_list:");
        pRedis->m_strKey.append(itr->first);
        pRedis->m_RedisCmd = "APPEND " + pRedis->m_strKey + " " + itr->second;
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedis);
    }
    return true;
}

bool CRuleLoadThread::ReloadDbBlankPhone2Redis(set<string> &Blackphone, const string &strSessionID)
{
    UInt32 uBlacklistNum = 0;
    set<string>::iterator itr = Blackphone.begin();
    for (; itr != Blackphone.end(); ++itr)
    {
        m_uBlistTotalNum++;
        uBlacklistNum++;
        TRedisReq *pRedis = new TRedisReq();
        string strkey = "";
        if(uBlacklistNum >= Blackphone.size())
        {
            pRedis->m_strSessionID = strSessionID;
            pRedis->m_pSender = this;
        }
        pRedis->m_strKey.append("black_list:");
        pRedis->m_strKey.append(*itr);
        pRedis->m_RedisCmd = "APPEND " + pRedis->m_strKey + " blank&";
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedis);
    }
    return true;
}

void CRuleLoadThread::ReloadRedisBlacklistFromDb(string strSessionID)
{
    //sys blacklist
    if("sysblacklist" == strSessionID )
    {
        Int32 iDbCount = getDBBlackList(m_Sysblacklist);
        while (iDbCount <= 0)
        {
            m_uBlistDBIndex++;
            m_uBlackListIndex = 0;

            if (m_uBlistDBIndex >= 6)
            {
                LogNotice("sysblacklist reload session[%s] finished get dbcount[%u]!", strSessionID.data(), m_uBlackListCount);
                strSessionID = "unsubscribeblacklist";

                m_uBlackListCount = 0;
                break;
            }
            else
            {
                iDbCount = getDBBlackList(m_Sysblacklist);
            }
        }
        if("sysblacklist" == strSessionID && iDbCount > 0)
        {
            if(iDbCount < GET_DB_BLACK_LIST_NUM)
            {
                m_uBlistDBIndex++;
                if (m_uBlistDBIndex >= 6)
                {
                    LogNotice("sysblacklist reload session[%s] finished get dbcount[%u]!", strSessionID.data(), m_uBlackListCount);
                    strSessionID = "unsubscribeblacklist";
                    m_uBlackListCount = 0;
                }
                m_uBlackListIndex = 0;
            }
            else if(GET_DB_BLACK_LIST_NUM == iDbCount)
            {
                LogNotice("sysblacklist reload session[%s] not finish! getSysBlacklistNum[%u]", strSessionID.data(), m_uBlackListCount);
            }

            ReloadDbBlackList2Redis(m_Sysblacklist, strSessionID);
            return;
        }

    }

    //channel blacklist
    if("unsubscribeblacklist" == strSessionID)
    {
        Int32 iDbCount = getDBUnsubBlackList(m_UnsubBlacklistInfo);

        if(GET_DB_BLACK_LIST_NUM == iDbCount )
        {
            ReloadDbBlackList2Redis(m_UnsubBlacklistInfo, strSessionID);
            LogNotice("channleblacklist reload session[%s] not finish! getChannleBlacklistNum[%u]", strSessionID.data(), m_uBlackListCount);
            return;
        }
        ReloadDbBlackList2Redis(m_UnsubBlacklistInfo, "channleblacklist");
        LogNotice("unsubscribeblacklist reload session[%s] finished get dbcount[%u]!", strSessionID.data(), m_uBlackListCount);

        m_UnsubBlacklistInfo.clear();
        m_Sysblacklist.clear();
        m_uBlackListIndex = 0;
        m_uBlackListCount = 0;
        m_uBlistDBIndex = 1;
    }

    //channel blacklist
    if("channleblacklist" == strSessionID)
    {
        Int32 iDbCount = getDBChannelBlackListMap(m_ChannelBlacklistInfo);

        if(GET_DB_BLACK_LIST_NUM == iDbCount )
        {
            ReloadDbBlackList2Redis(m_ChannelBlacklistInfo, strSessionID);
            LogNotice("channleblacklist reload session[%s] not finish! getChannleBlacklistNum[%u]", strSessionID.data(), m_uBlackListCount);
            return;
        }
        ReloadDbBlackList2Redis(m_ChannelBlacklistInfo, "blankphonelist");
        LogNotice("channleblacklist reload session[%s] finished get dbcount[%u]!", strSessionID.data(), m_uBlackListCount);

        m_ChannelBlacklistInfo.clear();
        m_UnsubBlacklistInfo.clear();
        m_uBlackListIndex = 0;
        m_uBlackListCount = 0;
    }

    if("blankphonelist" == strSessionID)
    {
        Int32 iDbCount = getDBBlankPhone(m_BlankPhonelist);

        if(GET_DB_BLACK_LIST_NUM == iDbCount )
        {
            ReloadDbBlankPhone2Redis(m_BlankPhonelist, strSessionID);
            LogNotice("blankphonelist reload session[%s] not finish! getDBBlankPhone[%u]", strSessionID.data(), m_uBlackListCount);
            return;
        }
        ReloadDbBlankPhone2Redis(m_BlankPhonelist, "reload_over");
        LogNotice("blankphonelist reload session[%s] finished get dbcount[%u]!", strSessionID.data(), m_uBlackListCount);

        LogNotice("reload dbblacklist over totalnum[%u]", m_uBlistTotalNum);
        m_uBlackListSwitch = COMPONENT_BLACKLIST_OPEN;
        m_BlankPhonelist.clear();
        m_uBlackListIndex = 0;
        m_uBlackListCount = 0;
    }
    return;
}
