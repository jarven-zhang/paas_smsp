#ifndef __C_TEST_ROUTER_STRATEGY_H__
#define __C_TEST_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"

  
class CTestRouterStrategy : public CRouterStrategy
{
public:
    CTestRouterStrategy();
    virtual ~CTestRouterStrategy();

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);
};

#endif    //__C_TEST_ROUTER_STRATEGY_H__