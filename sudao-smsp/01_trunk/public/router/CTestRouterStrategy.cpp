#include "CTestRouterStrategy.h"
#include "CRouterThread.h"

CTestRouterStrategy::CTestRouterStrategy()
 : CRouterStrategy()
{
	
}

CTestRouterStrategy::~CTestRouterStrategy()
{

}


/*函数作用: 通道路由
 *返回值:  0: 成功; 1: 因流控导致的失败; 2:获取通道组失败 99:其他失败
 */
Int32 CTestRouterStrategy::GetRoute(models::Channel& smsChannel)
{   
	Int32 channel_get_ret;
	ChannelScheduler* channelSchedule = m_pRouterThread->GetChannelScheduler();
	//get channel info
	if(false == channelSchedule->getChannel(m_pSMSSmsInfo->m_uChannleId,smsChannel))
	{
		LogWarn("TestChannelId[%u]get ChannelInfo   %s failure", m_pSMSSmsInfo->m_uChannleId,m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str());
		return CHANNEL_GET_FAILURE_OTHER;
	}
	
	//check phone and Channel operater
	if((FOREIGN != m_pSMSSmsInfo->m_uOperater) &&
		(0 != smsChannel.operatorstyle) && 
		(smsChannel.operatorstyle != m_pSMSSmsInfo->m_uOperater))
	{
		LogWarn("[%s:%s] phone operatorstype[%d] is not match channel[%d] operatorstype[%d]", m_pSMSSmsInfo->m_strSmsId.c_str(), 
			m_pSMSSmsInfo->m_strPhone.c_str(), m_pSMSSmsInfo->m_uOperater,smsChannel.channelID,smsChannel.operatorstyle);
		return CHANNEL_GET_FAILURE_OTHER;
	}
		
	string strReason = "";
	channel_get_ret = CheckRoute(smsChannel,strReason);				
	LogNotice("channel==smsId:%s,phone:%s,channel:%d,res:%d.Reason:%s",
		m_pSMSSmsInfo->m_strSmsId.data(),m_pSMSSmsInfo->m_strPhone.data(),m_pSMSSmsInfo->m_uChannleId,channel_get_ret,strReason.data());

    return channel_get_ret;
}

