#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>

#include "defaultconfigmanager.h"
#include "CRuleLoadThread.h"
#include "CAlarmThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CMQConsumerThread.h"
#include "CMQDBConsumerThread.h"
#include "CSqlFileThread.h"
#include "CDBMQProducerThread.h"
#include "CBigDataThread.h"

class DefaultConfigManager;
class CRuleLoadThread;
class CAlarmThread;
class CHostIpThread;
class CDBThread;
class CDBThreadPool;
class CLogThread;
class CMQDBConsumerThread;
class CSqlFileThread;

#define RULELOAD_GET_MQ_CONFIG
#define RULELOAD_GET_COMPONENT_SWITCH

extern DefaultConfigManager*                g_pDefaultConfigManager;
extern CRuleLoadThread*                     g_pRuleLoadThread;
extern CAlarmThread*                        g_pAlarmThread;
extern CDBThread*                           g_pDBThread;
extern CDBThreadPool*                       g_pDBThreadPool;
extern CDBThreadPool*                      	g_pDisPathDBThreadPool;
extern CRedisThreadPool*                    g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CLogThread*                          g_pLogThread;
extern CDBMQProducerThread*                 g_pMqSendDbProducerThread;
extern CMQAckConsumerThread*             	g_pMQDBConsumerThread;
extern CSqlFileThread*                  	g_pSqlFileThread;
extern CBigDataThread*                      g_pBigDataThread;

extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;

#ifndef SMSP_CONSUMER
#define SMSP_CONSUMER
#endif

#endif
/*__MAIN_H__*/
