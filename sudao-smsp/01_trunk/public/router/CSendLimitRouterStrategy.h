#ifndef CSENDLIMITROUTERSTRATEGY_H
#define CSENDLIMITROUTERSTRATEGY_H

#include "CRouterStrategy.h"


class CSendLimitRouterStrategy : public CRouterStrategy
{
public:
    CSendLimitRouterStrategy();
    virtual ~CSendLimitRouterStrategy();

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);
    Int32 BaseRoute(models::Channel& smsChannel,string& strReason);

protected:
    virtual Int32 CheckRoute(const models::Channel& smsChannel,string& strReason);

//private:
//    bool CheckChannelSignType(const models::Channel& smsChannel);
};

#endif