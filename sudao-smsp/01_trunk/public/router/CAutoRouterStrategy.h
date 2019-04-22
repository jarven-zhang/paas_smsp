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

////////////////////////////////  �Զ�·�ɲ���ʹ�õ�Ȩ�ؼ����� //////////////////////////////////
class CAutorouteWeightCalculator : public CDefaultChannelWeightCalculator
{
    public:
        CAutorouteWeightCalculator(CRouterThread* pRouterThread,
                                   TSmsRouterReq* pSMSSmsInfo,
                                   ChannelGroupWeight* pChlGrpWeight);
        // ʵ�ֻ���ӿ�
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);

        ChannelPoolPolicy m_channelPoolPolicyByClient;
        UInt32 m_uOperator;
        UInt32 m_uSmsType;

        // ��Ҫ�����ͨ����������ͨ��������Ȩ��֮��
        int m_iTotalConfigWeight;

        // SMSType��Ӧ��ͨ�����ɹ��ʼ����Ȩ�ص��ܺ�
        int m_iTotalChannelSuccWeightBySmsType;
        // Operator��Ӧ�ļ۸��������Ȩ�ص��ܺ�
        int m_iTotalChannelPriceWeightByOperator;
};


#endif    //__C_AUTO_ROUTER_STRATEGY_H__
