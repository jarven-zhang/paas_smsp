#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "CThreadMonitor.h"
#include "RuleLoadThread.h"
#include "MqManagerThread.h"
#include "Cond.h"

DefaultConfigManager*               g_pDefaultConfigManager;
CMPPServiceThread*                  g_pCMPPServiceThread;
CMPP3ServiceThread*                 g_pCMPP3ServiceThread;
SMPPServiceThread*                  g_pSMPPServiceThread;
SMGPServiceThread*                  g_pSMGPServiceThread;
CSgipRtAndMoThread*                 g_pSgipRtAndMoThread;
SGIPServiceThread*                  g_pSGIPServiceThread;
ReportReceiveThread*                g_pReportReceiveThread;
CRuleLoadThread*                    g_pRuleLoadThread;
CDBThreadPool*                      g_pDisPathDBThreadPool;
CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
CRedisThreadPool*                   g_pStatRedisThreadPool;
CLogThread*                         g_pLogThread;
CAlarmThread*                       g_pAlarmThread;
CUnityThread*                       g_pUnitypThread;

RptMoGetThread*                  g_pRptMoGetThread;

CMonitorMQProducerThread*        g_pMQMonitorPublishThread = NULL;

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

    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("component_id", g_uComponentId);

    if(g_uSecSleep <=0 || g_uSecSleep >= 1000*1000)
    {
        printf("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return false;
    }

    printf("DefaultConfigManager init is over!\r\n");
    return true;
}

bool InitLogThread()
{
    std::string prefixname = "smsp_c2s";
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
    std::string dbhost = "";
    unsigned int dbport = 0;
    std::string dbuser = "";
    std::string dbpwd = "";
    std::string dbname = "";
    PropertyUtils::GetValue("dispath_dbhost", dbhost);
    PropertyUtils::GetValue("dispath_dbport", dbport);
    PropertyUtils::GetValue("dispath_dbuser", dbuser);
    PropertyUtils::GetValue("dispath_dbpwd",  dbpwd);
    PropertyUtils::GetValue("dispath_dbname", dbname);

    LogDebug(" dbhost:%s|dbport:%d|dbuser:%s|dbname:%s",
        dbhost.data(),
        dbport,
        dbuser.data(),
        dbname.data());

    g_pRuleLoadThread = new CRuleLoadThread("RuleLoadThread");
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThread);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->Init(dbhost, dbport, dbuser, dbpwd, dbname));
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->CreateThread());

    g_pRuleLoadThreadV2 = new RuleLoadThread(dbhost, dbport, dbuser, dbpwd, dbname);
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

    return redisPoolsInitiator.CreateThreadPools(g_pRedisThreadPool,
                                                 sizeof(g_pRedisThreadPool)/sizeof(g_pRedisThreadPool[0]));
}

bool InitCMPP3ServiceThread(string hostIp)
{
    std::string CMPPServerHost  = "";
    unsigned int CMPPServerPort = 0;
    CMPPServerHost = hostIp;
    map<string,ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string,ListenPort>::iterator iter = listenPortInfo.begin();iter != listenPortInfo.end();iter ++)
    {
        if(iter->first == "cmpp3")
        {
            CMPPServerPort = iter->second.m_uPortValue;
            printf("cmpp3server port is %d",CMPPServerPort);
        }
    }
    g_pCMPP3ServiceThread = new CMPP3ServiceThread("CMPPServerThd");
    if(NULL == g_pCMPP3ServiceThread)
    {
        printf("g_pCMPP3ServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pCMPP3ServiceThread->Init(CMPPServerHost, CMPPServerPort))
    {
        printf("g_pCMPP3ServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pCMPP3ServiceThread->CreateThread())
    {
        printf("g_pCMPP3ServiceThread create is failed!\n");
        return false;
    }
    LogNotice("CMPP3ServiceThread ! host:%s|port:%d", CMPPServerHost.data(), CMPPServerPort);
    return true;
}

bool InitCMPPServiceThread(string hostIp)
{
    std::string CMPPServerHost  = "";
    unsigned int CMPPServerPort = 0;
    CMPPServerHost = hostIp;
    map<string,ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string,ListenPort>::iterator iter = listenPortInfo.begin();iter != listenPortInfo.end();iter ++)
    {
        if(iter->first == "cmpp")
        {
            CMPPServerPort = iter->second.m_uPortValue;
            printf("cmppserver port is %d",CMPPServerPort);
        }
    }
    g_pCMPPServiceThread = new CMPPServiceThread("CMPPServerThd");
    if(NULL == g_pCMPPServiceThread)
    {
        printf("g_pCMPPServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pCMPPServiceThread->Init(CMPPServerHost, CMPPServerPort))
    {
        printf("g_pCMPPServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pCMPPServiceThread->CreateThread())
    {
        printf("g_pCMPPServiceThread create is failed!\n");
        return false;
    }
    LogNotice("CMPPServiceThread ! host:%s|port:%d", CMPPServerHost.data(), CMPPServerPort);
    return true;
}

bool InitSMGPServiceThread(string hostIp)
{
    std::string SMGPServerHost  = "";
    unsigned int SMGPServerPort = 0;
    SMGPServerHost = hostIp;
    map<string,ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string,ListenPort>::iterator iter = listenPortInfo.begin();iter != listenPortInfo.end();iter ++)
    {
        if(iter->first == "smgp")
        {
            SMGPServerPort = iter->second.m_uPortValue;
            printf("smgpserver port is %d",SMGPServerPort);
        }
    }

    g_pSMGPServiceThread = new SMGPServiceThread("SMGPServerThd");
    if(NULL == g_pSMGPServiceThread)
    {
        printf("g_pSMGPServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pSMGPServiceThread->Init(SMGPServerHost, SMGPServerPort))
    {
        printf("g_pSMGPServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pSMGPServiceThread->CreateThread())
    {
        printf("g_pSMGPServiceThread create is failed!\n");
        return false;
    }
    LogNotice("SMGPServiceThread! host:%s|port:%d", SMGPServerHost.data(), SMGPServerPort);
    return true;
}

bool InitSGIPServiceThread(string hostIp)
{
    std::string SGIPServerHost  = "";
    unsigned int SGIPServerPort = 0;
    SGIPServerHost = hostIp;
    map<string,ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string,ListenPort>::iterator iter = listenPortInfo.begin();iter != listenPortInfo.end();iter ++)
    {
        if(iter->first == "sgip")
        {
            SGIPServerPort = iter->second.m_uPortValue;
            printf("sgipserver port is %d",SGIPServerPort);
        }
    }

    g_pSGIPServiceThread = new SGIPServiceThread("SGIPSer");
    if(NULL == g_pSGIPServiceThread)
    {
        printf("g_pSGIPServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pSGIPServiceThread->Init(SGIPServerHost, SGIPServerPort))
    {
        printf("g_pSGIPServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pSGIPServiceThread->CreateThread())
    {
        printf("g_pSGIPServiceThread create is failed!\n");
        return false;
    }
    LogNotice("SGIPServiceThread! host:%s|port:%d",SGIPServerHost.data(), SGIPServerPort);
    return true;
}

bool InitSgipRtandMoThread()
{
    g_pSgipRtAndMoThread = new CSgipRtAndMoThread("SgipRtMo");
    if(NULL == g_pSgipRtAndMoThread)
    {
        printf("g_pSgipRtAndMoThread new is failed!\n");
        return false;
    }
    if (false == g_pSgipRtAndMoThread->Init())
    {
        printf("g_pSgipRtAndMoThread init is failed!\n");
        return false;
    }
    if (false == g_pSgipRtAndMoThread->CreateThread())
    {
        printf("g_pSgipRtAndMoThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitSMPPServiceThread(string hostIp)
{
    std::string SMPPServerHost  = "";
    unsigned int SMPPServerPort = 0;
    SMPPServerHost = hostIp;
    map<string,ListenPort> listenPortInfo;
    g_pRuleLoadThread->getListenPort(listenPortInfo);
    for(map<string,ListenPort>::iterator iter = listenPortInfo.begin();iter != listenPortInfo.end();iter ++)
    {
        if(iter->first == "smpp")
        {
            SMPPServerPort = iter->second.m_uPortValue;
            printf("smppserver port is %d",SMPPServerPort);
        }
    }

    g_pSMPPServiceThread = new SMPPServiceThread("SMPPServer");
    if(NULL == g_pSMPPServiceThread)
    {
        printf("g_pSMPPServiceThread new is failed!\n");
        return false;
    }
    if (false == g_pSMPPServiceThread->Init(SMPPServerHost, SMPPServerPort))
    {
        printf("g_pSMPPServiceThread init is failed!\n");
        return false;
    }
    if (false == g_pSMPPServiceThread->CreateThread())
    {
        printf("g_pSMPPServiceThread create is failed!\n");
        return false;
    }
    LogNotice("SMPPServiceThread! host:%s|port:%d", SMPPServerHost.data(),SMPPServerPort);
    return true;
}

bool InitReportReceiveThread()
{
    g_pReportReceiveThread= new ReportReceiveThread("ReportReceive");
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

bool InitRptMoGetThread()
{
    g_pRptMoGetThread = new RptMoGetThread("RptMoGet");
    if(NULL == g_pRptMoGetThread)
    {
        printf("g_pRptMoGetThread new is failed!\n");
        return false;
    }
    if (false == g_pRptMoGetThread->Init())
    {
        printf("g_pRptMoGetThread init is failed!\n");
        return false;
    }
    if (false == g_pRptMoGetThread->CreateThread())
    {
        printf("g_pRptMoGetThread create is failed!\n");
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

bool InitMonitorPublishThread( map<UInt32,MiddleWareConfig>& middleWareConfigInfo, ComponentConfig & componentConfig )
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == MIDDLEWARE_TYPE_MQ_MONITOR )     //2£ºMQ_C2S_io
        {
            mwconf = it->second;
            break;
        }
    }

    LogNoticeP("InitMonitorPublishThread ip[%s], port[%d], mqusername[%s], mqpwd[%s] componentSwi[%u]\n",
            mwconf.m_strHostIp.c_str(), mwconf.m_uPort,
            mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str(), componentConfig.m_uMonitorSwitch );

    g_pMQMonitorPublishThread = new CMonitorMQProducerThread("CMonitorMQProducer");
    if( NULL == g_pMQMonitorPublishThread )
    {
        printf("g_pMQMonitorPublishThread new is failed!\n");
        return false;
    }

    if(false == g_pMQMonitorPublishThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,
             mwconf.m_strUserName,mwconf.m_strPassWord, componentConfig))
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
    CThreadMonitor::ThreadMonitorAdd( g_pRuleLoadThread, "RuleLoadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pCMPPServiceThread, "CMPPServiceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSMGPServiceThread, "SMGPServiceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSGIPServiceThread, "SGIPServiceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSMPPServiceThread, "SMPPServiceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSgipRtAndMoThread, "SgipRtAndMoQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pReportReceiveThread, "ReportReceiveQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAlarmThread, "AlarmQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pUnitypThread, "UnitypQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqManagerThread, "MqManagerThreadQueueSize" );

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
    INIT_CHK_FUNC_RET_FALSE(LoadConfig());
    INIT_CHK_FUNC_RET_FALSE(InitLogThread());

    string strVersin = GetVersionInfo();
    LogNotice("\r\n%s",strVersin.data());
    LogNotice("[step 1] LogThread init is over! ");

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRuleLoadThread());


    ComponentConfig componentInfo;
    g_pRuleLoadThread->getComponentConfig(componentInfo);

    map<UInt32,MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMonitorPublishThread(middleWareConfigInfo, componentInfo));

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitDispthDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool(componentInfo.m_uRedisThreadNum));

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitReportReceiveThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitUnityThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRptMoGetThread());

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitCMPP3ServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitCMPPServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitSMGPServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitSGIPServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitSMPPServiceThread(componentInfo.m_strHostIp));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitSgipRtandMoThread());

    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}
