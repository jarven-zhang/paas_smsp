#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "Comm.h"
#include "CThreadMonitor.h"

DefaultConfigManager*               g_pDefaultConfigManager;
CRuleLoadThread*                    g_pRuleLoadThread;
CAlarmThread*                       g_pAlarmThread;
CDBThreadPool*                      g_pDBThreadPool;
CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
CDBThreadPool*                      g_pDisPathDBThreadPool = NULL;
CLogThread*                         g_pLogThread;
CDBMQProducerThread*                g_pMqSendDbProducerThread;
CMQAckConsumerThread*               g_pMQDBConsumerThread;
CSqlFileThread*                     g_pSqlFileThread;
CBigDataThread*                     g_pBigDataThread;


UInt32 g_uiConsumerType = 0;
UInt32 g_uSecSleep = 0;
UInt64 g_uComponentId = 0;


bool getComponentCfg()
{
    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    if (g_uSecSleep <= 0 || g_uSecSleep >= 1000*1000)
    {
        LogNoticeP("invalid usec_sleep(%"PRIu32"). should be [1,1000*1000).", g_uSecSleep);
        return false;
    }

    PropertyUtils::GetValue("component_id", g_uComponentId);
    PropertyUtils::GetValue("consumer_database", g_uiConsumerType);
    if ((0 != g_uiConsumerType) && (1 != g_uiConsumerType))
    {
        LogNoticeP("invalid consumer_database(%u).", g_uiConsumerType);
        return false;
    }

    LogNoticeP("usec_sleep(%"PRIu32"), component_id(%"PRIu64"), "
        "consumer_database(%"PRIu32").",
        g_uSecSleep,
        g_uComponentId,
        g_uiConsumerType);

    return true;
}

bool LoadConfigure()
{
    g_pDefaultConfigManager = new DefaultConfigManager();

    CHK_NULL_RETURN_FALSE(g_pDefaultConfigManager);

    if(false == g_pDefaultConfigManager->Init())
    {
        LogNoticeP("DefaultConfigManager init is over!");
        return false;
    }

    if (false == g_pDefaultConfigManager->LoadConf())
    {
        LogNoticeP("DefaultConfigManager loadconf is over!");
        return false;
    }

    LogNoticeP("Call LoadConfigure success.");

    return true;
}

bool InitLogThread()
{
    std::string prefixname = "smsp_consumer";
    std::string logpath = g_pDefaultConfigManager->GetPath("logs");
    g_pLogThread = new CLogThread("LogThread");
    if(NULL == g_pLogThread)
    {
        printf("g_pLogThread new is failed!\r\n");
        return false;
    }
    if (false == g_pLogThread->Init(prefixname, logpath))
    {
        printf("g_pLogThread init is failed!\r\n");
        return false;
    }
    if (false == g_pLogThread->CreateThread())
    {
        printf("g_pLogThread create is failed!\r\n");
        return false;
    }
    string strVersin = GetVersionInfo();
    LogNotice("\r\n%s",strVersin.data());
    LogNotice("LogThread ! name:%s|logpath:%s", prefixname.data(), logpath.data());

    LogNoticeP("Call InitLogThread success.");
    return true;
}


void SendMonitorInfo()
{

    CThreadMonitor::ThreadMonitorAdd( g_pLogThread, "LogQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDBThreadPool, "runDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAlarmThread, "AlarmQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQDBConsumerThread, "MqDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pBigDataThread, "bigDataQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqSendDbProducerThread, "MQDBProducerThd" );

    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pRedisThreadPool[i]) {
            CThreadMonitor::ThreadMonitorAdd(g_pRedisThreadPool[i], "RedisPoolQueue" + Comm::int2str(i));
        }
    }
    CThreadMonitor::ThreadMonitorRun();
}


bool InitRedisThreadPool(UInt32 uRedisNum)
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator itr = middleWareInfo.find(0);
    if (itr == middleWareInfo.end())
    {
        printf("middleware_type 0 is not find.\n");
        return false;
    }

    std::string redishost = itr->second.m_strHostIp;
    unsigned int redisport = itr->second.m_uPort;

    std::string strRedisDBsConf;
    if (PropertyUtils::GetValue("redis_DB_Ids", strRedisDBsConf) == false) {
        printf("KEY <redis_DB_Ids> NOT configured!\n");
        return false;
    }

    if (strRedisDBsConf.empty()) {
        LogWarn("KEY <redis_DB_Ids> empty!");
    }

    CRedisThreadPoolsInitiator redisPoolsInitiator(redishost, redisport, uRedisNum);
    if (redisPoolsInitiator.ParseRedisDBIdxConfig(strRedisDBsConf) == false)
        return false;

    if (redisPoolsInitiator.CreateThreadPools(g_pRedisThreadPool,
                                                 sizeof(g_pRedisThreadPool)/sizeof(g_pRedisThreadPool[0])) == false) {
        return false;
    }

    if (g_pRedisThreadPool[REDIS_COMMON_USE_CLEAN_5] ==  NULL) {
        printf("redis DB[REDIS_COMMON_USE_CLEAN_5] not configured??\n");
        return false;
    }

    return true;
}

bool InitSqlFileThread()
{
    string prefixname = "smsp_consumer";
    string strSqlFilePath = "";
    strSqlFilePath = g_pDefaultConfigManager->GetPath("sqlfile");
    g_pSqlFileThread = new CSqlFileThread("SqlFile");
    if(NULL == g_pSqlFileThread)
    {
        printf("g_pSqlFileThread new is failed!\n");
        return false;
    }
    if (false == g_pSqlFileThread->Init(prefixname, strSqlFilePath))
    {
        printf("g_pSqlFileThread init is failed!\n");
        return false;
    }
    if (false == g_pSqlFileThread->CreateThread())
    {
        printf("g_pSqlFileThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitBigDataThread()
{
    string prefixname;
    if (g_uiConsumerType == 0)
    {
        prefixname = "smsp_access";
    }
    if (g_uiConsumerType == 1)
    {
        prefixname = "smsp_record";
    }

    string strSqlFilePath = "";
    PropertyUtils::GetValue("bigdata_dir", strSqlFilePath);
    
    //strSqlFilePath = g_pDefaultConfigManager->GetPath("bigData");
    g_pBigDataThread = new CBigDataThread("bigData");
    if(NULL == g_pBigDataThread)
    {
        printf("g_pSqlFileThread new is failed!\n");
        return false;
    }
    if (false == g_pBigDataThread->Init(prefixname, strSqlFilePath))
    {
        printf("g_pSqlFileThread init is failed!\n");
        return false;
    }
    if (false == g_pBigDataThread->CreateThread())
    {
        printf("g_pSqlFileThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitMqDBConsumerThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator iter;

    if (0 == g_uiConsumerType)//access
    {
        iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_C2S_DB);
    }
    else if(1 == g_uiConsumerType)//record
    {
        iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_SNED_DB);
    }
    else
    {
        printf("g_uiConsumerType:%d is invalid.\n", g_uiConsumerType);
        return false;
    }

    if (iter == middleWareInfo.end())
    {
        printf("middleware_type 4 is not find.\n");
        return false;
    }

    g_pMQDBConsumerThread = new CMQDBConsumerThread("DBMQcon", g_uiConsumerType);
    if(NULL == g_pMQDBConsumerThread)
    {
        printf("g_pMQDBConsumerThread new is failed!\n");
        return false;
    }

    if (false == g_pMQDBConsumerThread->Init(iter->second.m_strHostIp,iter->second.m_uPort,iter->second.m_strUserName,iter->second.m_strPassWord))
    {
        printf("g_pMQDBConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQDBConsumerThread->CreateThread())
    {
        printf("g_pMQDBConsumerThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitAlarmThread()
{
    g_pAlarmThread = new CAlarmThread("AlarmThread");
    if(NULL == g_pAlarmThread)
    {
        printf("g_pAlarmThread new is failed!");
        return false;
    }
    if (false == g_pAlarmThread->Init())
    {
        printf("g_pAlarmThread init is failed!");
        return false;
    }
    if (false == g_pAlarmThread->CreateThread())
    {
        printf("g_pAlarmThread create is failed!");
        return false;
    }

    return true;

}

bool InitRunDBThreadPool()
{
    std::string Run_dbhost = "";
    UInt32      Run_dbport = 0;
    std::string Run_dbuser = "";
    std::string Run_dbpwd = "";
    std::string Run_dbname = "";

    PropertyUtils::GetValue("run_dbhost", Run_dbhost);
    PropertyUtils::GetValue("run_dbport", Run_dbport);
    PropertyUtils::GetValue("run_dbuser", Run_dbuser);
    PropertyUtils::GetValue("run_dbpwd", Run_dbpwd);
    PropertyUtils::GetValue("run_dbname", Run_dbname);
    UInt32 Run_dbThreadNum = 1;
    PropertyUtils::GetValue("run_db_thread_num", Run_dbThreadNum);

    g_pDBThreadPool = new CDBThreadPool("DBThreads", Run_dbThreadNum);
    if(NULL == g_pDBThreadPool)
    {
        printf("g_pDBThreadPool new is failed!");
        return false;
    }
    if (false == g_pDBThreadPool->Init(Run_dbhost, Run_dbport, Run_dbuser, Run_dbpwd, Run_dbname))
    {
        printf("g_pDBThreadPool init is failed!");
        return false;
    }

    return true;
}


bool InitRuleLoadThread()
{
    std::string Dis_dbhost = "";
    unsigned int Dis_dbport = 0;
    std::string Dis_dbuser = "";
    std::string Dis_dbpwd = "";
    std::string Dis_dbname = "";
    PropertyUtils::GetValue("dispath_dbhost", Dis_dbhost);
    PropertyUtils::GetValue("dispath_dbport", Dis_dbport);
    PropertyUtils::GetValue("dispath_dbuser", Dis_dbuser);
    PropertyUtils::GetValue("dispath_dbpwd", Dis_dbpwd);
    PropertyUtils::GetValue("dispath_dbname", Dis_dbname);

    g_pRuleLoadThread = new CRuleLoadThread("RuleLoad");
    CHK_NULL_RETURN_FALSE(g_pRuleLoadThread);

    if (!g_pRuleLoadThread->Init(Dis_dbhost,
                                Dis_dbport,
                                Dis_dbuser,
                                Dis_dbpwd,
                                Dis_dbname,
                                g_uiConsumerType))
    {
        LogNoticeP("g_pRuleLoadThread init is failed!");
        return false;
    }

    if (!g_pRuleLoadThread->CreateThread())
    {
        LogNoticeP("g_pRuleLoadThread create is failed!");
        return false;
    }

    LogNoticeP("Call InitRuleLoadThread success.");
    return true;
}







/*void setMqProducerHandle2DbThread(map<string, string> mapTmp)
{
    CHK_NULL_RETURN(g_pDBThreadPool);

    for(UInt32 i = 0; i < g_pDBThreadPool->getThreadNum(); ++i)
    {
        CDBThread* pThread = g_pDBThreadPool->getThread(i);
        CHK_NULL_CONTINUE(pThread);
        pThread->setHandle(g_pMqSendDbProducerThread);
    }
}*/

bool InitMqSendDbProducerThread()
{
    if (0 == g_uiConsumerType) //access
    {

    }
    else if(1 == g_uiConsumerType) //record
    {
    }
    else
    {
        printf("g_uiConsumerType:%d is invalid.\n", g_uiConsumerType);
        return false;
    }

    CHK_NULL_RETURN_FALSE(g_pRuleLoadThread);

    MiddleWareConfig mwconf;
    map<UInt32,MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);

    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin();
        it != middleWareConfigInfo.end(); ++it)
    {
        if (0 == g_uiConsumerType)
        {
            if (MIDDLEWARE_TYPE_MQ_C2S_DB == it->second.m_uMiddleWareType)
            {
                mwconf = it->second;
                break;
            }
        }
        else
        {
            if (MIDDLEWARE_TYPE_MQ_SNED_DB == it->second.m_uMiddleWareType)
            {
                mwconf = it->second;
                break;
            }
        }
    }

    printf("InitMqSendDbProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s].",
        mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort,
        mwconf.m_strUserName.c_str(),
        mwconf.m_strPassWord.c_str());

    g_pMqSendDbProducerThread = new CDBMQProducerThread("MQDBProducerThd");

    if (NULL == g_pMqSendDbProducerThread)
    {
        printf("new CDBMQProducerThread failed.\n");
        return false;
    }

    if (!g_pMqSendDbProducerThread->Init(mwconf.m_strHostIp,
                                                mwconf.m_uPort,
                                                mwconf.m_strUserName,
                                                mwconf.m_strPassWord))
    {
        printf("Call CDBMQProducerThread::Init() failed.\n");
        return false;
    }

    if (!g_pMqSendDbProducerThread->CreateThread())
    {
        printf("Call CDBMQProducerThread::CreateThread() failed.\n");
        return false;
    }

    return true;
}

int main()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfigure());
    INIT_CHK_FUNC_RET_FALSE(getComponentCfg());

    INIT_CHK_FUNC_RET_FALSE(InitLogThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRuleLoadThread());


    map<UInt64,ComponentConfig> componentConfigInfo;
    g_pRuleLoadThread->getComponentConfig(componentConfigInfo);

    map<UInt64,ComponentConfig>::iterator itrCom = componentConfigInfo.find(g_uComponentId);
    if (itrCom == componentConfigInfo.end())
    {
        printf("componentid:%lu is not find in t_sms_component_configure.\n",g_uComponentId);
        return -1;
    }
    ComponentConfig componentInfo = itrCom->second;


    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRunDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitSqlFileThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitBigDataThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMqSendDbProducerThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool(componentInfo.m_uRedisThreadNum));

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMqDBConsumerThread());

    if (COMPONENT_BLACKLIST_OPEN == componentInfo.m_uBlacklistSwitch && CONSUMER_RELOAD_BLACKLIST == g_uiConsumerType)
    {
        g_pRuleLoadThread->FlushReidsBlacklist();
    }

    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}


