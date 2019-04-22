#include "ChannelScheduler.h"
#include <vector>
#include "main.h"

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
    return 0;
}



