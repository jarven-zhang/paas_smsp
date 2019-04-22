#ifndef __C_DOMESTIC_ROUTER_H__
#define __C_DOMESTIC_ROUTER_H__

#include "CBaseRouter.h"
#include "CRouterThread.h"
//Domestic number section routing

class CDomesticRouter : public CBaseRouter
{
public:
    CDomesticRouter();
    virtual ~CDomesticRouter();

public:
    virtual bool GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator);
};

#endif    //__C_DOMESTIC_ROUTER_H__