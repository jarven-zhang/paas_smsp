#ifndef __MQ_GROUP_WEIGHT__H__
#define __MQ_GROUP_WEIGHT__H__

#include <string>
#include <map>
#include "WeightInfo.h"

using namespace models;

//////////////////////////////// 单个MQ的权重信息 //////////////////////////////////
typedef WeightCalculatorBase<UInt64> MQWeightDefaultCalculator;

class MQWeightInfo : public WeightInfo<UInt64>
{
    public:
        MQWeightInfo(UInt64 mqId, int iWeight);
};


//////////////////////////////// MQ组的权重信息 //////////////////////////////////
class MQWeightGroup : public WeightInfoGroup<UInt64>
{
    public:
        MQWeightGroup(const std::string& strMQGroupName);
        virtual ~MQWeightGroup();

        /*
         * 按权重排序，并使用MQWeightDefaultCalculator& 实现类做有效权重的更新
         */
        bool weightSort(MQWeightDefaultCalculator& );
        bool updateSelectedMQ(iterator iterMQ);

        /*
         * 加入新的通道，包含channelId 和 权重信息
         */
        void addMQWeightInfo(UInt64 mqId, int iWeight);


        std::string getMQGroupName() const;

    protected:

        std::string m_strMQGroupName;
};

typedef std::map<std::string, MQWeightGroup> mq_weightgroups_map_t ;

#endif
