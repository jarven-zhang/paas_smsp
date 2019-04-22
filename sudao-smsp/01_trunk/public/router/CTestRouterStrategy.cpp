#include "CTestRouterStrategy.h"
#include "CRouterThread.h"

CTestRouterStrategy::CTestRouterStrategy()
 : CRouterStrategy()
{
	
}

CTestRouterStrategy::~CTestRouterStrategy()
{

}


/*��������: ͨ��·��
 *����ֵ:  0: �ɹ�; 1: �����ص��µ�ʧ��; 2:��ȡͨ����ʧ�� 99:����ʧ��
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

