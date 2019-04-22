#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "ReportReceiveThread.h"
#include "CRuleLoadThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CAlarmThread.h"
#include "CHttpServiceThread.h"
#include "CReportPushThread.h"
#include "CMQConsumerThread.h"
#include "CMQIOConsumerThread.h"
#include "CMQProducerThread.h"
#include "CDBMQProducerThread.h"
#include "CMonitorMQProducerThread.h"
#include "UTFString.h"
#include "ErrorCode.h"

#define RULELOAD_GET_MQ_CONFIG
#define SMSP_HTTP

class DefaultConfigManager;
class ReportReceiveThread;
class CRuleLoadThread;
class CHostIpThread;
class CDBThread;
class CRedisThreadPool;
class CLogThread;
class CAlarmThread;
class CHttpServiceThread;
class CReportPushThread;

class CMQIOConsumerThread;
class CMQProducerThread;

class CMQIOConsumerThread;
class CMonitorMQProducerThread;

extern DefaultConfigManager            *g_pDefaultConfigManager;
extern ReportReceiveThread             *g_pReportReceiveThread;
extern CRuleLoadThread                 *g_pRuleLoadThread;
extern CHostIpThread                   *g_pHostIpThread;
extern CDBThreadPool                   *g_pDisPathDBThreadPool; ////dispath DB
extern CDBThreadPool                   *g_pRunDBThreadPool;
extern CDBThreadPool                   *g_pDBThreadPool;    ////running DB
extern CRedisThreadPool                *g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CRedisThreadPool                *g_pStatRedisThreadPool;
extern CLogThread                      *g_pLogThread;
extern CAlarmThread                    *g_pAlarmThread;
extern CHttpServiceThread              *g_pHttpServiceThread;
extern CReportPushThread               *g_pReportPushThread;

extern CMQProducerThread               *g_pMQDBProducerThread;
extern CMQProducerThread               *g_pMQC2sIOProducerThread;

extern CMQConsumerThread               *g_pMqioConsumerThread;

extern CMonitorMQProducerThread        *g_pMQMonitorPublishThread;

extern string g_strMQDBExchange;
extern string g_strMQDBRoutingKey;
extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;

#endif
/*__MAIN_H__*/
