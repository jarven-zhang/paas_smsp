#ifndef __C_INTELLIGENCE_ROUTER_STRATEGY_H__
#define __C_INTELLIGENCE_ROUTER_STRATEGY_H__

#include "CRouterStrategy.h"
#include "ChannelGroupWeight.h"
#include "CRouterThread.h"

  
class CIntelligenceRouterStrategy : public CRouterStrategy
{
public:
    CIntelligenceRouterStrategy();
    virtual ~CIntelligenceRouterStrategy();
    virtual weight_calculator_ptr_t GetWeightCalculator(CRouterThread* pRouterThread, TSmsRouterReq* pSMSSmsInfo, ChannelGroupWeight* pChlGrpWeight);

public:
    virtual Int32 GetRoute(models::Channel& smsChannel);
};

////////////////////////////////  智能路由策略使用的权重计算类 //////////////////////////////////
class CIntelWeightCalculator : public CDefaultChannelWeightCalculator
{
    public:
        CIntelWeightCalculator(CRouterThread* pRouterThread,
                               TSmsRouterReq* pSMSSmsInfo,
                               ChannelGroupWeight* pChlGrpWeight);
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);

        // 将要计算的通道组里所有通道的配置权重之和, 由通道权重组传入值
        int m_iTotalConfigWeight;

        UInt32 m_uSmsType;

        // 通道所占权重因子 
        UInt32 m_uChannelWeightFactor;
        // 通道成功率所占权重因子 
        UInt32 m_uChannelSuccWeightFactor;
        // 总权重因子和 <系统配置项CHANNELGTOUP_WEIGHT_RATIO>
        UInt32 m_uTotalWeightFactor;

        // SMSType对应的通道区成功率间相关权重的总和
        int m_iTotalWeightBySmstype; 
};


#endif    //__C_INTELLIGENCE_ROUTER_STRATEGY_H__
