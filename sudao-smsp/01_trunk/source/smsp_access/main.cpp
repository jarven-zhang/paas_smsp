#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "CThreadMonitor.h"
#include "MiddleWareConfig.h"
#include "CMonitorMQProducerThread.h"
#include "boost/function.hpp"
#include "LogMacro.h"
#include "Fmt.h"
#include "CAuditThread.h"
#include "RuleLoadThread.h"

#include <dirent.h>


DefaultConfigManager*               g_pDefaultConfigManager;
CRuleLoadThread*                    g_pRuleLoadThread;
CDBThreadPool*                      g_pDisPathDBThreadPool;
CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
CRedisThreadPool*                   g_pOverrateRedisThreadPool[REDIS_USE_INDEX_MAX];
CRedisThreadPool*                   g_pStatRedisThreadPool;
CLogThread*                         g_pLogThread;
CAlarmThread*                       g_pAlarmThread;
CMQProducerThread*                  g_pMQDBAuditProducerThread;
CMQProducerThread*                  g_pMQIOProducerThread;
CMQProducerThread*                  g_pMQSendIOProducerThread;
CMonitorMQProducerThread*           g_pMQMonitorProducerThread;
CMQProducerThread*                  g_pMQC2SDBProducerThread;

CMQConsumerThread*                  g_pMqioConsumerThread;
CMQAckConsumerThread*               g_pMqAuditConsumerThread;
CGetChannelMqSizeThread*            g_pGetChannelMqSizeThread;

CMQChannelUnusualConsumerThread*            g_pMqChannelUnusualConsumerThread = NULL;
CMQOldNoPriQueueDataTransferConsumer*       g_pMqOldNoPriQueueDataTransferThread = NULL;
CMQConsumerThread*                          g_pMqioRebackConsumerThread;


unsigned int g_uSecSleep = 0;
UInt64 g_uComponentId = 0;

bool InitMQAuditConsumerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);

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

bool InitRedisThreadPool(CRedisThreadPool* redisThreadPool[], int size, int type, UInt32 uRedisNum){
    map <UInt32,MiddleWareConfig> middleWareInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareInfo);
    map <UInt32,MiddleWareConfig>::iterator itr = middleWareInfo.find(type);
    if (itr == middleWareInfo.end())
    {
        printf("middleware_type %d is not find.\n", type);
        return false;
    }

    std::string redishost = itr->second.m_strHostIp;
    unsigned int redisport = itr->second.m_uPort;

    std::string strRedisDBsConf;

    if(MIDWARE_TYPE_REDIS_OVERRATE == type){
        if (PropertyUtils::GetValue("redis_overrate_DB_Ids", strRedisDBsConf) == false) {
            printf("KEY <redis_overrate_DB_Ids> NOT configured!\n");
            return false;
        }
    }
    else{
        if (PropertyUtils::GetValue("redis_DB_Ids", strRedisDBsConf) == false) {
            printf("KEY <redis_DB_Ids> NOT configured!\n");
            return false;
        }
    }

    if (strRedisDBsConf.empty()) {
        LogWarn("KEY <redis_DB_Ids> empty!");
    }

    CRedisThreadPoolsInitiator redisPoolsInitiator(redishost, redisport, uRedisNum);
    if (redisPoolsInitiator.ParseRedisDBIdxConfig(strRedisDBsConf) == false)
        return false;

    return redisPoolsInitiator.CreateThreadPools(redisThreadPool, size);
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

    g_pRuleLoadThread = new CRuleLoadThread("CRuleLoadThread");
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThread);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname));
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->CreateThread());

    g_pRuleLoadThreadV2 = new RuleLoadThread(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname);
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThreadV2);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->CreateThread());

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

    string openccConfigPath = g_pDefaultConfigManager->GetOpenccConfPath();
    DIR *dirptr = NULL;
    if((dirptr = opendir(openccConfigPath.data())) == NULL)
    {
        printf("Cannot find opencc config files, path should be %s%s\n", g_pDefaultConfigManager->GetPath("conf").data(), "opencc/");
        return false;
    }
    else
    {
        closedir(dirptr);
    }


    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("component_id", g_uComponentId );


    if (g_uSecSleep <=0 || g_uSecSleep >= 1000*1000)
    {
        printf("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return -1;
    }

    return true;
}

bool InitLogThread()
{

    std::string prefixname = "smsp_access";
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

bool InitMQAuditConsumerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 3 )  // 3£ºMQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQAuditConsumerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]\n",
        mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort,
        mwconf.m_strUserName.c_str(),
        mwconf.m_strPassWord.c_str());

    g_pMqAuditConsumerThread = new CMQAuditConsumerThread("MQAuditConsThd");
    if(NULL == g_pMqAuditConsumerThread)
    {
        printf("g_pMqioConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqAuditConsumerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMqioConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMqAuditConsumerThread->CreateThread())
    {
        printf("g_pMqioConsumerThread create is failed!\n");
        return false;
    }

    return true;
}

bool InitMQIOConsumerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 1 )      //1£ºMQ_c2s_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQIOConsumerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMqioConsumerThread = new CMQIOConsumerThread("MQIOConsThd");
    if(NULL == g_pMqioConsumerThread)
    {
        printf("g_pMqioConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqioConsumerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
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

bool InitMQDBAuditProducerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 3 )      //3£ºMQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }

    g_pMQDBAuditProducerThread = new CMQProducerThread("MQDBAUDITProducerThd");
    if(NULL == g_pMQDBAuditProducerThread)
    {
        printf("g_pMQDBAuditProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQDBAuditProducerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMQDBAuditProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQDBAuditProducerThread->CreateThread())
    {
        printf("g_pMQDBAuditProducerThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMQSendIOProducerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 2 )      //2£ºMQ_send_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQSendIOProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQSendIOProducerThread = new CMQProducerThread("MQSENDIOConsThd");
    if(NULL == g_pMQSendIOProducerThread)
    {
        printf("g_pMQSendIOProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQSendIOProducerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMQSendIOProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQSendIOProducerThread->CreateThread())
    {
        printf("g_pMQSendIOProducerThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMQIOProducerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 1 )      //1£ºMQ_c2s_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQIOProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQIOProducerThread = new CMQProducerThread("MQC2SIOConsThd");
    if(NULL == g_pMQIOProducerThread)
    {
        printf("g_pMQIOProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQIOProducerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMQIOProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQIOProducerThread->CreateThread())
    {
        printf("g_pMQIOProducerThread create is failed!\n");
        return false;
    }

    return true;
}



bool InitMonitorMQProducerThread( map<UInt32,MiddleWareConfig>& middleWareConfigInfo, ComponentConfig &componentConfig )
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == MIDDLEWARE_TYPE_MQ_MONITOR )     //1£ºMQ_c2s_io
        {
            mwconf = it->second;
            break;
        }
    }

    printf("InitMonitorMQProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQMonitorProducerThread = new CMonitorMQProducerThread("MQMonitorProducerThread");
    if( NULL == g_pMQMonitorProducerThread )
    {
        printf("g_pMQMonitorProducerThread new is failed!\n");
        return false;
    }

    if (false == g_pMQMonitorProducerThread->Init( mwconf.m_strHostIp,mwconf.m_uPort,
                 mwconf.m_strUserName,mwconf.m_strPassWord, componentConfig ))
    {
        printf("g_pMQIOProducerThread init is failed!\n");
        return false;
    }

    if (false == g_pMQMonitorProducerThread->CreateThread())
    {
        printf("g_pMQIOProducerThread create is failed!\n");
        return false;
    }

    return true;

}


bool InitGetChannelMqSizeThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 2 )      //2£ºMQ_send_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitGetChannelMqSizeThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pGetChannelMqSizeThread = new CGetChannelMqSizeThread("getchSize");
    if(NULL == g_pGetChannelMqSizeThread)
    {
        printf("g_pGetChannelMqSizeThread new is failed!\n");
        return false;
    }
    if (false == g_pGetChannelMqSizeThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pGetChannelMqSizeThread init is failed!\n");
        return false;
    }
    if (false == g_pGetChannelMqSizeThread->CreateThread())
    {
        printf("g_pGetChannelMqSizeThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMQC2SDBProducerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 3 )      //3:MQ_c2s_db
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQC2SDBProducerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMQC2SDBProducerThread = new CDBMQProducerThread("MQC2SDBConsThd");
    if(NULL == g_pMQC2SDBProducerThread)
    {
        printf("g_pMQC2SDBProducerThread new is failed!\n");
        return false;
    }
    if (false == g_pMQC2SDBProducerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMQC2SDBProducerThread init is failed!\n");
        return false;
    }
    if (false == g_pMQC2SDBProducerThread->CreateThread())
    {
        printf("g_pMQC2SDBProducerThread create is failed!\n");
        return false;
    }
    return true;
}

bool InitMQChannelUnusualConsumerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 2 )      //1ï¼šMQ_send_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQChannelUnusualConsumerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s].\n",
        mwconf.m_strHostIp.c_str(), mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());


    g_pMqChannelUnusualConsumerThread = new CMQChannelUnusualConsumerThread("unusual");
    if(NULL == g_pMqChannelUnusualConsumerThread)
    {
        printf("g_pMqChannelUnusualConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqChannelUnusualConsumerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMqChannelUnusualConsumerThread init is failed!\n");
        return false;
    }
    if (false == g_pMqChannelUnusualConsumerThread->CreateThread())
    {
        printf("g_pMqChannelUnusualConsumerThread create is failed!\n");
        return false;
    }
    LogNotice("[step 14] g_pMqChannelUnusualConsumerThread init is over!");
    return true;
}


bool InitMQOldNoPriQueueDataTransferConsumer(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 2 )      //1ï¼šMQ_send_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQOldNoPriQueueDataTransferConsumer ip[%s], port[%d], mqusername[%s], mqpwd[%s].\n",
        mwconf.m_strHostIp.c_str(), mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());


    g_pMqOldNoPriQueueDataTransferThread = new CMQOldNoPriQueueDataTransferConsumer("oldPriorityDataTransfer");
    if(NULL == g_pMqOldNoPriQueueDataTransferThread)
    {
        printf("g_pMqOldNoPriQueueDataTransferThread new is failed!\n");
        return false;
    }
    if (false == g_pMqOldNoPriQueueDataTransferThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("g_pMqOldNoPriQueueDataTransferThread init is failed!\n");
        return false;
    }
    if (false == g_pMqOldNoPriQueueDataTransferThread->CreateThread())
    {
        printf("g_pMqOldNoPriQueueDataTransferThread create is failed!\n");
        return false;
    }
    LogNotice("[step 14-1] g_pMqOldNoPriQueueDataTransferThread init is over!");
    return true;
}

bool InitMQIORebackConsumerThread(map<UInt32,MiddleWareConfig>& middleWareConfigInfo)
{
    MiddleWareConfig mwconf;
    for(map<UInt32,MiddleWareConfig>::iterator it = middleWareConfigInfo.begin(); it != middleWareConfigInfo.end(); it++)
    {
        if(it->second.m_uMiddleWareType == 1 )      //1ï¼šMQ_c2s_io
        {
            mwconf = it->second;
            break;
        }
    }
    printf("InitMQIORebackConsumerThread ip[%s], port[%d], mqusername[%s], mqpwd[%s]\n", mwconf.m_strHostIp.c_str(),
        mwconf.m_uPort, mwconf.m_strUserName.c_str(), mwconf.m_strPassWord.c_str());

    g_pMqioRebackConsumerThread = new CMQIORebackConsumerThread("MQIORebackConsThd");
    if(NULL == g_pMqioRebackConsumerThread)
    {
        printf("InitMQIORebackConsumerThread new is failed!\n");
        return false;
    }
    if (false == g_pMqioRebackConsumerThread->Init(mwconf.m_strHostIp,mwconf.m_uPort,mwconf.m_strUserName,mwconf.m_strPassWord))
    {
        printf("InitMQIORebackConsumerThread init is failed!\n");
        return false;
    }

    if (false == g_pMqioRebackConsumerThread->CreateThread())
    {
        printf("InitMQIORebackConsumerThread create is failed!\n");
        return false;
    }
    return true;
}

void SendMonitorInfo()
{
    CThreadMonitor::ThreadMonitorAdd( g_pLogThread, "LogQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pDisPathDBThreadPool, "DisPathDBQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRuleLoadThread, "ruleLoadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAlarmThread, "alarmQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pAuditThread, "AuditThreadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pOverRateThread, "OverRateThreadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQDBAuditProducerThread, "mqDbAuditProduceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQIOProducerThread, "mqIOProduceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQSendIOProducerThread, "mqSendIOProduceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqioConsumerThread, "mqioConsumerProduceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pGetChannelMqSizeThread, "getChannelMqSizeQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqAuditConsumerThread, "mqAuditConsumerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMQC2SDBProducerThread, "mqC2SDBProduceQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pSessionThread, "CSessionThreadQueueSize" );
        CThreadMonitor::ThreadMonitorAdd( g_pMqOldNoPriQueueDataTransferThread, "MQOldNoPriQueueTransfer" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqioRebackConsumerThread, "MQIORebackConsumerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pMqChannelUnusualConsumerThread, "MQChannelUnusualConsumerQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pTemplateThread, "CTemplateThreadQueueSize" );
    CThreadMonitor::ThreadMonitorAdd( g_pRouterThread, "CRouterThreadQueueSize" );

    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pRedisThreadPool[i]) {
            CThreadMonitor::ThreadMonitorAdd(g_pRedisThreadPool[i], "RedisPoolQueue-" + Comm::int2str(i));
        }
    }
    for(UInt32 i = REDIS_COMMON_USE_NO_CLEAN_0; i < REDIS_USE_INDEX_MAX; i++ )
    {
        if (g_pOverrateRedisThreadPool[i]) {
            CThreadMonitor::ThreadMonitorAdd(g_pOverrateRedisThreadPool[i], "OverrateRedisPoolQueue-" + Comm::int2str(i));
        }
    }
    CThreadMonitor::ThreadMonitorRun();

}

int main()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfigure());
    INIT_CHK_FUNC_RET_FALSE(InitLogThread());

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRuleLoadThread());


    map<string, ComponentConfig> componentConfigInfoMap;
    g_pRuleLoadThread->getComponentConfig(componentConfigInfoMap);
    string s = Fmt<64>("%lu%s", g_uComponentId, COMPONENT_TYPE_ACCESS.data());
    map<string, ComponentConfig>::iterator itcomconf = componentConfigInfoMap.find(s);
    if (itcomconf == componentConfigInfoMap.end())
    {
        printf("g_uComponentId_type[%s] not find in componentConfigInfoMap.\n", s.data());
        MAIN_INIT_OVER
        usleep(1*1000*1000);
        return -1;
    }

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitDispathDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool(g_pRedisThreadPool,
                                        sizeof(g_pRedisThreadPool) / sizeof(g_pRedisThreadPool[0]),
                                        MIDWARE_TYPE_REDIS,
                                        itcomconf->second.m_uRedisThreadNum));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitRedisThreadPool(g_pOverrateRedisThreadPool,
                                        sizeof(g_pOverrateRedisThreadPool) / sizeof(g_pOverrateRedisThreadPool[0]),
                                        MIDWARE_TYPE_REDIS_OVERRATE,
                                        itcomconf->second.m_uRedisThreadNum));

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitAlarmThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(startTemplateThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(startRouterThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(startAuditThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(startOverRateThread());
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(startSessionThread());

    map<UInt32,MiddleWareConfig> middleWareConfigInfo;
    g_pRuleLoadThread->getMiddleWareConfig(middleWareConfigInfo);
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQC2SDBProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQDBAuditProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQSendIOProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQIOProducerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMonitorMQProducerThread(middleWareConfigInfo, itcomconf->second));

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitGetChannelMqSizeThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQIOConsumerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQAuditConsumerThread(middleWareConfigInfo));


    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQIORebackConsumerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQChannelUnusualConsumerThread(middleWareConfigInfo));
    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(InitMQOldNoPriQueueDataTransferConsumer(middleWareConfigInfo));


    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}
