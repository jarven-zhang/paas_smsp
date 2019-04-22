#ifndef __CHANNEL_GROUP_WEIGHT__H__
#define __CHANNEL_GROUP_WEIGHT__H__

#include <string>
#include <map>
#include "ChannelWeight.h"
#include "SMSSmsInfo.h"
#include "LogMacro.h"

using namespace models;

class ChannelGroupWeight : public WeightInfoGroup<UInt32>
{
    public:
        ChannelGroupWeight(UInt32 uChlGrpID);
        virtual ~ChannelGroupWeight();

        /*
         * 按权重排序，并使用ChannelWeightCalculatorBase& 实现类做有效权重的更新
         */
        bool weightSort(ChannelWeightCalculatorBase& );
        bool updateSelectedChannel(iterator iterChannel);

        /*
         * 加入新的通道，包含channelId 和 权重信息
         */
        void addChannelWeightInfo(UInt32 uChannelId, int iWeight);


        UInt32 getChannelGroupID() const;

    protected:

        UInt32 m_uChlGrpId;
};

typedef std::map<UInt32, ChannelGroupWeight> channelgroups_weight_t;

/////////////////////////////////////////////////////////////////////////
class CDefaultWeightCalculator : public ChannelWeightCalculatorBase
{
    public:
        // 实现基类接口
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);
};


#endif
