#include "CRuleLoadThread.h"
#include "main.h"

using namespace models;

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

    getAllParamFromDB();
    getAllTableUpdateTime();
    setDBClientForceExitStop();

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

bool CRuleLoadThread::TimeRegCheck(string strTime)      //check format like 08:20
{
    if(strTime.empty())
    {
        return false;
    }

    string strKeyWord;
    cmatch what;
    if(regex_search(strTime.c_str(), what, m_regTime))
    {
        if(what.size() > 0)
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

    m_clientForceExitUpdateTime = getTableLastUpdateTime("t_sms_client_force_exit");
}

std::string CRuleLoadThread::getTableLastUpdateTime(std::string tableName)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return "";
    }

    std::string updateTime;
    char sql[1024] = { 0 };

    snprintf(sql, 1024,"SELECT tablename, updatetime FROM t_sms_table_update_time where tablename='%s';", tableName.data());

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
        LogError("sql[%s]execute err!", sql);
    }

    return false;
}

void CRuleLoadThread::checkDbUpdate()
{
    if(isTableUpdate("t_user_gw", m_UserGwLastUpdateTime))
    {
        m_UserGwMap.clear();
        getDBUserGwMap(m_UserGwMap);
        TUpdateUserGwReq* pMsg = new TUpdateUserGwReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
        pMsg->m_UserGwMap = m_UserGwMap;
        g_pUnitypThread->PostMsg(pMsg);

    }

    if(isTableUpdate("t_sms_param", m_SysParamLastUpdateTime))
    {
        m_SysParamMap.clear();
        getDBSysParamMap(m_SysParamMap);

        TUpdateSysParamRuleReq* pMsg2Log = new TUpdateSysParamRuleReq();
        pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pMsg2Log->m_SysParamMap = m_SysParamMap;
        g_pLogThread->PostMsg(pMsg2Log);

        if (NULL != g_pAlarmThread)
        {
            TUpdateSysParamRuleReq* pMsg2Alarm = new TUpdateSysParamRuleReq();
            pMsg2Alarm->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsg2Alarm->m_SysParamMap = m_SysParamMap;
            g_pAlarmThread->PostMsg(pMsg2Alarm);
        }

        TUpdateSysParamRuleReq* pHttpSend = new TUpdateSysParamRuleReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pHttpSend->m_SysParamMap = m_SysParamMap;
        g_pUnitypThread->PostMsg(pHttpSend);

        TUpdateSysParamRuleReq* pRtpMsg = new TUpdateSysParamRuleReq();
        pRtpMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRtpMsg->m_SysParamMap = m_SysParamMap;
        g_pRptMoGetThread->PostMsg(pRtpMsg);

        TUpdateSysParamRuleReq* pRtpRev = new TUpdateSysParamRuleReq();
        pRtpRev->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRtpRev->m_SysParamMap = m_SysParamMap;
        g_pReportReceiveThread->PostMsg(pRtpRev);



        TUpdateSysParamRuleReq* pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );

        MONITOR_UPDATE_SYS( g_pMQMonitorPublishThread, m_SysParamMap );

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


    if(isTableUpdate("t_sms_account", m_SmsAccountLastUpdateTime))
    {
        m_SmsAccountMap.clear();
        getDBSmsAccontMap(m_SmsAccountMap);
        ///reportreceive
        TUpdateSmsAccontReq* pReportMsg = new TUpdateSmsAccontReq();
        pReportMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pReportMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pReportReceiveThread->PostMsg(pReportMsg);

        ////httpsend
        TUpdateSmsAccontReq* pHttpSendMsg = new TUpdateSmsAccontReq();
        pHttpSendMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pHttpSendMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pUnitypThread->PostMsg(pHttpSendMsg);

        ///cmppservice
        TUpdateSmsAccontReq* pCMPPMsg = new TUpdateSmsAccontReq();
        pCMPPMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pCMPPMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pCMPPServiceThread->PostMsg(pCMPPMsg);

        //cmpp3service
        TUpdateSmsAccontReq* pCMPP3Msg = new TUpdateSmsAccontReq();
        pCMPP3Msg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pCMPP3Msg->m_SmsAccountMap = m_SmsAccountMap;
        g_pCMPP3ServiceThread->PostMsg(pCMPP3Msg);

        ///smppservice
        TUpdateSmsAccontReq* pSMPPMsg = new TUpdateSmsAccontReq();
        pSMPPMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pSMPPMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pSMPPServiceThread->PostMsg(pSMPPMsg);

        ///smgpservice
        TUpdateSmsAccontReq* pSMGPtMsg = new TUpdateSmsAccontReq();
        pSMGPtMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pSMGPtMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pSMGPServiceThread->PostMsg(pSMGPtMsg);

        //sgipRtAndMo
        TUpdateSmsAccontReq* pSGIPRtAndMoMsg = new TUpdateSmsAccontReq();
        pSGIPRtAndMoMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pSGIPRtAndMoMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pSgipRtAndMoThread->PostMsg(pSGIPRtAndMoMsg);

        ///sgipservice
        TUpdateSmsAccontReq* pSGIPtMsg = new TUpdateSmsAccontReq();
        pSGIPtMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pSGIPtMsg->m_SmsAccountMap = m_SmsAccountMap;
        g_pSGIPServiceThread->PostMsg(pSGIPtMsg);

    }

    if(isTableUpdate("t_sms_clientid_signport", m_SmsClientidSignPortUpdateTime))
    {
        m_ClientIdAndSignMap.clear();
        m_ClientIdAndSignPortMap.clear();
        getDBClientAndSignMap(m_ClientIdAndSignMap,m_ClientIdAndSignPortMap);

        ///
        TUpdateClientIdAndSignReq* pReport = new TUpdateClientIdAndSignReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ;
        pReport->m_ClientIdAndSignMap = m_ClientIdAndSignPortMap;
        g_pReportReceiveThread->PostMsg(pReport);

        ////cunity
        TUpdateClientIdAndSignReq* pUnity = new TUpdateClientIdAndSignReq();
        pUnity->m_iMsgType = MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ;
        pUnity->m_ClientIdAndSignMap = m_ClientIdAndSignMap;
        g_pUnitypThread->PostMsg(pUnity);
    }

    if(isTableUpdate("t_sms_operater_segment", m_OperaterSegmentUpdateTime))
    {
        m_OperaterSegmentMap.clear();
        getDBOperaterSegmentMap(m_OperaterSegmentMap);  //

        TUpdateOperaterSegmentReq* pHttpSend = new TUpdateOperaterSegmentReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ;
        pHttpSend->m_OperaterSegmentMap = m_OperaterSegmentMap;
        g_pUnitypThread->PostMsg(pHttpSend);
    }
    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        TUpdateMqConfigReq* pMqConfig = new TUpdateMqConfigReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfig->m_mapMqConfig = m_mqConfigInfo;
        g_pUnitypThread->PostMsg(pMqConfig);

    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        TUpdateComponentRefMqReq* pMqConfig = new TUpdateComponentRefMqReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pMqConfig->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pUnitypThread->PostMsg(pMqConfig);
    }

    if (isTableUpdate("t_sms_component_configure", m_componentConfigUpdateTime))
    {
        getDBComponentConfig(m_componentConfigInfo);
        MONITOR_UPDATE_COMP(g_pMQMonitorPublishThread, m_componentConfigInfo );

    }

    if (isTableUpdate("t_sms_system_error_desc", m_systemErrorCodeUpdateTime))
    {
        m_mapSystemErrorCode.clear();
        getDBSystemErrorCode(m_mapSystemErrorCode);

        TUpdateSystemErrorCodeReq* pHttp = new TUpdateSystemErrorCodeReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
        pHttp->m_mapSystemErrorCode = m_mapSystemErrorCode;
        g_pUnitypThread->PostMsg(pHttp);
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

    if(isTableUpdate("t_sms_client_force_exit", m_clientForceExitUpdateTime))
    {
        m_clientForceExitMap.clear();
        getDBClientForceExit(m_clientForceExitMap);
        TUpdateClientForceExitReq* pMsg = new TUpdateClientForceExitReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CLIENTFORCEEXIT_UPDATE_REQ;
        pMsg->m_clientForceExitMap = m_clientForceExitMap;
        g_pUnitypThread->PostMsg(pMsg);
       
    }
    
}



void CRuleLoadThread::getClientForceExit(map<string, int>& clientForceExitMap)
{
    pthread_mutex_lock(&m_mutex);
    clientForceExitMap = m_clientForceExitMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSendIpIdMap(std::map<UInt32,string>& sendIpId)
{
    pthread_mutex_lock(&m_mutex);
    sendIpId = m_sendIpId;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getNoAuditKeyWordMap(map<string,RegList>& NoAuditKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    NoAuditKeyWordMap = m_NoAuditKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
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

void CRuleLoadThread::getSmsAccountMap(std::map<std::string, SmsAccount>& SmsAccountMap)
{
    pthread_mutex_lock(&m_mutex);
    SmsAccountMap = m_SmsAccountMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getSmsClientAndSignMap(std::map<string, ClientIdSignPort>& smsClientIdAndSignMap)
{
    pthread_mutex_lock(&m_mutex);
    smsClientIdAndSignMap = m_ClientIdAndSignMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmsClientAndSignPortMap(map<string,ClientIdSignPort>& smsClientIdAndSignPortMap)
{
    pthread_mutex_lock(&m_mutex);
    smsClientIdAndSignPortMap = m_ClientIdAndSignPortMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getUserGwMap(std::map<std::string,UserGw>& userGwMap)
{
    pthread_mutex_lock(&m_mutex);
    userGwMap = m_UserGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap)
{
    pthread_mutex_lock(&m_mutex);
    PhoneHeadMap = m_OperaterSegmentMap;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getBlackList(set<UInt64>& blackLists)
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

void CRuleLoadThread::getSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap)
{
    pthread_mutex_lock(&m_mutex);
    signextnoGwMap = m_SignextnoGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAgentInfo(map<UInt64, AgentInfo>& agentInfoMap)
{
    pthread_mutex_lock(&m_mutex);
    agentInfoMap = m_AgentInfoMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSystemErrorCode(map<string,string>& systemErrorCode)
{
    pthread_mutex_lock(&m_mutex);
    systemErrorCode = m_mapSystemErrorCode;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmppPriceMap(std::map<std::string, std::string>& priceMap)
{
    pthread_mutex_lock(&m_mutex);
    priceMap = m_PriceMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSmppSalePriceMap(std::map<std::string, std::string>& salePriceMap)
{
    pthread_mutex_lock(&m_mutex);
    salePriceMap = m_SalePriceMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAllTempltRule(std::map<string ,TempRule> &tempRuleMap)
{
    pthread_mutex_lock(&m_mutex);
    tempRuleMap = m_TempRuleMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getPhoneAreaMap(std::map<std::string, int>& phoneAreaMap)
{
    pthread_mutex_lock(&m_mutex);
    phoneAreaMap = m_PhoneAreaMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getOverRateWhiteList(set<string>& overRateWhiteList)
{
    pthread_mutex_lock(&m_mutex);
    overRateWhiteList = m_overRateWhiteList;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    middleWareConfigInfo = m_middleWareConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getComponentConfig(ComponentConfig& componentConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfo = m_componentConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getListenPort(map<string,ListenPort>& listenPortInfo)
{
    pthread_mutex_lock(&m_mutex);
    listenPortInfo = m_listenPortInfo;
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

void CRuleLoadThread::getSmsTemplateMap(map<std::string, SmsTemplate>& smsTemplateMap)
{
    pthread_mutex_lock(&m_mutex);
    smsTemplateMap = m_mapSmsTemplate;
    pthread_mutex_unlock(&m_mutex);
}


bool CRuleLoadThread::getAllParamFromDB()
{
    getDBSysParamMap(m_SysParamMap);
    getDBSysUserMap(m_SysUserMap);
    getDBSysNoticeMap(m_SysNoticeMap);
    getDBSysNoticeUserList(m_SysNoticeUserList);
    getDBSmsAccontMap(m_SmsAccountMap);
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
    return true;
}

bool CRuleLoadThread::getDBSystemErrorCode(map<string,string>& systemErrorCode)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_system_error_desc");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strKey = rs[i]["syscode"];
            string strValue = rs[i]["errordesc"];

            systemErrorCode.insert(make_pair(strKey,strValue));
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
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
            middleWareConfigInfo.insert(make_pair(middleWare.m_uMiddleWareType,middleWare));
        }
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBComponentConfig(ComponentConfig& componentConfigInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_component_configure where component_id=%lu",g_uComponentId);
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
            componentConfigInfo.m_uMonitorSwitch  = atoi(rs[i]["monitor_switch"].data());
            break;
        }
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBListenPort(map<string,ListenPort>& listenPortInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_listen_port_configure where component_type ='00'");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ListenPort listenInfo;
            listenInfo.m_strComponentType = rs[i]["component_type"];
            listenInfo.m_strPortKey = rs[i]["port_key"];
            listenInfo.m_uPortValue = atoi(rs[i]["port_value"].data());
            listenPortInfo.insert(make_pair(listenInfo.m_strPortKey,listenInfo));
        }
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return true;
}

bool CRuleLoadThread::getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_component_ref_mq");
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
                snprintf(temp,250,"%lu_%s_%u_%lu",refInfo.m_uComponentId,refInfo.m_strMessageType.data(),refInfo.m_uMode,refInfo.m_uMqId);
            }
            else
            {
                snprintf(temp,250,"%lu_%s_%u",refInfo.m_uComponentId,refInfo.m_strMessageType.data(),refInfo.m_uMode);
            }
            string strKey;
            strKey.assign(temp);

            componentRefMqInfo.insert(make_pair(strKey,refInfo));
        }
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBUserGwMap(std::map<std::string,UserGw>& userGwMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_user_gw where state = '1'");
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

            userGw.upstream= atoi(rs[i]["upstream"].data());
            userGw.report= atoi(rs[i]["report"].data());
            userGw.m_uSmstype = atoi(rs[i]["smstype"].data());

            userGw.m_strCSign = rs[i]["csign"];
            userGw.m_strESign = rs[i]["esign"];

            userGw.m_uUnblacklist = atoi(rs[i]["unblacklist"].data());

            userGwMap[rs[i]["userid"] + "_" + rs[i]["smstype"]] = userGw;       //KEY: userid_smstype
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBKeyWordList(std::list<std::string>& list)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"SELECT keyword FROM t_sms_keyword_list;");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strKeyWords = rs[i]["keyword"].data();
            if(strKeyWords.length() < 50*1000)
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
            if(strList.length() > 50*1000)
            {
                LogDebug("fjx1071, strList>50*1000.");
                strList.append(")");

                regex reg;
                try{
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
            try{
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
        ///////////////////
        /////LogDebug("m_listKeyWordReg.size[%d].", m_listKeyWordReg.size());

        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
}

bool CRuleLoadThread::getDBSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_signextno_gw");
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
            signExtnoGw.appendid= rs[i]["appendid"];
            signExtnoGw.username = rs[i]["username"];
            signExtnoGw.m_strClient = rs[i]["clientid"];

            /*key = signExtnoGw.channelid + "&" + signExtnoGw.sign + "&" signExtnoGw.clientid*/
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmppPriceMap(std::map<std::string, std::string>& priceMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_tariff");
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
            fee= rs[i]["fee"];

            key = channelid + "&" + prefix;
            priceMap[key] = fee;
            key.clear();
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmppSalePriceMap(std::map<std::string, std::string>& salePriceMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_client_tariff");

    std::string fee = "";
    std::string prefix    = "";

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            prefix = rs[i]["prefix"];
            fee= rs[i]["fee"];
            salePriceMap[prefix] = fee;
            prefix.clear();
            fee.clear();
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBAllTempltRule(std::map<string ,TempRule>& tempRuleMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_template_overrate");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            TempRule tempRule;

            tempRule.m_strSmsType = rs[i]["smstype"];
            tempRule.m_strUserID = rs[i]["userid"];
            tempRule.m_overRate_num_s= atoi(rs[i]["overRate_num_s"].data());
            tempRule.m_overRate_time_s = atoi(rs[i]["overRate_time_s"].data());
            tempRule.m_overRate_num_h = atoi(rs[i]["overRate_num_h"].data());
            tempRule.m_overRate_time_h= atoi(rs[i]["overRate_time_h"].data());

            if(!tempRule.m_strUserID.empty())
                tempRuleMap[tempRule.m_strUserID + "_" + tempRule.m_strSmsType] = tempRule;     //KEY:userID_templetID
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBPhoneAreaMap(std::map<std::string, int>& phoneAreaMap)
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
    snprintf(sql, 500,"select * from t_sms_phone_section");
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
}

bool CRuleLoadThread::getDBSendIpIdMap(std::map<UInt32,string>& sendIpId)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_sendip_id");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strIpPort = "";

            UInt32 uSendId = atoi(rs[i]["sendid"].data());
            strIpPort = rs[i]["sendip"];
            strIpPort.append(":");
            strIpPort.append(rs[i]["sendport"].data());
            ////LogDebug("sendid[%d],ipAndPort[%s].",uSendId,strIpPort.data());
            sendIpId[uSendId] = strIpPort;
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_operater_segment");
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }

}

bool CRuleLoadThread::getDBOverRateWhiteList(set<string>& overRateWhiteList)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql,500,"SELECT * FROM t_sms_overrate_white_list where status = '1'");

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
        LogError("sql[%s]execute err!", sql);
        return false;
    }

}

bool CRuleLoadThread::getDBClientAndSignMap(std::map<std::string,ClientIdSignPort>& smsClientIdAndSign,std::map<std::string,ClientIdSignPort>& smsClientIdAndSignPort)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_clientid_signport where status=0");
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::setDBClientForceExitStop()
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    int iResult = -1;
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),"update t_sms_client_force_exit set state = 0 where component_id = %lu;", g_uComponentId);
    iResult = mysql_query(m_DBConn, sql);
    if (iResult == 0)
    {
        int affrow = mysql_affected_rows(m_DBConn);
        LogNotice("affrow = %d", affrow);
    }
    else
    {
        LogErrorP("excute sql[%s] fail", sql);
        return false;
    }
    return true;
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
        LogError("sql[%s]execute err!", sql);
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
        LogError("sql[%s]execute err!", sql);
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
            //sysNoticeUser.m_Mobile = rs[i]["user_mobile"];
            //sysNoticeUser.m_Email  = rs[i]["user_email"];

            sysNoticeUserList.push_back(sysNoticeUser);
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmsAccontMap(std::map<string, SmsAccount>& smsAccountMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_account");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsAccount smsAccount;

            smsAccount.m_strAccount = rs[i]["clientid"].data();
            smsAccount.m_strPWD = rs[i]["password"].data();
            smsAccount.m_strname = rs[i]["name"].data();
            ////smsAccount.m_fFee = atoi(rs[i]["fee"].data());
            smsAccount.m_strSid = rs[i]["sid"].data();
            smsAccount.m_uStatus = atoi(rs[i]["status"].data());
            smsAccount.m_uNeedReport = atoi(rs[i]["needreport"].data());
            smsAccount.m_uNeedMo = atoi(rs[i]["needmo"].data());
            smsAccount.m_uSupportLongMo = atoi(rs[i]["supportlongmo"].data());
            smsAccount.m_uNeedaudit = atoi(rs[i]["needaudit"].data());
            smsAccount.m_strCreattime = rs[i]["createtime"].data();
            smsAccount.m_strIP = rs[i]["ip"].data();
            smsAccount.m_strMoUrl = rs[i]["mourl"].data();
            smsAccount.m_uNodeNum = atoi(rs[i]["nodenum"].data());
            smsAccount.m_uPayType = atoi(rs[i]["paytype"].data());
            smsAccount.m_uNeedExtend = atoi(rs[i]["needextend"].data());
            smsAccount.m_uNeedSignExtend = atoi(rs[i]["signextend"].data());
            smsAccount.m_strDeliveryUrl = rs[i]["deliveryurl"].data();

            smsAccount.m_uAgentId = atol(rs[i]["agent_id"].data());
            smsAccount.m_uSmsFrom = atoi(rs[i]["smsfrom"].data());
            smsAccount.m_strSmsType = rs[i]["smstype"].data();
            smsAccount.m_strSpNum = rs[i]["spnum"].data();
            smsAccount.m_uOverRateCharge = atoi(rs[i]["isoverratecharge"].data());

            smsAccount.m_strSgipRtMoIp = rs[i]["moip"].data();
            smsAccount.m_uSgipRtMoPort = atoi(rs[i]["moport"].data());
            smsAccount.m_uSgipNodeId = atoi(rs[i]["nodeid"].data());
            smsAccount.m_uIdentify = atoi(rs[i]["identify"].data());
            smsAccount.m_uClientLeavel = atoi(rs[i]["client_level"].data());
            smsAccount.m_strExtNo = rs[i]["extendport"];
            smsAccount.m_uSignPortLen = atoi(rs[i]["signportlen"].data());
            smsAccount.m_uBelong_sale = atol(rs[i]["belong_sale"].data());
            smsAccount.m_uAccountExtAttr = Comm::strToUint32(rs[i]["accountExtAttr"].data());
            smsAccount.m_uiSpeed = Comm::strToUint32(rs[i]["access_speed"].data());

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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmsAccontStatusMap(std::map<string, SmsAccountState>& smsAccountStateMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_account_login_status");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsAccountState smsAccountState;
            smsAccountState.m_strAccount = rs[i]["clientid"].data();
            smsAccountState.m_uCode = atoi(rs[i]["code"].data());
            smsAccountState.m_strRemarks = rs[i]["remarks"].data();
            smsAccountState.m_strLockTime = rs[i]["locktime"].data();
            smsAccountState.m_uStatus= atoi(rs[i]["status"].data());
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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBAgentInfo(map<UInt64, AgentInfo>& agentInfoMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_agent_info where status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt64 agentId = 0;

            AgentInfo agentInfo;
            agentId = atoi(rs[i]["agent_id"].c_str());
            agentInfo.m_iAgent_type= atoi(rs[i]["agent_type"].c_str());


            agentInfoMap[agentId] = agentInfo;
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBClientForceExit(map<string, int>& clientForceExitMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[512] = { 0 };
    snprintf(sql, sizeof(sql),"select * from t_sms_client_force_exit where component_id=%lu", g_uComponentId);

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            clientForceExitMap[rs[i]["clientid"]] = atoi(rs[i]["state"].c_str());
        }
        return true;
    }
    else
    {
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmsTemplateMap(map<std::string, SmsTemplate>& smsTemplateMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_template where check_status = '1'");

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
        LogError("sql[%s]execute err!", sql);
        return false;
    }
    return false;
}



