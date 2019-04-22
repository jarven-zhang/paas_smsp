#include "ChannelWeight.h"
#include "ChannelGroupWeight.h"

////////////////////////////////  更新有效权重 //////////////////////////////////
ChannelGroupWeight::ChannelGroupWeight(UInt32 uChlGrpID)
    : m_uChlGrpId(uChlGrpID)
{
}

ChannelGroupWeight::~ChannelGroupWeight()
{
}

bool ChannelGroupWeight::weightSort(ChannelWeightCalculatorBase& calculator)
{
    // 通道选择可能失败，所以第二个参数为true
    bool ret = sortDataByWeight(calculator, true);
    std::string strAllWeightInfo;
    dumpAllWeightInfo(strAllWeightInfo, " >><< ");
    LogNotice("ChannelGroup-WeightInfo<%u>: %s", this->m_uChlGrpId, strAllWeightInfo.c_str());

    return ret;
}

void ChannelGroupWeight::addChannelWeightInfo(UInt32 uChannelId, int iWeight)
{
    weightinfo_ptr_t ptrNewWeightInfo(new ChannelWeightInfo(uChannelId, iWeight));
    addWeightInfo(ptrNewWeightInfo);
}

UInt32 ChannelGroupWeight::getChannelGroupID() const
{
    return m_uChlGrpId;
}

bool ChannelGroupWeight::updateSelectedChannel(ChannelGroupWeight::iterator iterChannel)
{
    return updateSelectedCurrentWeight(iterChannel);
}

/////////////////////////////////////////////////////////////////////////
bool CDefaultWeightCalculator::init()
{
    return true;
}

bool CDefaultWeightCalculator::updateEffectiveWeight(ChannelGroupWeight::weightinfo_ptr_t weightInfoPtr)
{
    weightInfoPtr->m_iEffectiveWeight = weightInfoPtr->m_iWeight;

    return true;
}


