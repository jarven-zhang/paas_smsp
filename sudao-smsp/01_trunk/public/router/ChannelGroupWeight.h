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
         * ��Ȩ�����򣬲�ʹ��ChannelWeightCalculatorBase& ʵ��������ЧȨ�صĸ���
         */
        bool weightSort(ChannelWeightCalculatorBase& );
        bool updateSelectedChannel(iterator iterChannel);

        /*
         * �����µ�ͨ��������channelId �� Ȩ����Ϣ
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
        // ʵ�ֻ���ӿ�
        virtual bool init();
        virtual bool updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr);
};


#endif
