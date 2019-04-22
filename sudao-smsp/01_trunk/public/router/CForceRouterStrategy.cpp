#include "CForceRouterStrategy.h"
#include "Comm.h"

CForceRouterStrategy::CForceRouterStrategy()
 : CRouterStrategy()
{
	m_strStrategyName = "ForceRoute";
}

CForceRouterStrategy::~CForceRouterStrategy()
{

}

Int32 CForceRouterStrategy::BaseRoute(models::Channel& smsChannel,string& strReason)
{
	//检查目标号码/号段是否存在强制路由信息，并且匹配运营商
	//models::ChannelSegment channelSegment;
	ChannelSegmentList channelSegmentList;
	channelSegmentList.clear();
	if (false == m_pRouter->GetChannelSegment(channelSegmentList, m_pSMSSmsInfo->m_uPhoneOperator))
	{
		LogNotice("[%s:%s] not found channel in segment,sendtype:%d", 
			m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(),m_pSMSSmsInfo->m_uSendType);
		return CHANNEL_GET_FAILURE_OTHER;
	}

	ChannelSegmentList::iterator itr = channelSegmentList.begin();
    vector<std::string> vecChannelGroups;

	for(; itr != channelSegmentList.end(); itr++)
	{
        // 原来的channel_id 现在存储的是 channelgroup_id 
        UInt32 uChlGrpID = itr->channel_id;

		LogDebug("[%s:%s]  channelgroup[%u] in m_ChannelMap client_code[%s]", 
				m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), uChlGrpID, itr->client_code.data());

        vecChannelGroups.clear();
        vecChannelGroups.push_back(Comm::int2str(uChlGrpID));

		//匹配运营商类型，operatorstype = 0表示全网 国际号码不作运营商匹配
		if((FOREIGN != m_pSMSSmsInfo->m_uOperater) &&
			(0 != itr->operatorstype) && 
			(itr->operatorstype != m_pSMSSmsInfo->m_uPhoneOperator))
		{
			LogNotice("[%s:%s] phone operatorstype[%d] is not match channelgroup[%d] operatorstype[%d]", m_pSMSSmsInfo->m_strSmsId.c_str(), 
				m_pSMSSmsInfo->m_strPhone.c_str(), m_pSMSSmsInfo->m_uPhoneOperator, uChlGrpID,itr->operatorstype);
			continue;
		}
	
		//client_code为空表示此路由规则适用于所有用户
		if (0 == strncmp(itr->client_code.data(),"*",1))
		{
			LogNotice("[%s:%s] client_code is *, this channelgroup[%u] for all account", 
				m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), uChlGrpID);
            if (CHANNEL_GET_SUCCESS == SelectChlFromChlGrp(vecChannelGroups,smsChannel))
				return CHANNEL_GET_SUCCESS;
			else		
				continue;
		}

		//检查强制路由客户表是否存在该客户代码
		stl_map_str_list_str& ClientSegmentMap = m_pRouterThread->GetClientSegmentMap();
		stl_map_str_list_str::iterator itor_clientsegment = ClientSegmentMap.find(itr->client_code);
		if (itor_clientsegment == ClientSegmentMap.end())
		{
			LogNotice("[%s:%s] client_code[%s] does not found in client_segment", 
				m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), itr->client_code.c_str());
			continue;
			//return CHANNEL_GET_FAILURE_OTHER;
		}

		//检查客户是否适用于该路由规则
		// type规则:  0：包含的用户路由
		//            1：包含的用户不路由
		stl_list_str& clientCode = itor_clientsegment->second;
		stl_list_str::iterator itor_clientid = std::find(clientCode.begin(), clientCode.end(), m_pSMSSmsInfo->m_strClientId);
		if (0 == itr->type && itor_clientid == clientCode.end())
		{
			LogNotice("[%s:%s] channelSegment_type is 0, but clientid not found in client_code[%s]", 
				m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), itr->client_code.c_str());
			//return CHANNEL_GET_FAILURE_OTHER;
			continue;
		}
		else if (1 == itr->type && itor_clientid != clientCode.end())
		{
			LogNotice("[%s:%s] channelSegment_type is 1, but clientid found in client_code[%s]", 
				m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), itr->client_code.c_str());
			//return CHANNEL_GET_FAILURE_OTHER;
			continue;
		}

		if (CHANNEL_GET_SUCCESS == SelectChlFromChlGrp(vecChannelGroups, smsChannel))
			return CHANNEL_GET_SUCCESS;

	}
	return CHANNEL_GET_FAILURE_OTHER;
}

/*函数作用: 通道路由
 *返回值:  0: 成功; 1: 因流控导致的失败; 2:获取通道组失败 99:其他失败
 */
Int32 CForceRouterStrategy::GetRoute(models::Channel& smsChannel)
{
	string strReason = "";

	if (0 == m_pRouter->m_uRouterType)	//guonei
	{
		return BaseRoute(smsChannel,strReason);
	}
	else	//guoji
	{
		for (int i = 0; i < 4; i++)
		{
			if (CHANNEL_GET_SUCCESS == BaseRoute(smsChannel,strReason))
			{
				m_pRouter->ResetSelectNum();
				return CHANNEL_GET_SUCCESS;
			}
		}

		m_pRouter->ResetSelectNum();
		return CHANNEL_GET_FAILURE_OTHER;
	}
}

Int32 CForceRouterStrategy::CheckRoute(const models::Channel& smsChannel,string& strReason)
{
//	if (false == CheckChannelSignType(smsChannel))
//	{
//		return CHANNEL_GET_FAILURE_OTHER;
//	}
	
	return CRouterStrategy::CheckRoute(smsChannel,strReason);
}

bool CForceRouterStrategy::CheckChannelSignType(const models::Channel& smsChannel)
{
	if (0 != smsChannel.m_uChannelType)
	{
		LogWarn("[%s:%s] channel[%d] channentype[%u] error", 
			m_pSMSSmsInfo->m_strSmsId.c_str(), m_pSMSSmsInfo->m_strPhone.c_str(), smsChannel.channelID, smsChannel.m_uChannelType);
		return false;
	}
	
	return true;
}

