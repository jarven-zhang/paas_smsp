#include "ChannelWeight.h"
#include "main.h"

//////////////////////////////// ChannelWeightInfo //////////////////////////////////

ChannelWeightInfo::ChannelWeightInfo(UInt32 uChannelId, int iWeight)
    : WeightInfo<UInt32>(uChannelId, iWeight)
{
    this->m_dataName = "ChannelId";
}

//////////////////////////////// 单个通道实时权重信息 //////////////////////////////////
ChannelRealtimeWeightInfo::ChannelRealtimeWeightInfo()
    : low_consume_flag(0),
      customer_relation_flag(0),
      anti_complaint(0)
{
    memset(&channel_succ_weight[0], 0, sizeof(channel_succ_weight));
    memset(&channel_price_weight[0], 0, sizeof(channel_price_weight));
}

/////////////////////////////////// 通道池策略类 ///////////////////////////
ChannelPoolPolicy::ChannelPoolPolicy(const std::string& strPolicyName,
                                     UInt32 success_factor,
                                     UInt32 price_factor,
                                     UInt32 anti_complaint_factor,
                                     UInt32 customer_relation_factor,
                                     UInt32 low_consume_factor)
    : policy_name(strPolicyName),
      success_factor(success_factor),
      price_factor(price_factor),
      anti_complaint_factor(anti_complaint_factor),
      customer_relation_factor(customer_relation_factor), 
      low_consume_factor(low_consume_factor)
{
    calcFactorRatio();
}

ChannelPoolPolicy::ChannelPoolPolicy()
    : success_factor(100),
      price_factor(0),
      anti_complaint_factor(0),
      customer_relation_factor(0), 
      low_consume_factor(0) 
{
    calcFactorRatio();
}

void ChannelPoolPolicy::calcFactorRatio()
{
    total_factor = low_consume_factor 
        + customer_relation_factor 
        + success_factor 
        + price_factor
        + anti_complaint_factor;

    if (total_factor == 0) 
    {
        LogError("channel-pool-policy[%s]'s total_factor is 0, INVALID!", policy_name.c_str());

        success_factor_ratio = 0;
        price_factor_ratio = 0;
        anti_complaint_factor_ratio = 0;
        customer_relation_factor_ratio = 0;
        low_consume_factor_ratio = 0;

        return;
    }

    // 精度，1W%
    success_factor_ratio = ONE_WAN * success_factor / total_factor;
    price_factor_ratio = ONE_WAN * price_factor / total_factor;
    anti_complaint_factor_ratio = ONE_WAN * anti_complaint_factor / total_factor;
    customer_relation_factor_ratio = ONE_WAN * customer_relation_factor / total_factor;
    low_consume_factor_ratio = ONE_WAN * low_consume_factor / total_factor;
}

///////////////////////// 通道权重属性中ex_value对应短信类型/运营商的转换 ///////////////////
ChannelAttributeWeightExvalue ConvertWeightSmstypeOrOperatorToExvalue(int iValue, bool isSmstype)
{
    ChannelAttributeWeightExvalue exValue = CAWE_NONE;
    // 转换短信类型
    if (isSmstype) 
    {
        switch(iValue) {
            case BusType::SMS_TYPE_NOTICE:
                exValue = CAWE_SMSTYPE_NOTICE;
                break;
            case BusType::SMS_TYPE_VERIFICATION_CODE:
                exValue = CAWE_SMSTYPE_VERIFY_CODE;
                break;
            case BusType::SMS_TYPE_MARKETING:
                exValue = CAWE_SMSTYPE_MARKET;
                break;
            case BusType::SMS_TYPE_ALARM:
                exValue = CAWE_SMSTYPE_ALRM;
                break;
            default:
                break;
        }
    }
    else // 转换运营商类型
    {
        switch(iValue) {
            case BusType::YIDONG:
                exValue = CAWE_OPERATOR_YD;
                break;
            case BusType::LIANTONG:
                exValue = CAWE_OPERATOR_LT;
                break;
            case BusType::DIANXIN:
                exValue = CAWE_OPERATOR_DX;
                break;
            default:
                break;
        }
    }

    return exValue;
}
