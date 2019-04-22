#ifndef __C_RESEND_ROUTER_STRATEGY_H__
#define __C_RESEND_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"

class CReSendRouterStrategy : public CRouterStrategy
{
public:
    CReSendRouterStrategy();
    virtual ~CReSendRouterStrategy();

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);

	Int32 BaseRoute(models::Channel& smsChannel,string& strReason);

protected:
    virtual Int32 CheckRoute(const models::Channel& smsChannel,string& strReason);

private:
    bool CheckChannelSignType(const models::Channel& smsChannel);
};

#endif    //__C_RESEND_ROUTER_STRATEGY_H__