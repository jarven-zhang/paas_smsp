#include "CRuleLoadThread.h"
#include "Comm.h"
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

    getAllParamFromDB();
    getAllTableUpdateTime();

    /* send msg to LogThread, change default log level. */
    TUpdateSysParamRuleReq* pMsg2Log = new TUpdateSysParamRuleReq();
    pMsg2Log->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
    pMsg2Log->m_SysParamMap = m_SysParamMap;
    g_pLogThread->PostMsg(pMsg2Log);

    m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID_INIT, "", CHECK_TABLE_UPDATE_TIME_OUT);

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
    m_ChannelLastUpdateTime     = getTableLastUpdateTime("t_sms_channel");
    m_UserGwLastUpdateTime      = getTableLastUpdateTime("t_user_gw");
    m_AccountUpdateTime         = getTableLastUpdateTime("t_sms_account");
    m_SignextnoGwLastUpdateTime = getTableLastUpdateTime("t_signextno_gw");
    m_SysParamLastUpdateTime    = getTableLastUpdateTime("t_sms_param");
    m_SysNoticeLastUpdateTime        = getTableLastUpdateTime("t_sms_sys_notice");
    m_SysNoticeUserLastUpdateTime    = getTableLastUpdateTime("t_sms_sys_notice_user");
    m_SysUserLastUpdateTime          = getTableLastUpdateTime("t_sms_user");
    m_strChannelErrorCodeUpdateTime  = getTableLastUpdateTime("t_sms_channel_error_desc");
    //m_ChannelBlackListUpdateTime = getTableLastUpdateTime("t_sms_channel_black_list");
    m_ChannelExtendPortTableUpdateTime = getTableLastUpdateTime("t_sms_channel_extendport");

    ////smsp5.0
    m_mqConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_mq_configure");
    m_componentConfigInfoUpdateTime = getTableLastUpdateTime("t_sms_component_configure");
    m_systemErrorCodeUpdateTime = getTableLastUpdateTime("t_sms_system_error_desc");
    m_componentRefMqConfigUpdateTime = getTableLastUpdateTime("t_sms_component_ref_mq");

    m_strAccessTokenUpdateTime = getTableLastUpdateTime("t_sms_channel_token_log");

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
            else if (CHECK_UPDATA_TIMER_MSGID_INIT == pMsg->m_iSeq)
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
    if(isTableUpdate("t_sms_channel", m_ChannelLastUpdateTime))
    {
        std::map< int, Channel> channelMapOld = m_ChannelMap;
                
        m_ChannelMap.clear();
        m_mapSgipChannel.clear();
        getDBChannlelMap(m_ChannelMap);
        getDBSgipChannlelMap(m_mapSgipChannel);

        if (NULL != g_pSgipReportThread)
        {
            TUpdateChannelReq* pMsg2Sigp = new TUpdateChannelReq();
            pMsg2Sigp->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            pMsg2Sigp->m_ChannelMap = m_mapSgipChannel;
            g_pSgipReportThread->PostMsg(pMsg2Sigp);
        }
#if 0
        ////directSendThread
        TUpdateChannelReq* pChansMsg = new TUpdateChannelReq();
        pChansMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pChansMsg->m_ChannelMap = m_ChannelMap;
        g_pDirectSendThread->PostMsg(pChansMsg);
#endif
        ////mqChannelConsumer
        TUpdateChannelReq* pConsumer = new TUpdateChannelReq();
        pConsumer->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pConsumer->m_ChannelMap = m_ChannelMap;
        g_pMqChannelConsumerThread->PostMsg(pConsumer);

        if( g_CChannelThreadPool )
        {               
            TUpdateChannelReq* phttpChannelDel = new TUpdateChannelReq();
            phttpChannelDel->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
            phttpChannelDel->m_ChannelMap = m_ChannelMap;
            g_CChannelThreadPool->PostMsg(phttpChannelDel);

            TUpdateChannelReq* phttpChannelNew = new TUpdateChannelReq();
            phttpChannelNew->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_SGIP_UPDATE_REQ;
            phttpChannelNew->m_ChannelMap = m_mapSgipChannel;
            g_CChannelThreadPool->PostMsg(phttpChannelNew);
        }       

        TUpdateChannelReq* pReport = new TUpdateChannelReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ;
        pReport->m_ChannelMap = m_ChannelMap;
        g_pDeliveryReportThread->PostMsg(pReport);

    }

    if (isTableUpdate("t_sms_mq_configure", m_mqConfigInfoUpdateTime))
    {
        m_mqConfigInfo.clear();
        getDBMqConfig(m_mqConfigInfo);

        ////mqchannelconsumer
        TUpdateMqConfigReq* pMqConfig = new TUpdateMqConfigReq();
        pMqConfig->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pMqConfig->m_mapMqConfig = m_mqConfigInfo;
        g_pMqChannelConsumerThread->PostMsg(pMqConfig);

        ////directmo
        TUpdateMqConfigReq* pDirectMo = new TUpdateMqConfigReq();
        pDirectMo->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pDirectMo->m_mapMqConfig = m_mqConfigInfo;
        g_pDirectMoThread->PostMsg(pDirectMo);

        ////delivery report
        TUpdateMqConfigReq* pDirectRt = new TUpdateMqConfigReq();
        pDirectRt->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pDirectRt->m_mapMqConfig = m_mqConfigInfo;
        g_pDeliveryReportThread->PostMsg(pDirectRt);

        ////httpsend
        TUpdateMqConfigReq* pHttpSend = new TUpdateMqConfigReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pHttpSend->m_mapMqConfig = m_mqConfigInfo;
        g_CChannelThreadPool->PostMsg(pHttpSend);

        //report cache mq consumer
        TUpdateMqConfigReq* pReportMq = new TUpdateMqConfigReq();
        pReportMq->m_iMsgType = MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ;
        pReportMq->m_mapMqConfig = m_mqConfigInfo;
        g_pReportMQConsumerThread->PostMsg(pReportMq);

    }

    if (isTableUpdate("t_sms_component_ref_mq", m_componentRefMqConfigUpdateTime))
    {
        m_componentRefMqInfo.clear();
        getDBComponentRefMq(m_componentRefMqInfo);

        ////httpsend
        TUpdateComponentRefMqConfigReq* pHttpSend = new TUpdateComponentRefMqConfigReq();
        pHttpSend->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pHttpSend->m_componentRefMqInfo = m_componentRefMqInfo;
        g_CChannelThreadPool->PostMsg(pHttpSend);

        //save report mq
        TUpdateComponentRefMqConfigReq* pReportConsumer = new TUpdateComponentRefMqConfigReq();
        pReportConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pReportConsumer->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pReportMQConsumerThread->PostMsg(pReportConsumer);

        TUpdateComponentRefMqConfigReq* pReport = new TUpdateComponentRefMqConfigReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ;
        pReport->m_componentRefMqInfo = m_componentRefMqInfo;
        g_pDeliveryReportThread->PostMsg(pReport);
    }

    if (isTableUpdate("t_sms_component_configure", m_componentConfigInfoUpdateTime))
    {
        m_mapComponentConfigInfo.clear();
        getDBComponentConfig(m_mapComponentConfigInfo);

        TUpdateComponentConfigReq* pDirectMo = new TUpdateComponentConfigReq();
        pDirectMo->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        pDirectMo->m_mapComponentConfigInfo = m_mapComponentConfigInfo;
        g_pDirectMoThread->PostMsg(pDirectMo);

        TUpdateComponentConfigReq* pDirectRt = new TUpdateComponentConfigReq();
        pDirectRt->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        pDirectRt->m_mapComponentConfigInfo = m_mapComponentConfigInfo;
        g_pDeliveryReportThread->PostMsg(pDirectRt);

        //通知mq消费线程
        TMsg* pMsgToMqConsumer = new TMsg();
        pMsgToMqConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pMqChannelConsumerThread->PostMsg(pMsgToMqConsumer);

        TMsg* pMsgToReportMqConsumer = new TMsg();
        pMsgToReportMqConsumer->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
        g_pReportMQConsumerThread->PostMsg(pMsgToReportMqConsumer);

        map<UInt64, ComponentConfig>::iterator itr = m_mapComponentConfigInfo.find( g_uComponentId );
        if( itr != m_mapComponentConfigInfo.end()){
            MONITOR_UPDATE_COMP( g_pMQMonitorPublishThread, itr->second );
        }else{
            LogWarn("Monitor Publish Can't Find %lu", g_uComponentId );
        }

    }

    if (isTableUpdate("t_sms_system_error_desc", m_systemErrorCodeUpdateTime))
    {
        m_mapSystemErrorCode.clear();
        getDBSystemErrorCode(m_mapSystemErrorCode);

        TUpdateSystemErrorCodeReq* pHttp = new TUpdateSystemErrorCodeReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ;
        pHttp->m_mapSystemErrorCode = m_mapSystemErrorCode;
        g_CChannelThreadPool->PostMsg(pHttp);

    }

    if(isTableUpdate("t_sms_account", m_AccountUpdateTime))
    {
        m_accountMap.clear();
        getDBAccountMap(m_accountMap);

        TUpdateAccountReq* pUser = new TUpdateAccountReq();
        pUser->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
        pUser->m_accountMap = m_accountMap;
        g_pDirectMoThread->PostMsg(pUser);

        TUpdateAccountReq* pUser1 = new TUpdateAccountReq();
        pUser1->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
        pUser1->m_accountMap = m_accountMap;
        g_CChannelThreadPool->PostMsg(pUser1);

        TUpdateAccountReq* pReport = new TUpdateAccountReq();
        pReport->m_iMsgType = MSGTYPE_RULELOAD_USERGW_UPDATE_REQ;
        pReport->m_accountMap = m_accountMap;
        g_pDeliveryReportThread->PostMsg(pReport);

    }

    if (isTableUpdate("t_sms_channel_extendport",m_ChannelExtendPortTableUpdateTime))
    {
        m_ChannelExtendPortTable.clear();
        getDBChannelExtendPortTableMap(m_ChannelExtendPortTable);

        TUpdateChannelExtendPortReq* pMsg = new TUpdateChannelExtendPortReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ;
        pMsg->m_ChannelExtendPortTable = m_ChannelExtendPortTable;
        g_pDirectMoThread->PostMsg(pMsg);
    }

    if(isTableUpdate("t_signextno_gw", m_SignextnoGwLastUpdateTime))
    {
        m_SignextnoGwMap.clear();
        m_ClientIdAndAppendid.clear();
        getDBSignExtnoGwMap(m_ClientIdAndAppendid);

        TUpdateSignextnoGwReq* pDirMo = new TUpdateSignextnoGwReq();
        pDirMo->m_iMsgType = MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ;
        pDirMo->m_SignextnoGwMap = m_ClientIdAndAppendid;
        g_pDirectMoThread->PostMsg(pDirMo);

        TUpdateSignextnoGwReq* pDirSend = new TUpdateSignextnoGwReq();
        pDirSend->m_iMsgType = MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ;
        pDirSend->m_SignextnoGwMap = m_ClientIdAndAppendid;
        g_CChannelThreadPool->PostMsg(pDirSend);
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

        TUpdateSysParamRuleReq* pHttp = new TUpdateSysParamRuleReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pHttp->m_SysParamMap = m_SysParamMap;
        g_CChannelThreadPool->PostMsg(pHttp);

        TUpdateSysParamRuleReq* pDeliverReport = new TUpdateSysParamRuleReq();
        pDeliverReport->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pDeliverReport->m_SysParamMap = m_SysParamMap;
        g_pDeliveryReportThread->PostMsg(pDeliverReport);

        TUpdateSysParamRuleReq* pRedisMsg = new TUpdateSysParamRuleReq();
        pRedisMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;
        pRedisMsg->m_SysParamMap = m_SysParamMap;
        postRedisMngMsg(g_pRedisThreadPool, pRedisMsg );

        MONITOR_UPDATE_SYS( g_pMQMonitorPublishThread,  m_SysParamMap );

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
        g_pAlarmThread->PostMsg(pMsg);
    }
    if (isTableUpdate("t_sms_channel_error_desc",m_strChannelErrorCodeUpdateTime))
    {
        m_mapChannelErrorCode.clear();
        getDBChannelErrorCode(m_mapChannelErrorCode);

        TUpdateChannelErrorCodeReq* pHttp = new TUpdateChannelErrorCodeReq();
        pHttp->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ;
        pHttp->m_mapChannelErrorCode = m_mapChannelErrorCode;
        g_CChannelThreadPool->PostMsg(pHttp);

        if (NULL != g_pSgipReportThread)
        {
            TUpdateChannelErrorCodeReq* pSgip = new TUpdateChannelErrorCodeReq();
            pSgip->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ;
            pSgip->m_mapChannelErrorCode = m_mapChannelErrorCode;
            g_pSgipReportThread->PostMsg(pSgip);
        }
    }
    if(isTableUpdate("t_sms_channel_token_log", m_strAccessTokenUpdateTime))
    {
        m_mapAccessToken.clear();
        getDBChannelAccessTokenMap(m_mapAccessToken);

        TUpdateAccessTokenReq* pReq = new TUpdateAccessTokenReq();
        pReq->m_iMsgType = MSGTYPE_RULELOAD_ACCESSTOKEN_UPDATE_REQ;
        pReq->m_mapAccessToken = m_mapAccessToken;
        g_CChannelThreadPool->PostMsg(pReq);
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
    
}

void CRuleLoadThread::getSgipChannlelMap(std::map<int , Channel>& mapSgipChannel)
{
    pthread_mutex_lock(&m_mutex);
    mapSgipChannel = m_mapSgipChannel;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannlelMap(std::map<int , Channel>& channelmap)
{
    pthread_mutex_lock(&m_mutex);
    channelmap = m_ChannelMap;
    pthread_mutex_unlock(&m_mutex);
}

/*获取删除的通道列表*/
void CRuleLoadThread::getChannlelMapDel(std::map<int , Channel>& channelmap)
{
    pthread_mutex_lock(&m_mutex);
    channelmap = m_ChannelMapDeleted;
    pthread_mutex_unlock(&m_mutex);
}

/*void CRuleLoadThread::getUserGwMap(std::map<std::string,UserGw>& userGwMap)
{
    pthread_mutex_lock(&m_mutex);
    userGwMap = m_UserGwMap;
    pthread_mutex_unlock(&m_mutex);
}*/

void CRuleLoadThread::getAccountMap(std::map<std::string,SmsAccount>& sAccount)
{
    pthread_mutex_lock(&m_mutex);
    sAccount = m_accountMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackListMap)
{
    pthread_mutex_lock(&m_mutex);
    ChannelBlackListMap = m_ChannelBlackListMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getClientIdAndAppend(std::map<std::string,vector<SignExtnoGw> >& signextnoGwMap)
{
    pthread_mutex_lock(&m_mutex);
    signextnoGwMap = m_ClientIdAndAppendid;
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

void CRuleLoadThread::getChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode)
{
    pthread_mutex_lock(&m_mutex);
    channelErrorCode = m_mapChannelErrorCode;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable)
{
    pthread_mutex_lock(&m_mutex);
    channelExtendPortTable = m_ChannelExtendPortTable;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    middleWareConfigInfo = m_middleWareConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo)
{
    pthread_mutex_lock(&m_mutex);
    componentConfigInfo = m_mapComponentConfigInfo;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getComponentSwitch(UInt32 uComponentId)
{
    pthread_mutex_lock(&m_mutex);
    map<UInt64,ComponentConfig>::iterator itor = m_mapComponentConfigInfo.find(uComponentId);
    if(itor != m_mapComponentConfigInfo.end())
    {
        pthread_mutex_unlock(&m_mutex);
        return itor->second.m_uComponentSwitch;
    }
    else
    {
        pthread_mutex_unlock(&m_mutex);
        return false;//默认关闭
    }
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

void CRuleLoadThread::getChannelAccessToken(std::map<UInt32, AccessToken>& mapAccessToken)
{
    pthread_mutex_lock(&m_mutex);
    mapAccessToken = m_mapAccessToken;
    pthread_mutex_unlock(&m_mutex);
}

bool CRuleLoadThread::getAllParamFromDB()
{
    getDBChannlelMap(m_ChannelMap);
    getDBSgipChannlelMap(m_mapSgipChannel);
    getDBAccountMap(m_accountMap);
    getDBSignExtnoGwMap(m_ClientIdAndAppendid);
    getDBSysParamMap(m_SysParamMap);
    getDBSysUserMap(m_SysUserMap);
    getDBSysNoticeMap(m_SysNoticeMap);
    getDBSysNoticeUserList(m_SysNoticeUserList);
    getDBChannelErrorCode(m_mapChannelErrorCode);

    getDBChannelExtendPortTableMap(m_ChannelExtendPortTable);

    ////smsp5.0
    getDBMiddleWareConfig(m_middleWareConfigInfo);
    getDBComponentConfig(m_mapComponentConfigInfo);
    getDBListenPort(m_listenPortInfo);
    getDBMqConfig(m_mqConfigInfo);
    getDBComponentRefMq(m_componentRefMqInfo);
    getDBSystemErrorCode(m_mapSystemErrorCode);

    getDBChannelAccessTokenMap(m_mapAccessToken);
    getDBChannelBlackListMap(m_ChannelBlackListMap);
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
            comInfo.m_uMonitorSwitch   = atoi(rs[i]["monitor_switch"].data());
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
        LogError("sql table t_sms_middleware_configure execute err!");
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
            if(MESSAGE_TYPE_DB == refInfo.m_strMessageType)
            {//DB MQ key修改
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
        LogError("sql table t_sms_middleware_configure execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBChannlelMap(std::map<int , Channel>& channelmap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string sql = "SELECT * FROM t_sms_channel WHERE state='1' and sendid = " + Comm::int2str(g_uComponentId) + ";";

    LogDebug("sql[%s]", sql.data());

    RecordSet rs(m_DBConn);
    if (-1 != rs.ExecuteSQL(sql.data()))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            Channel chan;
            chan.m_uChannelType = atoi(rs[i]["channeltype"].data());
            chan.m_uWhiteList = atoi(rs[i]["iswhitelist"].data());
            chan.m_strServerId = rs[i]["serviceid"];
            chan.m_uSendNodeId = atol(rs[i]["sendid"].data());
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
            UInt32 uSpeed = atoi(rs[i]["speed"].data());
            chan.m_uSpeed= uSpeed;
            chan.m_uExtendSize = atoi(rs[i]["extendsize"].data());
            chan.m_uNeedPrefix = atoi(rs[i]["needprefix"].data());
            chan.m_uChannelIdentify= atoi(rs[i]["identify"].data());
            chan.m_uSliderWindow= atoi(rs[i]["sliderwindow"].data());
            chan.m_uChannelMsgQueueMaxSize= atoi(rs[i]["maxqueuesize"].data());
            chan.m_uMqId = atol(rs[i]["mq_id"].data());
            chan.m_strRemark = rs[i]["remark"].data();

            chan.m_strOauthUrl = rs[i]["oauth_url"];
            chan.m_strOauthData = rs[i]["oauth_post_data"];
            chan.m_uContentLength = atoi(rs[i]["contentlen"].c_str());

            /*增加归属商务解析*/
            chan.m_uBelong_business = atol(rs[i]["belong_business"].data());

            chan.m_uProtocolType = atoi(rs[i]["protocol_type"].data());

            chan.m_uClusterType = atoi(rs[i]["cluster_type"].data());
            chan.m_uclusterMaxNumber =  atoi(rs[i]["cluster_max_number"].data());
            chan.m_uClusterMaxTime   = atoi(rs[i]["cluster_max_time"].data());
            chan.m_strChannelLibName   = rs[i]["lib_name"];
            chan.m_uExValue = atoi(rs[i]["exvalue"].c_str());
            chan.m_uLinkNodenum = atoi(rs[i]["nodenum"].c_str());
            chan.m_strResendErrCode = rs[i]["failed_resend_errcode"];

            chan.m_strRespClientId = chan._clientID;
            chan.m_strRespPasswd = chan._password;
            chan.m_strExStr = rs[i]["exStr"];
            chan.m_uSplitRule   = atoi(rs[i]["split_rule"].data());
            chan.m_uCnLen       = atoi(rs[i]["cn_len"].data());
            chan.m_uEnLen       = atoi(rs[i]["en_len"].data());
            chan.m_uCnSplitLen  = atoi(rs[i]["cn_split_len"].data());
            chan.m_uEnSplitLen  = atoi(rs[i]["en_split_len"].data());

            // SGIP协议双账号，复用postdata
            if (chan.httpmode == LT_SGIP && !chan.postdata.empty())
            {
                std::vector<std::string> vTmp;
                Comm::splitExVectorSkipEmptyString(chan.postdata, ",", vTmp);
                if (vTmp.size() < 2)
                {
                    LogWarn("SGIP channel[%d] has invalid response user/password configure. 'username,password'", chan.channelID);
                }
                else
                {
                    chan.m_strRespClientId = vTmp[0];
                    chan.m_strRespPasswd = vTmp[1];
                    if (vTmp.size() >= 3)
                    {
                        chan.m_strSgipReportUrl = vTmp[2];
                    }
                }
                LogDebug("SGIP channel[%d] configured reponse-username/password[%s:%s]", chan.channelID, chan.m_strRespClientId.c_str(), chan.m_strRespPasswd.c_str());
            }

            if( 0 != chan.channelID  )
            {
                channelmap[chan.channelID] = chan;
            }

        }
        return true;
    }
    else
    {
        LogError("sql[ %s ] execute err!", sql.data());
        return false;
    }
}

bool CRuleLoadThread::getDBSgipChannlelMap(std::map<int , Channel>& channelmap)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    std::string sql = "SELECT * FROM t_sms_channel WHERE state='1' and httpmode='5';";

    LogNotice("sql[%s]", sql.data());

    RecordSet rs(m_DBConn);

    if (-1 != rs.ExecuteSQL(sql.data()))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            Channel chan;
            chan.m_uChannelType = atoi(rs[i]["channeltype"].data());        
            chan.m_uWhiteList = atoi(rs[i]["iswhitelist"].data());      
            chan.m_strServerId = rs[i]["serviceid"];            
            chan.m_uSendNodeId = atol(rs[i]["sendid"].data());      
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
            UInt32 uSpeed = atoi(rs[i]["speed"].data());
            chan.m_uSpeed= uSpeed;
            chan.m_uExtendSize = atoi(rs[i]["extendsize"].data());
            chan.m_uNeedPrefix = atoi(rs[i]["needprefix"].data());
            chan.m_uChannelIdentify= atoi(rs[i]["identify"].data());
            chan.m_uSliderWindow= atoi(rs[i]["sliderwindow"].data());
            chan.m_uChannelMsgQueueMaxSize= atoi(rs[i]["maxqueuesize"].data());
            chan.m_uMqId = atol(rs[i]["mq_id"].data());
            chan.m_strRemark = rs[i]["remark"].data();
            
            chan.m_strOauthUrl = rs[i]["oauth_url"];
            chan.m_strOauthData = rs[i]["oauth_post_data"];
            chan.m_uContentLength = atoi(rs[i]["contentlen"].c_str());

            /*增加归属商务解析*/
            chan.m_uBelong_business = atol(rs[i]["belong_business"].data());
            
            chan.m_uProtocolType = atoi(rs[i]["protocol_type"].data());

            chan.m_uClusterType = atoi(rs[i]["cluster_type"].data());
            chan.m_uclusterMaxNumber =  atoi(rs[i]["cluster_max_number"].data());
            chan.m_uClusterMaxTime   = atoi(rs[i]["cluster_max_time"].data());
            chan.m_strChannelLibName   = rs[i]["lib_name"];
            chan.m_uExValue = atoi(rs[i]["exvalue"].c_str());
            chan.m_uLinkNodenum = atoi(rs[i]["nodenum"].c_str());
            chan.m_strResendErrCode = rs[i]["failed_resend_errcode"];

            chan.m_strRespClientId = chan._clientID;
            chan.m_strRespPasswd = chan._password;
            chan.m_strExStr = rs[i]["exStr"];
            chan.m_uSplitRule   = atoi(rs[i]["split_rule"].data());
            chan.m_uCnLen       = atoi(rs[i]["cn_len"].data());
            chan.m_uEnLen       = atoi(rs[i]["en_len"].data());
            chan.m_uCnSplitLen  = atoi(rs[i]["cn_split_len"].data());
            chan.m_uEnSplitLen  = atoi(rs[i]["en_split_len"].data());


           // SGIP协议双账号，复用postdata                                  
            if (chan.httpmode == LT_SGIP && !chan.postdata.empty())          
            {                                                                
                std::vector<std::string> vTmp;                               
                Comm::splitExVectorSkipEmptyString(chan.postdata, ",", vTmp);
                if (vTmp.size() < 2)
                {
                    LogNotice("SGIP channel[%d] not config response user/password configure or config error. 'username,password'", chan.channelID);
                }
                else
                {
                    chan.m_strRespClientId = vTmp[0];
                    chan.m_strRespPasswd = vTmp[1];
                    if (vTmp.size() >= 3)
                    {
                        chan.m_strSgipReportUrl = vTmp[2];
                    }
                }
                LogDebug("SGIP channel[%d] configured reponse-username/password[%s:%s]", chan.channelID, chan.m_strRespClientId.c_str(), chan.m_strRespPasswd.c_str());
            }                                                                

            if( 0 != chan.channelID  )
            {
                channelmap[chan.channelID] = chan;
            }
        }

        return true;
    }
    else
    {
        LogError("sql[%s] execute err!", sql.data());
        return false;
    }
}

bool CRuleLoadThread::getDBAccountMap(std::map<std::string,SmsAccount>& accountMap)
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
            SmsAccount sAccount;
            sAccount.m_strAccount = rs[i]["clientid"];
            sAccount.m_strExtNo = rs[i]["extendport"];
            sAccount.m_uSignPortLen = atoi(rs[i]["signportlen"].data());
            sAccount.m_strAccount = rs[i]["clientid"];
            sAccount.m_uBelong_sale = atol(rs[i]["belong_sale"].data());
            sAccount.m_uSmsFrom = atoi(rs[i]["smsfrom"].data());
            sAccount.m_uFailedResendFlag = atoi(rs[i]["failed_resend_flag"].data());
            sAccount.m_uStatus = atoi(rs[i]["status"].data());
            accountMap[sAccount.m_strAccount] = sAccount;       //KEY: userid_smstype
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


bool CRuleLoadThread::getDBSignExtnoGwMap(std::map<std::string,vector<SignExtnoGw> >& clientIdAndAppend)
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
            SignExtnoGw signExtnoGw;

            signExtnoGw.channelid = rs[i]["channelid"];

            signExtnoGw.sign = rs[i]["sign"];
            signExtnoGw.appendid= rs[i]["appendid"];
            signExtnoGw.username = rs[i]["username"];
            signExtnoGw.m_strClient = rs[i]["clientid"];
            signExtnoGw.m_strFee_Terminal_Id = rs[i]["fee_terminal_id"];
            strKeyAppend = signExtnoGw.channelid + "&" + signExtnoGw.appendid;

            map<std::string,vector<SignExtnoGw> >::iterator it = clientIdAndAppend.find(strKeyAppend);
            if(it == clientIdAndAppend.end())
            {
                vector<SignExtnoGw> vec;
                vec.push_back(signExtnoGw);
                clientIdAndAppend[strKeyAppend] = vec;
                strKeyAppend.clear();
            }
            else
            {
                it->second.push_back(signExtnoGw);
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

bool CRuleLoadThread::getDBChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }
    RecordSet rs(m_DBConn);
    char sql[500] = {0};

    snprintf(sql, 500,"select * from t_sms_channel_error_desc");
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            channelErrorDesc errorDescInfo;
            errorDescInfo.m_strChannelId = rs[i]["channelid"];
            errorDescInfo.m_strType = rs[i]["type"];
            errorDescInfo.m_strErrorCode = rs[i]["errorcode"];
            errorDescInfo.m_strErrorDesc = rs[i]["errordesc"];
            errorDescInfo.m_strSysCode = rs[i]["syscode"];
            errorDescInfo.m_strInnnerCode = rs[i]["innerErrorcode"];

            string strKey = "";
            strKey.append(errorDescInfo.m_strChannelId);
            strKey.append("_");
            strKey.append(errorDescInfo.m_strType);
            strKey.append("_");
            strKey.append(errorDescInfo.m_strErrorCode);

            channelErrorCode.insert(make_pair(strKey,errorDescInfo));
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

bool CRuleLoadThread::getDBChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable)
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
            UInt32 uChannelId = 0;
            string strClientId = "";
            string strExtendPort = "";
            ChannelExtendPort channelExt;

            uChannelId = atoi(rs[i]["channelid"].data());
            strExtendPort= rs[i]["extendport"];
            strClientId = rs[i]["clientid"];

            channelExt.m_strClientId.assign(strClientId.data());
            channelExt.m_strExtendPort.assign(strExtendPort.data());
            channelExtendPortTable.insert(make_pair(uChannelId,channelExt));
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

bool CRuleLoadThread::getDBChannelAccessTokenMap(std::map<UInt32, AccessToken>& mapAccessToken)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    snprintf(sql, 500,"select * from (select * from t_sms_channel_token_log ORDER BY create_time desc) t group by channelid");

    if (-1 != rs.ExecuteSQL(sql))
    {
        UInt32 channelid;
        std::string access_token;
        Int64 expires_in;
        std::string refresh_token;
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            channelid = atoi(rs[i]["channelid"].data());
            access_token = rs[i]["access_token"];
            expires_in = atol(rs[i]["expires_in"].data());
            refresh_token= rs[i]["refresh_token"];

            AccessToken tAccessToken;
            tAccessToken.m_strAccessToken.assign(access_token);
            tAccessToken.m_iExpiresIn= expires_in;
            tAccessToken.m_strRefreshToken.assign(refresh_token);

            mapAccessToken.insert(make_pair(channelid, tAccessToken));
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
