#ifndef __C_BASE_ROUTER_H__
#define __C_BASE_ROUTER_H__

#include <stdio.h>
#include "CLogThread.h"
#include "SMSSmsInfo.h"
#include "ChannelSegment.h"
#include "ChannelWhiteKeyword.h"
#include "CRouterStrategy.h"
#include "protocol.h"

class TSmsRouterReq;
class CRouterStrategy;
class CRouterThread;
class CBaseRouter
{
public:
    CBaseRouter();
    virtual ~CBaseRouter();

public:
    virtual bool Init(CRouterThread *pRouterThread, TSmsRouterReq *pInfo);
    virtual Int32 GetRoute(models::Channel &smsChannel, ROUTE_STRATEGY strategy);

public:
    virtual bool GetChannelSegment(ChannelSegmentList &channelSegmentList, unsigned int phoneOperator) = 0;
    virtual bool CheckRoute(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckOverFlow(const models::Channel &smsChannel);
    bool CheckChanneOverrate(const models::Channel &smsChannel);
    void virtual ResetSelectNum();

private:
    string RouteStrategy2String(ROUTE_STRATEGY strategy);

public:
    virtual bool CheckChannelAttribute(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckFailedReSendChannels(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelOverrateReSendChannels(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelExValue(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelOperater(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelLoginStatus(const models::Channel &smsChannel);
    virtual bool CheckChannelQueueSize(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelSendTime(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelKeyWord(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelBlackWhiteList(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckChannelTemplateId(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckAndGetExtendPort(const models::Channel &smsChannel, string &strReason);
    virtual bool CheckProvinceRoute(const models::Channel &smsChannel, string &strReason);

public:
    CRouterThread *m_pRouterThread;
    TSmsRouterReq *m_pSMSSmsInfo;

    UInt32 m_uRouterType; ////0 guonei,1 guoji



protected:
    CRouterStrategy *m_pRouterStrategy;
};

#endif    //__C_BASE_ROUTER_H__
