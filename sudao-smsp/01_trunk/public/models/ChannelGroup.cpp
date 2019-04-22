#include "ChannelGroup.h"
namespace models
{

    ChannelGroup::ChannelGroup()
    {
        m_uChannelGroupID = 0;
        m_uOperater = 0;
		for(UInt32 i = 0 ; i < CHANNEL_GROUP_MAX_SIZE ; i++)
		{
			m_szChannelIDs[i] = 0;
		}
		
    }

    ChannelGroup::~ChannelGroup()
    {

    }

    ChannelGroup::ChannelGroup(const ChannelGroup& other)
    {
		this->m_uChannelGroupID = other.m_uChannelGroupID;
		this->m_strChannelGroupName = other.m_strChannelGroupName;
		this->m_uOperater = other.m_uOperater;

		for(UInt32 i = 0 ; i < CHANNEL_GROUP_MAX_SIZE ; i++)
		{
			this->m_szChannelIDs[i] = other.m_szChannelIDs[i];
			this->m_szChannelsWeight[i] = other.m_szChannelsWeight[i];
		}
    }

    ChannelGroup& ChannelGroup::operator =(const ChannelGroup& other)
    {
        this->m_uChannelGroupID = other.m_uChannelGroupID;
		this->m_strChannelGroupName = other.m_strChannelGroupName;
		this->m_uOperater = other.m_uOperater;

		for(UInt32 i = 0 ; i < CHANNEL_GROUP_MAX_SIZE ; i++)
		{
			this->m_szChannelIDs[i] = other.m_szChannelIDs[i];
			this->m_szChannelsWeight[i] = other.m_szChannelsWeight[i];
		}

        return *this;
    }


} /* namespace models */

