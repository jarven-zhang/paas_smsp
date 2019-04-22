#include "CIntelligenceRouterStrategy.h"
#include "ChannelGroupWeight.h"

// 在某因子未启用时相关权重的默认值 
const static UInt32 DEFAULT_IGNORED_WEIGHT_VALUE = 123456;

CIntelligenceRouterStrategy::CIntelligenceRouterStrategy()
 : CRouterStrategy()
{
    m_strStrategyName = "Intelligence";	
}

CIntelligenceRouterStrategy::~CIntelligenceRouterStrategy()
{

}


/*函数作用: 通道路由
 *返回值:  0: 成功; 1: 因流控导致的失败; 2:获取通道组失败 99:其他失败
 */
Int32 CIntelligenceRouterStrategy::GetRoute(models::Channel& smsChannel)
{
	models::UserGw userGw;
    vector<std::string> vecChannelGroups;
	ChannelSelect* chlSelect =  m_pRouterThread->GetChannelSelect();
    do
    {
        if(chlSelect->getChannelGroupVec(m_pSMSSmsInfo->m_strClientId,
                                         m_pSMSSmsInfo->m_strSid, 
                                         m_pSMSSmsInfo->m_strSmsType,
                                         m_pSMSSmsInfo->m_uSmsFrom,
                                         m_pSMSSmsInfo->m_uOperater,
                                         vecChannelGroups,
                                         userGw))
		{
			break;
		}
        
		if(chlSelect->getChannelGroupVec("smsp_access", 
                                          "smsp_access",
                                          m_pSMSSmsInfo->m_strSmsType, 
			                              m_pSMSSmsInfo->m_uSmsFrom,
                                          m_pSMSSmsInfo->m_uOperater,
                                          vecChannelGroups,
                                          userGw))
		{
			break;
		}

        LogError("[%s:%s] get channelGroups failed.  clientID[%s],  content[%s]",
        	               m_pSMSSmsInfo->m_strSmsId.data(),
                           m_pSMSSmsInfo->m_strPhone.data(),
                           m_pSMSSmsInfo->m_strClientId.data(),
                           m_pSMSSmsInfo->m_strContent.data());

        return CHANNEL_GET_FAILURE_NO_CHANNELGROUP;
    } while(0);

    return SelectChlFromChlGrp(vecChannelGroups, smsChannel);
}

// 使用CIntelWeightCalculator类做权重计算
weight_calculator_ptr_t CIntelligenceRouterStrategy::GetWeightCalculator(CRouterThread* pRouterThread,
                                                                         TSmsRouterReq* pSMSSmsInfo,
                                                                         ChannelGroupWeight* pChlGrpWeight)
{
    return weight_calculator_ptr_t(new CIntelWeightCalculator(pRouterThread, pSMSSmsInfo, pChlGrpWeight));
}

////////////////////////////////  智能路由策略使用的权重计算类 //////////////////////////////////
CIntelWeightCalculator::CIntelWeightCalculator(CRouterThread* pRouterThread,
                                               TSmsRouterReq* pSMSSmsInfo,
                                               ChannelGroupWeight* pChlGrpWeight)
    : CDefaultChannelWeightCalculator(pRouterThread, pSMSSmsInfo, pChlGrpWeight),
      m_iTotalConfigWeight(DEFAULT_IGNORED_WEIGHT_VALUE),
      m_iTotalWeightBySmstype(DEFAULT_IGNORED_WEIGHT_VALUE)
{
}

/*
 * 初始化相关的计算因子所需的数据
 * 如果某个因子初始化数据失败, 则忽略该因子
 */
bool CIntelWeightCalculator::init()
{
    CDefaultChannelWeightCalculator::init();

    // 1. 取系统权重配置
    std::vector<UInt32>& channelGroupWeightRatio = m_pRouterThread->GetChannelGroupWeightRatio();
    if (channelGroupWeightRatio.size() < 2)
    {
        LogError("%s Invalid Sys-Param 'CHANNELGTOUP_WEIGHT_RATIO'.", m_szLogPrefix);
        return false;
    }

    // 1. 取通道权重因子 和 通道成功率权重因子
    m_uChannelWeightFactor = channelGroupWeightRatio[0];
    m_uChannelSuccWeightFactor = channelGroupWeightRatio[1];

    // 2. 考虑通道权重因子
    do 
    {
        if (m_uChannelWeightFactor == 0) 
            break;

        if (m_pChlGrpWeight == NULL) 
        {
            LogWarn("%s NULL ChannelGroupWeight Pointer. Ignore channel-weight-factor", m_szLogPrefix);
            m_uChannelWeightFactor = 0;
            break;
        }

        m_iTotalConfigWeight = m_pChlGrpWeight->getTotalConfigWeight();
        if (m_iTotalConfigWeight == 0)
        {
            LogWarn("%s ChannelGroup[%u] 's total-configured-weight is 0! Ignore channel-weight-factor", 
                     m_szLogPrefix, m_pChlGrpWeight->getChannelGroupID());
            m_uChannelWeightFactor = 0;
            break;
        }

    } while(false);


    // 3. 考虑通道成功率因子
    do
    {
        if (m_uChannelSuccWeightFactor == 0) 
            break;

        if (m_pSMSSmsInfo == NULL || m_pRouterThread == NULL) 
        {
            LogWarn("%s pSMSSmsInfo/pHttpSendThread IS NULL. Ignore channel-succ-weight-factor", m_szLogPrefix);
            m_uChannelWeightFactor = 0;
        }
        // 取短信类型
        m_uSmsType = atoi(m_pSMSSmsInfo->m_strSmsType.c_str());

        channel_attribute_weight_config_t channelAttrWeightConfig = m_pRouterThread->GetChannelAttributeWeightConfig();

        ChannelAttributeWeightExvalue exValue = ConvertWeightSmstypeOrOperatorToExvalue(m_uSmsType, true);
        channel_attribute_weight_config_t::iterator iter = channelAttrWeightConfig.find(exValue);
        if (iter == channelAttrWeightConfig.end())
        {
            LogWarn("%s can't find smsType[%u]-exValue[%d] in channel-attribute-weight-conf", m_szLogPrefix, m_uSmsType, (int)exValue);
            m_uChannelWeightFactor = 0;
            break;
        }
        m_iTotalWeightBySmstype = iter->second.total_weight;

    } while(false);

    m_uTotalWeightFactor = m_uChannelWeightFactor + m_uChannelSuccWeightFactor;
    if (m_uChannelWeightFactor == 0 && m_uChannelSuccWeightFactor == 0) 
    {
        LogWarn("%s weight-factor&&succ-factor is 0&0. Ignore weight.", m_szLogPrefix);
        return false;
    }

    return true;
}

bool CIntelWeightCalculator::updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr)
{
    if (m_uChannelWeightFactor == 0 && m_uChannelSuccWeightFactor == 0)
    {
        LogWarn("%s weight_factor&succ_weight_factor is 0&0. Ignore weight.", m_szLogPrefix);
        return true;
    }

    UInt32 uRealtimeSuccWeight = DEFAULT_IGNORED_WEIGHT_VALUE;
    UInt32 uConfigWeight = DEFAULT_IGNORED_WEIGHT_VALUE;
    UInt32 uChannelId = weightInfoPtr->getData();

    UInt32 uPart1 = 0, uPart2 = 0;

    // 精度: 1W%
    // (通道权重因子 / 通道权重因子之和) *  通道权重/通道总权重 + (通道成功率权重因子 / 通道权重因子之和) * 通道成功率权重 / 通道成功率之和

    // 1. 通道权重因子计算
    do 
    {
        if (m_uChannelWeightFactor == 0) 
            break;
        
        uConfigWeight = weightInfoPtr->getWeight();
        // 除数不能为0
        if (this->m_uTotalWeightFactor == 0
                || this->m_iTotalConfigWeight == 0)
        {
            LogWarn("%s totalWeightFactor[%u] "
                    "or totalConfigWeight[%i] is 0. "
                    "Ignore channel-weight-factor", 
                    m_szLogPrefix,
                    this->m_uTotalWeightFactor,
                    this->m_iTotalConfigWeight);
            break;
        }
        uPart1 = (ONE_WAN * m_uChannelWeightFactor * uConfigWeight) / (this->m_uTotalWeightFactor * this->m_iTotalConfigWeight);

    } while(false);

    // 2. 通道成功率因子计算
    do 
    {
        if (m_uChannelSuccWeightFactor == 0)
            break;

        ChannelRealtimeWeightInfo channlRtWInfo;
        // 找到通道的成功率/价格等
        if (loadChannelRealtimeWeightInfo(uChannelId, channlRtWInfo) == false)
        {
            LogWarn("%s load channel-realtime-weight-info of channel[%u] failed. Ignore channel-weight-factor", m_szLogPrefix, uChannelId);
            break;
        }

        uRealtimeSuccWeight = channlRtWInfo.channel_succ_weight[m_uSmsType];

        // 除数不能为0
        if (this->m_uTotalWeightFactor == 0
                || m_iTotalWeightBySmstype == 0)
        {
            LogWarn("%s totalWeightFactor[%u] or totalWeightBySmstype[%i] is 0. Ignreo channel-weight-factor.", m_szLogPrefix, this->m_uTotalWeightFactor, m_iTotalWeightBySmstype);
            break;
        }
        uPart2 = (ONE_WAN * m_uChannelSuccWeightFactor * uRealtimeSuccWeight / (this->m_uTotalWeightFactor * m_iTotalWeightBySmstype));
    } while(false);

    int iNewEffectiveWeight = uPart1 + uPart2;
    weightInfoPtr->setEffectiveWeight(iNewEffectiveWeight);

    LogDebug("%s channel[%u] -> "
             " sysconfig-weight-ratio: [channel_weight:%d channel_succ_weight:%d total:%d]"
             " channelWeightInfo: [smstype:%d succ_weight:%d total_succ_weight:%d]"
             " channelGroupTotalWeight:[%d] thisChannelWeight:[%u] newEffectiveWeight: [%d]",
             m_szLogPrefix,
             weightInfoPtr->getData(),
             m_uChannelWeightFactor,
             m_uChannelSuccWeightFactor,
             m_uTotalWeightFactor,
             m_uSmsType,
             uRealtimeSuccWeight,
             m_iTotalWeightBySmstype,
             this->m_iTotalConfigWeight,
             uConfigWeight,
             iNewEffectiveWeight
            );


    return true;
}

