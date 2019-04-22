#ifndef __CHANNEL_WEIGHT_H__
#define __CHANNEL_WEIGHT_H__

#include "WeightInfo.h"
#include "platform.h"
#include "boost/shared_ptr.hpp"
#include "SMSSmsInfo.h"
#include "BusTypes.h"

#include <map>

using namespace models;

class CRouterThread;

//////////////////////////////// 单个通道的权重信息 //////////////////////////////////
class ChannelWeightInfo : public WeightInfo<UInt32>
{
    public:
        ChannelWeightInfo(UInt32 uChannelId, int iWeight);
};

//////////////////////////////// 单个通道实时权重信息 //////////////////////////////////
struct ChannelRealtimeWeightInfo 
{
    ChannelRealtimeWeightInfo();

    int low_consume_flag;
    int customer_relation_flag;
    int channel_succ_weight[BusType::SMS_TYPE_FLUSH_SMS+1];    // 成功率权重
    int channel_price_weight[BusType::FOREIGN+1];              // 价格权重
    int anti_complaint;
};

// 多个通道的实时权重信息
typedef std::map<UInt32, ChannelRealtimeWeightInfo> channels_realtime_weightinfo_t;

//////////////////////////////// 通道权重计算基类 //////////////////////////////////
class ChannelWeightCalculatorBase : public WeightCalculatorBase<UInt32>
{
};

////////////////////////////// 通道属性权重配置 ///////////////////////
enum ChannelAttributeWeightExvalue
{
   CAWE_NONE = -1,
   CAWE_SMSTYPE_VERIFY_CODE = 0,
   CAWE_SMSTYPE_NOTICE = 1,
   CAWE_SMSTYPE_MARKET = 2,
   CAWE_SMSTYPE_ALRM = 3,
   CAWE_OPERATOR_YD = 4, 
   CAWE_OPERATOR_LT = 5, 
   CAWE_OPERATOR_DX = 6, 
};

enum ChannelWeightInfoExtFlag {
    CUSTOMER_RELATION_IDX = 0,
    LOW_CONSUME_IDX = 1
};

ChannelAttributeWeightExvalue ConvertWeightSmstypeOrOperatorToExvalue(int iValue, bool isSmstype);

////////////////////////// 业务区间对应权重类 //////////////////////////
struct RangeWeight
{
    int start;
    int end;
    int weight;
};
typedef std::vector<RangeWeight> ranges_weight_t;

// 一个业务权重配置的所有区间
struct ChannelWeightRangeConf
{
    ranges_weight_t ranges_weight;  // 各个区间
    int total_weight;              // 所有区间对应的权重之和
};

// 各类型对应的区间
typedef std::map<ChannelAttributeWeightExvalue, ChannelWeightRangeConf> channel_attribute_weight_config_t;
typedef boost::shared_ptr<channel_attribute_weight_config_t> channel_attribute_weight_config_ptr_t;





/////////////////////////////////// 通道池策略类 ///////////////////////////
const int DEFAULT_POOL_POLICY_ID = -1;
struct ChannelPoolPolicy
{

    ChannelPoolPolicy(const std::string& strPolicyName,
                       UInt32 success_factor,
                       UInt32 price_factor,
                       UInt32 anti_complaint_factor,
                       UInt32 customer_relation_factor,
                       UInt32 low_consume_factor);
    ChannelPoolPolicy();

    // 计算权重因子占比
    void calcFactorRatio();

    ///////////////////////////////
    int id;
    std::string policy_name;

    UInt32 success_factor;                     // 通道成功率
    UInt32 price_factor;                       // 通道单价
    UInt32 anti_complaint_factor;              // 抗投诉率
    UInt32 customer_relation_factor;            // 客情
    UInt32 low_consume_factor;                 // 低销

    UInt32 total_factor;

    UInt32 success_factor_ratio;                     // 通道成功率占比
    UInt32 price_factor_ratio;                       // 通道单价占比
    UInt32 anti_complaint_factor_ratio;              // 抗投诉率占比
    UInt32 customer_relation_factor_ratio;            // 客情占比
    UInt32 low_consume_factor_ratio;                 // 低销占比
};

typedef std::map<int, ChannelPoolPolicy> channel_pool_policies_t;
#endif
