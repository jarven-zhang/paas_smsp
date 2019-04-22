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
		//t_sms_segment_channel表
        std::string phone_segment;	//国内手机号码或国外手机号段!!!
        UInt32 channel_id;			//通道ID
        std::string client_code;	//客户代码
        UInt32 type;				//类型，0:包含的用户路由；1:包含的用户不路由；用户是指客户代码中对应的用户，仅当client_code为空时，type为空
        UInt32 operatorstype;		//通道对应的运营商类型，0:全网 1:移动 2:联通 3:电信 4:国际
        UInt32 area_id;				//地区id(省)，对应t_sms_area表中一级area_id(省)
        UInt32 route_type;			//强制路由类型，0:国内手机号码 1:国内手机号段 2:国外手机号段
        UInt32 send_type;           //发送类型，0:行业   1:营销
    };

}

#endif    //__CHANNEL_SEGMENT_H__

