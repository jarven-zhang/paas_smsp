#include "WeightInfo.h"
#include "platform.h"
#include "boost/shared_ptr.hpp"
#include "BusTypes.h"
#include "MQWeightGroup.h"
#include "LogMacro.h"


////////////////////////////////  ������ЧȨ�� //////////////////////////////////
MQWeightInfo::MQWeightInfo(UInt64 uMQId, int iWeight)
    : WeightInfo<UInt64>(uMQId, iWeight)
{
    this->m_dataName = "MQWeight";
}

////////////////////////////////  ������ЧȨ�� //////////////////////////////////
MQWeightGroup::MQWeightGroup(const std::string& strMQGroupName)
    : m_strMQGroupName(strMQGroupName)
{
}

MQWeightGroup::~MQWeightGroup()
{
}

bool MQWeightGroup::weightSort(MQWeightDefaultCalculator& calculator)
{
    // ����ѡ�񣬵ڶ�������Ϊfalse���������治��Ҫ�ٵ���UpdateSelect...
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
