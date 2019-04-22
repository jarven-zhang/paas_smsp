#ifndef __C_AUTO_ROUTER_STRATEGY_H__
#define __C_AUTO_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"
#include "CRouterThread.h"

class CAutoRouterStrategy : public CRouterStrategy
{
public:
    CAutoRouterStrategy();
    virtual ~CAutoRouterStrategy();
    virtual weight_calculator_ptr_t GetWeightCalculator(CRouterThread* pRouterThread, TSmsRouterReq* pSMSSmsInfo, ChannelGroupWeight* pChlGrpWeight);

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);

};

////////////////////////////////  自动路由策略使用的权重计算类 //////////////////////////////////
class CAutorouteWeightCalculator : public CDefaultChannelWeightCalculator
{
    public:
        CAutorouteWeightCalculator(CRouterThread* pRouterThread,
                                   TSmsRouterReq* pSMSSmsInfo,
                                   ChannelGroupWeight* pChlGrpWeight);
        // 实现基类接口
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);

        ChannelPoolPolicy m_channelPoolPolicyByClient;
        UInt32 m_uOperator;
        UInt32 m_uSmsType;

        // 将要计算的通道组里所有通道的配置权重之和
        int m_iTotalConfigWeight;

        // SMSType对应的通道区成功率间相关权重的总和
        int m_iTotalChannelSuccWeightBySmsType;
        // Operator对应的价格区间相关权重的总和
        int m_iTotalChannelPriceWeightByOperator;
};


#endif    //__C_AUTO_ROUTER_STRATEGY_H__
