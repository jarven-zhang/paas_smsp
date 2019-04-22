#include "CAutoRouterStrategy.h"
#include "Comm.h"
#include "BusTypes.h"

CAutoRouterStrategy::CAutoRouterStrategy()
 : CRouterStrategy()
{
    m_strStrategyName = "AutoRoute";	
}

CAutoRouterStrategy::~CAutoRouterStrategy()
{

}

Int32 CAutoRouterStrategy::GetRoute(models::Channel& smsChannel)
{
    BusType::SmsType smsType = (BusType::SmsType)(atoi(m_pSMSSmsInfo->m_strSmsType.c_str()));
    if (smsType != BusType::SMS_TYPE_NOTICE
            && smsType != BusType::SMS_TYPE_VERIFICATION_CODE
            && smsType != BusType::SMS_TYPE_MARKETING
            && smsType != BusType::SMS_TYPE_ALARM)
    {
        LogError("[%s:%s:%s] Auto-Route FAILED! "
                 "Unsupported smsType [%s] -> None of [0/4/5/6]",
                  m_pSMSSmsInfo->m_strSmsId.data(),
                  m_pSMSSmsInfo->m_strPhone.data(),
                  m_pSMSSmsInfo->m_strClientId.data(),
                  m_pSMSSmsInfo->m_strSmsType.data());

        return CHANNEL_GET_FAILURE_NO_CHANNELGROUP;
    }

	models::UserGw userGw;
    vector<std::string> vecChannelGroups;
	ChannelSelect* chlSelect =  m_pRouterThread->GetChannelSelect();
    // 取通道池，由'*'标志
    if(false == chlSelect->getChannelGroupVec("*", 
                                              "smsp_access",
                                              m_pSMSSmsInfo->m_strSmsType, 
                                              m_pSMSSmsInfo->m_uSmsFrom,
                                              m_pSMSSmsInfo->m_uOperater,
                                              vecChannelGroups,
                                              userGw))
    {
        LogError("[%s:%s] get channelGroups failed.  clientID[%s],  content[%s]",
                  m_pSMSSmsInfo->m_strSmsId.data(),
                  m_pSMSSmsInfo->m_strPhone.data(),
                  m_pSMSSmsInfo->m_strClientId.data(),
                  m_pSMSSmsInfo->m_strContent.data());

        return CHANNEL_GET_FAILURE_NO_CHANNELGROUP;
    }

    return SelectChlFromChlGrp(vecChannelGroups, smsChannel);
}

// 使用CAutorouteWeightCalculator类做权重计算
weight_calculator_ptr_t CAutoRouterStrategy::GetWeightCalculator(CRouterThread* pRouterThread,
                                                                 TSmsRouterReq* pSMSSmsInfo,
                                                                 ChannelGroupWeight* pChlGrpWeight)
{
    return weight_calculator_ptr_t(new CAutorouteWeightCalculator(pRouterThread, pSMSSmsInfo, pChlGrpWeight));
}

////////////////////////////////  自动路由策略使用的权重计算类 //////////////////////////////////
CAutorouteWeightCalculator::CAutorouteWeightCalculator(CRouterThread* pRouterThread,
                                                       TSmsRouterReq* pSMSSmsInfo,
                                                       ChannelGroupWeight* pChlGrpWeight)
    : CDefaultChannelWeightCalculator(pRouterThread, pSMSSmsInfo, pChlGrpWeight)
{
}

bool CAutorouteWeightCalculator::init()
{
    if (CDefaultChannelWeightCalculator::init() == false)
    {
        return false;
    }

    if (m_pChlGrpWeight == NULL) 
    {
        LogWarn("NULL ChannelGroupWeight Pointer");
        return false;
    }

    m_iTotalConfigWeight = m_pChlGrpWeight->getTotalConfigWeight();
    if (m_iTotalConfigWeight == 0)
    {
        LogWarn("Auto-Pool-Route: ChannelGroup's total-configured-weight is 0!");
        return false;
    }

    // 1. 基本赋值
    this->m_uOperator = m_pSMSSmsInfo->m_uOperater;
    this->m_uSmsType = atoi(m_pSMSSmsInfo->m_strSmsType.c_str());

    // 2. 取得用户的权重因子配置
    std::map<string, SmsAccount>& accountMap = m_pRouterThread->GetAccountMap();
    std::map<string, SmsAccount>::iterator acntIter = accountMap.find(m_pSMSSmsInfo->m_strClientId);
    if (acntIter == accountMap.end())
    {
        LogError("account [%s] NOT FOUND!", m_pSMSSmsInfo->m_strClientId.c_str());
        return false;
    }

    channel_pool_policies_t& poolPolicies = m_pRouterThread->GetChannelPoolPolicyConf();
    channel_pool_policies_t::iterator cppIter = poolPolicies.find(acntIter->second.m_iPoolPolicyId);
    if (cppIter == poolPolicies.end())
    {
        LogWarn("channel-pool-policy-id[%d] of client[%s] NOT FOUND! Try default one.", 
                             acntIter->second.m_iPoolPolicyId, m_pSMSSmsInfo->m_strClientId.c_str());
        cppIter = poolPolicies.find(DEFAULT_POOL_POLICY_ID);
    }

    if (cppIter == poolPolicies.end())
    {
        LogError("Default channel-pool-policy-id [%d] NOT FOUND!", DEFAULT_POOL_POLICY_ID);
        return false;
    }

    LogDebug("use channel-pool-policy[%s]", cppIter->second.policy_name.c_str());

    this->m_channelPoolPolicyByClient = cppIter->second;

    // 国际短信的价格权重因子为0
    if (this->m_uOperator == BusType::FOREIGN)
    {
        this->m_channelPoolPolicyByClient.price_factor = 0;
        this->m_channelPoolPolicyByClient.price_factor_ratio = 0;
    }

    // 3. 取得通道成功率权重总和
    channel_attribute_weight_config_t channelAttrWeightConfig = m_pRouterThread->GetChannelAttributeWeightConfig();

    ChannelAttributeWeightExvalue exValue = ConvertWeightSmstypeOrOperatorToExvalue(m_uSmsType, true);
    channel_attribute_weight_config_t::iterator cawcIter = channelAttrWeightConfig.find(exValue);
    if (cawcIter == channelAttrWeightConfig.end())
    {
        LogError("can't find smsType[%u]-exValue[%d] in channel-attribute-weight-conf", m_uSmsType, (int)exValue);
        return false;
    }
    m_iTotalChannelSuccWeightBySmsType = cawcIter->second.total_weight;

    // 4. 取得通道价格权重总和
    exValue = ConvertWeightSmstypeOrOperatorToExvalue(m_uOperator, false);
    cawcIter = channelAttrWeightConfig.find(exValue);
    if (cawcIter == channelAttrWeightConfig.end())
    {
        LogError("can't find operator[%u]-exValue[%d] in channel-attribute-weight-conf", m_uOperator, (int)exValue);
        return false;
    }
    m_iTotalChannelPriceWeightByOperator = cawcIter->second.total_weight;

    return true;
}

bool CAutorouteWeightCalculator::updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr)
{
    UInt32 uChannelId = weightInfoPtr->getData();

    if (this->m_iTotalChannelSuccWeightBySmsType == 0 
            || this->m_iTotalChannelPriceWeightByOperator == 0)
    {
        LogWarn("auto-pool-route: this group of channel[%u]'s totalChannelSuccWeightBySmsType[%d] "
                "or totalChannelPriceWeightByOperator[%u] is 0!"
                "Ignore weight choose!",
                uChannelId, 
                this->m_iTotalChannelSuccWeightBySmsType,
                this->m_iTotalChannelPriceWeightByOperator);
        return true;
    }

    ChannelRealtimeWeightInfo channlRtWInfo;

    // 找到通道的成功率/价格等
    if (loadChannelRealtimeWeightInfo(uChannelId, channlRtWInfo) == false)
    {
        LogError("");
        return false;
    }

    const ChannelPoolPolicy& policyFactor = this->m_channelPoolPolicyByClient; 

    // 精度，1W%
    // 低销: 权重占比 x 低销标志
    UInt32 uLowConsumePart = policyFactor.low_consume_factor_ratio * channlRtWInfo.low_consume_flag;
    // 客情: 权重占比 x 客情标志
    UInt32 uConsumeRelationPart = policyFactor.customer_relation_factor_ratio * channlRtWInfo.customer_relation_flag;
    // 成功率: 权重占比 x 成功率权重占比
    UInt32 uSuccPart = policyFactor.success_factor_ratio * channlRtWInfo.channel_succ_weight[this->m_uSmsType] /this->m_iTotalChannelSuccWeightBySmsType;
    // 价格: 权重占比 x 价格权重占比
    UInt32 uPricePart = policyFactor.price_factor_ratio * channlRtWInfo.channel_price_weight[this->m_uOperator] /this->m_iTotalChannelPriceWeightByOperator;
    // 抗投诉率: 权重占比
    UInt32 uAntiComplainPart = policyFactor.anti_complaint_factor_ratio;

    int iNewEffectiveWeight = uLowConsumePart + uConsumeRelationPart + uSuccPart + uPricePart + uAntiComplainPart;

    weightInfoPtr->setEffectiveWeight(iNewEffectiveWeight);

    LogDebug("[%s:%s] channel:%u -> userFactor=> |low_consume_factor[%d] customer_relation_factor[%d] succ_factor[%d] price_factor[%d] total_factor[%d]|"
             "   channelWeightInfo = < smstype:[%d] operator:[%u] >"
             "    < low_consume_flag:%d customer_relation_flag:%d] >"
             "    < succ_weight:[%d] total_succ_weight:[%d] >"
             "    < price_weight:[%d] total_price_weight:[%d] >"
             "    < new_eff_weight:[%d] > |",
             m_pSMSSmsInfo->m_strSmsId.data(),
             m_pSMSSmsInfo->m_strPhone.data(),
             weightInfoPtr->getData(),
             policyFactor.low_consume_factor,
             policyFactor.customer_relation_factor,
             policyFactor.success_factor,
             policyFactor.price_factor,
             policyFactor.total_factor,
             this->m_uSmsType,
             this->m_uOperator,
             channlRtWInfo.customer_relation_flag,
             channlRtWInfo.low_consume_flag,
             channlRtWInfo.channel_succ_weight[this->m_uSmsType],
             this->m_iTotalChannelSuccWeightBySmsType,
             channlRtWInfo.channel_price_weight[this->m_uOperator],
             this->m_iTotalChannelPriceWeightByOperator,
             iNewEffectiveWeight
            );


    return true;
}
