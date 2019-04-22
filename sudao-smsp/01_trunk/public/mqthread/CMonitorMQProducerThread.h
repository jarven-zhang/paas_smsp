/******************************************************************************

  Copyright (C), 2001-2011, DCN Co., Ltd.

 ******************************************************************************
  File Name     : CDBMQProducerThread.h
  Version       : Initial Draft
  Author        : ljl
  Created       : 2017/10/19
  Last Modified :
  Description   : DB rabbitMQ Producer Thread
  Function List :
  History       :
  
******************************************************************************/

#ifndef __CMONITORMQPRODUCERTHREAD__
#define __CMONITORMQPRODUCERTHREAD__

#include <vector>
#include "CMQProducerThread.h"
#include "ComponentRefMq.h"
#include "ComponentConfig.h"
#include "MqConfig.h"
#include "CUDPSendThread.h"
#include "MonitorMeasure.h"

class TMonitorMQPublishMsg : public TMsg
{
	public:
		UInt32                m_strTableType;             
		KeyValMap      		  m_monitorInfo;
};

class TMonitorComponentConfMsg : public TMsg
{
public:
	TMonitorComponentConfMsg( ComponentConfig &comp )
	{
		this->m_componentConfig = comp ;	
		this->m_iMsgType = MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ;
	}
public:
	ComponentConfig       m_componentConfig;
};

class CMonitorMQProducerThread:public CMQProducerThread
{ 

public:
    CMonitorMQProducerThread(const char *name);
    ~CMonitorMQProducerThread();

	bool Init(string strIp, unsigned int uPort, string strUserName, 
			string strPassWord, ComponentConfig &componentConfig );
    void HandleMsg(TMsg* pMsg);
	string GetMonitorTableName( UInt32 tableType );
	bool  CheckAndEncodeData( TMsg* pMsg, string &strEncodeData );
	bool  GetSysParam( std::map<std::string, std::string> &sysParamMap, bool bInit );
	void  HandleCompentUpdate( TMsg* pMsg );
	bool  InitMonitorMQConnect( string strIp, unsigned int uPort, string strUserName, string strPassWord );
	bool  Encode( TMsg* pMsg,	string &strEncodeData );
	bool  HandlePublishMsg( TMsg* pMsg );

private:
	ComponentConfig m_ComponentConfig;  /* 组件配置信息*/		
	string          m_strExchange;
	string          m_strRoutingKey;
	CMonitorConf    m_monitorCnf;       /*系统监控配置加载*/
	MonitorMeasure  m_monitorMeasure;
	
};


#define MONITOR_INIT(t) TMonitorMQPublishMsg *pMonitorPushlish = new TMonitorMQPublishMsg();\
	                                          pMonitorPushlish->m_strTableType = t;\
	                                          pMonitorPushlish->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;\


#define MONITOR_TABLE_SET(t)  do{if(pMonitorPushlish){pMonitorPushlish->m_strTableType = t;}}while(0)


#define MONITOR_VAL_SET( k, v ) do{if(pMonitorPushlish){ pMonitorPushlish->m_monitorInfo[ k ] = v;}}while(0)

#define MONITOR_VAL_SET_INT( k, v ) do{if(pMonitorPushlish){pMonitorPushlish->m_monitorInfo[ k ] = Comm::int2str(v);}}while(0)

#define MONITOR_VAL_SET_FLOAT( k, v ) do{if(pMonitorPushlish){ \
										pMonitorPushlish->m_monitorInfo[ k ] = Comm::float2str(v);\
									}}while(0)

#define MONITOR_PUBLISH(thread ) do{thread->PostMsg(pMonitorPushlish);}while(0)

#define MONITOR_UPDATE_COMP( thread, comp ) do{ if( !thread ) break; \
	                                        TMonitorComponentConfMsg *pMsg = new TMonitorComponentConfMsg(comp);\
											thread->PostMsg(pMsg);}while(0);

#define MONITOR_UPDATE_SYS( thread, param ) do{ if(!thread )break; \
											TUpdateSysParamRuleReq *pMsg = new TUpdateSysParamRuleReq();\
											pMsg->m_iMsgType = MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ;\
											pMsg->m_SysParamMap = param;\
											thread->PostMsg(pMsg);}while(0);
#endif /////__CMQPRODUCERTHREAD__

