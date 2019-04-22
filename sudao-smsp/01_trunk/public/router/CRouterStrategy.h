#ifndef __C_ROUTER_STRATEGY_H__
#define __C_ROUTER_STRATEGY_H__

#include <stdio.h>
#include "CLogThread.h"
#include "Channel.h"
#include "protocol.h"
#include "ChannelGroupWeight.h"
#include "boost/shared_ptr.hpp"

class TSmsRouterReq;
class CBaseRouter;
class CRouterThread;
class CDefaultChannelWeightCalculator;
typedef boost::shared_ptr<CDefaultChannelWeightCalculator> weight_calculator_ptr_t;

class CRouterStrategy
{
public:
    CRouterStrategy();
    virtual ~CRouterStrategy();

public:
    virtual bool Init(CBaseRouter* pRouter);
    virtual Int32 GetRoute(models::Channel& smsChannel) = 0;

protected:
    virtual Int32 CheckRoute(const models::Channel& smsChannel,string& strReason);

    // 获取实际的权重计算类，不同的实现策略采用不同的计算方式。
    virtual weight_calculator_ptr_t GetWeightCalculator(CRouterThread* pRouterThread, TSmsRouterReq* pSMSSmsInfo, ChannelGroupWeight* pChlGrpWeight);
    
    // 从vecChannelGroups提供的组中根据权重选出通道
    Int32 SelectChlFromChlGrp(const std::vector<std::string>& vecChannelGroups, models::Channel& smsChannel);

protected:
    CBaseRouter* m_pRouter;
    CRouterThread* m_pRouterThread;
    TSmsRouterReq* m_pSMSSmsInfo;

    std::string m_strStrategyName;
};

//////////////////////////// 默认路由计算类 /////////////////////////////////
class CDefaultChannelWeightCalculator : public CDefaultWeightCalculator
{
    public:
        // 计算可能会需要从缓存数据、消息内容、通道组中获取相关数据
        CDefaultChannelWeightCalculator(CRouterThread* pRouterThread,
                                      TSmsRouterReq* pSMSSmsInfo,
                                      ChannelGroupWeight* pChlGrpWeight);  

        // 为子类提供加载通道实时权重信息的功能函数
        bool loadChannelRealtimeWeightInfo(UInt32 uChannelId, ChannelRealtimeWeightInfo& channelRealtimeWeightInfo);

    protected:
        CRouterThread* m_pRouterThread;  
        TSmsRouterReq* m_pSMSSmsInfo;
        ChannelGroupWeight* m_pChlGrpWeight;
        char m_szLogPrefix[64];

    public:
        // 实现基类接口
        virtual bool init();

};

#endif    //__C_ROUTER_STRATEGY_H__
