#ifndef __CHANNEL_SEGMENT_H__
#define __CHANNEL_SEGMENT_H__
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class ChannelSegment
    {
    public:
        ChannelSegment();
        virtual ~ChannelSegment();

        ChannelSegment(const ChannelSegment& other);

        ChannelSegment& operator =(const ChannelSegment& other);

    public:
		//t_sms_segment_channel��
        std::string phone_segment;	//�����ֻ����������ֻ��Ŷ�!!!
        UInt32 channel_id;			//ͨ��ID
        std::string client_code;	//�ͻ�����
        UInt32 type;				//���ͣ�0:�������û�·�ɣ�1:�������û���·�ɣ��û���ָ�ͻ������ж�Ӧ���û�������client_codeΪ��ʱ��typeΪ��
        UInt32 operatorstype;		//ͨ����Ӧ����Ӫ�����ͣ�0:ȫ�� 1:�ƶ� 2:��ͨ 3:���� 4:����
        UInt32 area_id;				//����id(ʡ)����Ӧt_sms_area����һ��area_id(ʡ)
        UInt32 route_type;			//ǿ��·�����ͣ�0:�����ֻ����� 1:�����ֻ��Ŷ� 2:�����ֻ��Ŷ�
        UInt32 send_type;           //�������ͣ�0:��ҵ   1:Ӫ��
    };

}

#endif    //__CHANNEL_SEGMENT_H__

