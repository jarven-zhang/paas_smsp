#ifndef __MQ_GROUP_WEIGHT__H__
#define __MQ_GROUP_WEIGHT__H__

#include <string>
#include <map>
#include "WeightInfo.h"

using namespace models;

//////////////////////////////// ����MQ��Ȩ����Ϣ //////////////////////////////////
typedef WeightCalculatorBase<UInt64> MQWeightDefaultCalculator;

class MQWeightInfo : public WeightInfo<UInt64>
{
    public:
        MQWeightInfo(UInt64 mqId, int iWeight);
};


//////////////////////////////// MQ���Ȩ����Ϣ //////////////////////////////////
class MQWeightGroup : public WeightInfoGroup<UInt64>
{
    public:
        MQWeightGroup(const std::string& strMQGroupName);
        virtual ~MQWeightGroup();

        /*
         * ��Ȩ�����򣬲�ʹ��MQWeightDefaultCalculator& ʵ��������ЧȨ�صĸ���
         */
        bool weightSort(MQWeightDefaultCalculator& );
        bool updateSelectedMQ(iterator iterMQ);

        /*
         * �����µ�ͨ��������channelId �� Ȩ����Ϣ
         */
        void addMQWeightInfo(UInt64 mqId, int iWeight);


        std::string getMQGroupName() const;

    protected:

        std::string m_strMQGroupName;
};

typedef std::map<std::string, MQWeightGroup> mq_weightgroups_map_t ;

#endif
