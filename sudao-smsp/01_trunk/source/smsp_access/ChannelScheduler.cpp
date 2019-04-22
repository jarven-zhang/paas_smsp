#include "ChannelScheduler.h"
#include <vector>
#include "main.h"
#include "Comm.h"

ChannelScheduler::ChannelScheduler()
{

}

ChannelScheduler::~ChannelScheduler()
{

}

int ChannelScheduler::init()
{
    g_pRuleLoadThread->getChannlelMap(m_ChannelMap);
    LogDebug("ChannelScheduler::init over.  m_ChannelMap.size[%d]", m_ChannelMap.size());
    //initChannel();
    return 0;
}

bool ChannelScheduler::getChannel(UInt32 uChannelID, models::Channel& smsChannel)
{	
	ChannelMAP::iterator it = m_ChannelMap.find(uChannelID);
	if(it == m_ChannelMap.end())
	{
		return false;
	}

	smsChannel = it->second;
	return true;
}



