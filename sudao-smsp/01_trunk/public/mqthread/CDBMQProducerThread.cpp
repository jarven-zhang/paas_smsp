#include "CDBMQProducerThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "main.h"
#include <sys/syscall.h>
#include "ssl/md5.h"
using namespace std;


CDBMQProducerThread::CDBMQProducerThread(const char *name):CMQProducerThread(name)
{
	
}

CDBMQProducerThread::~CDBMQProducerThread()
{
		
}
bool CDBMQProducerThread::Init(string strIp, unsigned int uPort,string strUserName,string strPassWord)
{
    if(false == CMQProducerThread::Init(strIp,uPort,strUserName,strPassWord))
		return false;
	return MQinfoInit();
}

bool CDBMQProducerThread::MQinfoInit()
{
	m_vDBMQInfo.clear();
	map<string,ComponentRefMq> componentRefMqInfo;
	g_pRuleLoadThread->getComponentRefMq(componentRefMqInfo);
	map<UInt64,MqConfig> mqConfigInfo;
	g_pRuleLoadThread->getMqConfig(mqConfigInfo);
	
	map<string,ComponentRefMq>::iterator itr =  componentRefMqInfo.begin();
	set<UInt64> mqIdSet;
	for(; itr != componentRefMqInfo.end() ; itr++)
	{
		if(g_uComponentId == itr->second.m_uComponentId && 
			MESSAGE_TYPE_DB == itr->second.m_strMessageType && 
			PRODUCT_MODE == itr->second.m_uMode)
		{	
			mqIdSet.insert(itr->second.m_uMqId);
		}
	}
	
	for(set<UInt64>::iterator itor = mqIdSet.begin(); itor != mqIdSet.end(); itor++)
	{
		map<UInt64,MqConfig>::iterator itrMq = mqConfigInfo.find(*itor);
		if (itrMq == mqConfigInfo.end())
		{
			printf("==except== mqid:%lu is not find in m_mqConfigInfo.\n",*itor);
			LogWarn("==except== mqid:%lu is not find in m_mqConfigInfo.",*itor);
			continue;
		}
		m_vDBMQInfo.push_back(itrMq->second);
	}
	
	if(DBMQ_NUM != m_vDBMQInfo.size())
	{
		printf("\nDBMQ size[%lu] is not %d,please check config!!\n",m_vDBMQInfo.size(),DBMQ_NUM);
		LogError("DBMQ size[%lu] is not %d,please check config!!",m_vDBMQInfo.size(),DBMQ_NUM);
		return false;
	}
	return true;
	
}
void CDBMQProducerThread::GetMQInfo(TMsg* pMsg)
{
	if (pMsg == NULL)
	{
		LogError("pMsg is NULL");
		return;
	}
	TMQPublishReqMsg* pMQ = (TMQPublishReqMsg*)pMsg;
	pMQ->m_strKey = GetReportUid(pMQ->m_strData);
	if(pMQ->m_strKey.empty() || m_vDBMQInfo.size() != DBMQ_NUM)
	{
		LogWarn("pMQ->m_strKey[%s],m_vDBMQInfo size[%u]",pMQ->m_strKey.c_str(),m_vDBMQInfo.size());
		pMQ->m_strExchange = m_vDBMQInfo[0].m_strExchange;
		pMQ->m_strRoutingKey = m_vDBMQInfo[0].m_strRoutingKey;
		return;
	}
	UInt32 index = 0;
	string strMd5;
	unsigned char md5[16] = { 0 };       
    MD5((const unsigned char*) pMQ->m_strKey.c_str(), pMQ->m_strKey.length(), md5);
	std::string HEX_CHARS = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
        strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
    }
	//get md5 head 4
	strMd5 = strMd5.substr(0, 4);
	UInt64 uKey = strtol(strMd5.data(), NULL, 16);
	index = uKey % DBMQ_NUM;
	pMQ->m_strExchange = m_vDBMQInfo[index].m_strExchange;
	pMQ->m_strRoutingKey = m_vDBMQInfo[index].m_strRoutingKey;
	return;
}
string CDBMQProducerThread::GetReportUid(string strData)
{	
	std::string::size_type pos = strData.find("RabbitMQFlag=");
	string strUid = "";		
	if (pos == std::string::npos)
	{	
		LogError("==MQDB== data[%s] is not find string RabbitMQFlag=.", strData.data());
		return strUid;
	}
	int iLen = strlen("RabbitMQFlag=");
	strUid = strData.substr(pos+iLen, 36);
	return strUid;
}
void CDBMQProducerThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    
    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);
	
    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_MQ_PUBLISH_REQ:
		{
			GetMQInfo(pMsg);
			HandlePublish(pMsg);
			break;
		}
        default:
        {
			LogWarn("msgType:0x%x is invalid.",pMsg->m_iMsgType);
            break;
        }
    }
}



