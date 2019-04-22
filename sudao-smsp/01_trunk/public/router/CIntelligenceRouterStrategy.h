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

////////////////////////////////  ����·�ɲ���ʹ�õ�Ȩ�ؼ����� //////////////////////////////////
class CIntelWeightCalculator : public CDefaultChannelWeightCalculator
{
    public:
        CIntelWeightCalculator(CRouterThread* pRouterThread,
                               TSmsRouterReq* pSMSSmsInfo,
                               ChannelGroupWeight* pChlGrpWeight);
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);

        // ��Ҫ�����ͨ����������ͨ��������Ȩ��֮��, ��ͨ��Ȩ���鴫��ֵ
        int m_iTotalConfigWeight;

        UInt32 m_uSmsType;

        // ͨ����ռȨ������ 
        UInt32 m_uChannelWeightFactor;
        // ͨ���ɹ�����ռȨ������ 
        UInt32 m_uChannelSuccWeightFactor;
        // ��Ȩ�����Ӻ� <ϵͳ������CHANNELGTOUP_WEIGHT_RATIO>
        UInt32 m_uTotalWeightFactor;

        // SMSType��Ӧ��ͨ�����ɹ��ʼ����Ȩ�ص��ܺ�
        int m_iTotalWeightBySmstype; 
};


#endif    //__C_INTELLIGENCE_ROUTER_STRATEGY_H__
