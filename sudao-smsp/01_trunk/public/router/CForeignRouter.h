#ifndef __C_FOREIGN_ROUTER_H__
#define __C_FOREIGN_ROUTER_H__

#include "CBaseRouter.h"
#include "CRouterThread.h"

class CForeignRouter : public CBaseRouter
{
public:
    CForeignRouter();
    virtual ~CForeignRouter();

public:
    virtual bool GetChannelSegment(ChannelSegmentList& channelSegmentList, unsigned int phoneOperator);
    virtual bool CheckRoute(const models::Channel& smsChannel,string& strReason);	
	virtual void ResetSelectNum();
private:
    bool GetSmppPrice(const models::Channel& smsChannel);
	Int32 m_uSelectNum;
};

#endif    //__C_FOREIGN_ROUTER_H__