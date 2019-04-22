#ifndef CHANNELGROUP_H_
#define CHANNELGROUP_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

const UInt32 CHANNEL_GROUP_MAX_SIZE = 10;		//ȫ��д

namespace models
{

    class ChannelGroup
    {
    public:
        ChannelGroup();
        virtual ~ChannelGroup();

        ChannelGroup(const ChannelGroup& other);

        ChannelGroup& operator =(const ChannelGroup& other);

    public:
        UInt32 m_uChannelGroupID;
        string m_strChannelGroupName;
        UInt32 m_uOperater;
		
		UInt32 m_szChannelIDs[CHANNEL_GROUP_MAX_SIZE];
		UInt32 m_szChannelsWeight[CHANNEL_GROUP_MAX_SIZE];   // �洢���ݿ��ж�����Ȩ������
    };

}

#endif

