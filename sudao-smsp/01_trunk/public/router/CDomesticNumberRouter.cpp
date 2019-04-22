#include "CDomesticNumberRouter.h"

CDomesticNumberRouter::CDomesticNumberRouter()
 : CBaseRouter()
{

}

CDomesticNumberRouter::~CDomesticNumberRouter()
{

}

//国内号码
bool CDomesticNumberRouter::GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator)
{
	stl_map_str_ChannelSegmentList& ChannelSegmentMap = m_pRouterThread->GetChannelSegmentMap();
	char temp_key[128] = {0};
	snprintf(temp_key, sizeof(temp_key), "%s_%u",m_pSMSSmsInfo->m_strPhone.data(),m_pSMSSmsInfo->m_uSendType);
	//查找国内号码
	string strkey = temp_key;
	stl_map_str_ChannelSegmentList::iterator itor_channelsegment = ChannelSegmentMap.find(strkey);
	
	if (itor_channelsegment == ChannelSegmentMap.end())
	{
		return false;
	}
	else
	{
		channelSegmentList = itor_channelsegment->second;
		return true;
	}
#if 0
	//匹配运营商类型，operatorstype = 0表示全网
	if(itor_channelsegment->second.operatorstype == 0 || itor_channelsegment->second.operatorstype == phoneOperator)
	{
		channelSegmentList = itor_channelsegment->second;
		return true;
	}
#endif
	return false;
}