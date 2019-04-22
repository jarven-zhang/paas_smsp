#include "CDomesticRouter.h"

CDomesticRouter::CDomesticRouter()
 : CBaseRouter()
{

}

CDomesticRouter::~CDomesticRouter()
{

}

//国内号段
bool CDomesticRouter::GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator)
{
	//找到这个号段对应的省
	stl_map_str_PhoneSection& PhoneSectionMap = m_pRouterThread->GetPhoneSectionMap();
	stl_map_str_PhoneSection::iterator itor_PhoneSection = PhoneSectionMap.find(m_pSMSSmsInfo->m_strPhone.substr(0, 7));

	if(itor_PhoneSection == PhoneSectionMap.end())
	{
		return false;
	}

	UInt32 area_id = itor_PhoneSection->second.code;	//路由代码，对应t_sms_area表中的一级area_id(省)

	//根据area_id去t_sms_segment_channel表匹配
	stl_map_str_ChannelSegmentList& ChannelSegmentMap = m_pRouterThread->GetChannelSegmentMap();
	char temp_key[128] = {0};
	snprintf(temp_key, sizeof(temp_key), "%u_%u",area_id,m_pSMSSmsInfo->m_uSendType);
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
	return false;
}