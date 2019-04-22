#include "CMonitorMQProducerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "ssl/md5.h"
#include "Comm.h"
#include "base64.h"

using namespace std;

CMonitorMQProducerThread::CMonitorMQProducerThread( const char *name ):CMQProducerThread(name)
{
	m_monitorCnf.bSysSwi = false;
	m_monitorCnf.bUseMQSwi = true;
	m_ComponentConfig.m_uMonitorSwitch = 1;
}

CMonitorMQProducerThread::~CMonitorMQProducerThread()
{
}

bool CMonitorMQProducerThread::InitMonitorMQConnect( string strIp, unsigned int uPort, string strUserName, string strPassWord )
{	
	bool ret = false;	
	
	map<string,ComponentRefMq> componentRefMqInfo;	
	map<UInt64,MqConfig> mqConfigInfo;
	g_pRuleLoadThread->getComponentRefMq(componentRefMqInfo);
	g_pRuleLoadThread->getMqConfig(mqConfigInfo);
	
	CMQProducerThread::Init( strIp, uPort,strUserName,strPassWord );
	
	map<string,ComponentRefMq>::iterator itr =	componentRefMqInfo.begin();
	for(; itr != componentRefMqInfo.end() ; itr++)
	{
		if( g_uComponentId == itr->second.m_uComponentId 
			&& PRODUCT_MODE == itr->second.m_uMode
			&& ( MESSAGE_TYPE_MONITOR_C2S_HTTP == itr->second.m_strMessageType 
				||MESSAGE_TYPE_MONITOR_ACCESS == itr->second.m_strMessageType
				||MESSAGE_TYPE_MONITOR_SEND_REPORT == itr->second.m_strMessageType
				||MESSAGE_TYPE_MONITOR_REBACK == itr->second.m_strMessageType
				||MESSAGE_TYPE_MONITOR_CHARGE == itr->second.m_strMessageType
				||MESSAGE_TYPE_MONITOR_AUDIT == itr->second.m_strMessageType
				||MESSAGE_TYPE_MONITOR_MID	== itr->second.m_strMessageType))
		{	
			map<UInt64,MqConfig>::iterator itrMq = mqConfigInfo.find( itr->second.m_uMqId );
			if ( itrMq == mqConfigInfo.end())
			{
				printf("==except== mqid:%lu is not find in m_mqConfigInfo.\n", itr->second.m_uMqId);
				LogWarn("==except== mqid:%lu is not find in m_mqConfigInfo.", itr->second.m_uMqId );
			}
			else
			{
				m_strExchange = itrMq->second.m_strExchange;
				m_strRoutingKey = itrMq->second.m_strRoutingKey;
				if( !m_strExchange.empty() && !m_strRoutingKey.empty())
				{					
					ret = true;
				}				
			}
			break;
		}	
	}

	return ret ;

}

bool CMonitorMQProducerThread::Init( string strIp, unsigned int uPort, 
				string strUserName, string strPassWord, ComponentConfig &componentConfig )
{
	map<string, string> sysParam;
	m_ComponentConfig = componentConfig;
	
	g_pRuleLoadThread->getSysParamMap( sysParam );		
	if( !GetSysParam( sysParam , true ))
	{
		LogError("get sys param error");
		return false;
	}

	/*初始化MQ连接*/
	InitMonitorMQConnect( strIp, uPort, strUserName, strPassWord );

	if( true == m_monitorCnf.bUseMQSwi 
		&& true == m_monitorCnf.bSysSwi 
		&& m_ComponentConfig.m_uMonitorSwitch == 1  )
	{
		if( !m_linkState || !RabbitMQConnect())
		{
			LogWarn("InitMonitorMQConnect strIp:%s, uPort:%u, strUsername:%s Error", 
					strIp.data(), uPort, strUserName.data());
			return false;
		}	
	}

	vector < MonitorDetailOption > vecOption;
	vecOption.clear();
	m_monitorMeasure.Init( vecOption );

	return true;	
	
}

/* 封装标准格式数据, 通过Monitor统一处理*/
bool CMonitorMQProducerThread::Encode( TMsg* pMsg,  string &strEncodeData )
{	
	TMonitorMQPublishMsg *pPublishMsg = ( TMonitorMQPublishMsg * ) pMsg;	

	/*key/val*/
	strEncodeData.append("measurement=");
	strEncodeData.append(Comm::int2str(pPublishMsg->m_strTableType));
	
	KeyValMap::iterator itr = pPublishMsg->m_monitorInfo.begin(); 
	for(;itr != pPublishMsg->m_monitorInfo.end(); itr++ )
	{
		if(!itr->second.empty())
		{
			strEncodeData.append("&");
			strEncodeData.append(itr->first);
			strEncodeData.append("=");
			strEncodeData.append(itr->second);
		}
	}
	return true;

}

bool CMonitorMQProducerThread::HandlePublishMsg( TMsg* pMsg )
{	
	TMonitorMQPublishMsg *pPublishMsg = ( TMonitorMQPublishMsg * ) pMsg;

	if( NULL == pPublishMsg ){
		LogError("Param Error");
		return false;
	}

	/*检查系统开关和组件开关*/
	if ( !m_monitorCnf.bSysSwi || 
		  m_ComponentConfig.m_uMonitorSwitch != 1 )
	{
		LogDebug("[ Monitor ] SysSwi =%s, CompSwi [%lu : %s]", 
				  m_monitorCnf.bSysSwi ? "true":"false",
				  g_uComponentId, m_ComponentConfig.m_uMonitorSwitch == 1 ? "true" : "false" );
		return false;
	}		

	string strData = "";

	/*如果使用MQ模式*/
	if( m_monitorCnf.bUseMQSwi )
	{
		TMQPublishReqMsg pMQ;
		Encode( pMsg, strData );
		pMQ.m_strData = Base64::Encode(strData);
		pMQ.m_strExchange = m_strExchange;
		pMQ.m_strRoutingKey = m_strRoutingKey;			
		HandlePublish( &pMQ );
	}
	else
	{	
		/* 如果不开启MQ 直接使用udp协议发送*/
		if(!m_monitorMeasure.Encode( pPublishMsg->m_strTableType,
					pPublishMsg->m_monitorInfo, strData ))
		{
			LogDebug("[Monitor] Encode Fail [%s]", strData.data());
			return false;
		}

		TUDPSendMsg *pMsg = new TUDPSendMsg();
		pMsg->m_strSendData = strData;
		pMsg->m_iMsgType = MSGTYPE_MONITOR_UDP_SEND;	
		if( false == CUDPSendThread::postSendMsg(pMsg))
		{
			delete((TMsg *) pMsg);
			return false;
		}
	}

	LogNotice("[ Monitor ] PubLishMode=[%s], Measurement=[%s], Data=[%s] ", 
			 m_monitorCnf.bUseMQSwi ? "MQ" : "UDP",
			 m_monitorMeasure.GetMonitorTableName(pPublishMsg->m_strTableType).data(),
			 strData.data());

	return true;

}


bool CMonitorMQProducerThread::GetSysParam( std::map<std::string, std::string> &sysParamMap, bool bInit )
{
	CMonitorConf monitorCnf_old = m_monitorCnf;
	if( true != MonitorMeasure::GetSysParam( sysParamMap, m_monitorCnf )){
		LogWarn("GetSysParam Error .....");
		return false;
	}

	if( bInit == true
		|| monitorCnf_old.bSysSwi != m_monitorCnf.bSysSwi 
		|| monitorCnf_old.bUseMQSwi  != m_monitorCnf.bUseMQSwi )
	{
		if( m_monitorCnf.bSysSwi == true
			&& m_ComponentConfig.m_uMonitorSwitch == 1 
			&& m_monitorCnf.bUseMQSwi == false )
		{
			return CUDPSendThread::gInit( m_monitorCnf.strAddrIp, 
					(UInt32)m_monitorCnf.iAddrPort, (UInt32)m_monitorCnf.iThreadCnt );
		}
		else		
		{
			if( true != bInit ){
				CUDPSendThread::gdeInit();
			}
		}
	}

	return true ;
	
}

void CMonitorMQProducerThread::HandleCompentUpdate( TMsg* pMsg )
{
	TMonitorComponentConfMsg* pCompentConfig = ( TMonitorComponentConfMsg * )pMsg;
	if( m_ComponentConfig.m_uMonitorSwitch != pCompentConfig->m_componentConfig.m_uMonitorSwitch )
	{
		LogNotice("[ monitor ] CompentSwitch Change[ %s ]--->[ %s ]", 
				m_ComponentConfig.m_uMonitorSwitch == 1 ? "true":"false",
				pCompentConfig->m_componentConfig.m_uMonitorSwitch == 1 ? "true":"false" );	

		if( m_monitorCnf.bSysSwi 
			&& pCompentConfig->m_componentConfig.m_uMonitorSwitch == 1 )
		{
			if( !m_monitorCnf.bUseMQSwi )
			{
				CUDPSendThread::gdeInit();
				CUDPSendThread::gInit( m_monitorCnf.strAddrIp, 
						(UInt32)m_monitorCnf.iAddrPort, (UInt32)m_monitorCnf.iThreadCnt );
			}
		}
	}

	m_ComponentConfig = pCompentConfig->m_componentConfig;

}


void CMonitorMQProducerThread::HandleMsg( TMsg* pMsg )
{
    if ( NULL == pMsg )
    {
        LogError("pMsg is NULL.");
        return;
    }
    
    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);

    switch ( pMsg->m_iMsgType )
    {	
        case MSGTYPE_MQ_PUBLISH_REQ:
		{
			HandlePublishMsg( pMsg );
			break;
		}

		/*更新组件配置信息*/
		case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
		{
			HandleCompentUpdate( pMsg );
			break;
		}

		/*更新系统配置信息*/
		case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
		{			
			TUpdateSysParamRuleReq* pSysParamReq = ( TUpdateSysParamRuleReq* )pMsg;
			GetSysParam( pSysParamReq->m_SysParamMap, false );
			break;
		}

        default:
        {
			LogWarn("msgType:0x%x is invalid.",pMsg->m_iMsgType);
            break;
        }		
		
    }
	
}
