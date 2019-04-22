#include "CForeignRouter.h"

CForeignRouter::CForeignRouter()
 : CBaseRouter(),m_uSelectNum(4)
{

}

CForeignRouter::~CForeignRouter()
{

}

void CForeignRouter::ResetSelectNum()
{
	m_uSelectNum = 4;
}
//guo ji
bool CForeignRouter::GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator)
{
	std::string tmpPhone = m_pSMSSmsInfo->m_strPhone.substr(2);    //去掉号码前面的00

	if (tmpPhone.length() < 1)
	{
		LogWarn("phone length < 1");
		return false;
	}
	
	std::string prefix;
	stl_map_str_ChannelSegmentList& ChannelSegmentMap = m_pRouterThread->GetChannelSegmentMap();

	prefix = tmpPhone.substr(0, m_uSelectNum--);
	char temp_key[128] = {0};
	snprintf(temp_key, sizeof(temp_key), "%s_%u",prefix.data(),m_pSMSSmsInfo->m_uSendType);
	string strkey = temp_key;
	stl_map_str_ChannelSegmentList::iterator itor_channelsegment = ChannelSegmentMap.find(strkey);
	if (itor_channelsegment != ChannelSegmentMap.end())
	{
		channelSegmentList = itor_channelsegment->second;
		return true;
	}
	
	return false;
}

bool CForeignRouter::CheckRoute(const models::Channel& smsChannel,string& strReason)
{
	if (false == CBaseRouter::CheckRoute(smsChannel,strReason) ||
		false == GetSmppPrice(smsChannel))
	{
		return false;
	}

	return true;
}

bool CForeignRouter::GetSmppPrice(const models::Channel& smsChannel)
{
	int iCostPrice = 0;
    int iSaleprice = 0;
	ChannelSelect* chlSelect =  m_pRouterThread->GetChannelSelect();
	if(false == chlSelect->getSmppPrice(m_pSMSSmsInfo->m_strPhone, smsChannel.channelID, iSaleprice, iCostPrice))	//get smppprice is 0
	{
		LogError("[%s:%s] channelid[%d],phone is not find price.", 
            m_pSMSSmsInfo->m_strSmsId.data(), m_pSMSSmsInfo->m_strPhone.data(), smsChannel.channelID);
        return false;
	}

    m_pSMSSmsInfo->m_dSaleFee = iSaleprice;
    m_pSMSSmsInfo->m_fCostFee = iCostPrice/1000000.0;
    LogDebug("==debug== salePrice[%d],costPrice[%d].", iSaleprice, iCostPrice);
    LogDebug("==debug== saleFee[%lu],costFee[%f].", m_pSMSSmsInfo->m_dSaleFee, m_pSMSSmsInfo->m_fCostFee);
	return true;
}