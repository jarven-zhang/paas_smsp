#include "main.h"
#include "defaultconfigmanager.h"
#include "propertyutils.h"
#include "Version.h"
#include "CThreadMonitor.h"
#include "ssl/md5.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "ListenPort.h"

#include "HttpServerThread.h"
#include "ChargeThread.h"
#include "DirectChargeThread.h"
#include "RuleLoadThread.h"



DefaultConfigManager*               g_pDefaultConfigManager;
CDBThreadPool*                      g_pDisPathDBThreadPool;
CLogThread*                         g_pLogThread;
CAlarmThread*                       g_pAlarmThread;

UInt32 g_uSecSleep = 0;
UInt64 g_uComponentId = 0;


using namespace std;

void handler(int sig)
{
    int dbQueueSize = g_pDisPathDBThreadPool->GetMsgSize();
    int httpServerQueueSize = g_pHttpServiceThread->GetMsgSize();
    int httpServerSessionSize = g_pHttpServiceThread->GetSessionMapSize();
    int chargeQueueSize = g_pChargeThread->GetMsgSize();
    int chargeSessionSize = g_pChargeThread->GetSessionMapSize();


    while (dbQueueSize != 0
        || httpServerQueueSize != 0
        || httpServerSessionSize != 0
        || chargeQueueSize != 0
        || chargeSessionSize != 0)
    {
        static int count = 0;
        count++;

        if (count == 500)
        {
            LogNotice("killing process, queue|session: dbThread[%d]|[0], httpServerThread[%d]|[%d], chargeThread[%d]|[%d]",
                dbQueueSize,
                httpServerQueueSize,
                httpServerSessionSize,
                chargeQueueSize,
                chargeSessionSize);

            count = 0;
        }

        usleep(1000 * 10);  //10ms

        dbQueueSize = g_pDisPathDBThreadPool->GetMsgSize();
        httpServerQueueSize = g_pHttpServiceThread->GetMsgSize();
        httpServerSessionSize = g_pHttpServiceThread->GetSessionMapSize();
        chargeQueueSize = g_pChargeThread->GetMsgSize();
        chargeSessionSize = g_pChargeThread->GetSessionMapSize();
    }

    LogNotice("kill process succced! queue|session: dbThread[%d]|[0], httpServerThread[%d]|[%d], chargeThread[%d]|[%d]",
        dbQueueSize,
        httpServerQueueSize,
        httpServerSessionSize,
        chargeQueueSize,
        chargeSessionSize);

    cout << "capture a sinterm signal, kill process.\r\n" << endl;
    sleep(1);   //1s
    exit(0);
}

bool LoadConfigure()
{
    g_pDefaultConfigManager = new DefaultConfigManager();
    INIT_CHK_NULL_RET_FALSE(g_pDefaultConfigManager);
    INIT_CHK_FUNC_RET_FALSE(g_pDefaultConfigManager->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pDefaultConfigManager->LoadConf());

    PropertyUtils::GetValue("usec_sleep", g_uSecSleep);
    PropertyUtils::GetValue("component_id", g_uComponentId);

    if (g_uSecSleep <= 0 || g_uSecSleep >= 1000 * 1000)
    {
        LogErrorP("g_uSecSleep[%d] is error. should be [1,1000*1000)", g_uSecSleep);
        return false;
    }

    LogNoticeP("===***===usec_sleep:%u.", g_uSecSleep);
    LogNoticeP("===***===component_id:%u.", g_uComponentId);

    return true;
}

bool InitLogThread()
{
    std::string prefixname = "smsp_charge";
    std::string logpath = g_pDefaultConfigManager->GetPath("logs");

    g_pLogThread = new CLogThread("LogThread");
    INIT_CHK_NULL_RET_FALSE(g_pLogThread);
    INIT_CHK_FUNC_RET_FALSE(g_pLogThread->Init(prefixname, logpath));
    INIT_CHK_FUNC_RET_FALSE(g_pLogThread->CreateThread());

    string strVersin = GetVersionInfo();
    LogNotice("\r\n%s", strVersin.data());
    LogNotice("LogThread ! name:%s|logpath:%s", prefixname.data(), logpath.data());
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

    g_pRuleLoadThread = new CRuleLoadThread("RuleLoadThread");
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThread);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname));
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThread->CreateThread());

    g_pRuleLoadThreadV2 = new RuleLoadThread(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname);
    INIT_CHK_NULL_RET_FALSE(g_pRuleLoadThreadV2);
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pRuleLoadThreadV2->CreateThread());

    return true;
}

bool InitAlarmThread()
{
    g_pAlarmThread = new CAlarmThread("AlarmThread");
    INIT_CHK_NULL_RET_FALSE(g_pAlarmThread);
    INIT_CHK_FUNC_RET_FALSE(g_pAlarmThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pAlarmThread->CreateThread());

    return true;
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
    PropertyUtils::GetValue("dispath_db_thread_num", Dis_dbThreadNum);

    g_pDisPathDBThreadPool = new CDBThreadPool("DBThreads", Dis_dbThreadNum);
    INIT_CHK_NULL_RET_FALSE(g_pDisPathDBThreadPool);
    INIT_CHK_FUNC_RET_FALSE(g_pDisPathDBThreadPool->Init(Dis_dbhost, Dis_dbport, Dis_dbuser, Dis_dbpwd, Dis_dbname));

    return true;
}

void SendMonitorInfo()
{
    CThreadMonitor::ThreadMonitorAdd(g_pLogThread, "LogQueueSize");
    CThreadMonitor::ThreadMonitorAdd(g_pRuleLoadThread, "RuleLoadQueueSize");
    CThreadMonitor::ThreadMonitorAdd(g_pAlarmThread, "AlarmQueueSize");
    CThreadMonitor::ThreadMonitorAdd(g_pHttpServiceThread, "HttpServerQueueSize");
    CThreadMonitor::ThreadMonitorAdd(g_pChargeThread, "ChargeThreadQueueSize");
    CThreadMonitor::ThreadMonitorAdd(g_pDirectChargeThread, "DirectChargeThreadQueueSize");

    for (UInt32 i = 0; i < g_pDisPathDBThreadPool->getThreadNum() ; ++i)
    {
        CThreadMonitor::ThreadMonitorAdd(g_pDisPathDBThreadPool->getThread(i), "DBThreadQueueSize");
    }

    CThreadMonitor::ThreadMonitorRun();
}

bool initmain()
{
    INIT_CHK_FUNC_RET_FALSE(LoadConfigure());
    INIT_CHK_FUNC_RET_FALSE(InitLogThread());
    INIT_CHK_FUNC_RET_FALSE(InitRuleLoadThread());
    INIT_CHK_FUNC_RET_FALSE(InitDispathDBThreadPool());
    INIT_CHK_FUNC_RET_FALSE(InitAlarmThread());

    INIT_CHK_FUNC_RET_FALSE(ChargeThread::start());
    INIT_CHK_FUNC_RET_FALSE(DirectChargeThread::start());
    INIT_CHK_FUNC_RET_FALSE(HttpServerThread::start());

    return true;
}

int main()
{
    signal(SIGTERM, handler);

    INIT_CHK_FUNC_RET_FALSE_AND_NOTIFY(initmain());

    MAIN_INIT_OVER

    SendMonitorInfo();
    return 0;
}

