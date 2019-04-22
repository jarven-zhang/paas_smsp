#ifndef  __MAIN_H__
#define  __MAIN_H__

#include <unistd.h>
#include "defaultconfigmanager.h"
#include "CMPPServiceThread.h"
#include "CMPP3ServiceThread.h"
#include "SMPPServiceThread.h"
#include "SMGPServiceThread.h"
#include "SGIPServiceThread.h"
//#include "CHTTPSendThread.h"
#include "ReportReceiveThread.h"
#include "CRuleLoadThread.h"
#include "CHostIpThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "CRedisThread.h"
#include "CRedisThreadPool.h"
#include "CLogThread.h"
#include "CAlarmThread.h"
#include "CUnityThread.h"
#include "SGIPRtAndMoThread.h"
#include "RptMoGetThread.h"

#include "CMQConsumerThread.h"
#include "CMQIOConsumerThread.h"
#include "CMQProducerThread.h"
#include "CDBMQProducerThread.h"
#include "UTFString.h"
#include "ErrorCode.h"
#include "CMonitorMQProducerThread.h"


#define RULELOAD_GET_MQ_CONFIG
//#define RULELOAD_GET_COMPONENT_SWITCH

class DefaultConfigManager;
class CMPP3ServiceThread;
class CMPPServiceThread;
class SMPPServiceThread;
class SMGPServiceThread;
class CSgipRtAndMoThread;
class SGIPServiceThread;
class CHTTPSendThread;
class ReportReceiveThread;
class CRuleLoadThread;
class CHostIpThread;
class CDBThread;
class CRedisThreadPool;
class CLogThread;
class CAlarmThread;
class CHttpServiceThread;
class CReportPushThread;
class CAuditGetThread;
class CUnityThread;
class CSendToSmspSendThread;
class RptMoGetThread;


/* BEGIN: Added by fxh, 2016/10/14   PN:RabbitMQ */
class CMQDBConsumerThread;
class CMQIOConsumerThread;
class CMQProducerThread;
class CSqlFileThread;
class CMQIOConsumerThread;
class CMQIOConsumerThread;
class CMQIOConsumerThread;
class CMQIOConsumerThread;
class CMQIOConsumerThread;
class CMQIOConsumerThread;

/* END:   Added by fxh, 2016/10/14   PN:RabbitMQ */

extern DefaultConfigManager*            g_pDefaultConfigManager;
extern CMPP3ServiceThread*              g_pCMPP3ServiceThread;
extern CMPPServiceThread*               g_pCMPPServiceThread;
extern SMPPServiceThread*               g_pSMPPServiceThread;
extern SMGPServiceThread*               g_pSMGPServiceThread;
extern CSgipRtAndMoThread*              g_pSgipRtAndMoThread;
extern SGIPServiceThread*               g_pSGIPServiceThread;
extern ReportReceiveThread*             g_pReportReceiveThread;
extern CRuleLoadThread*                 g_pRuleLoadThread;
extern CDBThreadPool*                   g_pDisPathDBThreadPool; ////dispath DB
extern CDBThreadPool*                   g_pDBThreadPool;    ////running DB
extern CRedisThreadPool*                g_pRedisThreadPool[REDIS_USE_INDEX_MAX];
extern CRedisThreadPool*                g_pStatRedisThreadPool;
extern CLogThread*                      g_pLogThread;
extern CAlarmThread*                    g_pAlarmThread;
extern CHttpServiceThread*              g_pHttpServiceThread;
extern CReportPushThread*               g_pReportPushThread;
extern CUnityThread*                    g_pUnitypThread;
extern RptMoGetThread*                  g_pRptMoGetThread;


//extern CMQProducerThread*               g_pMQDBProducerThread;
//extern CMQProducerThread*               g_pMQC2sIOProducerThread;

//extern CMQConsumerThread*             	g_pMqioConsumerThread;

extern CMonitorMQProducerThread*        g_pMQMonitorPublishThread;


extern string g_strMQDBExchange;
extern string g_strMQDBRoutingKey;
extern unsigned int g_uSecSleep;
extern UInt64 g_uComponentId;
//extern bool   g_bAllThreadOK;

#endif
/*__MAIN_H__*/
