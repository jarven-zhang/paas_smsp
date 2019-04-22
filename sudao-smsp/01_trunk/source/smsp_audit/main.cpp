#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "CThreadMonitor.h"
#include "MiddleWareConfig.h"
#include "CMQConsumerThread.h"
#include "CMQDBConsumerThread.h"
#include "CMQProducerThread.h"


DefaultConfigManager*               g_pDefaultConfigManager;
CAuditServiceThread*                g_pAuditServiceThread;
CRuleLoadThread*                    g_pRuleLoadThread;
CDBThreadPool*                      g_pDisPathDBThreadPool;
CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];

CLogThread*                         g_pLogThread;
CAlarmThread*                       g_pAlarmThread;
CMQConsumerThread*                  g_pMqdbConsumerThread;
CMQProducerThread*                  g_pMqC2sDbProducerThread;


unsigned int g_uSecSleep = 0;
UInt64 g_uComponentId = 0;
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

bool InitRedisThreadPool()
{
    map<UInt64, ComponentConfig> componentInfoMap;
    g_pRuleLoadThread->getComponentConfig(componentInfoMap);
    map<UInt64, ComponentConfig>::iterator itcomptConf = componentInfoMap.find(g_uComponentId);
    if(itcomptConf == componentInfoMap.end())
    {
        printf("g_uComponentId[%lu] not find in componentInfoMap.\n", g_uComponentId);
        return -1;
    }

    UInt32 uRedisNum = itcomptConf->second.m_uRedisThreadNum;

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

    return redisPoolsInitiator.CreateThreadPools(g_pRedisThreadPool,
                                                 sizeof(g_pRedisThreadPool)/sizeof(g_pRedisThreadPool[0]));
}


bool InitDispathDBThreadPool()
{
    UInt32 Dis_dbThreadNum = 1;
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

    g_pDisPathDBThreadPool = new CDBThreadPool("DBThreads", Dis_dbThreadNum);
    if(NULL == g_pDisPathDBThreadPool)
    {
        printf("g_pDisPathDBThreadPool new is failed!");
        return false;
    }
    if (false == g_pDisPathDBThreadPool->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname))
    {
        printf("g_pDisPathDBThreadPool init is failed!");
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
    if(NULL == g_pRuleLoadThread)
    {
        printf("g_pRuleLoadThread new is failed!\n");
        return false;
    }
    if (false == g_pRuleLoadThread->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname))
    {
        printf("g_pRuleLoadThread init is failed!\n");
        return false;
    }
    if (false == g_pRuleLoadThread->CreateThread())
    {
        printf("g_pRuleLoadThread create is failed!\n");
        return false;
    }
    return true;
}



bool LoadConfigure()
{
    g_pDefaultConfigManager = new DefaultConfigManager();
    if(NULL == g_pDefaultConfigManager)
    {
        printf("DefaultConfigManager new is over!\r\n");
        return false;
    }
    if(false == g_pDefaultConfigManager->Init())
    {
        printf("DefaultConfigManager init is over!\r\n");
        return false;
    }
    if (false == g_pDefaultConfigManager->LoadConf())
    {
        printf("DefaultConfigManager loadconf is over!\r\n");
        return false;
    }

    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("component_id",g_uComponentId);

    if(g_uSecSleep <=0 || g_uSecSleep >= 1000*1000)
    {
        printf("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return -1;
    }

    return true;
}

bool InitLogThread()
{

    std::string prefixname = "smsp_audit";
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
    LogNotice("name:%s|logpath:%s", prefixname.data(), logpath.data());
    return true;
}

bool InitAuditServerThread()
{
    g_pAuditServiceThread= new CAuditServiceThread("AuditServer");
    if(NULL == g_pAuditServiceThread)
    {
        printf("g_pAuditServiceThread new is failed!");
        return false;
    }
    if (false == g_pAuditServiceThread->Init())
    {
        printf("g_pAuditServiceThread init is failed!");
        return false;
    }
    if (false == g_pAuditServiceThread->CreateThread())
    {
        printf("g_pAuditServiceThread create is failed!");
        return false;
    }
    return true;
}

bool InitMQDBConsumerThread()
{
    map<UInt32,MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);

    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 3 )      //MQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQDBConsumerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]\n", mwconf.m_strHostIp.c_str(),
    mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMqdbConsumerThread = new CMQDBConsumerThread("MQDBConsThd");
    if(NULL == g_pMqdbConsumerThread)
    {
        printf("InitMQDBConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqdbConsumerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("InitMQDBConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMqdbConsumerThread->CreateThread())
    {
        printf("InitMQDBConsumerThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMqC2sDbProducerThread()
{
    map<UInt32,MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);

    MiddleWareConfig mwconf;

    for (map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin();
        it != middleWareConfigInfo.end(); it++)
    {
        if (it->second.m_uMiddleWareType == 3)      //MQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }

    LogNoticeP("ip[%s], port[%d], mqusername[%s], mqpwd[%s].",
        mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort,
        mwconf.m_strUserName.c_str(),
        mwconf.m_strPassWord.c_str());

    g_pMqC2sDbProducerThread = new CMQProducerThread("MQDBProducerThd");

    if (NULL == g_pMqC2sDbProducerThread)
    {
        LogNoticeP("Call new failed.");
        return false;
    }

    if (!g_pMqC2sDbProducerThread->Init(mwconf.m_strHostIp, mwconf.m_uPort, mwconf.m_strUserName, mwconf.m_strPassWord))
    {
        LogNoticeP("Call Init failed.");
        return false;
    }

    if (!g_pMqC2sDbProducerThread->CreateThread())
    {
        LogNoticeP("Call CreateThread failed.");
        return false;
    }

    return true;
}


void SendMonitorInfo()
{
    CThreadMonitor::ThreadMonitorAdd( g_pLogThread, "LogQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDisPathDBThreadPool, "DisPathDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAuditServiceThread, "auditServerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqdbConsumerThread, "mqdbConsumerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqC2sDbProducerThread, "mqdbProducerQueueSize" );

    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pRedisThreadPool[i]) {
            CThreadMonitor::ThreadMonitorAdd(g_pRedisThreadPool[i], "RedisPoolQueue" + Comm::int2str(i));
        }
    }
    CThreadMonitor::ThreadMonitorRun();
}


int main()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfigure());
    INIT_CHK_FUNC_RET_FALSE(InitLogThread());

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRuleLoadThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitDispathDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMqC2sDbProducerThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAuditServerThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQDBConsumerThread());


    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}
