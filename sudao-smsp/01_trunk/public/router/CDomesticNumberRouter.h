#ifndef __C_DOMESTIC_NUMBER_ROUTER_H__
#define __C_DOMESTIC_NUMBER_ROUTER_H__

#include "CBaseRouter.h"
#include "CRouterThread.h"

// Domestic number routing

class CDomesticNumberRouter : public CBaseRouter
{
public:
    CDomesticNumberRouter();
    virtual ~CDomesticNumberRouter();

public:
    virtual bool GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator);
};

#endif    //__C_DOMESTIC_NUMBER_ROUTER_H__