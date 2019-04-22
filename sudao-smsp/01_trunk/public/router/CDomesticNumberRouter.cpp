#include "CDomesticNumberRouter.h"

CDomesticNumberRouter::CDomesticNumberRouter()
 : CBaseRouter()
{

}

CDomesticNumberRouter::~CDomesticNumberRouter()
{

}

//���ں���
bool CDomesticNumberRouter::GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator)
{
	stl_map_str_ChannelSegmentList& ChannelSegmentMap = m_pRouterThread->GetChannelSegmentMap();
	char temp_key[128] = {0};
	snprintf(temp_key, sizeof(temp_key), "%s_%u",m_pSMSSmsInfo->m_strPhone.data(),m_pSMSSmsInfo->m_uSendType);
	//���ҹ��ں���
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
	//ƥ����Ӫ�����ͣ�operatorstype = 0��ʾȫ��
	if(itor_channelsegment->second.operatorstype == 0 || itor_channelsegment->second.operatorstype == phoneOperator)
	{
		channelSegmentList = itor_channelsegment->second;
		return true;
	}
#endif
	return false;
}