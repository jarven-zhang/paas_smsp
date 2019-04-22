#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "CSessionThread.h"
#include "CRuleLoadThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CAlarmThread.h"
#include "CAuditThread.h"
#include "CMQConsumerThread.h"
#include "CMQAuditConsumerThread.h"
#include "CMQIOConsumerThread.h"
#include "CMQProducerThread.h"
#include "CGetChannelMqSizeThread.h"
#include "CMonitorMQProducerThread.h"
#include "CDBMQProducerThread.h"
#include "LogMacro.h"
#include "CommClass.h"
#include "CMQChannelUnusualConsumerThread.h"
#include "CMQOldNoPriQueueDataTransferConsumer.h"
#include "CMQIORebackConsumerThread.h"


#define RULELOAD_GET_MQ_CONFIG
#define RULELOAD_GET_COMPONENT_SWITCH
class CAlarmThread;
class CLogThread;
class CRuleLoadThread;

extern DefaultConfigManager*            g_pDefaultConfigManager;
extern CRuleLoadThread*                 g_pRuleLoadThread;
extern CDBThreadPool*                   g_pDisPathDBThreadPool; ////dispath DB
extern CRedisThreadPool*                g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CRedisThreadPool*                g_pOverrateRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CRedisThreadPool*                g_pStatRedisThreadPool;
extern CAlarmThread*                    g_pAlarmThread;
extern CMQProducerThread*               g_pMQDBAuditProducerThread;
extern CMQProducerThread*               g_pMQSendIOProducerThread;
extern CMQProducerThread*               g_pMQIOProducerThread;
extern CMQConsumerThread*             	g_pMqioConsumerThread;
extern CGetChannelMqSizeThread*         g_pGetChannelMqSizeThread;
extern CMQAckConsumerThread*            g_pMqAuditConsumerThread;
extern CMonitorMQProducerThread*        g_pMQMonitorProducerThread;
extern CMQProducerThread*               g_pMQC2SDBProducerThread;

extern CMQConsumerThread*             	g_pMqioRebackConsumerThread;
extern CMQChannelUnusualConsumerThread* g_pMqChannelUnusualConsumerThread;
extern CMQOldNoPriQueueDataTransferConsumer*   	g_pMqOldNoPriQueueDataTransferThread;



extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;


#define MIDWARE_TYPE_REDIS					0
#define MIDWARE_TYPE_REDIS_OVERRATE			6

#endif
/*__MAIN_H__*/
