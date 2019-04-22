#ifndef __C_FORCE_ROUTER_STRATEGY_H__
#define __C_FORCE_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"
#include "CRouterThread.h"


class CForceRouterStrategy : public CRouterStrategy
{
public:
    CForceRouterStrategy();
    virtual ~CForceRouterStrategy();

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);

	Int32 BaseRoute(models::Channel& smsChannel,string& strReason);

protected:
    virtual Int32 CheckRoute(const models::Channel& smsChannel,string& strReason);

private:
    bool CheckChannelSignType(const models::Channel& smsChannel);
};

#endif    //__C_FORCE_ROUTER_STRATEGY_H__