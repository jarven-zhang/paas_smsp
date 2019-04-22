#ifndef __C_FOREIGN_ROUTER_REBACLK_H__
#define __C_FOREIGN_ROUTER_REBACLK_H__

#include "CForeignRouter.h"
#include "CRouterThread.h"

class CForeignRouterReback : public CForeignRouter
{
public:
    CForeignRouterReback();
    virtual ~CForeignRouterReback();

public:
    virtual bool CheckChannelQueueSize(const models::Channel& smsChannel);
};

#endif    //__C_FOREIGN_ROUTER_REBACLK_H__