#include "CRuleLoadThread.h"
#include "CDBThread.h"
#include "CUnityThread.h"
#include "GetReportServerThread.h"
#include "GetBalanceServerThread.h"
#include "CQueryReportServerThread.h"
#include "main.h"

using namespace models;

CRuleLoadThread::CRuleLoadThread(const char *name):
    CThread(name)
{
    m_bInited = false;
    m_pTimer = NULL;
}

CRuleLoadThread::~CRuleLoadThread()
{
    if(NULL != m_DBConn)
    {
        mysql_close(m_DBConn);
        m_DBConn = NULL;
    }
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

    string strReg = "([0-1][0-9]|[2][0-4]):[0-5][0-9]";
    try
    {
        LogDebug("generator time reg, strReg[%s].", strReg.data());
        m_regTime.assign(strReg.data());
    }
    catch(...)
    {
        LogError("time reg compile error. str[%s]", strReg.data());
        return false;
    }

    // 定时短信任务表要定期扫描，加载新任务
    m_uTimersendTaskTblRescanInterval = 5;
    m_pTimersendTaskRescanTimer = NULL;

    getAllParamFromDB();
    getAllTableUpdateTime();

    /* send msg to LogThread, change default log level. */
    TUpdateSysParamRuleReq *pMsg2Log = new TUpdateSysParamRuleReq();
    pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
    pMsg2Log->m_SysParamMap = m_SysParamMap;
    g_pLogThread->PostMsg(pMsg2Log);

    m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "sessionid-check-db-update", 5 * CHECK_TABLE_UPDATE_TIME_OUT);

    m_bInited = true;

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

bool CRuleLoadThread::TimeRegCheck(string strTime)		//check format like 08:20
{
    if(strTime.empty())
    {
        return false;
    }

    string strKeyWord = "";
    cmatch what;
    if(regex_search(strTime.c_str(), what, m_regTime))
    {
        if(!what.empty())
        {
            strKeyWord = what[0].str();
        }
        LogDebug("time reg match suc , keyword[%s]!", strKeyWord.data());
        return true;
    }
    else
    {
        LogDebug("time reg match fail");
        return false;
    }

}


void CRuleLoadThread::getAllTableUpdateTime()
{
    m_ChannelLastUpdateTime     = getTableLastUpdateTime("t_sms_channel");
    m_UserGwLastUpdateTime      = getTableLastUpdateTime("t_user_gw");
    m_SignextnoGwLastUpdateTime = getTableLastUpdateTime("t_signextno_gw");
    m_PriceLastUpdateTime       = getTableLastUpdateTime("t_sms_tariff");
    m_strSalePriceLastUpdateTime       = getTableLastUpdateTime("t_sms_client_tariff");
    m_TemplateOverrateLastUpdateTime = getTableLastUpdateTime("t_template_overrate");
    m_PhoneAreaLastUpdateTime        = getTableLastUpdateTime("t_sms_phone_section");
    m_OperaterSegmentUpdateTime = getTableLastUpdateTime("t_sms_operater_segment");
    m_OverRateWhiteListUpdateTime = getTableLastUpdateTime("t_sms_overrate_white_list");
    m_SysParamLastUpdateTime = getTableLastUpdateTime("t_sms_param");
    m_SysNoticeLastUpdateTime        = getTableLastUpdateTime("t_sms_sys_notice");
    m_SysNoticeUserLastUpdateTime    = getTableLastUpdateTime("t_sms_sys_notice_user");
    m_SysUserLastUpdateTime          = getTableLastUpdateTime("t_sms_user");
    m_SmsAccountLastUpdateTime       = getTableLastUpdateTime("t_sms_account");
    m_SmsTestAccountLastUpdateTime   = getTableLastUpdateTime("t_sms_test_account");
    m_SmsAccountStateLastUpdateTime  = getTableLastUpdateTime("t_sms_account_login_status");
    m_SmsSendIdIpLastUpdateTime      = getTableLastUpdateTime("t_sms_sendip_id");
    m_SmsClientidSignPortUpdateTime = getTableLastUpdateTime("t_sms_clientid_signport");
    m_NoAuditKeyWordUpdateTime = getTableLastUpdateTime("t_sms_user_noaudit_keyword");
    m_ChannelExtendPortTableUpdateTime = getTableLastUpdateTime("t_sms_channel_extendport");

    m_systemErrorCodeUpdateTime = getTableLastUpdateTime("t_sms_system_error_desc");

    ////smsp5.0
    m_mqConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_mq_configure");
    m_componentRefMqUpdateTime = getTableLastUpdateTime("t_sms_component_ref_mq");
    m_componentConfigUpdateTime = getTableLastUpdateTime("t_sms_component_configure");

    //template
    m_strSmsTemplateUpdateTime = getTableLastUpdateTime("t_sms_template");
    m_strmiddleWareUpdateTime  = getTableLastUpdateTime("t_sms_middleware_configure");
    m_AgentInfoUpdateTime = getTableLastUpdateTime("t_sms_agent_info");

    m_TimersendTaskInfoUpdateTime = getTableLastUpdateTime("t_sms_timer_send_task");

    m_strSysAuthenticationUpdateTime = getTableLastUpdateTime("t_sms_sys_authentication");
}

std::string CRuleLoadThread::getTableLastUpdateTime(std::string tableName)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return "";
    }

    std::string updateTime = "";
    char sql[1024] = { 0 };

    snprintf(sql, 1024, "SELECT tablename, updatetime FROM t_sms_table_update_time where tablename='%s';", tableName.data());

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
            SAFE_DELETE(m_pTimer);
            m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "", CHECK_TABLE_UPDATE_TIME_OUT);
        }
        else if (CHECK_DB_CONN_TIMER_MSGID == pMsg->m_iSeq)
        {
            DBPing();
            SAFE_DELETE(m_pDbTimer);
            m_pDbTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "", CHECK_DB_CONN_TIMEOUT);
        }
        else if (TIMERSEND_TASKS_RESCAN_TIMERID == pMsg->m_iSeq)
        {
            LogNotice("TimerSendTask rescanning, interval[%u] ....", m_uTimersendTaskTblRescanInterval);
            // 定时重新扫描加载任务
            m_mapTimersendTaskInfo.clear();
            getDBTimersendTaskInfo(m_mapTimersendTaskInfo);

            TUpdateTimerSendTaskInfoReq *pHttp = new TUpdateTimerSendTaskInfoReq();
            pHttp->m_iMsgType = MSGTYPE_RULELOAD_TIMERSEND_TASKINFO_UPDATE_REQ;
            pHttp->m_mapTimersendTaskInfo = m_mapTimersendTaskInfo;
            g_pUnitypThread->PostMsg(pHttp);

            tryResetTimersendRescanTimer(m_uTimersendTaskTblRescanInterval);
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

bool CRuleLoadThread::isTableUpdate(std::string tableName, std::string &lastUpdateTime)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    char sql[2048] = { 0 };

    snprintf(sql, 2048, "SELECT tablename, updatetime FROM t_sms_table_update_time \
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
    if(isTableUpdate("t_sms_channel", m_ChannelLastUpdateTime))
    {
        m_ChannelMap.clear();
        getDBChannlelMap(m_ChannelMap);

        if(g_pReportReceiveThread != NULL)
        {
            TUpdateChannelReq *pMsgC = new TUpdateChannelReq();
            pMsgC->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            pMsgC->m_ChannelMap = m_ChannelMap;
            g_pReportReceiveThread->PostMsg(pMsgC);
        }
    }


    if(isTableUpdate("t_user_gw", m_UserGwLastUpdateTime))
    {
        m_UserGwMap.clear();
        getDBUserGwMap(m_UserGwMap);
        TUpdateUserGwReq *pMsg = new TUpdateUserGwReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
        pMsg->m_UserGwMap = m_UserGwMap;
        g_pUnitypThread->PostMsg(pMsg);

    }

    if(isTableUpdate("t_sms_param", m_SysParamLastUpdateTime))
    {
        m_SysParamMap.clear();
        getDBSysParamMap(m_SysParamMap);

        TUpdateSysParamRuleReq *pMsg2Log = new TUpdateSysParamRuleReq();
        pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2Log->m_SysParamMap = m_SysParamMap;
        g_pLogThread->PostMsg(pMsg2Log);

        if (g_pAlarmThread)
        {
            TUpdateSysParamRuleReq *pMsg2Alarm = new TUpdateSysParamRuleReq();
            pMsg2Alarm->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsg2Alarm->m_SysParamMap = m_SysParamMap;
            g_pAlarmThread->PostMsg(pMsg2Alarm);
        }

        if(g_pGetBalanceServerThread)
        {
            TUpdateSysParamRuleReq* pMsgGetBlc = new TUpdateSysParamRuleReq();
            pMsgGetBlc->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsgGetBlc->m_SysParamMap = m_SysParamMap;
            g_pGetBalanceServerThread->PostMsg(pMsgGetBlc);
        }

        if (g_pQueryReportServerThread)
        {
            TUpdateSysParamRuleReq *pMsgQueryReport = new TUpdateSysParamRuleReq();
            pMsgQueryReport->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsgQueryReport->m_SysParamMap = m_SysParamMap;
            g_pQueryReportServerThread->PostMsg(pMsgQueryReport);
        }

        TUpdateSysParamRuleReq *pHttpSend = new TUpdateSysParamRuleReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pHttpSend->m_SysParamMap = m_SysParamMap;
        g_pUnitypThread->PostMsg(pHttpSend);

        TUpdateSysParamRuleReq *pHttpMsg = new TUpdateSysParamRuleReq();
        pHttpMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pHttpMsg->m_SysParamMap = m_SysParamMap;
        g_pHttpServiceThread->PostMsg(pHttpMsg);

        TUpdateSysParamRuleReq *pRtpRev = new TUpdateSysParamRuleReq();
        pRtpRev->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRtpRev->m_SysParamMap = m_SysParamMap;
        g_pReportReceiveThread->PostMsg(pRtpRev);

        TUpdateSysParamRuleReq *pReportPush = new TUpdateSysParamRuleReq();
        pReportPush->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pReportPush->m_SysParamMap = m_SysParamMap;
        g_pReportPushThread->PostMsg(pReportPush);

        TUpdateSysParamRuleReq *pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );

        if( NULL != g_pMQMonitorPublishThread )
        {
            TUpdateSysParamRuleReq *pMonitorMsg = new TUpdateSysParamRuleReq();
            pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pRedisMsg->m_SysParamMap = m_SysParamMap;
            g_pMQMonitorPublishThread->PostMsg(pMonitorMsg);
        }

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


    if(isTableUpdate("t_sms_account", m_SmsAccountLastUpdateTime))
    {
        m_SmsAccountMap.clear();
        getDBSmsAccontMap(m_SmsAccountMap);

        if (g_pQueryReportServerThread)
        {
            TUpdateSmsAccontReq *pMsg = new TUpdateSmsAccontReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
            pMsg->m_SmsAccountMap = m_SmsAccountMap;
            g_pQueryReportServerThread->PostMsg(pMsg);
        }

        if (g_pGetBalanceServerThread)
        {
            TUpdateSmsAccontReq *pMsg = new TUpdateSmsAccontReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
            pMsg->m_SmsAccountMap = m_SmsAccountMap;
            g_pGetBalanceServerThread->PostMsg(pMsg);
        }

        if (g_pGetReportServerThread)
        {
            TUpdateSmsAccontReq *pMsg = new TUpdateSmsAccontReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
            pMsg->m_SmsAccountMap = m_SmsAccountMap;
            g_pGetReportServerThread->PostMsg(pMsg);
        }


        ////reportpush
        TUpdateSmsAccontReq *phttpPushMsg = new TUpdateSmsAccontReq();
        phttpPushMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        phttpPushMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pReportPushThread->PostMsg(phttpPushMsg);

        ///reportreceive
        TUpdateSmsAccontReq *pReportMsg = new TUpdateSmsAccontReq();
        pReportMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pReportMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pReportReceiveThread->PostMsg(pReportMsg);

        ////httpsend
        TUpdateSmsAccontReq *pHttpSendMsg = new TUpdateSmsAccontReq();
        pHttpSendMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pHttpSendMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pUnitypThread->PostMsg(pHttpSendMsg);

        ////httpservice
        TUpdateSmsAccontReq *pHttpSerMsg = new TUpdateSmsAccontReq();
        pHttpSerMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pHttpSerMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pHttpServiceThread->PostMsg(pHttpSerMsg);

    }
    if(isTableUpdate("t_sms_test_account", m_SmsTestAccountLastUpdateTime))
    {
        m_SmsTestAccount.clear();
        getDBSmsTestAccont(m_SmsTestAccount);

        ////httpsend
        TUpdateSmsTestAccontReq *pHttpSendMsg = new TUpdateSmsTestAccontReq();
        pHttpSendMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSTESTACCOUNT_UPDATE_REQ;
        pHttpSendMsg->m_SmsTestAccount = m_SmsTestAccount;
        g_pUnitypThread->PostMsg(pHttpSendMsg);

    }
    if(isTableUpdate("t_sms_clientid_signport", m_SmsClientidSignPortUpdateTime))
    {
        m_ClientIdAndSignMap.clear();
        m_ClientIdAndSignPortMap.clear();
        getDBClientAndSignMap(m_ClientIdAndSignMap, m_ClientIdAndSignPortMap);

        ///
        TUpdateClientIdAndSignReq *pReport = new TUpdateClientIdAndSignReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ;
        pReport->m_ClientIdAndSignMap = m_ClientIdAndSignPortMap;
        g_pReportReceiveThread->PostMsg(pReport);

        ////cunity
        TUpdateClientIdAndSignReq *pUnity = new TUpdateClientIdAndSignReq();
        pUnity->m_iMsgType = MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ;
        pUnity->m_ClientIdAndSignMap = m_ClientIdAndSignMap;
        g_pUnitypThread->PostMsg(pUnity);
    }

    if(isTableUpdate("t_sms_operater_segment", m_OperaterSegmentUpdateTime))
    {
        m_OperaterSegmentMap.clear();
        getDBOperaterSegmentMap(m_OperaterSegmentMap);	//

        TUpdateOperaterSegmentReq *pHttpSend = new TUpdateOperaterSegmentReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ;
        pHttpSend->m_OperaterSegmentMap = m_OperaterSegmentMap;
        g_pUnitypThread->PostMsg(pHttpSend);
    }
    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        TUpdateMqConfigReq *pMqConfig = new TUpdateMqConfigReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfig->m_mapMqConfig = m_mqConfigInfo;
        g_pUnitypThread->PostMsg(pMqConfig);

        TUpdateMqConfigReq* pReq = new TUpdateMqConfigReq();
        pReq->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pReq->m_mapMqConfig = m_mqConfigInfo;
        g_pReportPushThread->PostMsg(pReq);
   
    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        TUpdateComponentRefMqReq *pMqConfig = new TUpdateComponentRefMqReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pMqConfig->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pUnitypThread->PostMsg(pMqConfig);

    }

    if (isTableUpdate("t_sms_component_configure", m_componentConfigUpdateTime))
    {
        getDBComponentConfig(m_componentConfigInfo);
     
        TUpdateComponentConfigReq* pReq = new TUpdateComponentConfigReq();
        pReq->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        pReq->m_componentConfig= m_componentConfigInfo;
        g_pReportPushThread->PostMsg(pReq);

        MONITOR_UPDATE_COMP( g_pMQMonitorPublishThread, m_componentConfigInfo );
    }

    if (isTableUpdate("t_sms_template", m_strSmsTemplateUpdateTime))
    {
        m_mapSmsTemplate.clear();
        getDBSmsTemplateMap(m_mapSmsTemplate);

        TUpdateSmsTemplateReq *pMsg = new TUpdateSmsTemplateReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SMS_TEMPLATE_UPDATE_REQ;
        pMsg->m_mapSmsTemplate = m_mapSmsTemplate;
        g_pHttpServiceThread->PostMsg(pMsg);
    }

    if (isTableUpdate("t_sms_system_error_desc", m_systemErrorCodeUpdateTime))
    {
        m_mapSystemErrorCode.clear();
        getDBSystemErrorCode(m_mapSystemErrorCode);

        TUpdateSystemErrorCodeReq *pHttp = new TUpdateSystemErrorCodeReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
        pHttp->m_mapSystemErrorCode = m_mapSystemErrorCode;
        g_pUnitypThread->PostMsg(pHttp);

        TUpdateSystemErrorCodeReq *pHttp2 = new TUpdateSystemErrorCodeReq();
        pHttp2->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
        pHttp2->m_mapSystemErrorCode = m_mapSystemErrorCode;
        g_pHttpServiceThread->PostMsg(pHttp2);
    }

    if (isTableUpdate("t_sms_timer_send_task", m_TimersendTaskInfoUpdateTime))
    {
        m_mapTimersendTaskInfo.clear();
        getDBTimersendTaskInfo(m_mapTimersendTaskInfo);

        TUpdateTimerSendTaskInfoReq *pHttp = new TUpdateTimerSendTaskInfoReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_TIMERSEND_TASKINFO_UPDATE_REQ;
        pHttp->m_mapTimersendTaskInfo = m_mapTimersendTaskInfo;
        g_pUnitypThread->PostMsg(pHttp);
    }

    if (isTableUpdate("t_sms_agent_info", m_AgentInfoUpdateTime))
    {
        m_AgentInfoMap.clear();
        getDBAgentInfo(m_AgentInfoMap);

        TUpdateAgentInfoReq *pMsg = new TUpdateAgentInfoReq();
        
        if (NULL != g_pHttpServiceThread)
        {
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ;
            pMsg->m_AgentInfoMap = m_AgentInfoMap;
            g_pHttpServiceThread->PostMsg(pMsg);
        }

        if (NULL != g_pGetBalanceServerThread)
        {
            TUpdateAgentInfoReq* pMsg = new TUpdateAgentInfoReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ;
            pMsg->m_AgentInfoMap = m_AgentInfoMap;
            g_pGetBalanceServerThread->PostMsg(pMsg);
        }
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

    if (isTableUpdate("t_sms_sys_authentication", m_strSysAuthenticationUpdateTime))
    {
        getDBSysAuthentication();

        DbUpdateReqHttp<SysAuthenticationMap> *pReq = new DbUpdateReqHttp<SysAuthenticationMap>();
        CHK_NULL_RETURN(pReq);
        pReq->m_map = m_mapSysAuthentication;
        pReq->m_iMsgType = MSGTYPE_RULELOAD_SYS_AUTHENTICATION_UPDATE_REQ;
        g_pGetBalanceServerThread->PostMsg(pReq);
    }

}

void CRuleLoadThread::getSendIpIdMap(std::map<UInt32, string> &sendIpId)
{
    pthread_mutex_lock(&m_mutex);
    sendIpId = m_sendIpId;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getNoAuditKeyWordMap(map<string, RegList> &NoAuditKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    NoAuditKeyWordMap = m_NoAuditKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannlelMap(std::map<int , Channel> &channelmap)
{
    pthread_mutex_lock(&m_mutex);
    channelmap = m_ChannelMap;
    pthread_mutex_unlock(&m_mutex);
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

void CRuleLoadThread::getSmsAccountMap(std::map<std::string, SmsAccount> &SmsAccountMap)
{
    pthread_mutex_lock(&m_mutex);
    SmsAccountMap = m_SmsAccountMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmsTestAccount(std::set<std::string> &SmsTestAccount)
{
    pthread_mutex_lock(&m_mutex);
    SmsTestAccount = m_SmsTestAccount;
    pthread_mutex_unlock(&m_mutex);
}
void CRuleLoadThread::getSmsClientAndSignMap(std::map<string, ClientIdSignPort> &smsClientIdAndSignMap)
{
    pthread_mutex_lock(&m_mutex);
    smsClientIdAndSignMap = m_ClientIdAndSignMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmsClientAndSignPortMap(map<string, ClientIdSignPort> &smsClientIdAndSignPortMap)
{
    pthread_mutex_lock(&m_mutex);
    smsClientIdAndSignPortMap = m_ClientIdAndSignPortMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getUserGwMap(std::map<std::string, UserGw> &userGwMap)
{
    pthread_mutex_lock(&m_mutex);
    userGwMap = m_UserGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getOperaterSegmentMap(map<string, UInt32> &PhoneHeadMap)
{
    pthread_mutex_lock(&m_mutex);
    PhoneHeadMap = m_OperaterSegmentMap;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getBlackList(set<UInt64> &blackLists)
{
    pthread_mutex_lock(&m_mutex);
    blackLists = m_blackLists;
    pthread_mutex_unlock(&m_mutex);
    return true;
}

bool CRuleLoadThread::getKeyWordList(list<regex> &listKeyWordReg)
{
    pthread_mutex_lock(&m_mutex);
    listKeyWordReg = m_listKeyWordReg;
    pthread_mutex_unlock(&m_mutex);
    return true;
}

void CRuleLoadThread::getSignExtnoGwMap(std::map<std::string, SignExtnoGw> &signextnoGwMap)
{
    pthread_mutex_lock(&m_mutex);
    signextnoGwMap = m_SignextnoGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAgentInfo(AgentInfoMap &agentInfoMap)
{
    pthread_mutex_lock(&m_mutex);
    agentInfoMap = m_AgentInfoMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getSmppPriceMap(std::map<std::string, std::string> &priceMap)
{
    pthread_mutex_lock(&m_mutex);
    priceMap = m_PriceMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmppSalePriceMap(std::map<std::string, std::string> &salePriceMap)
{
    pthread_mutex_lock(&m_mutex);
    salePriceMap = m_SalePriceMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAllTempltRule(std::map<string , TempRule> &tempRuleMap)
{
    pthread_mutex_lock(&m_mutex);
    tempRuleMap = m_TempRuleMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getPhoneAreaMap(std::map<std::string, int> &phoneAreaMap)
{
    pthread_mutex_lock(&m_mutex);
    phoneAreaMap = m_PhoneAreaMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getOverRateWhiteList(set<string> &overRateWhiteList)
{
    pthread_mutex_lock(&m_mutex);
    overRateWhiteList = m_overRateWhiteList;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    middleWareConfigInfo = m_middleWareConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getComponentConfig(ComponentConfig &componentConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfo = m_componentConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getListenPort(map<string, ListenPort> &listenPortInfo)
{
    pthread_mutex_lock(&m_mutex);
    listenPortInfo = m_listenPortInfo;
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

void CRuleLoadThread::getSysAuthentication(SysAuthenticationMap &mapSysAuthentication)
{
    pthread_mutex_lock(&m_mutex);
    mapSysAuthentication = m_mapSysAuthentication;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getSmsTemplateMap(map<std::string, SmsTemplate> &smsTemplateMap)
{
    pthread_mutex_lock(&m_mutex);
    smsTemplateMap = m_mapSmsTemplate;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSystemErrorCode(map<string, string> &systemErrorCode)
{
    pthread_mutex_lock(&m_mutex);
    systemErrorCode = m_mapSystemErrorCode;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getTimersendTaskInfo(map<string, timersend_taskinfo_ptr_t> &mapTimersendTaskInfo)
{
    pthread_mutex_lock(&m_mutex);
    mapTimersendTaskInfo = m_mapTimersendTaskInfo;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getAllParamFromDB()
{
    getDBChannlelMap(m_ChannelMap);
    getDBSysParamMap(m_SysParamMap);
    getDBSysUserMap(m_SysUserMap);
    getDBSysNoticeMap(m_SysNoticeMap);
    getDBSysNoticeUserList(m_SysNoticeUserList);
    getDBSmsAccontMap(m_SmsAccountMap);
    getDBSmsTestAccont(m_SmsTestAccount);
    getDBClientAndSignMap(m_ClientIdAndSignMap, m_ClientIdAndSignPortMap);
    getDBUserGwMap(m_UserGwMap);
    getDBSignExtnoGwMap(m_SignextnoGwMap);
    getDBSmppPriceMap(m_PriceMap);
    getDBSmppSalePriceMap(m_SalePriceMap);
    getDBAllTempltRule(m_TempRuleMap);
    getDBPhoneAreaMap(m_PhoneAreaMap);
    getDBSendIpIdMap(m_sendIpId);
    getDBOperaterSegmentMap(m_OperaterSegmentMap);
    getDBOverRateWhiteList(m_overRateWhiteList);
    getDBAgentInfo(m_AgentInfoMap);

    getDBSystemErrorCode(m_mapSystemErrorCode);

    ////smsp5.0
    getDBMiddleWareConfig(m_middleWareConfigInfo);
    getDBComponentConfig(m_componentConfigInfo);
    getDBListenPort(m_listenPortInfo);
    getDBMqConfig(m_mqConfigInfo);
    getDBComponentRefMq(m_componentRefMqInfo);

    getDBSmsTemplateMap(m_mapSmsTemplate);
    getDBTimersendTaskInfo(m_mapTimersendTaskInfo);

    getDBSysAuthentication();

    return true;
}

bool CRuleLoadThread::getDBSystemErrorCode(map<string, string> &systemErrorCode)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500, "select * from t_sms_system_error_desc");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strKey = rs[i]["syscode"];
            string strValue = rs[i]["errordesc"];

            systemErrorCode.insert(make_pair(strKey, strValue));
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

bool CRuleLoadThread::getDBTimersendTaskInfo(map<string, timersend_taskinfo_ptr_t> &mapTimersendTaskInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    std::map<std::string, timersend_taskinfo_ptr_t> tmpTimerSendTaskInfo;

    std::string strNow = Comm::getCurrentTime();
    // Status: 1-待处理 2-发送中
    // set_send_time = [forwardInterval, backwardInterval]
    snprintf(sql, 500, "SELECT * FROM t_sms_timer_send_task WHERE component_id=%lu AND status in (1,2) "
             "AND set_send_time >= date_add('%s', interval %d minute) "
             "AND set_send_time <= date_add('%s', interval %d minute)",
             g_uComponentId,
             strNow.c_str(),
             m_iTimersendTaskInfoForwardScanInterval,
             strNow.c_str(),
             m_iTimersendTaskInfoBackwardScanInterval
            );
    LogDebug("timersend task sql[%s]", sql);

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strTaskUuid = rs[i]["task_id"];
            string strSubmitTime = rs[i]["submit_time"];

            timersend_taskinfo_ptr_t tmpTaskPtr(new StTimerSendTaskInfo);
            if (!tmpTaskPtr)
            {
                LogError("Null tmpTaskPtr when insert timersend task[%s]", strTaskUuid.c_str());
                continue;
            }

            std::string strTblIdx = Comm::getLastMondayByDate(strSubmitTime);

            RecordSet rs2(m_DBConn);
            char sql2[500] = {0};
            // 该任务下已经处理的号码数
            snprintf(sql2, 500, "SELECT count(*) as total_handled FROM t_sms_timer_send_phones_%s "
                     "WHERE task_id = '%s' AND status = %u order by id",
                     strTblIdx.c_str(),
                     strTaskUuid.c_str(),
                     TimerSendPhoneState_Handled);

            if (-1 != rs2.ExecuteSQL(sql2))
            {
                tmpTaskPtr->uHandledPhonesNum = atoi(rs2[0]["total_handled"].c_str());
            }
            else
            {
                LogError("query handled phones num of task[%s] failed. Ignore it", strTaskUuid.c_str());
                continue;
            }

            tmpTaskPtr->strTaskUuid = strTaskUuid;
            tmpTaskPtr->strSmsId = rs[i]["smsid"];
            tmpTaskPtr->strClientId = rs[i]["clientid"];
            tmpTaskPtr->strContent = rs[i]["content"];
            tmpTaskPtr->bIsChinese = (rs[i]["is_china"] == "1");
            tmpTaskPtr->strSign = rs[i]["sign"];
            tmpTaskPtr->uChargeNum = atoi(rs[i]["charge_num"].c_str());
            tmpTaskPtr->uTotalPhonesNum = atoi(rs[i]["total_phones_num"].c_str());
            tmpTaskPtr->strExtendPort = rs[i]["extend"];
            tmpTaskPtr->strSrcPhone = rs[i]["srcphone"];
            tmpTaskPtr->uShowSignType = atoi(rs[i]["showsigntype"].c_str());
            tmpTaskPtr->strSubmitTime = strSubmitTime;
            tmpTaskPtr->strSetSendTime = rs[i]["set_send_time"];
            tmpTaskPtr->uSmsType = atol(rs[i]["smstype"].c_str());
            tmpTaskPtr->uSmsFrom = atoi(rs[i]["smsfrom"].c_str());
            tmpTaskPtr->strUid = rs[i]["uid"];
            tmpTaskPtr->uStatus = atoi(rs[i]["status"].c_str());
            tmpTaskPtr->strPhonesTblIdx = strTblIdx;

            tmpTimerSendTaskInfo.insert(make_pair(strTaskUuid, tmpTaskPtr));
        }
        mapTimersendTaskInfo.swap(tmpTimerSendTaskInfo);

        LogNotice("t_sms_timer_send_task updated, size[%d]", mapTimersendTaskInfo.size());

        return true;
    }
    else
    {
        LogError("query t_sms_timer_send_task sql execute err!");
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
            ////middleWareConfigInfo[middleWare.m_uMiddleWareType] = middleWare;
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

bool CRuleLoadThread::getDBComponentConfig(ComponentConfig &componentConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_component_configure where component_id=%lu", g_uComponentId);
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            componentConfigInfo.m_strComponentName = rs[i]["component_name"];
            componentConfigInfo.m_strComponentType = rs[i]["component_type"];
            componentConfigInfo.m_strHostIp = rs[i]["host_ip"];
            componentConfigInfo.m_uComponentId = atol(rs[i]["component_id"].data());
            componentConfigInfo.m_uNodeId = atoi(rs[i]["node_id"].data());
            componentConfigInfo.m_uRedisThreadNum = atoi(rs[i]["redis_thread_num"].data());
            componentConfigInfo.m_uSgipReportSwitch = atoi(rs[i]["sgip_report_switch"].data());
            componentConfigInfo.m_uMqId = atoi(rs[i]["mq_id"].data());
            componentConfigInfo.m_uComponentSwitch = atoi(rs[i]["component_switch"].data());
            componentConfigInfo.m_uMonitorSwitch   = atoi(rs[i]["monitor_switch"].data());
            break;
        }
    }
    else
    {
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBListenPort(map<string, ListenPort> &listenPortInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_listen_port_configure where component_type ='08'");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ListenPort listenInfo;
            listenInfo.m_strComponentType = rs[i]["component_type"];
            listenInfo.m_strPortKey = rs[i]["port_key"];
            listenInfo.m_uPortValue = atoi(rs[i]["port_value"].data());
            listenPortInfo.insert(make_pair(listenInfo.m_strPortKey, listenInfo));
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
            refInfo.m_uWeight = atoi(rs[i]["weight"].data());

            char temp[250] = {0};
            if(PRODUCT_MODE == refInfo.m_uMode)
            {
                //Under PRODUCT_MODE there maybe more than one queue with same message_type
                snprintf(temp, 250, "%lu_%s_%u_%lu", refInfo.m_uComponentId, refInfo.m_strMessageType.data(), refInfo.m_uMode, refInfo.m_uMqId);
            }
            else
            {
                snprintf(temp, 250, "%lu_%s_%u", refInfo.m_uComponentId, refInfo.m_strMessageType.data(), refInfo.m_uMode);
            }
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

bool CRuleLoadThread::getDBChannlelMap(std::map<int , Channel> &channelmap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    std::string sql = "SELECT * FROM t_sms_channel WHERE state='1' and cid != 0;";

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql.data()))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            Channel chan;
            chan.m_uChannelType = atoi(rs[i]["channeltype"].data());
            chan.m_uWhiteList = atoi(rs[i]["iswhitelist"].data());
            chan.m_strServerId = rs[i]["serviceid"];
            chan.m_uSendNodeId = atoi(rs[i]["sendid"].data());
            chan.m_strQueryUpPostData = rs[i]["queryuppostdata"];
            chan.m_strQueryUpUrl = rs[i]["queryupurl"];
            chan._showsigntype = atoi(rs[i]["showsigntype"].data());
            chan._accessID = rs[i]["accessid"];
            chan.channelID = atoi(rs[i]["cid"].data());
            chan._clientID = rs[i]["clientid"];
            chan._password = rs[i]["password"];
            chan.chanenlNum = atoi(rs[i]["id"].data());
            chan.channelname = rs[i]["channelname"];
            chan.operatorstyle = atoi(rs[i]["operatorstype"].data());
            chan.industrytype = atoi(rs[i]["industrytype"].data());
            chan.longsms = atoi(rs[i]["longsms"].data());
            chan.wappush = atoi(rs[i]["wappush"].data());
            chan.httpmode = atoi(rs[i]["httpmode"].data());
            chan.url = rs[i]["url"];
            chan.coding = rs[i]["coding"];
            chan.postdata = rs[i]["postdata"];
            chan.querystatepostdata = rs[i]["querystatepostdata"];
            chan.querystateurl = rs[i]["querystateurl"];
            chan.costprice = atof(rs[i]["costprice"].data());
            chan.balance = atof(rs[i]["balance"].data());
            chan.expenditure = atof(rs[i]["expenditure"].data());
            chan.shownum = atoi(rs[i]["shownum"].data());
            chan._channelRule = rs[i]["showsign"].data();
            chan.spID = rs[i]["spid"].data();
            chan.nodeID = atoi(rs[i]["node"].data());
            chan.m_uSpeed = atoi(rs[i]["speed"].data());
            chan.m_uChannelIdentify = atoi(rs[i]["identify"].data());

            chan.m_uClusterType = atoi(rs[i]["cluster_type"].data());
            chan.m_strChannelLibName = rs[i]["lib_name"];
            chan.m_uExValue = atoi(rs[i]["exvalue"].c_str());
            chan.m_strMoPrefix = rs[i]["moprefix"].data();
            chan.m_strResendErrCode = rs[i]["failed_resend_errcode"];
            channelmap[chan.channelID] = chan;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
}


bool CRuleLoadThread::getDBUserGwMap(std::map<std::string, UserGw> &userGwMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_user_gw where state = '1'");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UserGw userGw;
            userGw.userid = rs[i]["userid"];
            userGw.distoperators = atoi(rs[i]["distoperators"].data());

            userGw.m_strChannelID = rs[i]["channelid"];
            userGw.m_strYDChannelID = rs[i]["ydchannelid"].data();
            userGw.m_strLTChannelID = rs[i]["ltchannelid"].data();
            userGw.m_strDXChannelID = rs[i]["dxchannelid"].data();
            userGw.m_strGJChannelID = rs[i]["gjchannelid"].data();
            userGw.m_strXNYDChannelID = rs[i]["xnydchannelid"];
            userGw.m_strXNLTChannelID = rs[i]["xnltchannelid"];
            userGw.m_strXNDXChannelID = rs[i]["xndxchannelid"];

            userGw.upstream = atoi(rs[i]["upstream"].data());
            userGw.report = atoi(rs[i]["report"].data());
            userGw.m_uSmstype = atoi(rs[i]["smstype"].data());

            userGw.m_strCSign = rs[i]["csign"];
            userGw.m_strESign = rs[i]["esign"];
            userGw.m_uUnblacklist = atoi(rs[i]["unblacklist"].data());

            userGwMap[rs[i]["userid"] + "_" + rs[i]["smstype"]] = userGw;		//KEY: userid_smstype
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

bool CRuleLoadThread::getDBKeyWordList(std::list<std::string> &list)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "SELECT keyword FROM t_sms_keyword_list;");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strKeyWords = rs[i]["keyword"].data();
            if(strKeyWords.length() < 50 * 1000)
            {
                list.push_back(strKeyWords);
            }
            else
            {
                LogWarn("keyWords data error, each record.keyWords should not above 50*1000");
            }

        }

        ////////////////////
        m_listKeyWordReg.clear();

        string strList = "(";
        std::list<std::string>::iterator it = list.begin();
        bool bNeedSeparate = false;

        for(; it != list.end(); it++)
        {
            if(!bNeedSeparate)
            {
                bNeedSeparate = true;
            }
            else
            {
                strList = strList.append("|");
            }
            strList = strList.append(*it);

            //
            if(strList.length() > 50 * 1000)
            {
                LogDebug("fjx1071, strList>50*1000.");
                strList.append(")");

                regex reg;
                try
                {
                    reg.assign(strList.data());
                    m_listKeyWordReg.push_back(reg);
                }
                catch(...)
                {
                    LogError("reg compile error. str[%s]", strList.data());
                }

                strList.assign("(");
                bNeedSeparate = false;
                continue;
            }
        }

        if(strList != "(")
        {
            strList.append(")");

            regex reg;
            try
            {
                ////LogDebug("fjx1096, strList[%s].", strList.data());
                reg.assign(strList.data());
                m_listKeyWordReg.push_back(reg);
            }
            catch(...)
            {
                LogError("reg compile error. str[%s]", strList.data());
            }

            strList.assign("");
        }

        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
}

bool CRuleLoadThread::getDBSignExtnoGwMap(std::map<std::string, SignExtnoGw> &signextnoGwMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_signextno_gw");
    std::string key = "";
    std::string strKeyAppend = "";

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            key.assign("");
            SignExtnoGw signExtnoGw;

            signExtnoGw.channelid = rs[i]["channelid"];
            signExtnoGw.sign = rs[i]["sign"];
            signExtnoGw.appendid = rs[i]["appendid"];
            signExtnoGw.username = rs[i]["username"];
            signExtnoGw.m_strClient = rs[i]["clientid"];

            ////key = signExtnoGw.sign.data() + "&" + signExtnoGw.channelid.data() + "&" + signExtnoGw.m_strClient.data();
            key.append(signExtnoGw.sign.data());
            key.append("&");
            key.append(signExtnoGw.channelid.data());
            key.append("&");
            key.append(signExtnoGw.m_strClient.data());

            signextnoGwMap[key] = signExtnoGw;
            key.clear();
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

bool CRuleLoadThread::getDBSmppPriceMap(std::map<std::string, std::string> &priceMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_tariff");
    std::string key = "";
    std::string fee = "";
    std::string channelid = "";
    std::string prefix    = "";

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            channelid = rs[i]["channelid"];
            prefix = rs[i]["prefix"];
            fee = rs[i]["fee"];

            key = channelid + "&" + prefix;
            priceMap[key] = fee;
            key.clear();
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

bool CRuleLoadThread::getDBSmppSalePriceMap(std::map<std::string, std::string> &salePriceMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_client_tariff");

    std::string fee = "";
    std::string prefix    = "";

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            prefix = rs[i]["prefix"];
            fee = rs[i]["fee"];

            salePriceMap[prefix] = fee;
            prefix.clear();
            fee.clear();
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

bool CRuleLoadThread::getDBAllTempltRule(std::map<string , TempRule> &tempRuleMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_template_overrate");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            TempRule tempRule;

            tempRule.m_strSmsType = rs[i]["smstype"];
            tempRule.m_strUserID = rs[i]["userid"];
            tempRule.m_overRate_num_s = atoi(rs[i]["overRate_num_s"].data());
            tempRule.m_overRate_time_s = atoi(rs[i]["overRate_time_s"].data());
            tempRule.m_overRate_num_h = atoi(rs[i]["overRate_num_h"].data());
            tempRule.m_overRate_time_h = atoi(rs[i]["overRate_time_h"].data());

            if(!tempRule.m_strUserID.empty())
                tempRuleMap[tempRule.m_strUserID + "_" + tempRule.m_strSmsType] = tempRule;		//KEY:userID_templetID
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

bool CRuleLoadThread::getDBPhoneAreaMap(std::map<std::string, int> &phoneAreaMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    std::string phoneSection;
    int areaId = 0;
    snprintf(sql, 500, "select * from t_sms_phone_section");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (long i = 0; i < rs.GetRecordCount(); ++i)
        {
            phoneSection = rs[i]["phone_section"].data();
            areaId       = atoi(rs[i]["area_id"].data());
            phoneAreaMap[phoneSection] = areaId;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
}

bool CRuleLoadThread::getDBSendIpIdMap(std::map<UInt32, string> &sendIpId)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100, "select * from t_sms_sendip_id");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strIpPort = "";

            UInt32 uSendId = atoi(rs[i]["sendid"].data());
            strIpPort = rs[i]["sendip"];
            strIpPort.append(":");
            strIpPort.append(rs[i]["sendport"].data());
            sendIpId[uSendId] = strIpPort;
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

bool CRuleLoadThread::getDBOperaterSegmentMap(map<string, UInt32> &PhoneHeadMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500, "select * from t_sms_operater_segment");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strPhoneheads = rs[i]["numbersegment"];
            UInt32 uOperator = atoi(rs[i]["operater"].data());

            vector<string> vPhoneHeads;
            Comm::splitExVector(strPhoneheads, ",", vPhoneHeads);
            for(vector<string>::iterator it = vPhoneHeads.begin(); it != vPhoneHeads.end(); it++ )
            {
                PhoneHeadMap[*it] = uOperator;
            }
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBOverRateWhiteList(set<string> &overRateWhiteList)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "SELECT * FROM t_sms_overrate_white_list where status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strKey = rs[i]["clientid"];
            strKey.append("_");
            strKey.append(rs[i]["phone"]);

            if(!strKey.empty())
            {
                overRateWhiteList.insert(strKey);
            }
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBClientAndSignMap(std::map<std::string, ClientIdSignPort> &smsClientIdAndSign, std::map<std::string, ClientIdSignPort> &smsClientIdAndSignPort)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_clientid_signport where status=0");
    std::string key = "";
    std::string strKey1 = "";
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ClientIdSignPort clientAndSign;

            clientAndSign.m_strClientId = rs[i]["clientid"];
            clientAndSign.m_strSign = rs[i]["sign"];
            clientAndSign.m_strSignPort = rs[i]["signport"];


            key = clientAndSign.m_strClientId + "&" + clientAndSign.m_strSign;
            smsClientIdAndSign[key] = clientAndSign;
            key.clear();

            strKey1 = clientAndSign.m_strClientId + "&" + clientAndSign.m_strSignPort;
            smsClientIdAndSignPort[strKey1] = clientAndSign;
            strKey1.clear();
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

            if (paramKey == "TIMER_SEND_CFG")
            {
                std::vector<std::string> vCfg;
                Comm::splitExVectorSkipEmptyString(paramValue, ";", vCfg);
                if (vCfg.size() < 6)
                {
                    LogError("Invalid 'TIMER_SEND_CFG' value: [%s] reason:size<6", paramValue.c_str());
                    continue;
                }
                UInt32 uTimersendTaskTblRescanInterval = atoi(vCfg[0].c_str());
                m_iTimersendTaskInfoForwardScanInterval = atoi(vCfg[1].c_str());
                m_iTimersendTaskInfoBackwardScanInterval = atoi(vCfg[2].c_str());
                LogNotice("timersend task scan config: rescan-interval[%umin] range [%dmin, %dmin]",
                          uTimersendTaskTblRescanInterval,
                          m_iTimersendTaskInfoForwardScanInterval,
                          m_iTimersendTaskInfoBackwardScanInterval);
                tryResetTimersendRescanTimer(uTimersendTaskTblRescanInterval);
            }
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

void CRuleLoadThread::tryResetTimersendRescanTimer(UInt32 uTimersendTaskTblRescanInterval)
{
    // 使用旧参数
    if (uTimersendTaskTblRescanInterval < 5 || uTimersendTaskTblRescanInterval > 1440)
    {
        LogWarn("timersend-rescan interval reset failed: [%u] out of range [5min~1440min]", uTimersendTaskTblRescanInterval);
        uTimersendTaskTblRescanInterval = m_uTimersendTaskTblRescanInterval;
    }

    UInt32 uNextRescanMins = uTimersendTaskTblRescanInterval;
    // 扫描间隔变小的话，立即扫一次
    if (uTimersendTaskTblRescanInterval < m_uTimersendTaskTblRescanInterval
            && m_bInited /* 初始化好了才能立即扫描，or coredump */)
    {
        LogNotice("new rescan-interval[%u(%u)] is smaller, do it now!", uTimersendTaskTblRescanInterval, m_uTimersendTaskTblRescanInterval);
        uNextRescanMins = 0;
    }

    m_uTimersendTaskTblRescanInterval = uTimersendTaskTblRescanInterval;

    if (NULL != m_pTimersendTaskRescanTimer)
    {
    	SAFE_DELETE(m_pTimersendTaskRescanTimer);
    }
    LogDebug("reinstall timersend-rescan timer [interval:%umin]!", m_uTimersendTaskTblRescanInterval);
    m_pTimersendTaskRescanTimer = SetTimer(TIMERSEND_TASKS_RESCAN_TIMERID, "", uNextRescanMins * 60 * 1000);
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
bool CRuleLoadThread::getDBSmsTestAccont(std::set<string> &smsTestAccount)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_test_account");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strTestAccount = rs[i]["clientid"].data();
            if(!strTestAccount.empty())
            {
                smsTestAccount.insert(strTestAccount);
            }
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
bool CRuleLoadThread::getDBSmsAccontMap(std::map<string, SmsAccount> &smsAccountMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_account");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsAccount smsAccount;

            smsAccount.m_strAccount = rs[i]["clientid"].data();
            smsAccount.m_strPWD = rs[i]["password"].data();
            smsAccount.m_strname = rs[i]["name"].data();
            smsAccount.m_strSid = rs[i]["sid"].data();
            smsAccount.m_uStatus = atoi(rs[i]["status"].data());
            smsAccount.m_uNeedReport = atoi(rs[i]["needreport"].data());
            smsAccount.m_uNeedMo = atoi(rs[i]["needmo"].data());
            smsAccount.m_uNeedaudit = atoi(rs[i]["needaudit"].data());
            smsAccount.m_strCreattime = rs[i]["createtime"].data();
            smsAccount.m_strIP = rs[i]["ip"].data();
            smsAccount.m_strMoUrl = rs[i]["mourl"].data();
            smsAccount.m_uNodeNum = atoi(rs[i]["nodenum"].data());
            smsAccount.m_uPayType = atoi(rs[i]["paytype"].data());
            smsAccount.m_strSmsType = rs[i]["smstype"].data();
            smsAccount.m_uNeedExtend = atoi(rs[i]["needextend"].data());
            smsAccount.m_uNeedSignExtend = atoi(rs[i]["signextend"].data());
            smsAccount.m_strDeliveryUrl = rs[i]["deliveryurl"].data();

            smsAccount.m_uAgentId = atol(rs[i]["agent_id"].data());
            smsAccount.m_uSmsFrom = atoi(rs[i]["smsfrom"].data());
            smsAccount.m_strSmsType = rs[i]["smstype"].data();
            smsAccount.m_strSpNum = rs[i]["spnum"].data();
            smsAccount.m_uOverRateCharge = atoi(rs[i]["isoverratecharge"].data());
            smsAccount.m_uClientAscription = atoi(rs[i]["client_ascription"].data());

            smsAccount.m_strSgipRtMoIp = rs[i]["moip"].data();
            smsAccount.m_uSgipRtMoPort = atoi(rs[i]["moport"].data());
            smsAccount.m_uSgipNodeId = atoi(rs[i]["nodeid"].data());
            smsAccount.m_uIdentify = atoi(rs[i]["identify"].data());
            smsAccount.m_uClientLeavel = atoi(rs[i]["client_level"].data());
            smsAccount.m_strExtNo = rs[i]["extendport"];
            smsAccount.m_uSignPortLen = atoi(rs[i]["signportlen"].data());
            smsAccount.m_uBelong_sale = atol(rs[i]["belong_sale"].data());
            smsAccount.m_ucHttpProtocolType = atoi(rs[i]["http_protocol_type"].data());
            smsAccount.m_uAccountExtAttr = atoi(rs[i]["accountExtAttr"].data());

            if(!smsAccount.m_strAccount.empty())
            {
                string tmpIP;
                std::string::size_type pos;
                Comm::trim(smsAccount.m_strIP);
                if(std::string::npos == smsAccount.m_strIP.find(","))
                {
                    tmpIP = smsAccount.m_strIP;
                    if(tmpIP == "*")
                    {
                        smsAccount.m_bAllIpAllow = true;
                    }
                    else
                    {
                        pos = tmpIP.find("*");
                        if(std::string::npos != pos)
                        {
                            tmpIP = tmpIP.substr(0, pos);
                        }
                        smsAccount.m_IPWhiteList.push_back(tmpIP);
                    }
                }
                else
                {
                    std::vector<string> WhiteIpList;
                    Comm comm;
                    comm.splitExVector(smsAccount.m_strIP, ",", WhiteIpList);
                    for(std::vector<string>::iterator it = WhiteIpList.begin(); it != WhiteIpList.end(); it++)
                    {
                        Comm::trim(*it);
                        if((*it) == "*")
                        {
                            smsAccount.m_bAllIpAllow = true;
                            break;
                        }
                        else
                        {
                            tmpIP = *it;
                            pos = it->find("*");
                            if(std::string::npos != pos)
                            {
                                tmpIP.substr(0, pos);
                            }
                            smsAccount.m_IPWhiteList.push_back(tmpIP);
                        }
                    }
                }
                smsAccountMap[smsAccount.m_strAccount] = smsAccount;
            }
            else
            {
                LogWarn("t_sms_account m_strAccount is empty");
            }
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

bool CRuleLoadThread::getDBSmsAccontStatusMap(std::map<string, SmsAccountState> &smsAccountStateMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_account_login_status");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsAccountState smsAccountState;
            smsAccountState.m_strAccount = rs[i]["clientid"].data();
            smsAccountState.m_uCode = atoi(rs[i]["code"].data());
            smsAccountState.m_strRemarks = rs[i]["remarks"].data();
            smsAccountState.m_strLockTime = rs[i]["locktime"].data();
            smsAccountState.m_uStatus = atoi(rs[i]["status"].data());
            smsAccountState.m_strUpdateTime = rs[i]["updatetime"].data();

            if(!smsAccountState.m_strAccount.empty())
            {
                smsAccountStateMap[smsAccountState.m_strAccount] = smsAccountState;
            }
            else
            {
                LogWarn("t_sms_account_login_status m_strAccount is empty");
            }
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

bool CRuleLoadThread::getDBAgentInfo(AgentInfoMap &agentInfoMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_agent_info where status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt64 agentId = 0;

            AgentInfo agentInfo;
            agentId = atoi(rs[i]["agent_id"].c_str());
            agentInfo.m_iAgent_type = atoi(rs[i]["agent_type"].c_str());


            agentInfoMap[agentId] = agentInfo;
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

bool CRuleLoadThread::getDBSmsTemplateMap(map<std::string, SmsTemplate> &smsTemplateMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_template where check_status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        std::string strkey;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsTemplate smsTemplate;

            smsTemplate.m_strTemplateId = rs[i]["template_id"];
            smsTemplate.m_strType = rs[i]["type"];
            smsTemplate.m_strSmsType = rs[i]["sms_type"];
            smsTemplate.m_strContent = rs[i]["content"];
            smsTemplate.m_strSign = rs[i]["sign"];
            smsTemplate.m_strClientId = rs[i]["client_id"];
            smsTemplate.m_iAgentId = atoi(rs[i]["agent_id"].c_str());

            strkey.assign(smsTemplate.m_strClientId).append("_").append(smsTemplate.m_strTemplateId);
            smsTemplateMap[strkey] = smsTemplate;
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

bool CRuleLoadThread::getDBSysAuthentication()
{
    CHK_NULL_RETURN_FALSE(m_DBConn);

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, sizeof(sql), "select * from t_sms_sys_authentication;");

    if (-1 == rs.ExecuteSQL(sql))
    {
        LogError("Call ExecuteSQL failed. sql[%s].", sql);
        return false;
    }

    m_mapSysAuthentication.clear();

    for (int i = 0; i < rs.GetRecordCount(); ++i)
    {
        SysAuthentication sysAuthen;
        sysAuthen.m_id = rs[i]["id"];
        sysAuthen.m_serial_num = rs[i]["serial_num"];
        sysAuthen.m_component_md5 = rs[i]["component_md5"];
        sysAuthen.m_component_type = rs[i]["component_type"];
        sysAuthen.m_bind_ip = rs[i]["bind_ip"];
        sysAuthen.m_first_timestamp = rs[i]["first_timestamp"];
        sysAuthen.m_expire_timestamp = rs[i]["expire_timestamp"];
        sysAuthen.parseIp();

        m_mapSysAuthentication[sysAuthen.m_serial_num + ":" + sysAuthen.m_component_type] = sysAuthen;
    }

    return true;
}


