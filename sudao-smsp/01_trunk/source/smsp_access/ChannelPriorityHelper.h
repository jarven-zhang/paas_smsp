#ifndef __CHANNELPRIORITYHELPER_H__
#define __CHANNELPRIORITYHELPER_H__

#include "SMSSmsInfo.h"
#include "ClientPriority.h"

class smsSessionInfo;

class ChannelPriorityHelper {
public:
    static int GetClientChannelPriority(smsSessionInfo* pInfo, const client_priorities_t& clientPriorities );
};

#endif
