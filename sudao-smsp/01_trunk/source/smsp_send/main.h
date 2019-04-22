#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "CHTTPSendThread.h"
#include "CDirectSendThread.h"
#include "CDeliveryReportThread.h"

#include "CRuleLoadThread.h"
#include "CAlarmThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CSgipReportThread.h"
#include "CDirectMoThread.h"

#include "CMQConsumerThread.h"
#include "CMQChannelConsumerThread.h"
#include "CMQProducerThread.h"
#include "CMQReportConsumerThread.h"
#include "CChannelTxThreadPool.h"
#include "CChannelTxFlowCtr.h"
#include "CDBMQProducerThread.h"
#include "CMonitorMQProducerThread.h"


#define SEND_COMPONENT_SLIDER_WINDOW
#define RULELOAD_GET_MQ_CONFIG
#define RULELOAD_GET_COMPONENT_SWITCH

class DefaultConfigManager;
class CHTTPSendThread;
class CDirectSendThread;
class CDeliveryReportThread;

class CRuleLoadThread;
class CAlarmThread;
class CHostIpThread;
class CDBThread;
class CRedisThread;
class CRedisThreadPool;
class CLogThread;
class CSgipReportThread;
class CDirectMoThread;


class CMQProducerThread;
class CMQChannelConsumerThread;




extern DefaultConfigManager*                g_pDefaultConfigManager;
extern CHTTPSendThread*                     g_pHTTPSendThread;
//extern CDirectSendThread*                   g_pDirectSendThread;
extern CDeliveryReportThread*               g_pDeliveryReportThread;

extern CRuleLoadThread*                     g_pRuleLoadThread;
extern CAlarmThread*                        g_pAlarmThread;
extern CHostIpThread*                       g_pHostIpThread;
extern CDBThread*                           g_pDBThread;
extern CDBThreadPool*					    g_pDisPathDBThreadPool; ////dispath DB
extern CRedisThreadPool*                    g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CRedisThreadPool*                    g_pStatRedisThreadPool;
extern CLogThread*                          g_pLogThread;
extern CSgipReportThread*                   g_pSgipReportThread;
extern CDirectMoThread*                     g_pDirectMoThread;


extern CMQProducerThread*               	g_pRecordMQProducerThread;
extern CMQProducerThread*               	g_pMqAbIoProductThread;
extern CMQConsumerThread*           		g_pMqChannelConsumerThread; 
extern CMQProducerThread*               	g_pReportMQProducerThread;
extern CMQConsumerThread*                 	g_pReportMQConsumerThread;

extern CChannelThreadPool* 				    g_CChannelThreadPool;
extern CMonitorMQProducerThread*            g_pMQMonitorPublishThread;

extern string g_strMQReportExchange;
extern string g_strMQReportRoutingKey;
extern string g_strMQDBExchange;
extern string g_strMQDBRoutingKey;
extern unsigned int g_uSecSleep;
extern string g_strMQIOExchange;
extern string g_strMQIORoutingKey;

extern UInt64 g_uComponentId;

#endif
/*__MAIN_H__*/
