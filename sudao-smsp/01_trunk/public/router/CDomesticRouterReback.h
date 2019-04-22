#ifndef __C_DOMESTIC_ROUTER_REBACK_H__
#define __C_DOMESTIC_ROUTER_REBACK_H__

#include "CDomesticRouter.h"

class CDomesticRouterReback : public CDomesticRouter
{
public:
    CDomesticRouterReback();
    virtual ~CDomesticRouterReback();

public:
    virtual bool CheckChannelQueueSize(const models::Channel& smsChannel);
};

#endif    //__C_DOMESTIC_ROUTER_REBACK_H__