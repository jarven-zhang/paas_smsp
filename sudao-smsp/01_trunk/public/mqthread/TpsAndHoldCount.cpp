#include "TpsAndHoldCount.h"
#include "CMQProducerThread.h"
#include "main.h"

CTpsAndHoldCount::CTpsAndHoldCount()
{

}

CTpsAndHoldCount::~CTpsAndHoldCount()
{

}

void CTpsAndHoldCount::proTpsAndHoldReq(const string& strExchange,const string& strRoutingKey,string countType,CThread* pThread,const string& strClientId,UInt32 uNum)
{
	if (NULL == pThread)
	{
		LogError("pThread is NULL.");
		return;
	}

	UInt64 uTime = time(NULL);
	char cTime[25] = {0};
	snprintf(cTime,25,"%lu",uTime);

	string strData = "";

	if (COUNT_TYPE_TPS == countType)
	{
		strData.append("countType=0&clientid=");
		strData.append(strClientId);
		strData.append("&num=");
		char cTemp[25] = {0};
		snprintf(cTemp,25,"%u",uNum);
		strData.append(cTemp);
		strData.append("&time=");
		strData.append(cTime);
	}
	else if (COUNT_TYPE_OVER == countType)
	{
		strData.append("countType=1&clientid=");
		strData.append(strClientId);
		strData.append("&time=");
		strData.append(cTime);
	}
	else if (COUNT_TYPE_CHARGE == countType)
	{
		strData.append("countType=2&clientid=");
		strData.append(strClientId);
		strData.append("&time=");
		strData.append(cTime);
	}
	else
	{
		LogError("countType:%s,is invalid.",countType.data());
		return;
	}

	LogDebug("==tps_over_charge== push mq data:%s.",strData.data());

	TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
	pMQ->m_strData = strData;
	pMQ->m_strExchange = strExchange;		
	pMQ->m_strRoutingKey = strRoutingKey;	
	pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
	pThread->PostMsg(pMQ);

	return;
}