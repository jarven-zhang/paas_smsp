#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "CRuleLoadThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CAlarmThread.h"


extern DefaultConfigManager*            g_pDefaultConfigManager;
extern CRuleLoadThread*                 g_pRuleLoadThread;
extern CDBThreadPool*                   g_pDisPathDBThreadPool;
extern CLogThread*                      g_pLogThread;
extern CAlarmThread*                    g_pAlarmThread;

extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;

#endif
