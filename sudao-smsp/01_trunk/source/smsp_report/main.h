#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"

#include "ReportGetThread.h"
#include "ReportReceiveThread.h"
#include "CDispatchThread.h"
#include "CChargeThread.h"
#include "CRuleLoadThread.h"

#include "CAlarmThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "GetReportServerThread.h"
#include "GetBalanceServerThread.h"
#include "CMQProducerThread.h"
#include "CQueryReportServerThread.h"
#include "CMQReportConsumerThread.h"
#include "CDBMQProducerThread.h"
#include "CMonitorMQProducerThread.h"


class DefaultConfigManager;
class ReportReceiveThread;
class CDispatchThread;
class CHTTPSendThread;
class CChargeThread;
class CRuleLoadThread;
class CAlarmThread;
class CReportPushThread;

class CHostIpThread;
class CDBThread;
class CDBThreadPool;
class CRedisThread;
class CRedisThreadPool;
class CLogThread;
class ReportGetThread;
class GetReportServerThread;
class GetBalanceServerThread;
class CMQProducerThread;
class CQueryReportServerThread;

#define RULELOAD_GET_MQ_CONFIG
#define RULELOAD_GET_COMPONENT_SWITCH


extern DefaultConfigManager*                g_pDefaultConfigManager;
extern ReportReceiveThread*                 g_pReportReceiveThread;
extern CDispatchThread*                     g_pDispatchThread;
extern CHTTPSendThread*                     g_pHTTPSendThread;
extern CChargeThread*                       g_pChargeThread;
extern CRuleLoadThread*                     g_pRuleLoadThread;
extern CAlarmThread*                        g_pAlarmThread;
extern CHostIpThread*                       g_pHostIpThread;
extern CDBThread*                           g_pDBThread;
extern CDBThreadPool*						g_pDisPathDBThreadPool; ////dispath DB
extern CDBThreadPool*                       g_pRunDBThreadPool;
extern CRedisThreadPool*                    g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CLogThread*                          g_pLogThread;
extern ReportGetThread*                     g_pReportGetThread;
extern GetReportServerThread*               g_pGetReportServerThread;
extern GetBalanceServerThread*              g_pGetBalanceServerThread;
extern CQueryReportServerThread*            g_pQueryReportServerThread;

extern CMQProducerThread*               	g_pMQDbProducerThread;
extern CMQProducerThread*               	g_pMqAbIoProductThread;
extern CMQProducerThread*               	g_pRtAndMoProducerThread;
extern CMQProducerThread*               	g_pReportMQProducerThread;
extern CMQConsumerThread*                 	g_pReportMQConsumerThread;
extern CMonitorMQProducerThread*           	g_pMQMonitorPublishThread;

extern string g_strMQDBExchange;
extern string g_strMQDBRoutingKey;
extern string g_strMQReportExchange;
extern string g_strMQReportRoutingKey;
extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;
#endif
/*__MAIN_H__*/
