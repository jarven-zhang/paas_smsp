#ifndef __C_DOMESTIC_NUMBER_ROUTER_REBACK_H__
#define __C_DOMESTIC_NUMBER_ROUTER_REBACK_H__

#include "CDomesticNumberRouter.h"

class CDomesticNumberRouterReback : public CDomesticNumberRouter
{
public:
    CDomesticNumberRouterReback();
    virtual ~CDomesticNumberRouterReback();

public:
    virtual bool CheckChannelQueueSize(const models::Channel& smsChannel);
};

#endif 
