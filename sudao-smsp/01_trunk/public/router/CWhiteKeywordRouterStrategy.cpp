#include "CWhiteKeywordRouterStrategy.h"
#include "CRouterThread.h"
#include <set>

CWhiteKeywordRouterStrategy::CWhiteKeywordRouterStrategy()
 : CRouterStrategy()
{
	m_strStrategyName = "WhiteKeywordRouter";
}

CWhiteKeywordRouterStrategy::~CWhiteKeywordRouterStrategy()
{

}

Int32 CWhiteKeywordRouterStrategy::GetRoute(models::Channel& smsChannel)
{
	
	string strKey;	
	m_strSign = m_pSMSSmsInfo->m_strSign;
	m_strContent = m_pSMSSmsInfo->m_strContent;
	Comm::StrHandle(m_strSign);
	Comm::StrHandle(m_strContent);
	
	ChannelWhiteKeywordMap& channelWhiteKeywordMap = m_pRouterThread->GetChannelWhiteKeywordListMap();
	
	strKey.assign(m_pSMSSmsInfo->m_strClientId);
	strKey.append("_");
	strKey.append(Comm::int2str(m_pSMSSmsInfo->m_uSendType));
	if(SelectChannelFromWhiteKeyword(strKey,channelWhiteKeywordMap,smsChannel))
	{
		return CHANNEL_GET_SUCCESS;
	}
	else
	{
		strKey.assign("*_");
		strKey.append(Comm::int2str(m_pSMSSmsInfo->m_uSendType));
		if(SelectChannelFromWhiteKeyword(strKey,channelWhiteKeywordMap,smsChannel))
		{
			return CHANNEL_GET_SUCCESS;
		}
	}
	
	return CHANNEL_GET_FAILURE_OTHER;
}
bool CWhiteKeywordRouterStrategy::SelectChannelFromWhiteKeyword(string& strKeyword,ChannelWhiteKeywordMap& channelWhiteKeywordmap,models::Channel& smsChannel)
{
	ChannelWhiteKeywordMap::iterator it = channelWhiteKeywordmap.find(strKeyword);
	if(it != channelWhiteKeywordmap.end())
	{
		ChannelWhiteKeywordList& channelWhiteKeywordList = it->second;
		ChannelWhiteKeywordList::iterator itr = channelWhiteKeywordList.begin();
		for(; itr != channelWhiteKeywordList.end(); itr++)
		{
			//check channelgroup operatorstype
			if((0 != itr->operatorstype) && (itr->operatorstype != m_pSMSSmsInfo->m_uOperater))
			{
				LogDebug("[%s:%s] Operater[%u] is not match Operater[%u]",m_pSMSSmsInfo->m_strSmsId.c_str(),m_pSMSSmsInfo->m_strPhone.c_str(), 
					m_pSMSSmsInfo->m_uOperater,itr->operatorstype);
				continue;
			}
			
			//check white keyword
		    if(false == CheckWhiteKeyword(*itr))
		    {
				LogDebug("[%s:%s] sign[%s]or content[%s] is not match [%s]:[%s]",m_pSMSSmsInfo->m_strSmsId.c_str(),m_pSMSSmsInfo->m_strPhone.c_str(), 
					m_strSign.c_str(),m_strContent.c_str(),itr->sign.c_str(),itr->white_keyword.c_str());
				continue;
			}
			LogNotice("[%s:%s] match WhiteKeyWord[%s] : [%s],clentcode[%s]",m_pSMSSmsInfo->m_strSmsId.c_str(),m_pSMSSmsInfo->m_strPhone.c_str(),
				itr->sign.c_str(),itr->white_keyword.c_str(),itr->client_code.c_str());
			//check channel
			vector<std::string> vecChannelGroups;
			vecChannelGroups.clear();
			vecChannelGroups.push_back(Comm::int2str(itr->channelgroup_id));
			if (CHANNEL_GET_SUCCESS == SelectChlFromChlGrp(vecChannelGroups,smsChannel))
				return true;
		}
	}
	else
	{
		LogNotice("[%s:%s] strKeyword[%s] not find in WhiteKeyWordMap",m_pSMSSmsInfo->m_strSmsId.c_str(),m_pSMSSmsInfo->m_strPhone.c_str(),
			strKeyword.c_str());
	}
	return false;
}
bool CWhiteKeywordRouterStrategy::CheckWhiteKeyword(ChannelWhiteKeyword& channelWhiteKeyword)
{
	//check sign
	if(!channelWhiteKeyword.sign.empty() && m_strSign != channelWhiteKeyword.sign)
	{
		return false;
	}
	string strautosign = "{}";
	return (channelWhiteKeyword.white_keyword.empty() || Comm::PatternMatch(m_strContent,strautosign,channelWhiteKeyword.white_keyword));
}

