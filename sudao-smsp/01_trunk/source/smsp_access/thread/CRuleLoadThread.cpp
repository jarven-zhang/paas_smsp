#include "CRuleLoadThread.h"
#include "CChineseConvert.h"
#include "CotentFilter.h"
#include "CDBThread.h"
#include "main.h"
#include "Thread.h"
#include "Comm.h"
#include "global.h"


using namespace models;

#define SEND_MSG_TO_THREAD(msg_, msgid_, var_, thread_)\
    { \
        msg_* pMsg = new msg_(); \
        pMsg->m_iMsgType = msgid_; \
        pMsg->var_ = var_; \
        thread_->PostMsg(pMsg); \
    }


CRuleLoadThread::CRuleLoadThread(const char *name):
    CThread(name)
{
    m_iSysKeywordCovRegular = 0;
    m_iAuditKeywordCovRegular = 0;
    m_iSysKeywordCovRegularOld = 0;
    m_iAuditKeywordCovRegularOld = 0;
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

    m_pCChineseConvert = new CChineseConvert();
    if(!m_pCChineseConvert->Init()){
        LogError("m_pCChineseConvert Init Error");
        return false;
    }

    if(!DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName))
    {
        LogError( "DBConnect() fail.\n");
        return false;
    }

    m_pDbTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "db_connect_ping", CHECK_DB_CONN_TIMEOUT);

    string strReg = "([0-1][0-9]|[2][0-4]):[0-5][0-9]";
    try{
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

    /*********set old keyword config***********/
    m_iAuditKeywordCovRegularOld = m_iAuditKeywordCovRegular;
    m_iSysKeywordCovRegularOld = m_iSysKeywordCovRegular;
    /******************************************/

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
    m_ChannelLastUpdateTime     = getTableLastUpdateTime("t_sms_channel");
    m_UserGwLastUpdateTime      = getTableLastUpdateTime("t_user_gw");
    //m_BlackListLastUpdateTime   = getTableLastUpdateTime("t_sms_white_list");

    /*system keyword update begin*/
    m_SysKeywordLastUpdateTime     = getTableLastUpdateTime("t_sms_keyword_list");
    m_SysCGroupRefClientUpdateTime = getTableLastUpdateTime("t_sms_sys_cgroup_ref_client");
    m_SysClientGroupUpdateTime = getTableLastUpdateTime("t_sms_sys_client_group");
    m_SysKgroupRefCategoryUpdateTime = getTableLastUpdateTime("t_sms_sys_kgroup_ref_category");
    /*system keyword update end*/

    m_SignextnoGwLastUpdateTime = getTableLastUpdateTime("t_signextno_gw");
    m_PriceLastUpdateTime       = getTableLastUpdateTime("t_sms_tariff");
    m_strSalePriceLastUpdateTime       = getTableLastUpdateTime("t_sms_client_tariff");
    m_TemplateOverrateLastUpdateTime = getTableLastUpdateTime("t_template_overrate");
    m_PhoneAreaLastUpdateTime        = getTableLastUpdateTime("t_sms_phone_section");
    m_ChannelGroupUpdateTime = getTableLastUpdateTime("t_sms_channelgroup");
    m_ChannelRefChlGroupUpdateTime = getTableLastUpdateTime("t_sms_channel_ref_channelgroup");
    //m_ChannelBlackListUpdateTime = getTableLastUpdateTime("t_sms_channel_black_list");
    m_ChannelKeyWordUpdateTime = getTableLastUpdateTime("t_sms_channel_keyword_list");
    m_ChannelWhiteListUpdateTime = getTableLastUpdateTime("t_sms_channel_white_list");
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
    m_IgnoreAuditKeyWordUpdateTime = getTableLastUpdateTime("t_sms_user_audit_keyword_ignore");
    m_AuditKeyWordUpdateTime = getTableLastUpdateTime("t_sms_audit_keyword"); ////smsp4.5
    m_AuditCGroupRefClientUpdateTime = getTableLastUpdateTime("t_sms_audit_cgroup_ref_client");
    m_AuditClientGroupUpdateTime = getTableLastUpdateTime("t_sms_audit_client_group");
    m_AuditKgroupRefCategoryUpdateTime = getTableLastUpdateTime("t_sms_audit_kgroup_ref_category");
    m_OverRateKeyWordUpdateTime = getTableLastUpdateTime("t_sms_overrate_keyword");
    m_ChannelExtendPortTableUpdateTime = getTableLastUpdateTime("t_sms_channel_extendport");
    m_strChannelTemplateUpdateTime = getTableLastUpdateTime("t_sms_template_audit");
    m_AgentInfoUpdateTime = getTableLastUpdateTime("t_sms_agent_info");

    /* t_sms_agent_account   */
    m_strAgentAccountUpdateTime = getTableLastUpdateTime("t_sms_agent_account");

    ////smsp5.0
    m_mqConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_mq_configure");
    m_componentRefMqUpdateTime = getTableLastUpdateTime("t_sms_component_ref_mq");
    m_systemErrorCodeUpdateTime = getTableLastUpdateTime("t_sms_system_error_desc");

    m_ChannelSegmentUpdateTime = getTableLastUpdateTime("t_sms_segment_channel");
    m_ChannelWhiteKeywordUpdateTime = getTableLastUpdateTime("t_sms_white_keyword_channel");
    m_ClientSegmentUpdateTime = getTableLastUpdateTime("t_sms_segment_client");
    m_ChannelSegCodeUpdateTime = getTableLastUpdateTime("t_sms_segcode_channel");
    m_componentConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_component_configure");

    m_autoWhiteTemplateLastUpdateTime = getTableLastUpdateTime("t_sms_auto_template");
    m_autoBlackTemplateLastUpdateTime = getTableLastUpdateTime("t_sms_auto_black_template");

    m_setLoginChannelsUpdateTime = getTableLastUpdateTime("t_sms_channel_login_status");

    m_ChannelWeightInfoUpdateTime = getTableLastUpdateTime("t_sms_channel_attribute_realtime_weight");

    m_ChannelAttrWeightConfigUpdateTime = getTableLastUpdateTime("t_sms_channel_attribute_weight_config");

    m_ChannelPoolPolicyUpdateTime = getTableLastUpdateTime("t_sms_channel_pool_policy");

    m_strmiddleWareUpdateTime  = getTableLastUpdateTime("t_sms_middleware_configure");
    m_ChannelOverrateLastUpdateTime = getTableLastUpdateTime("t_sms_channel_overrate");

    m_clientPrioritiesUpdateTime = getTableLastUpdateTime("t_sms_client_priority");

    m_strChannelPropertyLogUpdateTime = getTableLastUpdateTime("t_sms_channel_property_log");
    m_strUserPriceLogUpdateTime = getTableLastUpdateTime("t_sms_user_price_log");
    m_strUserPropertyLogUpdateTime = getTableLastUpdateTime("t_sms_user_property_log");

    m_strBLGrpRefCategoryUpdateTime = getTableLastUpdateTime("t_sms_blacklist_group_ref_category");
    m_strBLUserConfigUpdateTime = getTableLastUpdateTime("t_sms_blacklist_user_config");

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

    CAdapter::InterlockedIncrement((long*)&m_iCount);

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
    if(isTableUpdate("t_user_gw", m_UserGwLastUpdateTime))
    {
        m_UserGwMap.clear();
        getDBUserGwMap(m_UserGwMap);

        TUpdateUserGwReq* pMsgTemplate = new TUpdateUserGwReq();
        if (pMsgTemplate)
        {
            pMsgTemplate->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
            pMsgTemplate->m_UserGwMap = m_UserGwMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemplate);
        }


        TUpdateUserGwReq* pMsgRouter = new TUpdateUserGwReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
            pMsgRouter->m_UserGwMap = m_UserGwMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateUserGwReq* pMsgBlack = new TUpdateUserGwReq();
        if (pMsgBlack)
        {
            pMsgBlack->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
            pMsgBlack->m_UserGwMap = m_UserGwMap;
            CommPostMsg(STATE_INIT, pMsgBlack);
        }
    }

    /*************************system keyword update begin********************************/
    if(isTableUpdate("t_sms_keyword_list", m_SysKeywordLastUpdateTime))
    {
        m_sysKeyWordMap.clear();
        getDBKeyWordMap(m_sysKeyWordMap, "t_sms_keyword_list", m_iSysKeywordCovRegular);

        TUpdateKeyWordReq* pMsgRouter = new TUpdateKeyWordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ;
            pMsgRouter->m_keyWordMap = m_sysKeyWordMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateKeyWordReq* pMsgTemp = new TUpdateKeyWordReq();
        if (pMsgTemp)
        {
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ;
            pMsgTemp->m_keyWordMap = m_sysKeyWordMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
    }

    if (isTableUpdate("t_sms_sys_cgroup_ref_client",m_SysCGroupRefClientUpdateTime))
    {
        m_sysClientGrpRefClientMap.clear();
        getDBCGroupRefClientMap(m_sysClientGrpRefClientMap, "t_sms_sys_cgroup_ref_client");

        TUpdateKeyWordReq* pMsgRouter = new TUpdateKeyWordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SYS_CGROUPREFCLIENT_UPDATE_REQ;
            pMsgRouter->m_clientGrpRefClientMap = m_sysClientGrpRefClientMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateKeyWordReq* pMsgTemp = new TUpdateKeyWordReq();
        if (pMsgTemp)
        {
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYS_CGROUPREFCLIENT_UPDATE_REQ;
            pMsgTemp->m_clientGrpRefClientMap = m_sysClientGrpRefClientMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
    }
    if (isTableUpdate("t_sms_sys_client_group",m_SysClientGroupUpdateTime))
    {
        m_sysclientGrpRefKeywordGrpMap.clear();
        getDBClientGroupMap(m_sysclientGrpRefKeywordGrpMap,m_uSysDefaultGroupId, "t_sms_sys_client_group", "t_sms_sys_keyword_group");

        TUpdateKeyWordReq* pMsgRouter = new TUpdateKeyWordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SYS_CLIENTGROUP_UPDATE_REQ;
            pMsgRouter->m_clientGrpRefKeywordGrpMap = m_sysclientGrpRefKeywordGrpMap;
            pMsgRouter->m_uDefaultGroupId = m_uSysDefaultGroupId;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateKeyWordReq* pMsgTemp = new TUpdateKeyWordReq();
        if (pMsgTemp)
        {
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYS_CLIENTGROUP_UPDATE_REQ;
            pMsgTemp->m_clientGrpRefKeywordGrpMap = m_sysclientGrpRefKeywordGrpMap;
            pMsgTemp->m_uDefaultGroupId = m_uSysDefaultGroupId;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
    }
    if (isTableUpdate("t_sms_sys_kgroup_ref_category", m_SysKgroupRefCategoryUpdateTime) || isTableUpdate("t_sms_sys_keyword_category", m_SysKgroupRefCategoryUpdateTime))
    {
        m_syskeywordGrpRefCategoryMap.clear();
        getDBKgroupRefCategoryMap(m_syskeywordGrpRefCategoryMap, "t_sms_sys_kgroup_ref_category", "t_sms_sys_keyword_category");

        TUpdateKeyWordReq* pMsgRouter = new TUpdateKeyWordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SYS_KGROUPREFCATEGORY_UPDATE_REQ;
            pMsgRouter->m_keywordGrpRefCategoryMap = m_syskeywordGrpRefCategoryMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateKeyWordReq* pMsgTemp = new TUpdateKeyWordReq();
        if (pMsgTemp)
        {
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYS_KGROUPREFCATEGORY_UPDATE_REQ;
            pMsgTemp->m_keywordGrpRefCategoryMap = m_syskeywordGrpRefCategoryMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
    }
    /*************************system keyword update end********************************/

    if(isTableUpdate("t_signextno_gw", m_SignextnoGwLastUpdateTime))
    {
        m_SignextnoGwMap.clear();
        getDBSignExtnoGwMap(m_SignextnoGwMap);

        TUpdateSignextnoGwReq* pMsgRouter = new TUpdateSignextnoGwReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ;
            pMsgRouter->m_SignextnoGwMap = m_SignextnoGwMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if (isTableUpdate("t_sms_channel_extendport",m_ChannelExtendPortTableUpdateTime))
    {
        m_ChannelExtendPortTable.clear();
        getDBChannelExtendPortTableMap(m_ChannelExtendPortTable);

        TUpdateChannelExtendPortReq* pMsgRouter = new TUpdateChannelExtendPortReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ;
            pMsgRouter->m_ChannelExtendPortTable = m_ChannelExtendPortTable;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_tariff", m_PriceLastUpdateTime))
    {
        m_PriceMap.clear();
        getDBSmppPriceMap(m_PriceMap);

        TUpdateSmppPriceReq* pMsgRouter = new TUpdateSmppPriceReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SMPPPRICE_UPDATE_REQ;
            pMsgRouter->m_PriceMap = m_PriceMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }
    if(isTableUpdate("t_sms_client_tariff", m_strSalePriceLastUpdateTime))
    {
        m_SalePriceMap.clear();
        getDBSmppSalePriceMap(m_SalePriceMap);

        TUpdateSmppSalePriceReq* pMsgRouter = new TUpdateSmppSalePriceReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SMPPSALEPRICE_UPDATE_REQ;
            pMsgRouter->m_salePriceMap = m_SalePriceMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }
    if(isTableUpdate("t_template_overrate", m_TemplateOverrateLastUpdateTime))
    {
        m_TempRuleMap.clear();
        m_KeyWordTempRuleMap.clear();
        getDBAllTempltRule(m_TempRuleMap,m_KeyWordTempRuleMap);

        {
            TUpdateKeyWordTempltRuleReq* pReq = new TUpdateKeyWordTempltRuleReq();
            pReq->m_iMsgType = MSGTYPE_RULELOAD_KEYWORDTEMPLATERULE_UPDATE_REQ;
            pReq->m_KeyWordTempRuleMap = m_KeyWordTempRuleMap;
            g_pOverRateThread->PostMsg(pReq);
        }

        {
            TUpdateTempltRuleReq* pReq = new TUpdateTempltRuleReq();
            pReq->m_iMsgType = MSGTYPE_RULELOAD_TEMPLATERULE_UPDATE_REQ;
            pReq->m_TempRuleMap = m_TempRuleMap;
            g_pOverRateThread->PostMsg(pReq);
        }
    }

    if(isTableUpdate("t_sms_channel_overrate", m_ChannelOverrateLastUpdateTime))
    {
        m_ChannelOverrateMap.clear();
        getDBChannelOverrate(m_ChannelOverrateMap);

        TUpdateChannelOverrateReq* pMsg = new TUpdateChannelOverrateReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNELOVERRATE_UPDATE_REQ;
        pMsg->m_ChannelOverrateMap = m_ChannelOverrateMap;
        g_pOverRateThread->PostMsg(pMsg);

        TUpdateChannelOverrateReq* pMsgRouter = new TUpdateChannelOverrateReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELOVERRATE_UPDATE_REQ;
            pMsgRouter->m_ChannelOverrateMap = m_ChannelOverrateMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_phone_section", m_PhoneAreaLastUpdateTime))
    {
        m_PhoneAreaMap.clear();
        getDBPhoneAreaMap(m_PhoneAreaMap);
        TUpdatePhoneAreaReq* pMsgRouter = new TUpdatePhoneAreaReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ;
            pMsgRouter->m_PhoneAreaMap = m_PhoneAreaMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        {
            TUpdatePhoneAreaReq* pMsgSession = new TUpdatePhoneAreaReq();
            if ( pMsgSession )
            {
                pMsgSession->m_iMsgType = MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ;
                pMsgSession->m_PhoneAreaMap = m_PhoneAreaMap;
                CommPostMsg( STATE_INIT, pMsgSession );
            }
        }

        TUpdatePhoneAreaReq* pAudit = new TUpdatePhoneAreaReq();
        pAudit->m_iMsgType = MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ;
        pAudit->m_PhoneAreaMap = m_PhoneAreaMap;
        g_pAuditThread->PostMsg(pAudit);
    }

    if(isTableUpdate("t_sms_channelgroup", m_ChannelGroupUpdateTime))
    {
        m_ChannelGroupMap.clear();
        getDBChannelGroupMap(m_ChannelGroupMap);

        TUpdateChannelGroupReq* pMsgRouter = new TUpdateChannelGroupReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELGROUP_UPDATE_REQ;
            pMsgRouter->m_ChannelGroupMap = m_ChannelGroupMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_channel_ref_channelgroup", m_ChannelRefChlGroupUpdateTime))     //TO DO
    {
        m_ChannelGroupMap.clear();
        getDBChannelGroupMap(m_ChannelGroupMap);

        TUpdateChannelGroupReq* pMsgRouter = new TUpdateChannelGroupReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELGROUP_UPDATE_REQ;
            pMsgRouter->m_ChannelGroupMap = m_ChannelGroupMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_channel_keyword_list", m_ChannelKeyWordUpdateTime))
    {
        m_ChannelKeyWordMap.clear();
        getDBChannelKeyWordMap(m_ChannelKeyWordMap);    //

        TUpdateChannelKeyWordReq* pMsgRouter = new TUpdateChannelKeyWordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELKEYWORD_UPDATE_REQ;
            pMsgRouter->m_ChannelKeyWordMap = m_ChannelKeyWordMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_channel_white_list", m_ChannelWhiteListUpdateTime))
    {
        m_ChannelWhiteListMap.clear();
        getDBChannelWhiteListMap(m_ChannelWhiteListMap);    //

        TUpdateChannelWhiteListReq* pMsgRouter = new TUpdateChannelWhiteListReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELWHITELIST_UPDATE_REQ;
            pMsgRouter->m_ChannelWhiteListMap = m_ChannelWhiteListMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if(isTableUpdate("t_sms_operater_segment", m_OperaterSegmentUpdateTime))
    {
        m_OperaterSegmentMap.clear();
        getDBOperaterSegmentMap(m_OperaterSegmentMap);  //

        TUpdateOperaterSegmentReq* pHttpSend = new TUpdateOperaterSegmentReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ;
        pHttpSend->m_OperaterSegmentMap = m_OperaterSegmentMap;
        CommPostMsg( STATE_INIT, pHttpSend );

        TUpdateOperaterSegmentReq* pAudit = new TUpdateOperaterSegmentReq();
        pAudit->m_iMsgType = MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ;
        pAudit->m_OperaterSegmentMap = m_OperaterSegmentMap;
        g_pAuditThread->PostMsg(pAudit);
    }

    if(isTableUpdate("t_sms_overrate_white_list", m_OverRateWhiteListUpdateTime))
    {
        m_overRateWhiteList.clear();
        getDBOverRateWhiteList(m_overRateWhiteList);

        SEND_MSG_TO_THREAD(TUpdateOverRateWhiteListReq, MSGTYPE_RULELOAD_OVERRATEWHITELIST_UPDATE_REQ, m_overRateWhiteList, g_pSessionThread);
    }

    if(isTableUpdate("t_sms_param", m_SysParamLastUpdateTime))
    {
        m_SysParamMap.clear();
        getDBSysParamMap(m_SysParamMap);
        if (m_iSysKeywordCovRegularOld != m_iSysKeywordCovRegular)
        {
            m_iSysKeywordCovRegularOld = m_iSysKeywordCovRegular;
            m_sysKeyWordMap.clear();
            getDBKeyWordMap(m_sysKeyWordMap, "t_sms_keyword_list", m_iSysKeywordCovRegular);

            TUpdateKeyWordReq* pMsgRoute = new TUpdateKeyWordReq();
            pMsgRoute->m_iMsgType = MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ;
            pMsgRoute->m_keyWordMap = m_sysKeyWordMap;
            CommPostMsg(STATE_ROUTE, pMsgRoute);

            TUpdateKeyWordReq* pMsgTemp = new TUpdateKeyWordReq();
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ;
            pMsgTemp->m_keyWordMap = m_sysKeyWordMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
        if (m_iAuditKeywordCovRegularOld != m_iAuditKeywordCovRegular)
        {
            m_iAuditKeywordCovRegularOld = m_iAuditKeywordCovRegular;
            m_AuditKeyWordMap.clear();
            getDBAuditKeyWordMap(m_AuditKeyWordMap);
            m_IgnoreAuditKeyWordMap.clear();
            getDBIgnoreAuditKeyWordMap(m_IgnoreAuditKeyWordMap);

            TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_AUDITKEYWORD_UPDATE_REQ;
            pMsg->m_AuditKeyWordMap = m_AuditKeyWordMap;
            g_pAuditThread->PostMsg(pMsg);

            pMsg = new TUpdateAuidtKeyWordReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_IGNORE_AUDITKEYWORD_UPDATE_REQ;
            pMsg->m_IgnoreAuditKeyWordMap = m_IgnoreAuditKeyWordMap;
            g_pAuditThread->PostMsg(pMsg);
        }

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

        TUpdateSysParamRuleReq* pMsgRoute = new TUpdateSysParamRuleReq();
        if (pMsgRoute)
        {
            pMsgRoute->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsgRoute->m_SysParamMap = m_SysParamMap;
            CommPostMsg(STATE_ROUTE, pMsgRoute);
        }

        TUpdateSysParamRuleReq* pMsgTemp = new TUpdateSysParamRuleReq();
        if (pMsgTemp)
        {
            pMsgTemp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pMsgTemp->m_SysParamMap = m_SysParamMap;
            CommPostMsg(STATE_TEMPLATE, pMsgTemp);
        }
        {
            TUpdateSysParamRuleReq* pReq = new TUpdateSysParamRuleReq();
            pReq->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pReq->m_SysParamMap = m_SysParamMap;
            g_pOverRateThread->PostMsg(pReq);
        }

        //audit get thred
        TUpdateSysParamRuleReq* pAuditThdMsg = new TUpdateSysParamRuleReq();
        pAuditThdMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pAuditThdMsg->m_SysParamMap = m_SysParamMap;
        g_pAuditThread->PostMsg(pAuditThdMsg);

        TUpdateSysParamRuleReq* pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );

        {
            TUpdateSysParamRuleReq* pSessionSysParam = new TUpdateSysParamRuleReq();
            pSessionSysParam->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
            pSessionSysParam->m_SysParamMap = m_SysParamMap;
            CommPostMsg( STATE_INIT, pSessionSysParam );
        }

        MONITOR_UPDATE_SYS( g_pMQMonitorProducerThread, m_SysParamMap );
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

        ////httpsend
        TUpdateSmsAccontReq* pHttpSendMsg = new TUpdateSmsAccontReq();
        pHttpSendMsg->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
        pHttpSendMsg->m_SmsAccountMap = m_SmsAccountMap;
        CommPostMsg( STATE_INIT, pHttpSendMsg );

        TUpdateSmsAccontReq* pMsgRouter = new TUpdateSmsAccontReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
            pMsgRouter->m_SmsAccountMap = m_SmsAccountMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        {
            TUpdateSmsAccontReq* pReq = new TUpdateSmsAccontReq();
            pReq->m_iMsgType = MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ;
            pReq->m_SmsAccountMap = m_SmsAccountMap;
            g_pAuditThread->PostMsg(pReq);
        }
    }


    if(isTableUpdate("t_sms_clientid_signport", m_SmsClientidSignPortUpdateTime))
    {
        m_ClientIdAndSignMap.clear();
        m_ClientIdAndSignPortMap.clear();
        getDBClientAndSignMap(m_ClientIdAndSignMap,m_ClientIdAndSignPortMap);


        /*
        ///
        TUpdateClientIdAndSignReq* pReport = new TUpdateClientIdAndSignReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ;
        pReport->m_ClientIdAndSignMap = m_ClientIdAndSignPortMap;
        g_pReportReceiveThread->PostMsg(pReport);
        */
    }

    if(isTableUpdate("t_sms_user_noaudit_keyword", m_NoAuditKeyWordUpdateTime))
    {
        m_NoAuditKeyWordMap.clear();
        getDBNoAuditKeyWordMap(m_NoAuditKeyWordMap);

        TUpdateNoAuidtKeyWordReq* pMsg = new TUpdateNoAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_NOAUDITKEYWORD_UPDATE_REQ;
        pMsg->m_NoAuditKeyWordMap = m_NoAuditKeyWordMap;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_overrate_keyword",m_OverRateKeyWordUpdateTime))
    {
        m_OverRateKeyWordMap.clear();
        getDBOverRateKeyWordMap(m_OverRateKeyWordMap);

        TUpdateOverRateKeyWordReq* pMsg = new TUpdateOverRateKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_OVERRATEKEYWORD_UPDATE_REQ;
        pMsg->m_OverRateKeyWordMap = m_OverRateKeyWordMap;
        g_pOverRateThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_user_audit_keyword_ignore",m_IgnoreAuditKeyWordUpdateTime))
    {
        m_IgnoreAuditKeyWordMap.clear();
        getDBIgnoreAuditKeyWordMap(m_IgnoreAuditKeyWordMap);

        TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_IGNORE_AUDITKEYWORD_UPDATE_REQ;
        pMsg->m_IgnoreAuditKeyWordMap = m_IgnoreAuditKeyWordMap;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_audit_keyword",m_AuditKeyWordUpdateTime))
    {
        m_AuditKeyWordMap.clear();
        getDBAuditKeyWordMap(m_AuditKeyWordMap);

        TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AUDITKEYWORD_UPDATE_REQ;
        pMsg->m_AuditKeyWordMap = m_AuditKeyWordMap;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_audit_cgroup_ref_client",m_AuditCGroupRefClientUpdateTime))
    {
        m_AuditCGroupRefClientMap.clear();
        getDBAuditCGroupRefClientMap(m_AuditCGroupRefClientMap);

        TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AUDIT_CGROUPREFCLIENT_UPDATE_REQ;
        pMsg->m_AuditCGroupRefClientMap = m_AuditCGroupRefClientMap;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_audit_client_group",m_AuditClientGroupUpdateTime))
    {
        m_AuditClientGroupMap.clear();
        getDBAuditClientGroupMap(m_AuditClientGroupMap,m_uDefaultGroupId);

        TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AUDIT_CLIENTGROUP_UPDATE_REQ;
        pMsg->m_AuditClientGroupMap = m_AuditClientGroupMap;
        pMsg->m_uDefaultGroupId = m_uDefaultGroupId;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_audit_kgroup_ref_category",m_AuditKgroupRefCategoryUpdateTime))
    {
        m_AuditKgroupRefCategoryMap.clear();
        getDBAuditKgroupRefCategoryMap(m_AuditKgroupRefCategoryMap);

        TUpdateAuidtKeyWordReq* pMsg = new TUpdateAuidtKeyWordReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AUDIT_KGROUPREFCATEGORY_UPDATE_REQ;
        pMsg->m_AuditKgroupRefCategoryMap = m_AuditKgroupRefCategoryMap;
        g_pAuditThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_agent_info",m_AgentInfoUpdateTime))
    {
        m_AgentInfoMap.clear();
        getDBAgentInfo(m_AgentInfoMap);

        TUpdateAgentInfoReq* pMsg = new TUpdateAgentInfoReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ;
        pMsg->m_AgentInfoMap = m_AgentInfoMap;
        CommPostMsg( STATE_INIT, pMsg );
    }

    //必须在t_sms_channel之前
    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        TUpdateMqConfigReq* pMqConfig = new TUpdateMqConfigReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfig->m_mapMqConfig = m_mqConfigInfo;
        g_pMqioConsumerThread->PostMsg(pMqConfig);

        TUpdateMqConfigReq* pAuditReslutMq = new TUpdateMqConfigReq();
        pAuditReslutMq->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pAuditReslutMq->m_mapMqConfig = m_mqConfigInfo;
        g_pMqAuditConsumerThread->PostMsg(pAuditReslutMq);

        TUpdateMqConfigReq* pMqConfig2 = new TUpdateMqConfigReq();
        pMqConfig2->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfig2->m_mapMqConfig = m_mqConfigInfo;
        CommPostMsg( STATE_INIT, pMqConfig2 );

        TUpdateMqConfigReq* pGet = new TUpdateMqConfigReq();
        pGet->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pGet->m_mapMqConfig = m_mqConfigInfo;
        g_pGetChannelMqSizeThread->PostMsg(pGet);

        TUpdateMqConfigReq* pAudit = new TUpdateMqConfigReq();
        pAudit->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pAudit->m_mapMqConfig = m_mqConfigInfo;
        g_pAuditThread->PostMsg(pAudit);

        if (NULL != g_pMqChannelUnusualConsumerThread)
        {
            TUpdateMqConfigReq* pUnusual = new TUpdateMqConfigReq();
            pUnusual->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
            pUnusual->m_mapMqConfig = m_mqConfigInfo;
            g_pMqChannelUnusualConsumerThread->PostMsg(pUnusual);
        }

        if (NULL != g_pMqOldNoPriQueueDataTransferThread)
        {
            TUpdateMqConfigReq* pTransfer = new TUpdateMqConfigReq();
            pTransfer->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
            pTransfer->m_mapMqConfig = m_mqConfigInfo;
            g_pMqOldNoPriQueueDataTransferThread->PostMsg(pTransfer);
        }

        TUpdateMqConfigReq* pMqConfigReback = new TUpdateMqConfigReq();
        pMqConfigReback->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfigReback->m_mapMqConfig = m_mqConfigInfo;
        g_pMqioRebackConsumerThread->PostMsg(pMqConfigReback);

    }

    if(isTableUpdate("t_sms_channel", m_ChannelLastUpdateTime))
    {
        m_ChannelMap.clear();
        getDBChannlelMap(m_ChannelMap);

        TUpdateChannelReq* pMsg = new TUpdateChannelReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pMsg->m_ChannelMap = m_ChannelMap;
        CommPostMsg( STATE_INIT, pMsg );


        TUpdateChannelReq* pMsgRouter = new TUpdateChannelReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            pMsgRouter->m_ChannelMap = m_ChannelMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateChannelReq* pGet = new TUpdateChannelReq();
        pGet->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pGet->m_ChannelMap = m_ChannelMap;
        g_pGetChannelMqSizeThread->PostMsg(pGet);

        TUpdateChannelReq* pMsg2 = new TUpdateChannelReq();
        pMsg2->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pMsg2->m_ChannelMap = m_ChannelMap;
        g_pMqOldNoPriQueueDataTransferThread->PostMsg(pMsg2);

        if (NULL != g_pMqChannelUnusualConsumerThread)
        {
            m_ChannelUnusualMap.clear();
            getDBChannlelUnusualMap(m_ChannelUnusualMap);
            TUpdateChannelReq* pUnusual = new TUpdateChannelReq();
            pUnusual->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            pUnusual->m_ChannelMap = m_ChannelUnusualMap;
            g_pMqChannelUnusualConsumerThread->PostMsg(pUnusual);
        }
    }

    if (isTableUpdate("t_sms_component_configure", m_componentConfigInfoUpdateTime))
    {
        m_componentConfigInfoMap.clear();
        getDBComponentConfig(m_componentConfigInfoMap);

        TMsg* pMsgToMqConsumerReback = new TMsg();
        pMsgToMqConsumerReback->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqioRebackConsumerThread->PostMsg(pMsgToMqConsumerReback);

        TMsg* pMsgToMqUnusualConsumer = new TMsg();
        pMsgToMqUnusualConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqChannelUnusualConsumerThread->PostMsg(pMsgToMqUnusualConsumer);

        TMsg* pMsgToMqOldNoPri = new TMsg();
        pMsgToMqOldNoPri->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqOldNoPriQueueDataTransferThread->PostMsg(pMsgToMqOldNoPri);

        // ç»„ä»¶é…ç½®
        TMsg* pMsgToMqConsumer = new TMsg();
        pMsgToMqConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqioConsumerThread->PostMsg(pMsgToMqConsumer);

        TMsg* pAuditReslutMq = new TMsg();
        pAuditReslutMq->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqAuditConsumerThread->PostMsg(pAuditReslutMq);

        {
            TUpdateComponentConfigReq* pReq = new TUpdateComponentConfigReq();
            pReq->m_componentConfigInfoMap = m_componentConfigInfoMap;
            pReq->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
            g_pAuditThread->PostMsg(pReq);
        }

        TUpdateComponentConfigReq* pComponetConfig = new TUpdateComponentConfigReq();
        pComponetConfig->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        pComponetConfig->m_componentConfigInfoMap = m_componentConfigInfoMap;
        CommPostMsg( STATE_INIT, pComponetConfig );

        TUpdateComponentConfigReq* pMsgRouter = new TUpdateComponentConfigReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
            pMsgRouter->m_componentConfigInfoMap = m_componentConfigInfoMap;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        string strCompentId = Comm::int2str( g_uComponentId );
        map< string, ComponentConfig>::iterator itr = m_componentConfigInfoMap.find(
                        strCompentId.append(COMPONENT_TYPE_ACCESS) );
        if( itr != m_componentConfigInfoMap.end()){
            MONITOR_UPDATE_COMP( g_pMQMonitorProducerThread, itr->second );
        }else{
            LogWarn("Monitor Publish Can't Find %s", strCompentId.data());
        }
    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        TUpdateComponentRefMqReq* pMqConfigReback = new TUpdateComponentRefMqReq();
        pMqConfigReback->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pMqConfigReback->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pMqioRebackConsumerThread->PostMsg(pMqConfigReback);

        if (NULL != g_pMqChannelUnusualConsumerThread)
        {
            TUpdateComponentRefMqReq* pUnusual = new TUpdateComponentRefMqReq();
            pUnusual->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
            pUnusual->m_componentRefMqInfo = m_componentRefMqInfo;
            g_pMqChannelUnusualConsumerThread->PostMsg(pUnusual);
        }

        if (NULL != g_pMqOldNoPriQueueDataTransferThread)
        {
            TUpdateComponentRefMqReq* pReq = new TUpdateComponentRefMqReq();
            pReq->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
            pReq->m_componentRefMqInfo = m_componentRefMqInfo;
            g_pMqOldNoPriQueueDataTransferThread->PostMsg(pReq);
        }

        TUpdateComponentRefMqReq* pMqConfig = new TUpdateComponentRefMqReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pMqConfig->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pMqioConsumerThread->PostMsg(pMqConfig);

        TUpdateComponentRefMqReq* pAuditReslutMq = new TUpdateComponentRefMqReq();
        pAuditReslutMq->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pAuditReslutMq->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pMqAuditConsumerThread->PostMsg(pAuditReslutMq);

        TUpdateComponentRefMqReq* pMqConfig2 = new TUpdateComponentRefMqReq();
        pMqConfig2->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pMqConfig2->m_componentRefMqInfo = m_componentRefMqInfo;
        CommPostMsg( STATE_INIT, pMqConfig2 );

        TUpdateComponentRefMqReq* pAudit = new TUpdateComponentRefMqReq();
        pAudit->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pAudit->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pAuditThread->PostMsg(pAudit);
    }

    if (isTableUpdate("t_sms_system_error_desc", m_systemErrorCodeUpdateTime))
    {
        m_mapSystemErrorCode.clear();
        getDBSystemErrorCode(m_mapSystemErrorCode);

        TUpdateSystemErrorCodeReq* pHttp = new TUpdateSystemErrorCodeReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
        pHttp->m_mapSystemErrorCode = m_mapSystemErrorCode;
        g_pAuditThread->PostMsg(pHttp);


        TUpdateSystemErrorCodeReq* pMsgTemplate = new TUpdateSystemErrorCodeReq();
        if (pMsgTemplate)
        {
            pMsgTemplate->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
            pMsgTemplate->m_mapSystemErrorCode = m_mapSystemErrorCode;
            CommPostMsg(STATE_TEMPLATE, pMsgTemplate);
        }

        TUpdateSystemErrorCodeReq* pMsgRouter = new TUpdateSystemErrorCodeReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
            pMsgRouter->m_mapSystemErrorCode = m_mapSystemErrorCode;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        TUpdateSystemErrorCodeReq* pMsgSession = new TUpdateSystemErrorCodeReq();
        if (pMsgSession)
        {
            pMsgSession->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
            pMsgSession->m_mapSystemErrorCode = m_mapSystemErrorCode;
            CommPostMsg(STATE_INIT, pMsgSession );
        }
    }

    if (isTableUpdate("t_sms_template_audit", m_strChannelTemplateUpdateTime))
    {
        m_mapChannelTemplate.clear();
        getDBChannelTemplateMap(m_mapChannelTemplate);

        TUpdateChannelTemplateReq* pMsgRouter = new TUpdateChannelTemplateReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNELTEMPLATE_UPDATE_REQ;
            pMsgRouter->m_mapChannelTemplate = m_mapChannelTemplate;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if (isTableUpdate("t_sms_segment_channel", m_ChannelSegmentUpdateTime))
    {
        m_mapChannelSegment.clear();
        getDBChannelSegmentMap(m_mapChannelSegment);

        TUpdateChannelSegmentReq* pMsgRouter = new TUpdateChannelSegmentReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_SEGMENT_UPDATE_REQ;
            pMsgRouter->m_mapChannelSegment = m_mapChannelSegment;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if (isTableUpdate("t_sms_white_keyword_channel", m_ChannelWhiteKeywordUpdateTime))
    {
        m_mapChannelWhiteKeyword.clear();
        getDBChannelWhiteKeywordMap(m_mapChannelWhiteKeyword);

        TUpdateChannelWhiteKeywordReq* pMsgRouter = new TUpdateChannelWhiteKeywordReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_WHITEKEYWORD_UPDATE_REQ;
            pMsgRouter->m_mapChannelWhiteKeyword = m_mapChannelWhiteKeyword;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    if (isTableUpdate("t_sms_segment_client", m_ClientSegmentUpdateTime))
    {
        m_mapClientSegment.clear();
        m_mapClientidSegment.clear();
        getDBClientSegmentMap(m_mapClientSegment,m_mapClientidSegment);

        TUpdateClientSegmentReq* pMsgRouter = new TUpdateClientSegmentReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CLIENT_SEGMENT_UPDATE_REQ;
            pMsgRouter->m_mapClientSegment = m_mapClientSegment;
            pMsgRouter->m_mapClientidSegment = m_mapClientidSegment;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }

        m_mapChannelWhiteKeyword.clear();
        getDBChannelWhiteKeywordMap(m_mapChannelWhiteKeyword);

        TUpdateChannelWhiteKeywordReq* pSendMsg = new TUpdateChannelWhiteKeywordReq();
        if (pSendMsg)
        {
            pSendMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_WHITEKEYWORD_UPDATE_REQ;
            pSendMsg->m_mapChannelWhiteKeyword = m_mapChannelWhiteKeyword;
            CommPostMsg(STATE_ROUTE, pSendMsg);
        }
    }

    if (isTableUpdate("t_sms_segcode_channel", m_ChannelSegCodeUpdateTime))
    {
        m_mapChannelSegCode.clear();
        getDBChannelSegCodeMap(m_mapChannelSegCode);

        TUpdateChannelSegCodeReq* pMsgRouter = new TUpdateChannelSegCodeReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_SEG_CODE_UPDATE_REQ;
            pMsgRouter->m_mapChannelSegCode = m_mapChannelSegCode;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    // 代理商账户配置
    if ( isTableUpdate("t_sms_agent_account", m_strAgentAccountUpdateTime ))
    {
        m_AgentAcctMap.clear();
        getDBAgentAccount( m_AgentAcctMap );
        TupdateAgentAcctInfoReq* pMsg = new TupdateAgentAcctInfoReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_AGENT_ACCOUNT_UPDATE_REQ;
        pMsg->m_AgentAcctMap = m_AgentAcctMap;
        CommPostMsg( STATE_INIT, pMsg );
    }

    // 白模板配置
    if(isTableUpdate("t_sms_auto_template", m_autoWhiteTemplateLastUpdateTime))
    {
        getDBAutoTemplateMap(SMS_TEMPLATE_WHITE, m_mapAutoWhiteTemplate, m_mapFixedAutoWhiteTemplate);

        TUpdateAutoWhiteTemplateReq * pMsgTemplate = new TUpdateAutoWhiteTemplateReq();
        if (pMsgTemplate)
        {
            pMsgTemplate->m_iMsgType = MSGTYPE_RULELOAD_AUTO_TEMPLATE_UPDATE_REQ;
            pMsgTemplate->m_mapAutoWhiteTemplate = m_mapAutoWhiteTemplate;
            pMsgTemplate->m_mapFixedAutoWhiteTemplate = m_mapFixedAutoWhiteTemplate;
            CommPostMsg(STATE_TEMPLATE, pMsgTemplate);
        }
    }

    if(isTableUpdate("t_sms_auto_black_template", m_autoBlackTemplateLastUpdateTime))
    {
        getDBAutoTemplateMap(SMS_TEMPLATE_BLACK, m_mapAutoBlackTemplate, m_mapFixedAutoBlackTemplate);

        TUpdateAutoBlackTemplateReq * pMsgTemplate = new TUpdateAutoBlackTemplateReq();
        if (pMsgTemplate)
        {
            pMsgTemplate->m_iMsgType = MSGTYPE_RULELOAD_AUTO_BLACK_TEMPLATE_UPDATE_REQ;
            pMsgTemplate->m_mapAutoBlackTemplate = m_mapAutoBlackTemplate;
            pMsgTemplate->m_mapFixedAutoBlackTemplate = m_mapFixedAutoBlackTemplate;
            CommPostMsg(STATE_TEMPLATE, pMsgTemplate);
        }
    }

    if(isTableUpdate("t_sms_channel_login_status", m_setLoginChannelsUpdateTime))
    {
        m_setLoginChannels.clear();
        getDBLoginChannels(m_setLoginChannels);

        TUpdateLoginChannelsReq * pMsgRouter = new TUpdateLoginChannelsReq();
        if (pMsgRouter)
        {
            pMsgRouter->m_setLoginChannels = m_setLoginChannels;
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_LOGIN_UPDATE_REQ;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    // 通道实时权重表
    if (isTableUpdate("t_sms_channel_attribute_realtime_weight", m_ChannelWeightInfoUpdateTime))
    {
        getDBChannelRealtimeWeightInfo(m_channelsRealtimeWeightInfo);

        TUpdateChannelRealtimeWeightInfo * pMsgRouter = new TUpdateChannelRealtimeWeightInfo();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_REALTIME_WEIGHTINFO_REQ;
            pMsgRouter->m_channelsRealtimeWeightInfo = m_channelsRealtimeWeightInfo;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    // ͨ通道权重属性配置
    if (isTableUpdate("t_sms_channel_attribute_weight_config", m_ChannelAttrWeightConfigUpdateTime))
    {
        getDBChannelAttributeWeightConfig(m_channelAttrWeightConfig);

        TUpdateChannelAttrWeightConfig * pMsgRouter = new TUpdateChannelAttrWeightConfig();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_ATTRIBUTE_WEIGHT_CONFIG_REQ;
            pMsgRouter->m_channelAttrWeightConfig = m_channelAttrWeightConfig;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    // ͨ通道策略池
    if (isTableUpdate("t_sms_channel_pool_policy", m_ChannelPoolPolicyUpdateTime))
    {
        getDBChannelPoolPolicies(m_channelPoolPolicies);

        TUpdateChannelPoolPolicyConfig * pMsgRouter = new TUpdateChannelPoolPolicyConfig();
        if (pMsgRouter)
        {
            pMsgRouter->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_POOL_POLICY_REQ;
            pMsgRouter->m_channelPoolPolicies = m_channelPoolPolicies;
            CommPostMsg(STATE_ROUTE, pMsgRouter);
        }
    }

    // 中间件配置表
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

    if (isTableUpdate("t_sms_client_priority", m_clientPrioritiesUpdateTime))
    {
        getDBClientPrioritiesConfig(m_clientPriorities);

        TUpdateClientPriorityReq * pMsg = new TUpdateClientPriorityReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CLIENT_PRIORITY_UPDATE_REQ;
        pMsg->m_clientPriorities = m_clientPriorities;
        g_pSessionThread->PostMsg(pMsg);

        if (g_pMqOldNoPriQueueDataTransferThread != NULL)
        {
            TUpdateClientPriorityReq * pMsg = new TUpdateClientPriorityReq();
            pMsg->m_iMsgType = MSGTYPE_RULELOAD_CLIENT_PRIORITY_UPDATE_REQ;
            pMsg->m_clientPriorities = m_clientPriorities;
            g_pMqOldNoPriQueueDataTransferThread->PostMsg(pMsg);
        }
    }

    if (isTableUpdate("t_sms_channel_property_log", m_strChannelPropertyLogUpdateTime))
    {
        map<UInt64, list<channelPropertyLog_ptr_t> >::iterator iterOld = m_channelPropertyLogMap.begin();
        for (; iterOld != m_channelPropertyLogMap.end(); iterOld++)
        {
            iterOld->second.clear();
        }
        m_channelPropertyLogMap.clear();

        getDBChannelPropertyLogMap(m_channelPropertyLogMap);
        TUpdateChannelPropertyLogReq * pMsg = new TUpdateChannelPropertyLogReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_PROPERTY_LOG_UPDATE_REQ;
        pMsg->m_channelPropertyLogMap = m_channelPropertyLogMap;
        CommPostMsg( STATE_INIT , pMsg );
    }

    if (isTableUpdate("t_sms_user_price_log", m_strUserPriceLogUpdateTime))
    {
        map<string, list<userPriceLog_ptr_t> >::iterator iterOld = m_userPriceLogMap.begin();
        for (; iterOld != m_userPriceLogMap.end(); iterOld++)
        {
            iterOld->second.clear();
        }
        m_userPriceLogMap.clear();

        getDBUserPriceLogMap(m_userPriceLogMap);
        TUpdateUserPriceLogReq * pMsg = new TUpdateUserPriceLogReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_USER_PRICE_LOG_UPDATE_REQ;
        pMsg->m_userPriceLogMap = m_userPriceLogMap;
        CommPostMsg( STATE_INIT , pMsg );
    }

    if (isTableUpdate("t_sms_user_property_log", m_strUserPropertyLogUpdateTime))
    {
        map<string, list<userPropertyLog_ptr_t> >::iterator iterOld = m_userPropertyLogMap.begin();
        for (; iterOld != m_userPropertyLogMap.end(); iterOld++)
        {
            iterOld->second.clear();
        }
        m_userPropertyLogMap.clear();

        getDBUserPropertyLogMap(m_userPropertyLogMap);
        TUpdateUserPropertyLogReq * pMsg = new TUpdateUserPropertyLogReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_USER_PROPERTY_LOG_UPDATE_REQ;
        pMsg->m_userPropertyLogMap = m_userPropertyLogMap;
        CommPostMsg( STATE_INIT , pMsg );
    }

    if (isTableUpdate("t_sms_blacklist_group_ref_category", m_strBLGrpRefCategoryUpdateTime))
    {
        getDBbgroupRefCategoryMap(m_uBgroupRefCategoryMap);

        TUpdateBlGrpRefCatConfigReq * pMsg = new TUpdateBlGrpRefCatConfigReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_BLGRP_REF_CATEGORY_UPDATE_REQ;
        pMsg->m_uBgroupRefCategoryMap = m_uBgroupRefCategoryMap;
        CommPostMsg(STATE_INIT, pMsg);
    }

    if (isTableUpdate("t_sms_blacklist_user_config", m_strBLUserConfigUpdateTime))
    {
        getDBBlacklistGroupMap(m_uBlacklistGrpMap);
        TUpdateBlUserConfigGrpReq * pMsg = new TUpdateBlUserConfigGrpReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_BLUSER_CONFIG_GROUP_UPDATE_REQ;
        pMsg->m_uBlacklistGrpMap = m_uBlacklistGrpMap;
        CommPostMsg(STATE_INIT, pMsg);
    }
}

// 获取白模板
void CRuleLoadThread::getAutoWhiteTemplateMap(vartemp_to_user_map_t & mapAutoWhiteTemplate, fixedtemp_to_user_map_t & mapFixedAutoWhiteTemplate)
{
    pthread_mutex_lock(&m_mutex);
    mapAutoWhiteTemplate = m_mapAutoWhiteTemplate;
    mapFixedAutoWhiteTemplate = m_mapFixedAutoWhiteTemplate;
    pthread_mutex_unlock(&m_mutex);
}

// 获取黑模板
void CRuleLoadThread::getAutoBlackTemplateMap(vartemp_to_user_map_t & mapAutoBlackTemplate, fixedtemp_to_user_map_t & mapFixedAutoBlackTemplate)
{
    pthread_mutex_lock(&m_mutex);
    mapAutoBlackTemplate = m_mapAutoBlackTemplate;
    mapFixedAutoBlackTemplate = m_mapFixedAutoBlackTemplate;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getLoginChannels(std::set<Int32> & setLoginChannels)
{
    pthread_mutex_lock(&m_mutex);
    setLoginChannels = m_setLoginChannels;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getSendIpIdMap(std::map<UInt32,string>& sendIpId)
{
    pthread_mutex_lock(&m_mutex);
    sendIpId = m_sendIpId;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getNoAuditKeyWordMap(map<string,list<string> >& NoAuditKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    NoAuditKeyWordMap = m_NoAuditKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAuditKeyWordMap(map<UInt32,list<string> >& AuditKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    AuditKeyWordMap = m_AuditKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getIgnoreAuditKeyWordMap(map<string,set<string> >& IgnoreAuditKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    IgnoreAuditKeyWordMap = m_IgnoreAuditKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAuditKgroupRefCategoryMap(map<UInt32,set<UInt32> >& AuditKgroupRefCategoryMap)
{
    pthread_mutex_lock(&m_mutex);
    AuditKgroupRefCategoryMap = m_AuditKgroupRefCategoryMap;
    pthread_mutex_unlock(&m_mutex);
}
void CRuleLoadThread::getAuditClientGroupMap(map<UInt32,UInt32>& AuditClientGroupMap,UInt32& uDefaultGroupId)
{
    pthread_mutex_lock(&m_mutex);
    AuditClientGroupMap = m_AuditClientGroupMap;
    uDefaultGroupId = m_uDefaultGroupId;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAuditCGroupRefClientMap(map<string,UInt32>& AuditCGroupRefClientMap)
{
    pthread_mutex_lock(&m_mutex);
    AuditCGroupRefClientMap = m_AuditCGroupRefClientMap;
    pthread_mutex_unlock(&m_mutex);
}


/*****************************get db keyword start*******************************/
void CRuleLoadThread::getSysKeywordMap(map<UInt32,list<string> >& KeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    KeyWordMap = m_sysKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}
void CRuleLoadThread::getSysKgroupRefCategoryMap(map<UInt32,set<UInt32> >& kGrpRefCategoryMap)
{
    pthread_mutex_lock(&m_mutex);
    kGrpRefCategoryMap = m_syskeywordGrpRefCategoryMap;
    pthread_mutex_unlock(&m_mutex);
}
void CRuleLoadThread::getSysClientGroupMap(map<UInt32,UInt32>& clientGroupMap,UInt32& uDefaultGroupId)
{
    pthread_mutex_lock(&m_mutex);
    clientGroupMap = m_sysclientGrpRefKeywordGrpMap;
    uDefaultGroupId = m_uSysDefaultGroupId;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getSysCGroupRefClientMap(map<string,UInt32>& cGroupRefClientMap)
{
    pthread_mutex_lock(&m_mutex);
    cGroupRefClientMap = m_sysClientGrpRefClientMap;
    pthread_mutex_unlock(&m_mutex);
}
/*****************************get db keyword end*******************************/

void CRuleLoadThread::getOverRateKeyWordMap(map<string,list<string> >& OverRateKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    OverRateKeyWordMap = m_OverRateKeyWordMap;
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

void CRuleLoadThread::getChannlelMap(std::map<int , Channel>& channelmap)
{
    pthread_mutex_lock(&m_mutex);
    channelmap = m_ChannelMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannlelUnusualMap(std::map<int , Channel>& channelmap)
{
    pthread_mutex_lock(&m_mutex);
    channelmap = m_ChannelUnusualMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getUserGwMap(std::map<std::string,UserGw>& userGwMap)
{
    pthread_mutex_lock(&m_mutex);
    userGwMap = m_UserGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelGroupMap(std::map<UInt32,ChannelGroup>& ChannelGroupMap)
{
    pthread_mutex_lock(&m_mutex);
    ChannelGroupMap = m_ChannelGroupMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackListMap)
{
    pthread_mutex_lock(&m_mutex);
    ChannelBlackListMap = m_ChannelBlackListMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelKeyWordMap(map<UInt32,list<string> >& ChannelKeyWordMap)
{
    pthread_mutex_lock(&m_mutex);
    ChannelKeyWordMap = m_ChannelKeyWordMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelWhiteListMap(map<UInt32, UInt64Set>& ChannelWhiteListMap)
{
    pthread_mutex_lock(&m_mutex);
    ChannelWhiteListMap = m_ChannelWhiteListMap;
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

void CRuleLoadThread::getSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap)
{
    pthread_mutex_lock(&m_mutex);
    signextnoGwMap = m_SignextnoGwMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelExtendPortTableMap(std::map< std::string,std::string>& channelExtendPortTable)
{
    pthread_mutex_lock(&m_mutex);
    channelExtendPortTable = m_ChannelExtendPortTable;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getAgentInfo(map<UInt64, AgentInfo>& agentInfoMap)
{
    pthread_mutex_lock(&m_mutex);
    agentInfoMap = m_AgentInfoMap;
    pthread_mutex_unlock(&m_mutex);
}

/*???????????????????*/
void CRuleLoadThread::getAgentAccount ( map<UInt64, AgentAccount>& agentAcctMap )
{
    pthread_mutex_lock(&m_mutex);
    agentAcctMap = m_AgentAcctMap;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getComponentSwitch(UInt32 uComponentId)
{
    char strComponentId[16] = {0};
    snprintf(strComponentId, sizeof(strComponentId), "%d01", uComponentId);
    pthread_mutex_lock(&m_mutex);
    map<string, ComponentConfig>::iterator itor = m_componentConfigInfoMap.find(string(strComponentId));
    if(itor != m_componentConfigInfoMap.end())
    {
        pthread_mutex_unlock(&m_mutex);
        return itor->second.m_uComponentSwitch;
    }
    else
    {
        pthread_mutex_unlock(&m_mutex);
        return false;//?????
    }
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


void CRuleLoadThread::getAllKeyWordTempltRule(std::map<string ,TempRule> &KeyWordtempRuleMap)
{
    pthread_mutex_lock(&m_mutex);
    KeyWordtempRuleMap = m_KeyWordTempRuleMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getPhoneAreaMap(std::map<std::string, PhoneSection>& phoneAreaMap)
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

void CRuleLoadThread::getComponentConfig(map<string, ComponentConfig>& componentConfigInfoMap)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfoMap = m_componentConfigInfoMap;
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
void CRuleLoadThread::getSystemErrorCode(map<string,string>& systemErrorCode)
{
    pthread_mutex_lock(&m_mutex);
    systemErrorCode = m_mapSystemErrorCode;
    pthread_mutex_unlock(&m_mutex);
}
void CRuleLoadThread::getChannelTemplateMap(std::map<std::string, std::string>& channelTemplateMap)
{
    pthread_mutex_lock(&m_mutex);
    channelTemplateMap = m_mapChannelTemplate;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelSegmentMap(stl_map_str_ChannelSegmentList& channelSegmentMap)
{
    pthread_mutex_lock(&m_mutex);
    channelSegmentMap = m_mapChannelSegment;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelWhiteKeywordListMap(ChannelWhiteKeywordMap& channelWhiteKeywordlistmap)
{
    pthread_mutex_lock(&m_mutex);
    channelWhiteKeywordlistmap = m_mapChannelWhiteKeyword;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getClientSegmentMap(std::map<std::string, StrList>& clientSegmentMap,map<string,set<std::string> >& clientidSegmentMap)
{
    pthread_mutex_lock(&m_mutex);
    clientSegmentMap = m_mapClientSegment;
    clientidSegmentMap = m_mapClientidSegment;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelSegCodeMap(std::map<UInt32, stl_list_int32>& channelSegCodeMap)
{
    pthread_mutex_lock(&m_mutex);
    channelSegCodeMap = m_mapChannelSegCode;
    pthread_mutex_unlock(&m_mutex);
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>> Â¡Â¤????????? BEGIN <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
void CRuleLoadThread::getChannelPoolPolicies(channel_pool_policies_t& channelPoolPolicies)
{
    pthread_mutex_lock(&m_mutex);
    channelPoolPolicies = m_channelPoolPolicies;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelsRealtimeWeightInfo(channels_realtime_weightinfo_t& channelsRealtimeWeightInfo)
{
    pthread_mutex_lock(&m_mutex);
    channelsRealtimeWeightInfo = m_channelsRealtimeWeightInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelAttributeWeightConfig( channel_attribute_weight_config_t& chnlAttrWghtConf)
{
    pthread_mutex_lock(&m_mutex);
    chnlAttrWghtConf = m_channelAttrWeightConfig;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getClientPriorities( client_priorities_t& clientPriorities)
{
    pthread_mutex_lock(&m_mutex);
    clientPriorities = m_clientPriorities;
    pthread_mutex_unlock(&m_mutex);
}
/* >>>>>>>>>>>>>>>>>>>>>>>>>> Â¡Â¤????????? END <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
void CRuleLoadThread::getChannelPropertyLog(map<UInt64, list<channelPropertyLog_ptr_t> > &channelPropertyLogMap)
{
    pthread_mutex_lock(&m_mutex);
    channelPropertyLogMap = m_channelPropertyLogMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getUserPriceLog(map<string, list<userPriceLog_ptr_t> > &userPriceLogMap)
{
    pthread_mutex_lock(&m_mutex);
    userPriceLogMap = m_userPriceLogMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getUserPropertyLog(map<string, list<userPropertyLog_ptr_t> > &userPropertyLogMap)
{
    pthread_mutex_lock(&m_mutex);
    userPropertyLogMap = m_userPropertyLogMap;
    pthread_mutex_unlock(&m_mutex);
}


void CRuleLoadThread::getBlacklistGrp(map<string,UInt32> &blacklistGrpMap)
{
    pthread_mutex_lock(&m_mutex);
    blacklistGrpMap = m_uBlacklistGrpMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getBlacklistCatAll(map<UInt32,UInt64> &blacklistCatMap)
{
    pthread_mutex_lock(&m_mutex);
    blacklistCatMap = m_uBgroupRefCategoryMap;
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
    getDBNoAuditKeyWordMap(m_NoAuditKeyWordMap);
    getDBIgnoreAuditKeyWordMap(m_IgnoreAuditKeyWordMap);
    getDBAuditKeyWordMap(m_AuditKeyWordMap);
    getDBAuditKgroupRefCategoryMap(m_AuditKgroupRefCategoryMap);
    getDBAuditClientGroupMap(m_AuditClientGroupMap,m_uDefaultGroupId);
    getDBAuditCGroupRefClientMap(m_AuditCGroupRefClientMap);
    getDBOverRateKeyWordMap(m_OverRateKeyWordMap);
    getDBChannlelMap(m_ChannelMap);
    getDBChannlelUnusualMap(m_ChannelUnusualMap);
    getDBUserGwMap(m_UserGwMap);

    /**************************/
    getDBKeyWordMap(m_sysKeyWordMap, "t_sms_keyword_list", m_iSysKeywordCovRegular);
    getDBCGroupRefClientMap(m_sysClientGrpRefClientMap, "t_sms_sys_cgroup_ref_client");
    getDBClientGroupMap(m_sysclientGrpRefKeywordGrpMap,m_uSysDefaultGroupId, "t_sms_sys_client_group", "t_sms_sys_keyword_group");
    getDBKgroupRefCategoryMap(m_syskeywordGrpRefCategoryMap, "t_sms_sys_kgroup_ref_category", "t_sms_sys_keyword_category");
    /***************************/

    getDBSignExtnoGwMap(m_SignextnoGwMap);
    getDBSmppPriceMap(m_PriceMap);
    getDBSmppSalePriceMap(m_SalePriceMap);
    getDBAllTempltRule(m_TempRuleMap,m_KeyWordTempRuleMap);
    getDBChannelOverrate(m_ChannelOverrateMap);

    getDBPhoneAreaMap(m_PhoneAreaMap);
    getDBSendIpIdMap(m_sendIpId);
    getDBChannelGroupMap(m_ChannelGroupMap);
    //getDBChannelBlackListMap(m_ChannelBlackListMap);
    getDBChannelKeyWordMap(m_ChannelKeyWordMap);
    getDBChannelWhiteListMap(m_ChannelWhiteListMap);
    getDBOperaterSegmentMap(m_OperaterSegmentMap);
    getDBOverRateWhiteList(m_overRateWhiteList);
    getDBChannelExtendPortTableMap(m_ChannelExtendPortTable);
    getDBAgentInfo(m_AgentInfoMap);

    getDBSystemErrorCode(m_mapSystemErrorCode);
    ////smsp5.0
    getDBMiddleWareConfig(m_middleWareConfigInfo);
    getDBComponentConfig(m_componentConfigInfoMap);
    getDBListenPort(m_listenPortInfo);
    getDBMqConfig(m_mqConfigInfo);
    getDBComponentRefMq(m_componentRefMqInfo);

    getDBChannelTemplateMap(m_mapChannelTemplate);

    getDBChannelSegmentMap(m_mapChannelSegment);
    getDBChannelWhiteKeywordMap(m_mapChannelWhiteKeyword);
    getDBClientSegmentMap(m_mapClientSegment,m_mapClientidSegment);
    getDBChannelSegCodeMap(m_mapChannelSegCode);
    getDBAgentAccount(m_AgentAcctMap);

    getDBAutoTemplateMap(SMS_TEMPLATE_WHITE, m_mapAutoWhiteTemplate, m_mapFixedAutoWhiteTemplate);
    getDBAutoTemplateMap(SMS_TEMPLATE_BLACK, m_mapAutoBlackTemplate, m_mapFixedAutoBlackTemplate);

    getDBLoginChannels(m_setLoginChannels);

    getDBChannelRealtimeWeightInfo(m_channelsRealtimeWeightInfo);

    getDBChannelAttributeWeightConfig(m_channelAttrWeightConfig);

    getDBChannelPoolPolicies(m_channelPoolPolicies);

    getDBClientPrioritiesConfig(m_clientPriorities);

    getDBChannelPropertyLogMap(m_channelPropertyLogMap);
    getDBUserPriceLogMap(m_userPriceLogMap);
    getDBUserPropertyLogMap(m_userPropertyLogMap);

    getDBbgroupRefCategoryMap(m_uBgroupRefCategoryMap);
    getDBBlacklistGroupMap(m_uBlacklistGrpMap);

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
        LogError("sql execute err!");
        return false;
    }
    return false;
}

/*****************************channel overrate start***********************************************/

void CRuleLoadThread::getChannelOverrateRule(std::map<string ,TempRule> &channelOverrate)
{
    pthread_mutex_lock(&m_mutex);
    channelOverrate = m_ChannelOverrateMap;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getDBChannelOverrate(std::map<string ,TempRule>& channelOverrate)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_channel_overrate where state = 1");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            TempRule tempRule;

            tempRule.m_strSmsType = rs[i]["smstype"];
            tempRule.m_strCid = rs[i]["cid"];
            tempRule.m_strSign = rs[i]["sign"];
            tempRule.m_overRate_num_s= atoi(rs[i]["overRate_num_s"].data());
            tempRule.m_overRate_time_s = atoi(rs[i]["overRate_time_s"].data());
            tempRule.m_overRate_num_m = atoi(rs[i]["overRate_num_m"].data());
            tempRule.m_overRate_time_m = atoi(rs[i]["overRate_time_m"].data());
            tempRule.m_overRate_num_h = atoi(rs[i]["overRate_num_d"].data());
            tempRule.m_overRate_time_h= atoi(rs[i]["overRate_time_d"].data()) * 24;
            tempRule.m_state = atoi(rs[i]["state"].data());

            if(!tempRule.m_strCid.empty())//
            {
                channelOverrate[tempRule.m_strCid + "_"  + tempRule.m_strSmsType + "_" + tempRule.m_strSign] = tempRule;
            }
        }

        map<string ,TempRule>::iterator iter = channelOverrate.begin();
        for (; iter != channelOverrate.end(); iter++)
        {
            LogDebug("first[%s],second(%d,%d/%d,%d/%d,%d/%d)", iter->first.data(), iter->second.m_state, iter->second.m_overRate_num_s
                , iter->second.m_overRate_time_s, iter->second.m_overRate_num_m, iter->second.m_overRate_time_m, iter->second.m_overRate_num_h, iter->second.m_overRate_time_h);
        }
        return true;

    }
    else
    {
        LogError("sql execute err!");
        return false;
    }


}

/*****************************channel overrate end***********************************************/

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

bool CRuleLoadThread::getDBComponentConfig(map<string, ComponentConfig>& componentConfigInfoMap)
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
            ComponentConfig componentConfigInfo;
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
            componentConfigInfoMap[rs[i]["component_id"].append(rs[i]["component_type"])] = componentConfigInfo;
            componentConfigInfoMap[rs[i]["component_id"]] = componentConfigInfo;
        }
    }
    else
    {
        LogError("sql table t_sms_component_configure execute err!");
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
    snprintf(sql, 500,"select * from t_sms_listen_port_configure where component_type ='02'");
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
        LogError("sql table t_sms_listen_port_configure execute err!");
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
            mqInfo.m_uMqType = atoi(rs[i]["mqtype"].data());

            mqConfigInfo.insert(make_pair(mqInfo.m_uMqId,mqInfo));
        }
    }
    else
    {
        LogError("sql table t_sms_mq_configure execute err!");
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

            char temp[250] = {0};
            // 在同message_type下有多队列时要区分，key里多加一个uMqId
            if( (refInfo.m_uMode == CONSUMER_MODE
                    && (refInfo.m_strMessageType == MESSAGE_TYPE_YD_HY  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_YD_YX  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_LD_HY  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_LD_YX  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_DX_HY  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_DX_YX  ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_HY     ||
                        refInfo.m_strMessageType == MESSAGE_TYPE_YX ))
                    || (refInfo.m_uMode == PRODUCT_MODE && refInfo.m_strMessageType == MESSAGE_AUDIT_CONTENT_REQ)
                    || (refInfo.m_uMode == PRODUCT_MODE && refInfo.m_strMessageType == MESSAGE_TYPE_DB))
            {
                snprintf(temp,250,"%lu_%s_%u_%lu",refInfo.m_uComponentId,refInfo.m_strMessageType.data(),refInfo.m_uMode, refInfo.m_uMqId);
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
        LogError("sql table t_sms_component_ref_mq execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBLoginChannels(std::set<Int32> & setLoginChannels)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string sql = "SELECT * FROM t_sms_channel_login_status WHERE login_status=1;";

//  LogDebug("getDBLoginChannels, sql[%s]", sql.data());

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql.data()))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            setLoginChannels.insert(atoi(rs[i]["cid"].data()));
        }

        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql.data());
        return false;
    }
}

bool CRuleLoadThread::getDBChannlelMap(std::map<int , Channel>& channelmap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string sql = "SELECT * FROM t_sms_channel WHERE state='1';";

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
            chan.balance = atof(rs[i]["balance"].data());
            chan.expenditure = atof(rs[i]["expenditure"].data());
            chan.shownum = atoi(rs[i]["shownum"].data());
            chan._channelRule = rs[i]["showsign"].data();
            chan.spID = rs[i]["spid"].data();
            chan.nodeID = atoi(rs[i]["node"].data());
            chan.m_uSpeed= atoi(rs[i]["speed"].data());
            chan.m_uAccessSpeed= atoi(rs[i]["access_speed"].data());
            chan.m_uExtendSize = atoi(rs[i]["extendsize"].data());
            chan.m_uNeedPrefix = atoi(rs[i]["needprefix"].data());
            chan.m_strRemark = rs[i]["remark"].data();
            string strTimeArea = rs[i]["sendtimearea"].data();
            chan.m_uChannelIdentify= atoi(rs[i]["identify"].data());
            chan.m_uMqId= atoi(rs[i]["mq_id"].data());
            chan.m_strOauthUrl = rs[i]["oauth_url"];
            chan.m_strOauthData = rs[i]["oauth_post_data"];
            chan.m_uContentLength = atoi(rs[i]["contentlen"].c_str());
            chan.m_uChannelMsgQueueMaxSize = atoi(rs[i]["maxqueuesize"].c_str());
            chan.m_uSegcodeType = atoi(rs[i]["segcode_type"].c_str());
            chan.m_uExValue = atoi(rs[i]["exvalue"].c_str());

            if(strTimeArea.empty())
            {
                chan.m_strBeginTime ="";
                chan.m_strEndTime ="";
            }
            else
            {
                //check strTimeArea format
                std::string::size_type pos = strTimeArea.find_last_of("|");
                if(string::npos != pos)
                {
                    chan.m_strBeginTime = strTimeArea.substr(0, pos);
                    chan.m_strEndTime = strTimeArea.substr(pos+1);

                    if(TimeRegCheck(chan.m_strBeginTime) && TimeRegCheck(chan.m_strEndTime))
                    {
                        //format check suc
                        LogDebug("channel[%s], BeginTime[%s], EndTime[%s]", chan.channelname.data(), chan.m_strBeginTime.data(), chan.m_strEndTime.data());
                    }
                    else
                    {
                        //format error
                        chan.m_strBeginTime ="";
                        chan.m_strEndTime ="";
                        LogDebug("TimeArea format error. channel[%s], timeArea[%s]", chan.channelname.data(), strTimeArea.data());
                    }
                }
                else
                {
                    //find no '|'
                    chan.m_strBeginTime ="";
                    chan.m_strEndTime ="";
                    LogDebug("strTimeArea format error. channel[%s], timeArea[%s]", chan.channelname.data(), strTimeArea.data());
                }
            }

            if(0 != chan.channelID)
            {
                channelmap[chan.channelID] = chan;
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

bool CRuleLoadThread::getDBChannlelUnusualMap(std::map<int , Channel>& channelmap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string sql = "SELECT * FROM t_sms_channel WHERE state='0' and cid > 0;";

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql.data()))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            Channel chan;
            chan._accessID = rs[i]["accessid"];
            chan.channelID = atoi(rs[i]["cid"].data());
            chan.m_uSpeed= atoi(rs[i]["speed"].data());
            chan.m_uAccessSpeed= atoi(rs[i]["access_speed"].data());
            chan.m_uMqId = atoi(rs[i]["mq_id"].c_str());

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
            userGw.m_uFreeKeyword = atoi(rs[i]["free_keyword"].data());
            userGw.m_uFreeChannelKeyword = atoi(rs[i]["free_channel_keyword"].data());
            userGw.m_strFailedReSendChannelID = rs[i]["failed_resend_channel"];
            userGwMap[rs[i]["userid"] + "_" + rs[i]["smstype"]] = userGw;       //KEY: userid_smstype
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
        LogError("sql execute err!");
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
            ////LogNotice("=======cost===========fee[%s]",fee.data());

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

            ////LogNotice("=======sale===========fee[%s]",fee.data());
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

bool CRuleLoadThread::getDBAllTempltRule(std::map<string ,TempRule>& tempRuleMap,std::map<string ,TempRule>& KeyWordtempRuleMap)
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
            tempRule.m_strSign = rs[i]["sign"];
            tempRule.m_overRate_num_s= atoi(rs[i]["overRate_num_s"].data());
            tempRule.m_overRate_time_s = atoi(rs[i]["overRate_time_s"].data());
            tempRule.m_overRate_num_m = atoi(rs[i]["overRate_num_m"].data());
            tempRule.m_overRate_time_m = atoi(rs[i]["overRate_time_m"].data());
            tempRule.m_overRate_num_h = atoi(rs[i]["overRate_num_h"].data());
            tempRule.m_overRate_time_h= atoi(rs[i]["overRate_time_h"].data());
            tempRule.m_state = atoi(rs[i]["state"].data());

            if(!tempRule.m_strUserID.empty())// overrate_mode'Â³Â¬Ã†ÂµÃÂ³Â¼Ã†Â·Â½ÃŠÂ½:0Â°Â´Â¶ÃŒÃÃ…Ã€Ã ÃÃÃÂ³Â¼Ã†Â£Â¬1Â°Â´Â¹Ã˜Â¼Ã¼Ã—Ã–ÃÂ³Â¼Ã†'
            {
                if(0 == atoi(rs[i]["overrate_mode"].data()))
                {
                    tempRuleMap[tempRule.m_strUserID + "_" + tempRule.m_strSmsType] = tempRule;     //KEY:userID_templetID
                }
                else if(1 == atoi(rs[i]["overrate_mode"].data()))
                {
                    KeyWordtempRuleMap[tempRule.m_strUserID + "_" + tempRule.m_strSign] = tempRule; ///key:user id
                }
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

bool CRuleLoadThread::getDBPhoneAreaMap(std::map<std::string, PhoneSection>& phoneAreaMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500,"select * from t_sms_phone_section");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (long i = 0; i < rs.GetRecordCount(); ++i)
        {
            PhoneSection section;

            section.phone_section = rs[i]["phone_section"].data();
            section.area_id = atoi(rs[i]["area_id"].data());
            section.code = atoi(rs[i]["code"].data());
            phoneAreaMap[section.phone_section] = section;
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
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
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBChannelGroupMap(map<UInt32,ChannelGroup>& ChannelGroupMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_channelgroup where status = '1'");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ChannelGroup chlGroup;

            chlGroup.m_strChannelGroupName = rs[i]["channelgroupname"];
            chlGroup.m_uChannelGroupID = atoi(rs[i]["channelgroupid"].data());
            chlGroup.m_uOperater = atoi(rs[i]["operater"].data());
            ChannelGroupMap[atoi(rs[i]["channelgroupid"].data())] = chlGroup;
        }

        m_ChannelGroupMap = ChannelGroupMap;
        getDBChannelRefChannelGroupMap();
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }
    return false;
}
//通道属性变更记录表
bool CRuleLoadThread::getDBChannelPropertyLogMap(map<UInt64, list<channelPropertyLog_ptr_t> >& ChannelPropertyLogMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_channel_property_log where property = 'cost_price' order by effect_date desc");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt64 channelID = (UInt64)atoi(rs[i]["channel_id"].data());
            map<UInt64, list<channelPropertyLog_ptr_t> >::iterator iter = ChannelPropertyLogMap.find(channelID);

            channelPropertyLog_ptr_t pChannelPropertyTemp(new ChannelPropertyLog);
            if (!pChannelPropertyTemp)
            {
                LogError("Null pChannelPropertyTemp when select t_sms_channel_property_log");
                continue;
            }

            pChannelPropertyTemp->m_strProperty = rs[i]["property"];
            pChannelPropertyTemp->m_dChannelCostPrice = strtod(rs[i]["value"].data(), NULL);
            pChannelPropertyTemp->m_strEffectDate = rs[i]["effect_date"];
            if (iter == ChannelPropertyLogMap.end())
            {
                list<channelPropertyLog_ptr_t> listTemp;
                listTemp.push_back(pChannelPropertyTemp);
                ChannelPropertyLogMap[channelID] = listTemp;
            }
            else
            {
                iter->second.push_back(pChannelPropertyTemp);
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

//后付费原表t_sms_user_price_log
bool CRuleLoadThread::getDBUserPriceLogMap(map<string, list<userPriceLog_ptr_t> >& userPriceLogMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_user_price_log order by effect_date desc");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            userPriceLog_ptr_t puserPriceLogMap(new UserPriceLog);
            if (!puserPriceLogMap)
            {
                LogError("Null puserPriceLogMap when select t_sms_user_price_log");
                continue;
            }

            puserPriceLogMap->m_strClientID = rs[i]["clientid"];
            puserPriceLogMap->m_strSmsType = rs[i]["smstype"];
            puserPriceLogMap->m_dUserPrice = strtod(rs[i]["user_price"].data(), NULL);
            puserPriceLogMap->m_strEffectDate = rs[i]["effect_date"];
            string strKey = puserPriceLogMap->m_strClientID + "_" + puserPriceLogMap->m_strSmsType;
            map<string, list<userPriceLog_ptr_t> >::iterator iter = userPriceLogMap.find(strKey);

            if (iter == userPriceLogMap.end())
            {
                list<userPriceLog_ptr_t> listTemp;
                listTemp.push_back(puserPriceLogMap);
                userPriceLogMap[strKey] = listTemp;
            }
            else
            {
                iter->second.push_back(puserPriceLogMap);
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

void CRuleLoadThread::setUserPropertyLogMap(userPropertyLog_ptr_t userPropertyLogMap
    ,string strProperty, string strSet)
{
        if (strProperty == "yd_sms_price")
        {
            userPropertyLogMap->m_dydPrice = strtod(strSet.data(), NULL);
        }
        if (strProperty == "lt_sms_price")
        {
            userPropertyLogMap->m_dltPrice = strtod(strSet.data(), NULL);
        }
        if (strProperty == "dx_sms_price")
        {
            userPropertyLogMap->m_ddxPrice = strtod(strSet.data(), NULL);
        }
        if (strProperty == "gj_sms_discount")
        {
            userPropertyLogMap->m_dfjDiscount = strtod(strSet.data(), NULL);
        }
}

//用户属性变更记录,扣主账户后付费
bool CRuleLoadThread::getDBUserPropertyLogMap(map<string, list<userPropertyLog_ptr_t> >& userPropertyLogMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"SELECT * FROM t_sms_user_property_log ORDER BY effect_date desc;");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            map<string, list<userPropertyLog_ptr_t> >::iterator iter = userPropertyLogMap.find(rs[i]["clientid"]);

            if (iter == userPropertyLogMap.end())
            {
                list<userPropertyLog_ptr_t> listTemp;
                userPropertyLog_ptr_t puserPropertyLogMap(new UserPropertyLog);
                if (!puserPropertyLogMap)
                {
                    LogError("Null puserPropertyLogMap when select t_sms_user_property_log");
                    continue;
                }

                puserPropertyLogMap->m_strClientID = rs[i]["clientid"];
                setUserPropertyLogMap(puserPropertyLogMap, rs[i]["property"], rs[i]["value"]);

                puserPropertyLogMap->m_strEffectDate = rs[i]["effect_date"];
                listTemp.push_back(puserPropertyLogMap);
                userPropertyLogMap[puserPropertyLogMap->m_strClientID] = listTemp;
            }
            else
            {
                list<userPropertyLog_ptr_t>::iterator itlist = iter->second.begin();
                for (; itlist != iter->second.end(); itlist++)
                {
                    if (rs[i]["effect_date"] == (**itlist).m_strEffectDate)
                    {
                        setUserPropertyLogMap(*itlist, rs[i]["property"], rs[i]["value"]);
                        break;
                    }
                }
                if (itlist == iter->second.end())
                {
                    userPropertyLog_ptr_t puserPropertyLogMap(new UserPropertyLog);
                    if (!puserPropertyLogMap)
                    {
                        LogError("Null puserPropertyLogMap when select t_sms_user_property_log");
                        continue;
                    }
                    puserPropertyLogMap->m_strClientID = rs[i]["clientid"];
                    setUserPropertyLogMap(puserPropertyLogMap, rs[i]["property"], rs[i]["value"]);
                    puserPropertyLogMap->m_strEffectDate = rs[i]["effect_date"];
                    iter->second.push_back(puserPropertyLogMap);
                }
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

bool CRuleLoadThread::getDBChannelRefChannelGroupMap()
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_channel_ref_channelgroup where status = 1");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            //???????insert???channelGroupMap?D????
            UInt32 ChlGrpID = atoi(rs[i]["channelgroupid"].data());
            UInt32 uSort = atoi(rs[i]["sort"].data());
            int iWeight = atoi(rs[i]["weight"].data());
            if(ChlGrpID != 0 && uSort >=0 && uSort <= CHANNEL_GROUP_MAX_SIZE)
            {
                map<UInt32,ChannelGroup>::iterator it = m_ChannelGroupMap.find(ChlGrpID);
                if(it != m_ChannelGroupMap.end())
                {
                    UInt32 uChannelId = atoi(rs[i]["channelid"].data());
                    it->second.m_szChannelIDs[uSort] =  uChannelId;
                    it->second.m_szChannelsWeight[uSort] = iWeight;
                    printf("ChannelGroupWeight-Init: GID[%u] CID[%u] weight[%d]\n", ChlGrpID, uChannelId, iWeight);
                }
                else
                {
                    LogWarn("t_sms_channel_ref_channelgroup, can not find channelgroupid[%d] from m_ChannelGroupMap", ChlGrpID);
                }

            }
            else
            {
                LogWarn("t_sms_channel_ref_channelgroup param error. channelgroupid[%d], uSort[%d]", ChlGrpID, uSort);
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



bool CRuleLoadThread::getDBChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackListMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_channel_black_list where status = '1' ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        //get db info into map string
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {

            UInt32 uChannelID = atoi(rs[i]["cid"].data());
            string strPhone = rs[i]["phone"];

            map<UInt32, UInt64Set>::iterator it = ChannelBlackListMap.find(uChannelID);
            if(it == ChannelBlackListMap.end())
            {
                //find no channelid, then new it
                set<UInt64> u64Set;
                u64Set.insert(atol(strPhone.data()));       //new a set, and add phone

                ChannelBlackListMap[uChannelID] = u64Set;   //map add set
            }
            else
            {
                it->second.insert(atol(strPhone.data()));   //set add phone
                //blackListStrList[uChannelID] = it->second;        //map add set, no need?
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

bool CRuleLoadThread::getDBAutoTemplateMap(SMSTemplateBusType tempBusType, vartemp_to_user_map_t & mapTemplate, fixedtemp_to_user_map_t & mapFixedTemplate)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string strTblName;
    switch(tempBusType) {
        case SMS_TEMPLATE_WHITE:
            LogNotice("Init auto white template.");
            strTblName = "t_sms_auto_template";
            break;
        case SMS_TEMPLATE_BLACK:
            LogNotice("Init auto black template.");
            strTblName = "t_sms_auto_black_template";
            break;
        default:
            LogError("Unknown template-bus-type: [%d]. Only support white[0]/black[1]", (int)tempBusType);
            return false;
    }

    RecordSet rs(m_DBConn);
    char sql[128] = { 0 };
    // ÃŠÂ¹Ã“ÃƒcontentÃ„Ã¦ÃÃ²Â£Â¬Â½Â«ÃŠÂµÃÃ–Ã—Ã®Â³Â¤Ã†Â¥Ã…Ã¤
    snprintf(sql, 128, "SELECT * FROM %s WHERE state = 1 ORDER BY LENGTH(content) DESC", strTblName.c_str());

    if (-1 != rs.ExecuteSQL(sql))
    {
        vartemp_to_user_map_t mapTemplateTmp;
        fixedtemp_to_user_map_t mapFixedTemplateTmp;

        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string  Key;
            string  strClientID     = rs[i]["client_id"];
            string  strSmsType      = rs[i]["sms_type"];
            string  content         = rs[i]["content"];
            string  sign            = rs[i]["sign"];
            int     iTemplateType   = atoi(rs[i]["template_type"].data());
            int     iTemplateLevel  = atoi(rs[i]["template_level"].data());
            int     iTemplateId     = atoi(rs[i]["template_id"].data());

            Key = strClientID + strSmsType;

            Comm::ConvertPunctuations(content);
            Comm::ConvertPunctuations(sign);
            Comm::Sbc2Dbc(content);
            Comm::Sbc2Dbc(sign);

            string contentSimple;
            m_pCChineseConvert->ChineseTraditional2Simple(content, contentSimple);
            content = contentSimple;
            Comm::trim(content);

            string signSimple = sign;
            if (!sign.empty())      /* ÃˆÂ«Â¾Ã–Ã„Â£Â°Ã¥Ã”Ã²Ã‡Â©ÃƒÃ»Â¿Ã‰Ã„ÃœÃŽÂªÂ¿Ã• */
            {
                m_pCChineseConvert->ChineseTraditional2Simple(sign, signSimple);
            }
            Key.append(signSimple);   /* Key has three possible types:
                                         1. clientId+smsType+sign  (client template)
                                         2. '*'+smsType+sign  (global template)
                                         3. '*'+smsType (global template, sign is empty) */

            LogDebug("%s Key[%s], iTemplateType[%d], iTemplateLevel[%d], Content[%s]",
                    strTblName.c_str(), Key.data(), iTemplateType, iTemplateLevel, content.data());

            StTemplateInfo tempInfo;
            tempInfo.iTemplateId = iTemplateId;
            tempInfo.iTemplateLevel = iTemplateLevel;
            tempInfo.iTemplateType = iTemplateType;

            if(iTemplateType == AUTO_TEMPLATE_TYPE_FIXED)
            {
                fixedtemp_to_user_map_t::iterator it = mapFixedTemplateTmp.find(Key);

                // Fixed-template stores 'content' directly
                tempInfo.fixedTempContent = content;
                if(it == mapFixedTemplateTmp.end())
                {
                    // 'set' makes fixedTempContent unique.
                    fixedtemp_set_t setFixedTemplate;
                    setFixedTemplate.insert(tempInfo);
                    mapFixedTemplateTmp.insert(make_pair(Key, setFixedTemplate));
                }
                else
                {
                    it->second.insert(tempInfo);
                }
            }
            else if(iTemplateType == AUTO_TEMPLATE_TYPE_VARIABLE)
            {
                // Variable-template needs to seperate 'content' by '{}' first
                Comm::splitExVectorSkipEmptyString(content, AUTO_TEMPLATE_SEPARATOR, tempInfo.varTempElements);
                tempInfo.bHasLastSeperator = false;
                tempInfo.bHasHeaderSeperator = false;
                if (content.size() > 1)
                {
                    if (content[content.size()-2] == '{' && content[content.size()-1] == '}')
                    {
                        tempInfo.bHasLastSeperator = true;
                    }
                    if (content[0] == '{' && content[1] == '}')
                    {
                        tempInfo.bHasHeaderSeperator = true;
                    }
                }

                vartemp_to_user_map_t::iterator it = mapTemplateTmp.find(Key);
                if(it == mapTemplateTmp.end())
                {
                    vartemp_list_t listTemplate;
                    listTemplate.push_back(tempInfo);
                    mapTemplateTmp.insert(make_pair(Key, listTemplate));
                }
                else
                {
                    vartemp_list_t & listTemplate = it->second;
                    listTemplate.push_back(tempInfo);
                }
            }
            else
            {
                LogError("%s template_type is %d", strTblName.c_str(), iTemplateType);
            }
        }

        // Only update cache when everything is ok.
        mapTemplate = mapTemplateTmp;
        mapFixedTemplate = mapFixedTemplateTmp;

        return true;
    }
    else
    {
        LogError("%s sql execute err!", strTblName.c_str());
        return false;
    }
}

bool CRuleLoadThread::getDBChannelKeyWordMap(map<UInt32,list<string> >& ChannelKeyWordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    map<UInt32,string> keywordStrList;

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_channel_keyword_list where status = '1' ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uChannelID = atoi(rs[i]["cid"].data());
            string strkeyWord = rs[i]["keyword"];

            Comm::ToLower( strkeyWord );

            #if 0
            string strkeyWordSimple;
            m_pCChineseConvert->ChineseTraditional2Simple(strkeyWord, strkeyWordSimple);
            strkeyWord = strkeyWordSimple;
            #endif

            map<UInt32, string>::iterator it = keywordStrList.find(uChannelID);
            if(it == keywordStrList.end())
            {
                keywordStrList[uChannelID] = strkeyWord;
            }
            else
            {
                it->second.append(strkeyWord);
            }
        }

        ChannelKeyWordMap.clear();
        for (map<UInt32,string>::iterator iter = keywordStrList.begin(); iter != keywordStrList.end();++iter)
        {
            list<string> listSet;
            Comm::split_Ex_List(iter->second,"|",listSet);
            ChannelKeyWordMap.insert(make_pair(iter->first,listSet));
        }

        return true;
    }
    else
    {
        LogError("t_sms_channel_keyword_list sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBChannelWhiteListMap(map<UInt32, UInt64Set>& ChannelWhiteListMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_channel_white_list where status = '1' ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        //get db info into map string
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uChannelID = atoi(rs[i]["cid"].data());
            string strPhone = rs[i]["phone"];

            map<UInt32, UInt64Set>::iterator it = ChannelWhiteListMap.find(uChannelID);
            if(it == ChannelWhiteListMap.end())
            {
                //find no channelid, then new it
                set<UInt64> u64Set;
                u64Set.insert(atol(strPhone.data()));

                ChannelWhiteListMap[uChannelID] = u64Set;
            }
            else
            {
                it->second.insert(atol(strPhone.data()));
                //whiteListStrList[uChannelID] = it->second;
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
        LogError("sql execute err!");
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
        LogError("sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBNoAuditKeyWordMap(map<string, list<string> >& NoAuditKeyWordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    map<string, string> keywordStrList;

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_user_noaudit_keyword where status = '1' ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strClientID = rs[i]["clientid"];
            string strkeyWord = rs[i]["keyword"];

            Comm::ToLower( strkeyWord );

            if( m_iAuditKeywordCovRegular & 1 )
            {
                string strkeyWordSimple;
                m_pCChineseConvert->ChineseTraditional2Simple(strkeyWord, strkeyWordSimple);
                strkeyWord = strkeyWordSimple;
            }

            map<string,string>::iterator it = keywordStrList.find(strClientID);
            if(it == keywordStrList.end())
            {
                keywordStrList[strClientID] = strkeyWord;
            }
            else
            {
                it->second.append(strkeyWord);
            }
        }

        for (map<string,string>::iterator iter = keywordStrList.begin(); iter != keywordStrList.end();++iter)
        {
            list<string> listSet;
            Comm::split_Ex_List(iter->second,"|",listSet);
            NoAuditKeyWordMap.insert(make_pair(iter->first,listSet));
        }

        return true;
    }
    else
    {
        LogError("t_sms_user_noaudit_keyword sql execute err!");
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
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getKeywordFilterRegular(std::map<std::string, std::string>& sysParamMap)
{
    string strSysPara = "KEYWORD_CONVERT_REGULAR";
    std::map<std::string, std::string>::iterator itr = sysParamMap.find(strSysPara);
    if (itr == sysParamMap.end())
    {
        LogError("Can not find system parameter(%s).", strSysPara.c_str());
        return false;
    }

    if (itr->second.empty())
    {
        LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
        return false;
    }
    std::map<std::string,std::string> mapSet;
    Comm::splitMap(itr->second, "|", ";", mapSet);
    m_iSysKeywordCovRegular = atoi(mapSet["1"].data());
    m_iAuditKeywordCovRegular = atoi(mapSet["2"].data());
    LogDebug("m_iSysKeywordCovRegular[%d], m_iAuditKeywordCovRegular[%d].", m_iSysKeywordCovRegular, m_iAuditKeywordCovRegular);
    return true;

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
        getKeywordFilterRegular(sysParamMap);

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
            //sysNoticeUser.m_Mobile = rs[i]["user_mobile"];
            //sysNoticeUser.m_Email  = rs[i]["user_email"];

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
            smsAccount.m_uClientAscription = atoi(rs[i]["client_ascription"].data());
            smsAccount.m_uGroupsendLimCfg = atoi(rs[i]["groupsendlim_cfg"].data());
            smsAccount.m_uGroupsendLimUserflag = atoi(rs[i]["groupsendlim_userflag"].data());
            smsAccount.m_uGroupsendLimTime = atoi(rs[i]["groupsendlim_time"].data());
            smsAccount.m_uGroupsendLimNum = atoi(rs[i]["groupsendlim_num"].data());

            /*?????????????????????????????????????*/
            smsAccount.m_uBelong_sale = atoi(rs[i]["belong_sale"].data());

            smsAccount.m_iPoolPolicyId = atoi(rs[i]["policy_id"].data());


            if(!smsAccount.m_strAccount.empty())
            {
                string tmpIP;
                std::string::size_type pos;
                trim(smsAccount.m_strIP);
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
                        trim(*it);
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
        LogError("sql execute err!");
        return false;
    }
    return false;
}

bool CRuleLoadThread::getDBSmsSendIDIpList(std::list<SmsSendNode>& smsSendIDipList)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_sendip_id");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            SmsSendNode smsNode;
            smsNode.m_strIp  = rs[i]["sendip"].data();
            smsNode.m_uPort  = atoi(rs[i]["sendport"].data());

            if(!smsNode.m_strIp.empty() && smsNode.m_uPort != 0)
            {
                smsSendIDipList.push_back(smsNode);
            }
            else
            {
                LogWarn("t_sms_sendip_id ip or port is empty");
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

bool CRuleLoadThread::getDBChannelExtendPortTableMap(std::map<std::string,std::string>& channelExtendPortTable)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_channel_extendport");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strChannelId = "";
            string strClientId = "";
            string strExtendPort = "";
            string strKey = "";

            strChannelId = rs[i]["channelid"];
            strExtendPort= rs[i]["extendport"];
            strClientId = rs[i]["clientid"];
            /////key = channelid + & + clientid
            strKey.append(strChannelId);
            strKey.append("&");
            strKey.append(strClientId);

            channelExtendPortTable[strKey] = strExtendPort;
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
            agentInfo.m_iOperation_mode= atoi(rs[i]["operation_mode"].c_str());
            agentInfo.m_iAgentPaytype= atoi(rs[i]["paytype"].c_str());
            LogDebug("agentid[%d],agent_type[%d],paytype[%d] ", agentId, agentInfo.m_iAgent_type, agentInfo.m_iAgentPaytype);
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

bool CRuleLoadThread::getDBChannelTemplateMap(std::map<std::string, std::string>& channelTemplateMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_template_audit where status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        std::string strTemplateId;
        std::string strChannelId;
        std::string strChannelTemplateId;
        std::string strkey;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            strTemplateId = rs[i]["template_id"];
            strChannelId = rs[i]["channelid"];
            strChannelTemplateId = rs[i]["channel_tempid"];

            if (strChannelTemplateId.empty())
            {
                continue;
            }

            strkey.assign(strTemplateId).append("_").append(strChannelId);
            channelTemplateMap[strkey] = strChannelTemplateId;
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
bool CRuleLoadThread::getDBOverRateKeyWordMap(map<string, list<string> >& OverRateKeyWordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[250] = {0};
    snprintf(sql, 250,"select * from t_sms_overrate_keyword");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strClientID = rs[i]["clientid"];
            string strkeyWord = rs[i]["keyword"];

            Comm::ToLower( strkeyWord );
            if(strClientID.empty())
                continue;
            map<string, StrList>::iterator it = OverRateKeyWordMap.find(strClientID);
            if(it == OverRateKeyWordMap.end())
            {
                StrList strList;
                strList.push_back(strkeyWord);
                OverRateKeyWordMap.insert(make_pair(strClientID,strList));
            }
            else
            {
                it->second.push_back(strkeyWord);
            }

        }

        return true;
    }
    else
    {
        LogError("t_sms_overrate_keyword  sql execute err!");
        return false;
    }

}


bool CRuleLoadThread::getDBIgnoreAuditKeyWordMap(map<string, set<string> >& IgnoreAuditKeyWordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[100] = { 0 };
    snprintf(sql, 100,"select * from t_sms_user_audit_keyword_ignore where status = '0' ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            string strClientID = rs[i]["clientid"];
            string strKeyWord = rs[i]["ignore_keyword"];
            string strSmsType = rs[i]["smstype"];
            if(strClientID.empty() || strKeyWord.empty())
            {
                LogWarn("clientid[%s] or keyword[%s] is empty!!",strClientID.c_str(),strKeyWord.c_str());
                continue;
            }
            string strAuditKeyword = strKeyWord;
            if (m_iAuditKeywordCovRegular & 0x01)
            {
                m_pCChineseConvert->ChineseTraditional2Simple(strKeyWord, strAuditKeyword);
            }
            filter::ContentFilter sysKeywordFilter;
            string dbAuditKeyword;
            sysKeywordFilter.contentFilter(strAuditKeyword, dbAuditKeyword, m_iAuditKeywordCovRegular);
            string strkey = strClientID + "_" + strSmsType;
            map<string, set<string> >::iterator it = IgnoreAuditKeyWordMap.find(strkey);
            if(it == IgnoreAuditKeyWordMap.end())
            {
                set<string> strSet;
                strSet.insert(dbAuditKeyword);
                IgnoreAuditKeyWordMap[strkey] = strSet;
            }
            else
            {
                it->second.insert(dbAuditKeyword);
            }
        }

        return true;
    }
    else
    {
        LogError("t_sms_user_audit_keyword_ignore sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBAuditKeyWordMap(map<UInt32, list<string> >& AuditKeyWordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[250] = {0};
    snprintf(sql, 250,"select * from t_sms_audit_keyword_list ");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCategoryID = atoi(rs[i]["category_id"].c_str());//Ã€Ã Â±Ã°ID
            string strKeyWord = rs[i]["keyword"];
            string strAuditKeyword = strKeyWord;
            if (m_iAuditKeywordCovRegular & 0x01)
            {
                m_pCChineseConvert->ChineseTraditional2Simple(strKeyWord, strAuditKeyword);
            }
            filter::ContentFilter sysKeywordFilter;
            string dbAuditKeywordList;
            sysKeywordFilter.contentFilter(strAuditKeyword, dbAuditKeywordList, m_iAuditKeywordCovRegular);

            map<UInt32, list<string> >::iterator it = AuditKeyWordMap.find(uCategoryID);
            if(it == AuditKeyWordMap.end())
            {
                list<string> strList;
                strList.push_back(dbAuditKeywordList);
                AuditKeyWordMap.insert(make_pair(uCategoryID,strList));
            }
            else
            {
                it->second.push_back(dbAuditKeywordList);
            }
        }

        return true;
    }
    else
    {
        LogError("t_sms_audit_keyword  sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBAuditKgroupRefCategoryMap(map<UInt32,set<UInt32> >& AuditKgroupRefCategoryMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[250] = {0};
    snprintf(sql, 250,"select t1.kgroup_id,t1.category_id from t_sms_audit_kgroup_ref_category t1 "
        "inner join t_sms_audit_keyword_category t2 on t1.category_id = t2.category_id;");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uKgroupID = atoi(rs[i]["kgroup_id"].c_str());//??????????ID
            UInt32 uCategoryID = atoi(rs[i]["category_id"].c_str());//????????????
            map<UInt32,set<UInt32> >::iterator itr = AuditKgroupRefCategoryMap.find(uKgroupID);
            if(itr == AuditKgroupRefCategoryMap.end())
            {
                set<UInt32> CategorySet;
                CategorySet.insert(uCategoryID);
                AuditKgroupRefCategoryMap.insert(make_pair(uKgroupID,CategorySet));
            }
            else
            {
                itr->second.insert(uCategoryID);
            }
        }
        return true;
    }
    else
    {
        LogError("t_sms_audit_cgroup_ref_client  sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBAuditClientGroupMap(map<UInt32,UInt32>& AuditClientGroupMap,UInt32& uDefaultGroupId)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[250] = {0};
    snprintf(sql, 250,"select t1.cgroup_id,t1.kgroup_id,t1.is_default from t_sms_audit_client_group t1 "
        "inner join t_sms_audit_keyword_group t2 on t1.kgroup_id = t2.kgroup_id;");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCgroupID = atoi(rs[i]["cgroup_id"].c_str());//?????????ID
            UInt32 uKgroupID = atoi(rs[i]["kgroup_id"].c_str());//??????????ID
            Int32  IsDefault = atoi(rs[i]["is_default"].c_str());//?????????
            AuditClientGroupMap[uCgroupID] = uKgroupID;
            if(1 == IsDefault)
                uDefaultGroupId = uKgroupID;//???????????
        }
        return true;
    }
    else
    {
        LogError("t_sms_audit_cgroup_ref_client  sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBAuditCGroupRefClientMap(map<string,UInt32>& AuditCGroupRefClientMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[250] = {0};
    snprintf(sql, 250,"select * from t_sms_audit_cgroup_ref_client");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCgroupID = atoi(rs[i]["cgroup_id"].c_str());//?????????ID
            string strClient = rs[i]["clientid"];
            AuditCGroupRefClientMap[strClient] = uCgroupID;
        }
        return true;
    }
    else
    {
        LogError("t_sms_audit_cgroup_ref_client  sql execute err!");
        return false;
    }

}


/**************************keyword classify syskeyword begin****************************************************/
//get keyword list e.g. t_sms_keyword_list
bool CRuleLoadThread::getDBKeyWordMap(map<UInt32, list<string> >& KeyWordMap, const string& tableName, int keywordConvertRegular)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[256] = {0};
    snprintf(sql, sizeof(sql),"select * from %s ", tableName.data());
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCategoryID = atoi(rs[i]["category_id"].c_str());//ç±»åˆ«ID
            string strKeyWord = rs[i]["keyword"];
            string strKeyword = strKeyWord;
            if (keywordConvertRegular & 0x01)
            {
                m_pCChineseConvert->ChineseTraditional2Simple(strKeyWord, strKeyword);
            }
            filter::ContentFilter sysKeywordFilter;
            string dbKeywordList;
            sysKeywordFilter.contentFilter(strKeyword, dbKeywordList, keywordConvertRegular);

            map<UInt32, list<string> >::iterator it = KeyWordMap.find(uCategoryID);
            if(it == KeyWordMap.end())
            {
                list<string> strList;
                strList.push_back(dbKeywordList);
                KeyWordMap.insert(make_pair(uCategoryID,strList));
            }
            else
            {
                it->second.push_back(dbKeywordList);
            }
        }

        return true;
    }
    else
    {
        LogError("sql[%s] excute err!", sql);
        return false;
    }

}

//keyword group ref keyword catefory
bool CRuleLoadThread::getDBKgroupRefCategoryMap(map<UInt32,set<UInt32> >& kgroupRefCategoryMap
                                                           ,const string& kgrpRefCatTable
                                                           , const string& keywordCatTable)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),"select t1.kgroup_id,t1.category_id from %s t1 "
        "inner join %s t2 on t1.category_id = t2.category_id;", kgrpRefCatTable.data(), keywordCatTable.data());
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uKgroupID = atoi(rs[i]["kgroup_id"].c_str());
            UInt32 uCategoryID = atoi(rs[i]["category_id"].c_str());
            map<UInt32,set<UInt32> >::iterator itr = kgroupRefCategoryMap.find(uKgroupID);
            if(itr == kgroupRefCategoryMap.end())
            {
                set<UInt32> CategorySet;
                CategorySet.insert(uCategoryID);
                kgroupRefCategoryMap.insert(make_pair(uKgroupID,CategorySet));
            }
            else
            {
                itr->second.insert(uCategoryID);
            }
        }
        return true;
    }
    else
    {
        LogError("getDBKgroupRefCategoryMap  sql execute err!");
        return false;
    }

}

bool CRuleLoadThread::getDBClientGroupMap(map<UInt32,UInt32>& clientGroupMap,UInt32& uDefaultGroupId
                                                  ,const string& clientGrpTable, const string& keywordGrpTable)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),"select t1.cgroup_id,t1.kgroup_id,t1.is_default from %s t1 "
        "inner join %s t2 on t1.kgroup_id = t2.kgroup_id;", clientGrpTable.data(), keywordGrpTable.data());
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCgroupID = atoi(rs[i]["cgroup_id"].c_str());
            UInt32 uKgroupID = atoi(rs[i]["kgroup_id"].c_str());
            Int32  IsDefault = atoi(rs[i]["is_default"].c_str());
            clientGroupMap[uCgroupID] = uKgroupID;
            if(1 == IsDefault)
                uDefaultGroupId = uKgroupID;
        }
        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return false;
    }

}

bool CRuleLoadThread::getDBCGroupRefClientMap(map<string,UInt32>& cGroupRefClientMap, const string& cGrRefClientTable)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[256] = {0};
    snprintf(sql, sizeof(sql),"select * from %s", cGrRefClientTable.data());
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uCgroupID = atoi(rs[i]["cgroup_id"].c_str());
            string strClient = rs[i]["clientid"];
            cGroupRefClientMap[strClient] = uCgroupID;
        }
        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return false;
    }

}
/*********************************keyword classify syskeyword end*********************************************/


bool CRuleLoadThread::getDBChannelWhiteKeywordMap(ChannelWhiteKeywordMap& channelWhiteKeywordMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select a.clientid, b.* from t_sms_segment_client a RIGHT JOIN t_sms_white_keyword_channel b on a.client_code = b.client_code "
                          "where b.`status` = 1 order by a.clientid desc, b.sign desc, length(white_keyword) desc; ");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ChannelWhiteKeyword channelWhiteKeyword;
            string strContent;
            string strSign;
            string strKey;
            string strClientid;
            strSign = rs[i]["sign"];
            strContent = rs[i]["white_keyword"];
            strClientid = rs[i]["clientid"];
            channelWhiteKeyword.channelgroup_id = atoi(rs[i]["channel_id"].c_str());
            channelWhiteKeyword.client_code = rs[i]["client_code"];
            channelWhiteKeyword.operatorstype = atoi(rs[i]["operatorstype"].c_str());
            channelWhiteKeyword.send_type = atoi(rs[i]["send_type"].c_str());
            Comm::StrHandle(strContent);
            Comm::StrHandle(strSign);

            if(strContent.empty() && strSign.empty())
            {
                LogWarn("sign and whitekeyword all null");
                continue;
            }
            if((channelWhiteKeyword.client_code != "*") && (strClientid.empty()))
            {
                LogWarn("clientcode[%s] is not find in t_sms_segment_client",channelWhiteKeyword.client_code.c_str());
                continue;
            }
            LogDebug("get sign[%s],strWhiteKeyword[%s],clientcode[%s],sendtype[%u]",
                strSign.c_str(),strContent.c_str(),channelWhiteKeyword.client_code.c_str(),channelWhiteKeyword.send_type);
            channelWhiteKeyword.sign = strSign;
            channelWhiteKeyword.white_keyword = strContent;
            //key clientid + sendtype
            if(channelWhiteKeyword.client_code == "*")
            {
                strClientid = "*";
            }
            strKey.assign(strClientid + "_");
            strKey.append(Comm::int2str(channelWhiteKeyword.send_type));
            ChannelWhiteKeywordMap::iterator it = channelWhiteKeywordMap.find(strKey);
            if(it == channelWhiteKeywordMap.end())
            {
                ChannelWhiteKeywordList KeyWordList;
                KeyWordList.push_back(channelWhiteKeyword);
                channelWhiteKeywordMap[strKey] = KeyWordList;
            }
            else
            {
                it->second.push_back(channelWhiteKeyword);
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
bool CRuleLoadThread::getDBChannelSegmentMap(stl_map_str_ChannelSegmentList& channelSegmentMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500, "select * from t_sms_segment_channel where status = '1'");

    if (-1 != rs.ExecuteSQL(sql))
    {
        ChannelSegment channelSeg;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            channelSeg.phone_segment = rs[i]["phone_segment"];
            channelSeg.channel_id = atoi(rs[i]["channel_id"].c_str());
            channelSeg.client_code = rs[i]["client_code"];
            channelSeg.type = atoi(rs[i]["type"].c_str());
            channelSeg.operatorstype = atoi(rs[i]["operatorstype"].c_str());
            channelSeg.area_id = atoi(rs[i]["area_id"].c_str());
            channelSeg.route_type = atoi(rs[i]["route_type"].c_str());
            channelSeg.send_type = atoi(rs[i]["send_type"].c_str());
            //?????????Â¦Â²???????????
            //????????key????
            char temp_key[128] = {0};
            if(channelSeg.route_type != 1)
            {
                snprintf(temp_key,sizeof(temp_key),"%s_%u",channelSeg.phone_segment.data(),channelSeg.send_type);
            }
            else//route_type = 1???????????????????Â¡Â¤????????????????area_id?????
            {
                snprintf(temp_key,sizeof(temp_key), "%u_%u",channelSeg.area_id,channelSeg.send_type);
            }
            string strkey = temp_key;
            stl_map_str_ChannelSegmentList::iterator it = channelSegmentMap.find(strkey);
            if(it == channelSegmentMap.end())
            {
                ChannelSegmentList ChannelSegList;
                ChannelSegList.push_back(channelSeg);
                channelSegmentMap[strkey] = ChannelSegList;
            }
            else
            {
                if( channelSeg.client_code == "*" )
                {// * ??????Â§Ã¡??

                    it->second.push_back(channelSeg);//??????Â¦Ã‚
                }
                else
                {
                    it->second.push_front(channelSeg);//???????
                }
            }

#if 0
            if(channelSeg.route_type != 1)
            {
                channelSegmentMap[channelSeg.phone_segment] = channelSeg;//key?phone_segment??????
            }
            else//route_type = 1???????????????????Â¡Â¤????????????????area_id?????
            {
                channelSegmentMultimap.insert(make_pair(channelSeg.area_id, channelSeg));//key?area_id??????
            }
#endif

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

bool CRuleLoadThread::getDBClientSegmentMap(map<std::string, StrList>& clientSegmentMap,map<string,set<std::string> >& clientidSegmentMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_segment_client");

    if (-1 != rs.ExecuteSQL(sql))
    {
        std::string client_code;
        std::string clientid;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            client_code = rs[i]["client_code"];
            clientid = rs[i]["clientid"];

            map<std::string, StrList>::iterator it = clientSegmentMap.find(client_code);
            if(it == clientSegmentMap.end())
            {
                StrList strList;
                strList.push_back(clientid);

                clientSegmentMap[client_code] = strList;
            }
            else
            {
                it->second.push_back(clientid);
            }

            if((false == client_code.empty()) && (false == clientid.empty()))
            {
                map<std::string, set<std::string> >::iterator itr = clientidSegmentMap.find(clientid);
                if(itr == clientidSegmentMap.end())
                {
                    set<std::string> strList;
                    strList.insert(client_code);
                    clientidSegmentMap[clientid] = strList;
                }
                else
                {
                    itr->second.insert(client_code);
                }
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

bool CRuleLoadThread::getDBChannelSegCodeMap(map<UInt32, stl_list_int32>& channelSegCodeMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from t_sms_segcode_channel");

    if (-1 != rs.ExecuteSQL(sql))
    {
        UInt32 channel_id;
        UInt32 code;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            channel_id = atoi(rs[i]["channel_id"].c_str());
            code = atoi(rs[i]["code"].data());

            map<UInt32, stl_list_int32>::iterator it = channelSegCodeMap.find(channel_id);
            if(it == channelSegCodeMap.end())
            {
                stl_list_int32 codeList;
                codeList.push_back(code);

                channelSegCodeMap[channel_id] = codeList;
            }
            else
            {
                it->second.push_back(code);
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


bool CRuleLoadThread::getDBAgentAccount( map<UInt64, AgentAccount>& agentAcctMap )
{
    char sql[500] = { 0 };
    UInt64 agentId = 0;

    if ( NULL == m_DBConn )
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs( m_DBConn );

    snprintf(sql, 500, "select * from t_sms_agent_account a "
                       "left join t_sms_agent_info b on a.agent_id = b.agent_id "
                       "where b.status = '1'" );

    if ( -1 != rs.ExecuteSQL(sql) )
    {
        for ( int i = 0; i < rs.GetRecordCount(); ++i )
        {
            agentId = 0;
            AgentAccount acct;
            agentId = atol(rs[i]["agent_id"].c_str());
            acct.m_fbalance = atof(rs[i]["balance"].c_str());
            acct.m_fcurrent_credit = atof(rs[i]["current_credit"].c_str());
            acct.m_fdeposit = atof(rs[i]["deposit"].c_str());
            acct.m_fcommission_income = atof(rs[i]["commission_income"].c_str());
            acct.m_frebate_income = atof(rs[i]["rebate_income"].c_str());
            agentAcctMap[ agentId ] = acct;
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

/* >>>>>>>>>>>>>>>>>>>>>>>>>> Â¡Â¤????????? BEGIN <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

bool CRuleLoadThread::getDBChannelRealtimeWeightInfo(channels_realtime_weightinfo_t& channelsRealtimeWeightInfo)
{
    channelsRealtimeWeightInfo.clear();

    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL");
        return false;
    }

    RecordSet rs( m_DBConn );
    char sql[256] = {0};

    snprintf(sql, sizeof(sql)/sizeof(sql[0]), "SELECT * FROM t_sms_channel_attribute_realtime_weight");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uChannelId = atoi(rs[i]["channelid"].c_str());

            ChannelRealtimeWeightInfo realtimeWeightInfo;

            // ?????
            realtimeWeightInfo.channel_succ_weight[BusType::SMS_TYPE_VERIFICATION_CODE] = atoi(rs[i]["yz_success_weight"].c_str());

            // ??
            realtimeWeightInfo.channel_succ_weight[BusType::SMS_TYPE_NOTICE] = atoi(rs[i]["tz_success_weight"].c_str());

            // ???
            realtimeWeightInfo.channel_succ_weight[BusType::SMS_TYPE_MARKETING] = atoi(rs[i]["yx_success_weight"].c_str());

            // ?ÂÃ“
            realtimeWeightInfo.channel_succ_weight[BusType::SMS_TYPE_ALARM] = atoi(rs[i]["gj_success_weight"].c_str());
            // ???
            realtimeWeightInfo.channel_price_weight[BusType::YIDONG] = atoi(rs[i]["yd_price_weight"].c_str());

            // j?
            realtimeWeightInfo.channel_price_weight[BusType::LIANTONG] = atoi(rs[i]["lt_price_weight"].c_str());

            // ????
            realtimeWeightInfo.channel_price_weight[BusType::DIANXIN] = atoi(rs[i]["dx_price_weight"].c_str());

            // ????/???????
            int ex_flag = atoi(rs[i]["ex_flag"].c_str());
            realtimeWeightInfo.customer_relation_flag = ((ex_flag >> CUSTOMER_RELATION_IDX)&0x01) ? 1 : 0;
            realtimeWeightInfo.low_consume_flag = ((ex_flag >> LOW_CONSUME_IDX)&0x01) ? 1 : 0;

            // ???????
            realtimeWeightInfo.anti_complaint = atoi(rs[i]["anti_complaint"].c_str());

            channelsRealtimeWeightInfo.insert(make_pair(uChannelId, realtimeWeightInfo));
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBChannelAttributeWeightConfig(channel_attribute_weight_config_t& chnlAttrWghtConf)
{
    chnlAttrWghtConf.clear();

    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL");
        return false;
    }

    RecordSet rs( m_DBConn );
    char sql[256] = {0};

    snprintf(sql, sizeof(sql)/sizeof(sql[0]), "SELECT * FROM t_sms_channel_attribute_weight_config");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ChannelAttributeWeightExvalue ex_value = static_cast<ChannelAttributeWeightExvalue>(atoi(rs[i]["ex_value"].c_str()));
            int start, end;
            // SMSTYPE_ALRM??????????4Â¦Ã‹Â§Â³??
            if (ex_value > CAWE_SMSTYPE_ALRM)
            {
                start = atof(rs[i]["start_line"].c_str())*1000;
                end = atof(rs[i]["end_line"].c_str())*1000;
            }
            else
            {
                start = atoi(rs[i]["start_line"].c_str());
                end = atoi(rs[i]["end_line"].c_str());
            }
            int weight = atoi(rs[i]["weight"].c_str());
            RangeWeight range;
            range.start = start;
            range.end = end;
            range.weight = weight;

            channel_attribute_weight_config_t::iterator iter = chnlAttrWghtConf.find(ex_value);
            if (iter == chnlAttrWghtConf.end())
            {
                ChannelWeightRangeConf rangesConf;
                rangesConf.ranges_weight.push_back(range);
                rangesConf.total_weight = weight;
                chnlAttrWghtConf.insert(make_pair(ex_value, rangesConf));
            }
            else
            {
                ChannelWeightRangeConf& rangesConf = iter->second;
                rangesConf.ranges_weight.push_back(range);
                rangesConf.total_weight += weight;
            }
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBChannelPoolPolicies(channel_pool_policies_t& channelPoolPolicies)
{
    channelPoolPolicies.clear();

    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL");
        return false;
    }

    RecordSet rs( m_DBConn );
    char sql[256] = {0};

    snprintf(sql, sizeof(sql)/sizeof(sql[0]), "SELECT * FROM t_sms_channel_pool_policy");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            int id = atoi(rs[i]["id"].c_str());
            std::string strPolicyName = rs[i]["policy_name"];
            UInt32 uSuccFactor = atoi(rs[i]["success_weight"].c_str());
            UInt32 uPriceFactor = atoi(rs[i]["price_weight"].c_str());
            UInt32 uAntiComplaintFactor = atoi(rs[i]["anti_complaint_weight"].c_str());
            UInt32 uLowConsumeFactor = atoi(rs[i]["low_consume_weight"].c_str());
            UInt32 uCustomerRelationFactor = atoi(rs[i]["customer_relation_weight"].c_str());
            UInt32 uDefault = atoi(rs[i]["is_default"].c_str());
            if (uDefault == 1)
            {
                id = DEFAULT_POOL_POLICY_ID;
            }

            ChannelPoolPolicy chnlPlPy(strPolicyName,
                                       uSuccFactor,
                                       uPriceFactor,
                                       uAntiComplaintFactor,
                                       uCustomerRelationFactor,
                                       uLowConsumeFactor);

            channelPoolPolicies.insert(make_pair(id, chnlPlPy));
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

    return true;
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>> Â¡Â¤????????? END <<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */

bool CRuleLoadThread::getDBClientPrioritiesConfig(client_priorities_t& clientPriorities)
{
    clientPriorities.clear();

    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL");
        return false;
    }

    RecordSet rs( m_DBConn );
    char sql[256] = {0};

    snprintf(sql, sizeof(sql)/sizeof(sql[0]), "SELECT * FROM t_sms_client_priority");

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            ClientPriority clientPri;
            clientPri.m_strClientId = rs[i]["clientid"];
            std::string strSmsType = rs[i]["smstype"];
            clientPri.m_uSmsType = atoi(strSmsType.c_str());
            clientPri.m_strSign = rs[i]["sign"];
            clientPri.m_iPriority = atoi(rs[i]["priority"].c_str());

            std::string strKey = clientPri.m_strClientId + "_" + clientPri.m_strSign + "_" + strSmsType;

            clientPriorities.insert(make_pair(strKey, clientPri));
        }
        return true;
    }
    else
    {
        LogError("sql execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBbgroupRefCategoryMap(map<UInt32,UInt64>& bgroupRefCategoryMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),"select t1.bgroup_id,t1.category_id from t_sms_blacklist_group_ref_category t1 "
        "inner join t_sms_blacklist_category t2 on t1.category_id = t2.category_id;");
    if (-1 != rs.ExecuteSQL(sql))
    {
        bgroupRefCategoryMap.clear();
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uBgroupID = atoi(rs[i]["bgroup_id"].c_str());
            UInt64 uCategoryID = atoi(rs[i]["category_id"].c_str());
            map<UInt32,UInt64 >::iterator itr = bgroupRefCategoryMap.find(uBgroupID);
            if(itr == bgroupRefCategoryMap.end())
            {
                bgroupRefCategoryMap.insert(make_pair(uBgroupID,uCategoryID));
            }
            else
            {
                itr->second |= uCategoryID;
            }
        }
        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return false;
    }

}

bool CRuleLoadThread::getDBBlacklistGroupMap(map<string,UInt32>& blacklistGrpMap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),"select t1.bgroup_id,t1.is_default,t2.clientid,t2.smstype from t_sms_blacklist_group t1 "
        "inner join t_sms_blacklist_user_config t2 on t1.bgroup_id = t2.bgroup_id;");
    if (-1 != rs.ExecuteSQL(sql))
    {
        blacklistGrpMap.clear();
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            UInt32 uBgrpID = atoi(rs[i]["bgroup_id"].c_str());
            Int32  IsDefault = atoi(rs[i]["is_default"].c_str());
            string strKey = rs[i]["clientid"] + "_" + rs[i]["smstype"];
            if (rs[i]["clientid"] == "*")
            {
                if (IsDefault == 1)
                {
                     blacklistGrpMap[strKey] = uBgrpID;
                }
                else
                {
                    LogError("t_sms_blacklist_user_config *_%s:[%u] is not default"
                        , rs[i]["smstype"].c_str(), uBgrpID);
                }
            }
           else
           {
                blacklistGrpMap[strKey] = uBgrpID;
           }
        }
        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql);
        return false;
    }
}

