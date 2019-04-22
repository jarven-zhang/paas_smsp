#include "WeightInfo.h"
#include "platform.h"
#include "boost/shared_ptr.hpp"
#include "BusTypes.h"
#include "MQWeightGroup.h"
#include "LogMacro.h"


////////////////////////////////  更新有效权重 //////////////////////////////////
MQWeightInfo::MQWeightInfo(UInt64 uMQId, int iWeight)
    : WeightInfo<UInt64>(uMQId, iWeight)
{
    this->m_dataName = "MQWeight";
}

////////////////////////////////  更新有效权重 //////////////////////////////////
MQWeightGroup::MQWeightGroup(const std::string& strMQGroupName)
    : m_strMQGroupName(strMQGroupName)
{
}

MQWeightGroup::~MQWeightGroup()
{
}

bool MQWeightGroup::weightSort(MQWeightDefaultCalculator& calculator)
{
    // 正常选择，第二个参数为false，这样后面不需要再调用UpdateSelect...
    bool ret = sortDataByWeight(calculator, false);
    std::string strAllWeightInfo;
    dumpAllWeightInfo(strAllWeightInfo, " >><< ");
    LogNotice("MQGroup-WeightInfo<%u>: %s", this->m_strMQGroupName.c_str(), strAllWeightInfo.c_str());

    return ret;
}

void MQWeightGroup::addMQWeightInfo(UInt64 mqId, int iWeight)
{
    weightinfo_ptr_t ptrNewWeightInfo(new MQWeightInfo(mqId, iWeight));
    addWeightInfo(ptrNewWeightInfo);
}

std::string MQWeightGroup::getMQGroupName() const
{
    return m_strMQGroupName;
}
