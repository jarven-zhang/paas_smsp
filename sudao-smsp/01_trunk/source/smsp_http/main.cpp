#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "CThreadMonitor.h"
#include "CHttpsPushThread.h"
#include "compress.h"

#include "CUnityThread.h"

#include "GetReportServerThread.h"
#include "GetBalanceServerThread.h"
#include "CQueryReportServerThread.h"
#include "MqManagerThread.h"
#include "RuleLoadThread.h"


DefaultConfigManager                *g_pDefaultConfigManager;
ReportReceiveThread                 *g_pReportReceiveThread;
CRuleLoadThread                     *g_pRuleLoadThread;
CHostIpThread                       *g_pHostIpThread;
CDBThreadPool                       *g_pDisPathDBThreadPool;
CDBThreadPool                       *g_pRunDBThreadPool;
CRedisThreadPool                    *g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
CRedisThreadPool                    *g_pStatRedisThreadPool;
CLogThread                          *g_pLogThread;
CAlarmThread                        *g_pAlarmThread;
CHttpServiceThread                  *g_pHttpServiceThread;
CReportPushThread                   *g_pReportPushThread;
CUnityThread                        *g_pUnitypThread;

CMQProducerThread               	*g_pMQDBProducerThread;
CMQProducerThread               	*g_pMQC2sIOProducerThread;
CMQConsumerThread               	*g_pMqioConsumerThread;


CMonitorMQProducerThread            *g_pMQMonitorPublishThread = NULL;
GetReportServerThread               *g_pGetReportServerThread = NULL;
GetBalanceServerThread              *g_pGetBalanceServerThread = NULL;
CQueryReportServerThread            *g_pQueryReportServerThread = NULL;


string g_strMQDBExchange = "";
string g_strMQDBRoutingKey = "";
unsigned int g_uSecSleep = 0;

UInt64 g_uComponentId = 0;
bool LoadConfig()
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
    //get usec_sleep
    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    if(g_uSecSleep <= 0 || g_uSecSleep >= 1000 * 1000)
    {
        printf("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return false;
    }

    printf("DefaultConfigManager init is over!\r\n");
    return true;

}

bool InitLogThread()
{
    std::string prefixname = "smsp_http";
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
    LogNotice(" name:%s|logpath:%s", prefixname.data(), logpath.data());
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
    PropertyUtils::GetValue("dispath_dbpwd",  Dis_dbpwd);
    PropertyUtils::GetValue("dispath_dbname", Dis_dbname);

    LogDebug(" dbhost:%s|dbport:%d|dbuser:%s|dbname:%s",
             Dis_dbhost.data(), Dis_dbport, Dis_dbuser.data(), Dis_dbname.data());
	
	g_pRuleLoadThread = new CRuleLoadThread("RuleLoadThread");
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThread);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname));
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->CreateThread());

    g_pRuleLoadThreadV2 = new RuleLoadThread(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname);
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThreadV2);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->CreateThread());

    g_pMqManagerThread = new MqManagerThread();
    INIT_CHK_NULL_RET_FALSE(g_pMqManagerThread);
    INIT_CHK_FUNC_RET_FALSE(g_pMqManagerThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pMqManagerThread->CreateThread());
    return true;
}

bool InitDispthDBThreadPool()
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
    PropertyUtils::GetValue("dispath_db_thread_num", Dis_dbThreadNum);
    g_pDisPathDBThreadPool = new CDBThreadPool("DBThreads", Dis_dbThreadNum);
    if(NULL == g_pDisPathDBThreadPool)
    {
        printf("g_pDisPathDBThreadPool new is failed!\n");
        return false;
    }
    if (false == g_pDisPathDBThreadPool->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname))
    {
        printf("g_pDisPathDBThreadPool init is failed!\n");
        return false;
    }
    LogDebug("dbhost:%s|dbport:%d|dbuser:%s|dbname:%s",
             Dis_dbhost.data(), Dis_dbport, Dis_dbuser.data(), Dis_dbname.data());
    return true;
}

bool InitRunDBThreadPool()
{
    UInt32 Run_dbThreadNum = 1;
    std::string Run_dbhost = "";
    unsigned int Run_dbport = 0;
    std::string Run_dbuser = "";
    std::string Run_dbpwd = "";
    std::string Run_dbname = "";
    PropertyUtils::GetValue("run_dbhost", Run_dbhost);
    PropertyUtils::GetValue("run_dbport", Run_dbport);
    PropertyUtils::GetValue("run_dbuser", Run_dbuser);
    PropertyUtils::GetValue("run_dbpwd", Run_dbpwd);
    PropertyUtils::GetValue("run_dbname", Run_dbname);

    g_pRunDBThreadPool = new CDBThreadPool("RunDB", Run_dbThreadNum);
    if(NULL == g_pRunDBThreadPool)
    {
        printf("g_pRunDBThreadPool new is failed!");
        return false;
    }
    if (false == g_pRunDBThreadPool->Init(Run_dbhost, Run_dbport, Run_dbuser, Run_dbpwd, Run_dbname))
    {
        printf("g_pRunDBThreadPool init is failed!");
        return false;
    }

    return true;
}

bool InitRedisThreadPool(UInt32 uRedisNum)
{
    map <UInt32, MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32, MiddleWareConfig>::iterator itr = middleWareInfo.find(0);
    if (itr == middleWareInfo.end())
    {
        printf("middleware_type 0 is not find.\n");
        return false;
    }

    std::string redishost = itr->second.m_strHostIp;
    unsigned int redisport = itr->second.m_uPort;

    std::string strRedisDBsConf;
    if (PropertyUtils::GetValue("redis_DB_Ids", strRedisDBsConf) == false)
    {
        printf("KEY <redis_DB_Ids> NOT configured!\n");
        return false;
    }

    if (strRedisDBsConf.empty())
    {
        LogWarn("KEY <redis_DB_Ids> empty!");
    }

    CRedisThreadPoolsInitiator redisPoolsInitiator(redishost, redisport, uRedisNum);
    if (redisPoolsInitiator.ParseRedisDBIdxConfig(strRedisDBsConf) == false)
        return false;

    return redisPoolsInitiator.CreateThreadPools(g_pRedisThreadPool,
            sizeof(g_pRedisThreadPool) / sizeof(g_pRedisThreadPool[0]));
}

bool InitHostIPThread()
{
    g_pHostIpThread = new CHostIpThread("HostIpThread");
    if(NULL == g_pHostIpThread)
    {
        printf("g_pHostIpThread new is failed!\n");
        return false;
    }
    if (false == g_pHostIpThread->Init())
    {
        printf("g_pHostIpThread init is failed!\n");
        return false;
    }
    if (false == g_pHostIpThread->CreateThread())
    {
        printf("g_pHostIpThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitReportReceiveThread()
{
    g_pReportReceiveThread = new ReportReceiveThread("ReportReceive");
    if(NULL == g_pReportReceiveThread)
    {
        printf("g_pReportReceiveThread new is failed!\n");
        return false;
    }
    if (false == g_pReportReceiveThread->Init())
    {
        printf("g_pReportReceiveThread init is failed!\n");
        return false;
    }
    if (false == g_pReportReceiveThread->CreateThread())
    {
        printf("g_pReportReceiveThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitHttpServiceThread(string hostIp)
{
    std::string HttpServerHost  = "";
    unsigned int HttpServerPort = 0;
    HttpServerHost = hostIp;
    map<string, ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string, ListenPort>::iterator iter = listenPortInfo.begin(); iter != listenPortInfo.end(); iter ++)
    {
        if(iter->first == "http")
        {
            HttpServerPort = iter->second.m_uPortValue;
            printf("httpserver port is %d", HttpServerPort);
        }
    }
    g_pHttpServiceThread = new CHttpServiceThread("HttpService");
    if(NULL == g_pHttpServiceThread)
    {
        printf("g_pHttpServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pHttpServiceThread->Init(HttpServerHost, HttpServerPort))
    {
        printf("g_pHttpServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pHttpServiceThread->CreateThread())
    {
        printf("g_pHttpServiceThread create is failed!\n");
        return false;
    }
    LogNotice("HttpServiceThread! host:%s|port:%d", HttpServerHost.data(), HttpServerPort);
    return true;
}

bool InitReportPushThread()
{
    g_pReportPushThread = new CReportPushThread("ReportPush");
    if(NULL == g_pReportPushThread)
    {
        printf("g_pReportPushThread new is failed!\n");
        return false;
    }
    if (false == g_pReportPushThread->Init())
    {
        printf("g_pReportPushThread init is failed!\n");
        return false;
    }
    if (false == g_pReportPushThread->CreateThread())
    {
        printf("g_pReportPushThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitGetReportServerThread()
{
    g_pGetReportServerThread = new GetReportServerThread("getReportSe");
    if(NULL == g_pGetReportServerThread)
    {
        printf("g_pGetReportServerThread new is failed!");
        return false;
    }
    if (false == g_pGetReportServerThread->Init())
    {
        printf("g_pGetReportServerThread init is failed!");
        return false;
    }
    if (false == g_pGetReportServerThread->CreateThread())
    {
        printf("g_pGetReportServerThread create is failed!");
        return false;
    }

    return true;
}

bool InitGetBalanceServerThread()
{
    g_pGetBalanceServerThread = new GetBalanceServerThread("getBalanceSer");
    if(NULL == g_pGetBalanceServerThread)
    {
        printf("g_pGetBalanceServerThread new is failed!");
        return false;
    }
    if (false == g_pGetBalanceServerThread->Init())
    {
        printf("g_pGetBalanceServerThread init is failed!");
        return false;
    }
    if (false == g_pGetBalanceServerThread->CreateThread())
    {
        printf("g_pGetBalanceServerThread create is failed!");
        return false;
    }

    return true;
}

bool InitQueryReportServerThread()
{
    g_pQueryReportServerThread = new CQueryReportServerThread("queryReport");
    if(NULL == g_pQueryReportServerThread)
    {
        printf("g_pQueryReportServerThread new is failed!");
        return false;
    }
    if (false == g_pQueryReportServerThread->Init())
    {
        printf("g_pQueryReportServerThread init is failed!");
        return false;
    }
    if (false == g_pQueryReportServerThread->CreateThread())
    {
        printf("g_pQueryReportServerThread create is failed!");
        return false;
    }
    return true;
}


bool InitAlarmThread()
{
    g_pAlarmThread = new CAlarmThread("AlarmThread");
    if(NULL == g_pAlarmThread)
    {
        printf("g_pAlarmThread new is failed!\n");
        return false;
    }
    if (false == g_pAlarmThread->Init())
    {
        printf("g_pAlarmThread init is failed!\n");
        return false;
    }
    if (false == g_pAlarmThread->CreateThread())
    {
        printf("g_pAlarmThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitUnityThread()
{
    unsigned int SwitchNumber   = 0;
    PropertyUtils::GetValue("SwitchNumber", SwitchNumber);
    g_pUnitypThread = new CUnityThread("Unity");
    if (NULL == g_pUnitypThread)
    {
        printf("g_pUnitypThread is NULL.");
        return false;
    }
    if (false == g_pUnitypThread->Init(SwitchNumber))
    {
        printf("g_pUnitypThread init is failed.");
        return false;
    }
    if (false == g_pUnitypThread->CreateThread())
    {
        printf("g_pUnitypThread create is failed!");
        return false;
    }
    return true;
}
bool InitMQDBProducerThread(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32, MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 3 )      //3??MQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQDBProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
           mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQDBProducerThread = new CDBMQProducerThread("MQDBConsThd");
    if(NULL == g_pMQDBProducerThread)
    {
        printf("g_pMQDBProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQDBProducerThread->Init(mwconf.m_strHostIp, mwconf.m_uPort, mwconf.m_strUserName, mwconf.m_strPassWord))
    {
        printf("g_pMQDBProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQDBProducerThread->CreateThread())
    {
        printf("g_pMQDBProducerThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMQC2sIOProducerThread(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32, MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 1 )      //2??MQ_C2S_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQDBProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
           mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQC2sIOProducerThread = new CMQProducerThread("MQSENDIOConsThd");
    if(NULL == g_pMQC2sIOProducerThread)
    {
        printf("g_pMQSendIOProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQC2sIOProducerThread->Init(mwconf.m_strHostIp, mwconf.m_uPort, mwconf.m_strUserName, mwconf.m_strPassWord))
    {
        printf("g_pMQSendIOProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQC2sIOProducerThread->CreateThread())
    {
        printf("g_pMQSendIOProducerThread create is failed!\n");
        return false;
    }
    return true;

}

bool InitMQRtAndMoConsumerThread(map<UInt32, MiddleWareConfig> &middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32, MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 1 )      //1??MQ_c2s_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQDBProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
           mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());


    g_pMqioConsumerThread = new CMQIOConsumerThread("MQIOConsThd");
    if(NULL == g_pMqioConsumerThread)
    {
        printf("g_pMqioConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqioConsumerThread->Init(mwconf.m_strHostIp, mwconf.m_uPort, mwconf.m_strUserName, mwconf.m_strPassWord))
    {
        printf("g_pMqioConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMqioConsumerThread->CreateThread())
    {
        printf("g_pMqioConsumerThread create is failed!\n");
        return false;
    }
    return true;

}

bool InitMonitorPublishThread( map<UInt32, MiddleWareConfig> &middleWareConfigInfo, ComponentConfig &componentConfig )
{
    MiddleWareConfig mwconf;
    for(map<UInt32, MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == MIDDLEWARE_TYPE_MQ_MONITOR )     //2??MQ_C2S_io
        {
            mwconf = it->second;
            break;
        }
    }

    printf("InitMonitorPublishThread ip[%s], port[%d], mqusername[%s], mqpwd[%s] componentSwi[%u]\n",
           mwconf.m_strHostIp.c_str(), mwconf.m_uPort,
           mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str(), componentConfig.m_uMonitorSwitch );

    g_pMQMonitorPublishThread = new CMonitorMQProducerThread("CMonitorMQProducer");
    if( NULL == g_pMQMonitorPublishThread )
    {
        printf("g_pMQMonitorPublishThread new is failed!\n");
        return false;
    }

    if(false == g_pMQMonitorPublishThread->Init(mwconf.m_strHostIp, mwconf.m_uPort,
            mwconf.m_strUserName, mwconf.m_strPassWord, componentConfig))
    {
        printf("g_pMQMonitorPublishThread init is failed!\n");
        return false;
    }

    if (false == g_pMQMonitorPublishThread->CreateThread())
    {
        printf("g_pMQMonitorPublishThread create is failed!\n");
        return false;
    }

    return true;

}


void SendMonitorInfo()
{
    CThreadMonitor::ThreadMonitorAdd( g_pLogThread, "LogQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDisPathDBThreadPool, "DisPathDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRunDBThreadPool, "RunDbQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pHostIpThread, "HostIpQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRuleLoadThread, "RuleLoadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pHttpServiceThread, "HttpServiceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pReportReceiveThread, "ReportReceiveQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pReportPushThread, "ReportPushQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pGetReportServerThread, "GetReportServerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pGetBalanceServerThread, "GetBalanceServerQueueSize");    
    CThreadMonitor::ThreadMonitorAdd( g_pQueryReportServerThread, "QueryReportServerQueueSize");
    CThreadMonitor::ThreadMonitorAdd( g_pAlarmThread, "AlarmQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pUnitypThread, "UnitypQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQDBProducerThread, "MQDBProducerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQC2sIOProducerThread, "MQC2sIOProducerQueueSize" );
	CThreadMonitor::ThreadMonitorAdd( g_pMqManagerThread, "MqManagerThreadQueueSize" );

    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pRedisThreadPool[i])
        {
            CThreadMonitor::ThreadMonitorAdd(g_pRedisThreadPool[i], "RedisPoolQueue" + Comm::int2str(i));
        }
    }
    CThreadMonitor::ThreadMonitorRun();

}

int main()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfig());


    unsigned int https_push_thread_nums = 2;

    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("component_id", g_uComponentId);
    PropertyUtils::GetValue("https_push_thread_nums", https_push_thread_nums);

    if (g_uSecSleep <= 0 || g_uSecSleep >= 1000 * 1000)
    {
        printf("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return -1;
    }

    if (0 >= https_push_thread_nums || 10 < https_push_thread_nums)
    {
        https_push_thread_nums = 2;
    }

    INIT_CHK_FUNC_RET_FALSE(InitLogThread());

    string strVersin = GetVersionInfo();
    LogNotice("\r\n%s", strVersin.data());
    LogNotice("[step 1] LogThread init is over! ");
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRuleLoadThread());

    ComponentConfig componentInfo;
    g_pRuleLoadThread->getComponentConfig(componentInfo);

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitDispthDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRunDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool(componentInfo.m_uRedisThreadNum));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitHostIPThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitHttpServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitReportReceiveThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitReportPushThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitGetReportServerThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitGetBalanceServerThread());    
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitQueryReportServerThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitUnityThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(CHttpsSendThread::InitHttpsSendPool(https_push_thread_nums));

    map<UInt32, MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQDBProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQC2sIOProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMonitorPublishThread(middleWareConfigInfo, componentInfo));

    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}

