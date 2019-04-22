#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "CAuditServiceThread.h"
#include "CRuleLoadThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CAlarmThread.h"
#include "CMQConsumerThread.h"
#include "CMQProducerThread.h"

#define RULELOAD_GET_MQ_CONFIG
#define RULELOAD_GET_COMPONENT_SWITCH


class DefaultConfigManager;
class CAuditServiceThread;
class CRuleLoadThread;
class CHostIpThread;
class CLogThread;
class CAlarmThread;



extern DefaultConfigManager*               g_pDefaultConfigManager;
extern CAuditServiceThread*                g_pAuditServiceThread;
extern CRuleLoadThread*                    g_pRuleLoadThread;
extern CDBThreadPool*                      g_pDisPathDBThreadPool;
extern CRedisThreadPool*                   g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CLogThread*                         g_pLogThread;
extern CAlarmThread*                       g_pAlarmThread;
extern CMQConsumerThread*             	   g_pMqdbConsumerThread;
extern CMQProducerThread*                  g_pMqC2sDbProducerThread;


extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;
#endif
/*__MAIN_H__*/
