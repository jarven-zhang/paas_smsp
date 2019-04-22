#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "Comm.h"
#include <vector>

#include<fstream>
#include<string>
#include<iostream>
#include "CSmsCluster.h"
#include "CThreadMonitor.h"
#include "CHttpsPushThread.h"

using namespace std;
using namespace boost;


DefaultConfigManager*               g_pDefaultConfigManager;
CHTTPSendThread*                    g_pHTTPSendThread;
CDirectSendThread*                  g_pDirectSendThread;
CDeliveryReportThread*              g_pDeliveryReportThread;
CRuleLoadThread*                    g_pRuleLoadThread;
CAlarmThread*                       g_pAlarmThread;
CHostIpThread*                      g_pHostIpThread;
CDBThreadPool*                      g_pDisPathDBThreadPool;
CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
CRedisThreadPool*                   g_pStatRedisThreadPool;
CLogThread*                         g_pLogThread;
CSgipReportThread*                  g_pSgipReportThread;
CDirectMoThread*                    g_pDirectMoThread;


CMQProducerThread*                  g_pRecordMQProducerThread;
CMQProducerThread*                  g_pMqAbIoProductThread;
CMQProducerThread*                  g_pReportMQProducerThread;
CMQConsumerThread*                  g_pReportMQConsumerThread;
CMQConsumerThread*                  g_pMqChannelConsumerThread;

CChannelThreadPool*                 g_CChannelThreadPool;
CSMSCluster*                        g_CClusterThread;
CMonitorMQProducerThread*           g_pMQMonitorPublishThread = NULL;


string g_strMQDBExchange = "";
string g_strMQDBRoutingKey = "";
string g_strMQReportExchange = "";
string g_strMQReportRoutingKey = "";
unsigned int g_uSecSleep = 0;
UInt64 g_uComponentId = 0;
UInt32 g_uiHttpsThreadNums = 2;




void SendMonitorInfo()
{
    CThreadMonitor::ThreadMonitorAdd( g_pLogThread, "LogQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDisPathDBThreadPool,"DisPathDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pHostIpThread,"HostIpQueueSize");
    CThreadMonitor::ThreadMonitorAdd( g_pDeliveryReportThread, "DeliverReportQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAlarmThread, "AlarmQueueSize" );
    //CThreadMonitor::ThreadMonitorAdd( g_pDirectSendThread,"DirectSendQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRecordMQProducerThread, "RecordMqQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqAbIoProductThread, "MqAbIoQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqChannelConsumerThread, "MqChannelQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRuleLoadThread, "ruleLoadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSgipReportThread, "sgipRptQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDirectMoThread, "directMoQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_CChannelThreadPool, "ChannelThreadPoolQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_CClusterThread, "CClusterThreadQueueSize");
    CThreadMonitor::ThreadMonitorAdd( g_pReportMQProducerThread, "ReportMqProducerQueueSize");
    CThreadMonitor::ThreadMonitorAdd( g_pReportMQConsumerThread, "ReportMqConsumerQueueSize");
    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pRedisThreadPool[i]) {
            CThreadMonitor::ThreadMonitorAdd(g_pRedisThreadPool[i], "RedisPoolQueue" + Comm::int2str(i));
        }
    }
    CThreadMonitor::ThreadMonitorRun();
}

bool InitMqChannelConsumerThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator itrMq = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_SEND_IO);
    if (itrMq == middleWareInfo.end())
    {
        printf("middleware_type 2 is not find.\n");
        return false;
    }


    g_pMqChannelConsumerThread = new CMQChannelConsumerThread("channelMq");
    if(NULL == g_pMqChannelConsumerThread)
    {
        printf("g_pMqChannelConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqChannelConsumerThread->Init(itrMq->second.m_strHostIp,itrMq->second.m_uPort,itrMq->second.m_strUserName,itrMq->second.m_strPassWord))
    {
        printf("g_pMqChannelConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMqChannelConsumerThread->CreateThread())
    {
        printf("g_pMqChannelConsumerThread create is failed!\n");
        return false;
    }

    return true;
}


bool InitMqAbIoProductThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_C2S_IO);
    if (iter == middleWareInfo.end())
    {
        printf("middleware_type 1 is not find.\n");
        return false;
    }

    g_pMqAbIoProductThread = new CMQProducerThread("MQIOPro");
    if(NULL == g_pMqAbIoProductThread)
    {
        printf("g_pMqAbIoProductThread new is failed!\n");
        return false;
    }
    if (false == g_pMqAbIoProductThread->Init(iter->second.m_strHostIp,iter->second.m_uPort,iter->second.m_strUserName,iter->second.m_strPassWord))
    {
        printf("g_pMqAbIoProductThread init is failed!\n");
        return false;
    }
    if (false == g_pMqAbIoProductThread->CreateThread())
    {
        printf("g_pMqAbIoProductThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitReportConsumerMQThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_SNED_DB);
    if (iter == middleWareInfo.end())
    {
        printf("middleware_type 4 is not find.\n");
        return false;
    }

    g_pReportMQConsumerThread = new CMQReportConsumerThread("ReportMQConsumer");
    if(NULL == g_pReportMQConsumerThread)
    {
        printf("g_pReportMQProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pReportMQConsumerThread->Init(iter->second.m_strHostIp,iter->second.m_uPort,iter->second.m_strUserName,iter->second.m_strPassWord))
    {
        printf("g_pReportMQProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pReportMQConsumerThread->CreateThread())
    {
        printf("g_pReportMQProducerThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitReportProductMQThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_SNED_DB);
    if (iter == middleWareInfo.end())
    {
        printf("middleware_type 4 is not find.\n");
        return false;
    }

        map <string,ComponentRefMq> refInfo;
    g_pRuleLoadThread->getComponentRefMq(refInfo);

    char temp[250] = {0};
    snprintf(temp,250,"%lu_%s_%u",g_uComponentId,MESSAGE_TYPE_REPORT.data(),PRODUCT_MODE);
    string strKey;
    strKey.assign(temp);
    map <string,ComponentRefMq>::iterator itr = refInfo.find(strKey);
    if (itr == refInfo.end())
    {
        printf("key:%s is not find in componentRefMq.\n",strKey.data());
        return false;
    }


    map <UInt64,MqConfig> mqInfo;
    g_pRuleLoadThread->getMqConfig(mqInfo);
    map <UInt64,MqConfig>::iterator itrMq = mqInfo.find(itr->second.m_uMqId);
    if (itrMq == mqInfo.end())
    {
        printf("mqid:%lu is not find mqconfig.\n",itr->second.m_uMqId);
        return false;
    }

    g_strMQReportExchange = itrMq->second.m_strExchange;
    g_strMQReportRoutingKey = itrMq->second.m_strRoutingKey;

    g_pReportMQProducerThread = new CMQProducerThread("ReportMQProducer");
    if(NULL == g_pReportMQProducerThread)
    {
        printf("g_pReportMQProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pReportMQProducerThread->Init(iter->second.m_strHostIp,iter->second.m_uPort,iter->second.m_strUserName,iter->second.m_strPassWord))
    {
        printf("g_pReportMQProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pReportMQProducerThread->CreateThread())
    {
        printf("g_pReportMQProducerThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitRecordProductMQThread()
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator iter = middleWareInfo.find(MIDDLEWARE_TYPE_MQ_SNED_DB);
    if (iter == middleWareInfo.end())
    {
        printf("middleware_type 4 is not find.\n");
        return false;
    }
#if 0
    map <string,ComponentRefMq> refInfo;
    g_pRuleLoadThread->getComponentRefMq(refInfo);

    char temp[250] = {0};
    snprintf(temp,250,"%lu_%s_%u",g_uComponentId,MESSAGE_TYPE_DB.data(),PRODUCT_MODE);
    string strKey;
    strKey.assign(temp);
    map <string,ComponentRefMq>::iterator itr = refInfo.find(strKey);
    if (itr == refInfo.end())
    {
        printf("key:%s is not find in componentRefMq.\n",strKey.data());
        return false;
    }

    map <UInt64,MqConfig> mqInfo;
    g_pRuleLoadThread->getMqConfig(mqInfo);
    map <UInt64,MqConfig>::iterator itrMq = mqInfo.find(itr->second.m_uMqId);
    if (itrMq == mqInfo.end())
    {
        printf("mqid:%lu is not find mqconfig.\n",itr->second.m_uMqId);
        return false;
    }

    g_strMQDBExchange = itrMq->second.m_strExchange;
    g_strMQDBRoutingKey = itrMq->second.m_strRoutingKey;
#endif
    g_pRecordMQProducerThread = new CDBMQProducerThread("MQProducer");
    if(NULL == g_pRecordMQProducerThread)
    {
        printf("g_pRecordMQProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pRecordMQProducerThread->Init(iter->second.m_strHostIp,iter->second.m_uPort,iter->second.m_strUserName,iter->second.m_strPassWord))
    {
        printf("g_pRecordMQProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pRecordMQProducerThread->CreateThread())
    {
        printf("g_pRecordMQProducerThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitDirectMoThread()
{
    g_pDirectMoThread = new CDirectMoThread("DirectMo");
    if(NULL == g_pDirectMoThread)
    {
        printf("g_pDirectMoThread new is failed!");
        return false;
    }
    if (false == g_pDirectMoThread->Init())
    {
        printf("g_pDirectMoThread init is failed!");
        return false;
    }
    if (false == g_pDirectMoThread->CreateThread())
    {
        printf("g_pDirectMoThread create is failed!");
        return false;
    }

    return true;
}

bool InitSgipReportThread(const ComponentConfig& componentInfo)
{
    if (1 != componentInfo.m_uSgipReportSwitch)
    {
        LogWarnP("Skip InitSgipReportThread.");
        return true;
    }

    const string& strIp = componentInfo.m_strHostIp;

    map <string,ListenPort> listenInfo;
    g_pRuleLoadThread->getListenPort(listenInfo);
    map <string,ListenPort>::iterator iter = listenInfo.find("sgip_rt");
    if (iter == listenInfo.end())
    {
        printf("port_key:sgip_rt in smsp_send is not find.\n");
        return false;
    }

    g_pSgipReportThread = new CSgipReportThread("SgipReport");
    if (NULL == g_pSgipReportThread)
    {
       printf("g_pSgipReportThread new failed!");
       return false;
    }

    if (false == g_pSgipReportThread->Init(strIp, iter->second.m_uPortValue))
    {
        printf("g_pSgipReportThread init failed!");
        return false;
    }

    if (false == g_pSgipReportThread->CreateThread())
    {
        printf("g_pSgipReportThread create failed!");
        return false;
    }

    return true;
}


bool InitChannelThreadPool()
{
    UInt32 uThreadCount = 0;

    /*获取通道处理线程数量*/
    if(PropertyUtils::GetValue( "smsTxThreadNum", uThreadCount ) == false)
    {
        printf("smsTxThreadNum is not configure");
        return false;
    }

    g_CChannelThreadPool = new CChannelThreadPool("ChannelThreadPool" );

    if( NULL == g_CChannelThreadPool )
    {
       printf("CChannelThreadPool new is failed!");
       return false;
    }
    if (false == g_CChannelThreadPool->Init(uThreadCount))
    {
       printf("g_CChannelThreadPool init is failed!\n");
       return false;
    }

    if (false == g_CChannelThreadPool->CreateThread())
    {
       printf("g_pHTTPSendThread create is failed!");
       return false;
    }

    return true;
}



bool InitClusterThread()
{
    g_CClusterThread = new CSMSCluster("CSMSCluster");

    if( NULL == g_CClusterThread )
    {
       printf("g_CCLusterThread new is failed!");
       return FAILURE;
    }
    if (false == g_CClusterThread->Init())
    {
       printf("g_CCLusterThread init is failed!");
       return FAILURE;
    }

    if (false == g_CClusterThread->CreateThread())
    {
       printf("g_CCLusterThread create is failed!");
       return false;
    }
    return true;
}

#if 0
bool InitDirectSendThread()
{
    /*初始化流控*/
    CChanelTxFlowCtr::channelFlowInit();

    g_pDirectSendThread = new CDirectSendThread("DirectSend");
    if(NULL == g_pDirectSendThread)
    {
        printf("g_pDirectSendThread new is failed!");
        return false;
    }
    if (false == g_pDirectSendThread->Init())
    {
        printf("g_pDirectSendThread init is failed!");
        return false;
    }
    if (false == g_pDirectSendThread->CreateThread())
    {
        printf("g_pDirectSendThread create is failed!");
        return false;
    }

    return true;
}
#endif

bool InitDeliveryReportThread()
{
    g_pDeliveryReportThread = new CDeliveryReportThread("DeliveryReport");
    if(NULL == g_pDeliveryReportThread)
    {
        printf("g_pDeliveryReportThread new is failed!");
        return false;
    }
    if (false == g_pDeliveryReportThread->Init())
    {
        printf("g_pDeliveryReportThread init is failed!");
        return false;
    }
    if (false == g_pDeliveryReportThread->CreateThread())
    {
        printf("g_pDeliveryReportThread create is failed!");
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

bool InitHostIpTrhead()
{
    g_pHostIpThread = new CHostIpThread("HostIp");
    if(NULL == g_pHostIpThread)
    {
        printf("g_pHostIpThread new is failed!");
        return false;
    }
    if (false == g_pHostIpThread->Init())
    {
        printf("g_pHostIpThread init is failed!");
        return false;
    }
    if (false == g_pHostIpThread->CreateThread())
    {
        printf("g_pHostIpThread create is failed!");
        return false;
    }

    return true;
}



bool InitRedisThreadPool(UInt32 uRedisNum)
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator itr = middleWareInfo.find(MIDDLEWARE_TYPE_REDIS);
    if (itr == middleWareInfo.end())
    {
        printf("middleware_type 0 is not find.\n");
        return false;
    }

    std::string redishost = itr->second.m_strHostIp;
    unsigned int redisport = itr->second.m_uPort;
    std::string redispwd   = itr->second.m_strPassWord;


    std::string strRedisDBsConf;
    if (PropertyUtils::GetValue("redis_DB_Ids", strRedisDBsConf) == false) {
        printf("KEY <redis_DB_Ids> NOT configured!\n");
        return false;
    }

    if (strRedisDBsConf.empty()) {
        LogWarn("KEY <redis_DB_Ids> empty!");
    }

    CRedisThreadPoolsInitiator redisPoolsInitiator(redishost, redisport, uRedisNum, redispwd );
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
    PropertyUtils::GetValue("https_thread_nums", g_uiHttpsThreadNums);

    if (g_uSecSleep <=0 || g_uSecSleep >= 1000*1000)
    {
        LogNoticeP("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return -1;
    }

    if (0 >= g_uiHttpsThreadNums || 10 < g_uiHttpsThreadNums)
    {
        LogWarnP("Invalid https_thread_nums[%u]. Will set to default value[2].", g_uiHttpsThreadNums);
        g_uiHttpsThreadNums = 2;
    }

    LogNoticeP("===***===usec_sleep:%u.", g_uSecSleep);
    LogNoticeP("===***===component_id:%lu.", g_uComponentId);
    LogNoticeP("===***===https_thread_nums:%u.", g_uiHttpsThreadNums);

    return true;
}

bool InitLogThread()
{

    std::string prefixname = "smsp_send";
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
    LogNotice("LogThread! name:%s|logpath:%s", prefixname.data(), logpath.data());
    return true;
}

bool InitMonitorPublishThread( ComponentConfig & componentConfig )
{
    map <UInt32,MiddleWareConfig> middleWareInfo;
    MiddleWareConfig mwconf;

    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator itrMq = middleWareInfo.find( MIDDLEWARE_TYPE_MQ_MONITOR );
    if ( itrMq != middleWareInfo.end())
    {
        mwconf = itrMq->second;
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

bool initmain()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfigure());
    INIT_CHK_FUNC_RET_FALSE(InitLogThread());
    INIT_CHK_FUNC_RET_FALSE(InitRuleLoadThread());


    map<UInt64,ComponentConfig> componentConfigInfo;
    g_pRuleLoadThread->getComponentConfig(componentConfigInfo);

    map<UInt64,ComponentConfig>::iterator itrCom = componentConfigInfo.find(g_uComponentId);
    if (itrCom == componentConfigInfo.end())
    {
        LogErrorP("componentid:%lu is not find in t_sms_component_configure.", g_uComponentId);
        return false;
    }

    ComponentConfig componentInfo = itrCom->second;

    INIT_CHK_FUNC_RET_FALSE(InitDispathDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE(InitRedisThreadPool(componentInfo.m_uRedisThreadNum));
    INIT_CHK_FUNC_RET_FALSE(InitHostIpTrhead());
    INIT_CHK_FUNC_RET_FALSE(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE(InitDeliveryReportThread());
    INIT_CHK_FUNC_RET_FALSE(InitChannelThreadPool());
    INIT_CHK_FUNC_RET_FALSE(InitClusterThread());
    INIT_CHK_FUNC_RET_FALSE(InitSgipReportThread(componentInfo));
    INIT_CHK_FUNC_RET_FALSE(InitDirectMoThread());
    INIT_CHK_FUNC_RET_FALSE(InitRecordProductMQThread());
    INIT_CHK_FUNC_RET_FALSE(InitMqAbIoProductThread());
    INIT_CHK_FUNC_RET_FALSE(CHttpsSendThread::InitHttpsSendPool(g_uiHttpsThreadNums));

    INIT_CHK_FUNC_RET_FALSE(InitReportProductMQThread());
    INIT_CHK_FUNC_RET_FALSE(InitMonitorPublishThread(componentInfo));

    INIT_CHK_FUNC_RET_FALSE(InitMqChannelConsumerThread());
    INIT_CHK_FUNC_RET_FALSE(InitReportConsumerMQThread());

    return true;
}

int main()
{
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(initmain());

    MAIN_INIT_OVER

    SendMonitorInfo();

    return 0;
}

