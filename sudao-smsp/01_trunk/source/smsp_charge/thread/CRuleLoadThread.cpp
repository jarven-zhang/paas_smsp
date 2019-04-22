#include "CRuleLoadThread.h"
#include "ChargeThread.h"
#include "main.h"

using namespace models;

CRuleLoadThread* g_pRuleLoadThread = NULL;


CRuleLoadThread::CRuleLoadThread(const char* name):CThread(name)
{
    m_CustomerOrderUpdateTime = "0";
    m_OEMClientPoolUpdateTime = "0";

    m_uOrderReadIntval = 0;
}

CRuleLoadThread::~CRuleLoadThread()
{
    if (NULL != m_DBConn)
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

    if (false == CThread::Init())
    {
        return false;
    }

    if (!DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName))
    {
        LogError("DBConnect() fail.\n");
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

    unsigned int uInterval = 0;
    PropertyUtils::GetValue("order_read_interval", uInterval);

    if (uInterval <= 0 || uInterval >= 10000)
    {
        m_uOrderReadIntval = 30 * 1000;
    }
    else
    {
        m_uOrderReadIntval = uInterval * 1000;
    }

    LogNotice("m_uOrderReadIntval[%u], confix.order_read_interval[%u]", m_uOrderReadIntval, uInterval);

    m_pTimer = SetTimer(CHECK_UPDATA_TIMER_MSGID, "sessionid-check-db-update", 5 * CHECK_TABLE_UPDATE_TIME_OUT);
    m_pClientOrderTimer = SetTimer(CHECK_T_SMS_CLIENTORDER_UPDATA_TIMER_MSGID, "get-t_sms_client_order-timer", m_uOrderReadIntval);

    return true;
}

bool CRuleLoadThread::DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname)
{
    MYSQL* conn;
    conn = mysql_init(NULL);

    if (NULL == conn)
    {
        LogError("mysql_init fail.\n");
        return false;
    }

    if (NULL == mysql_real_connect(conn, dbhost.data(), dbuser.data(), dbpwd.data(), dbname.data(), dbport, NULL, 0))
    {
        LogError("connect to database fail error: %s", mysql_error(conn));
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

    if (m_DBConn == NULL)
    {
        LogError("m_DBConn is NULL, reconnect DB.");
        ret =  DBConnect(m_DBHost, m_DBPort, m_DBUser, m_DBPwd, m_DBName);
    }

    //m_DBConn != NULL
    if ((m_DBConn != NULL) && (0 != mysql_ping(m_DBConn)))
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
    //m_CustomerOrderUpdateTime = getTableLastUpdateTime("t_sms_client_order");     t_sms_client_order->m_CustomerOrderUpdateTime should manage by this thread, not trigger
    m_SysNoticeLastUpdateTime        = getTableLastUpdateTime("t_sms_sys_notice");
    m_SysNoticeUserLastUpdateTime    = getTableLastUpdateTime("t_sms_sys_notice_user");
    m_SysUserLastUpdateTime          = getTableLastUpdateTime("t_sms_user");
    m_SysParamLastUpdateTime    = getTableLastUpdateTime("t_sms_param");

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
        if (0 < rs.GetRecordCount())
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

                if (NULL != m_pTimer)
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

                if (NULL != m_pDbTimer)
                {
                    delete m_pDbTimer;
                }

                m_pDbTimer = NULL;
                m_pDbTimer = SetTimer(CHECK_DB_CONN_TIMER_MSGID, "", CHECK_DB_CONN_TIMEOUT);
            }
            //add by fangjinxiong 20161014, fix smsp_charge get fewer order
            else if (CHECK_T_SMS_CLIENTORDER_UPDATA_TIMER_MSGID == pMsg->m_iSeq)
            {
                //on timer to get t_sms_client_order
                m_CustomerOrderMap.clear();
                m_CustomerOrderSetKey.clear();
                getDBCustomerOrderMap(m_CustomerOrderMap, m_CustomerOrderSetKey);

                if (m_CustomerOrderMap.size() > 0 && g_pChargeThread != NULL)
                {
                    TUpdateCustomerOrderReq* req = new TUpdateCustomerOrderReq();
                    req->m_CustomerOrderMap = m_CustomerOrderMap;
                    req->m_CustomerOrderSetKey = m_CustomerOrderSetKey;
                    req->m_iMsgType = MSGTYPE_RULELOAD_CUSTOMERORDER_UPDATE_REQ;
                    g_pChargeThread->PostMsg(req);
                }

                m_OEMClientPoolMap.clear();
                m_OEMClientPoolSetKey.clear();
                getDBOEMClientPoolsMap(m_OEMClientPoolMap, m_OEMClientPoolSetKey);

                if (m_OEMClientPoolMap.size() > 0 && g_pChargeThread != NULL)
                {
                    TUpdateOEMClientPoolReq* req = new TUpdateOEMClientPoolReq();
                    req->m_OEMClientPoolMap = m_OEMClientPoolMap;
                    req->m_OEMClientPoolSetKey = m_OEMClientPoolSetKey;
                    req->m_iMsgType = MSGTYPE_RULELOAD_OEMCLIENTPOOL_UPDATE_REQ;
                    g_pChargeThread->PostMsg(req);
                }

                if (m_pClientOrderTimer != NULL)
                {
                    delete m_pClientOrderTimer;
                    m_pClientOrderTimer = NULL;
                }

                m_pClientOrderTimer = SetTimer(CHECK_T_SMS_CLIENTORDER_UPDATA_TIMER_MSGID, "get-t_sms_client_order-timer", m_uOrderReadIntval);
            }
            ////add by fangjinxiong 20161014 over
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

    snprintf(sql, 2048, "SELECT tablename, updatetime FROM t_sms_table_update_time \
			where tablename='%s' and updatetime>'%s';", tableName.data(), lastUpdateTime.data());

    RecordSet rs(m_DBConn);

    if (-1 != rs.ExecuteSQL(sql))
    {
        if (0 < rs.GetRecordCount())
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
    if (isTableUpdate("t_sms_param", m_SysParamLastUpdateTime))
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

    }

    if (isTableUpdate("t_sms_sys_notice", m_SysNoticeLastUpdateTime))
    {
        m_SysNoticeMap.clear();
        getDBSysNoticeMap(m_SysNoticeMap);
        TUpdateSysNoticeReq* pMsg = new TUpdateSysNoticeReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_UPDATE_REQ;
        pMsg->m_SysNoticeMap = m_SysNoticeMap;
        g_pAlarmThread->PostMsg(pMsg);
    }

    if (isTableUpdate("t_sms_sys_notice_user", m_SysNoticeUserLastUpdateTime))
    {
        m_SysNoticeUserList.clear();
        getDBSysNoticeUserList(m_SysNoticeUserList);
        TUpdateSysNoticeUserReq* pMsg = new TUpdateSysNoticeUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_NOTICE_USER_UPDATE_REQ;
        pMsg->m_SysNoticeUserList = m_SysNoticeUserList;
        g_pAlarmThread->PostMsg(pMsg);
    }

    if (isTableUpdate("t_sms_user", m_SysUserLastUpdateTime))
    {
        m_SysUserMap.clear();
        getDBSysUserMap(m_SysUserMap);
        TUpdateSysUserReq* pMsg = new TUpdateSysUserReq();
        pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYS_USER_UPDATE_REQ;
        pMsg->m_SysUserMap = m_SysUserMap;
        g_pAlarmThread->PostMsg(pMsg);
    }
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

void CRuleLoadThread::getSysParamMap(std::map<std::string, std::string>& sysParamMap)
{
    pthread_mutex_lock(&m_mutex);
    sysParamMap = m_SysParamMap;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getCustomerOrderMap(std::map<string, CustomerOrderLIST>& customerOrders, std::set<UInt64>& customerOrdersKey)
{
    pthread_mutex_lock(&m_mutex);
    customerOrders = m_CustomerOrderMap;
    customerOrdersKey = m_CustomerOrderSetKey;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getOEMClientPoolsMap(std::map<string, OEMClientPoolLIST>& OEMClientPools, std::set<UInt64>& OEMClientPoolsKey)
{
    pthread_mutex_lock(&m_mutex);
    OEMClientPools = m_OEMClientPoolMap;
    OEMClientPoolsKey = m_OEMClientPoolSetKey;
    pthread_mutex_unlock(&m_mutex);
}

void CRuleLoadThread::getMiddleWareConfig(map<UInt32, MiddleWareConfig>& middleWareConfigInfo)
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

void CRuleLoadThread::getListenPort(map<string, ListenPort>& listenPortInfo)
{
    pthread_mutex_lock(&m_mutex);
    listenPortInfo = m_listenPortInfo;
    pthread_mutex_unlock(&m_mutex);
}


bool CRuleLoadThread::getAllParamFromDB()
{
    bool ret = true;
    ret = ret && getDBCustomerOrderMap(m_CustomerOrderMap, m_CustomerOrderSetKey);
    ret = ret && getDBSysUserMap(m_SysUserMap);
    ret = ret && getDBSysNoticeMap(m_SysNoticeMap);
    ret = ret && getDBSysNoticeUserList(m_SysNoticeUserList);
    ret = ret && getDBSysParamMap(m_SysParamMap);
    ret = ret && getDBOEMClientPoolsMap(m_OEMClientPoolMap, m_OEMClientPoolSetKey);     //
    ret = ret && getDBMiddleWareConfig(m_middleWareConfigInfo);
    ret = ret && getDBComponentConfig(m_componentConfigInfo);
    ret = ret && getDBListenPort(m_listenPortInfo);
    //ret = ret && getDBChannelTemplateMap(m_mapChannelTemplate);
    return ret;
}

bool CRuleLoadThread::getDBCustomerOrderMap(std::map<string, CustomerOrderLIST>& customerOrders, std::set<UInt64>& customerOrdersKey)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    string strTime = Comm::getCurrentTime();

    snprintf(sql, 500, "select * from t_sms_client_order \
						where status=1 and end_time>=NOW() and remain_quantity>0.000001 and client_id != '' \
						ORDER BY end_time, unit_price");

    m_CustomerOrderUpdateTime = strTime;

    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            CustomerOrder Orders;
            Orders.m_strClientID    = rs[i]["client_id"];
            Orders.m_uProductType   = atoi(rs[i]["product_type"].data());
            Orders.m_uSubID         = strtoul(rs[i]["sub_id"].data(), NULL, 0);
            Orders.m_strDeadTime    = rs[i]["end_time"];
            Orders.m_fUnitPrice     = atof(rs[i]["unit_price"].data());

            if (Orders.m_uProductType == PRODUCT_TYPE_GUOJI)
            {
                Orders.m_fRemainAmount  = atof(rs[i]["remain_quantity"].data());
            }
            else
            {
                Orders.m_iRemainCount   = atoi(rs[i]["remain_quantity"].data());
            }

            Orders.m_fProductCost   = atof(rs[i]["product_cost"].data());
            Orders.m_fQuantity      = atof(rs[i]["quantity"].data());
            Orders.m_fSalePrice     = atof(rs[i]["sale_price"].data());
            Orders.m_uOperator      = atoi(rs[i]["operator_code"].data());

            map<string, CustomerOrderLIST>::iterator it = customerOrders.find(Orders.m_strClientID);

            if (it != customerOrders.end())
            {
                it->second.push_back(Orders);
            }
            else
            {
                CustomerOrderLIST list;
                list.push_back(Orders);
                customerOrders.insert(make_pair(Orders.m_strClientID, list));
            }

            customerOrdersKey.insert(Orders.m_uSubID);
        }

        return true;
    }
    else
    {
        LogError("sql execute err!, sql[%s]", sql);
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

bool CRuleLoadThread::getDBSysUserMap(std::map<int, SysUser>& sysUserMap)
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

bool CRuleLoadThread::getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap)
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

bool CRuleLoadThread::getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList)
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

bool CRuleLoadThread::getDBOEMClientPoolsMap(std::map<string, OEMClientPoolLIST>& OEMClientPools, std::set<UInt64>& OemOrdersKey)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = { 0 };
    string strTime = Comm::getCurrentTime();
    snprintf(sql, 500, "SELECT * from t_sms_oem_client_pool	\
						WHERE \
							status=0 \
						AND due_time>=NOW() \
						AND client_id != '' \
						AND \
						   ( \
							(product_type != %d AND remain_number > 0) \
							OR \
							(product_type = %d AND remain_amount > 0.000001) \
						   ) \
						ORDER BY \
							due_time, \
							unit_price", PRODUCT_TYPE_GUOJI, PRODUCT_TYPE_GUOJI);

    m_OEMClientPoolUpdateTime = strTime;

    //exculet
    if (-1 != rs.ExecuteSQL(sql))
    {
        for (int i = 0; i < rs.GetRecordCount(); ++i)
        {
            OEMClientPool Orders;

            Orders.m_strClientID        = rs[i]["client_id"];
            Orders.m_uProductType       = atoi(rs[i]["product_type"].data());
            Orders.m_iRemainTestNumber  = atoi(rs[i]["remain_test_number"].data());
            Orders.m_uOperator          = atoi(rs[i]["operator_code"].data());
            Orders.m_strDeadTime        = rs[i]["due_time"];
            Orders.m_uClientPoolId      = strtoul(rs[i]["client_pool_id"].data(), NULL, 0);
            Orders.m_fUnitPrice         = atof(rs[i]["unit_price"].data());
            Orders.m_uRemainCount       = atoi(rs[i]["remain_number"].data());
            Orders.m_fRemainAmount      = atof(rs[i]["remain_amount"].data());//?

            map<string, OEMClientPoolLIST>::iterator itor = OEMClientPools.find(Orders.m_strClientID);

            if (itor != OEMClientPools.end())
            {
                itor->second.push_back(Orders);
            }
            else
            {
                OEMClientPoolLIST list;
                list.push_back(Orders);
                OEMClientPools.insert(make_pair(Orders.m_strClientID, list));
            }

            OemOrdersKey.insert(Orders.m_uClientPoolId);
        }

        return true;
    }
    else
    {
        LogError("sql execute err!, sql[%s]", sql);
        return false;
    }

    return false;
}

bool CRuleLoadThread::getDBMiddleWareConfig(map<UInt32, MiddleWareConfig>& middleWareConfigInfo)
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

bool CRuleLoadThread::getDBComponentConfig(ComponentConfig& componentConfigInfo)
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
            break;
        }
    }
    else
    {
        LogError("sql table t_sms_component_configure execute err!");
        return false;
    }

    return true;
}

bool CRuleLoadThread::getDBListenPort(map<string, ListenPort>& listenPortInfo)
{
    if (NULL == m_DBConn)
    {
        LogError("m_DBConn is NULL.");
        return false;
    }

    RecordSet rs(m_DBConn);
    char sql[500] = {0};
    snprintf(sql, 500, "select * from t_sms_listen_port_configure where component_type ='05'");

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
        LogError("sql table t_sms_listen_port_configure execute err!");
        return false;
    }

    return true;
}



