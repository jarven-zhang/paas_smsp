#ifndef __CHANNEL_WHITEKEYWORD_H__
#define __CHANNEL_WHITEKEYWORD_H__
#include "platform.h"

#include <string>
#include <vector>
#include <iostream>
#include "boost/regex.hpp"
using namespace std;
using namespace boost;
namespace models
{

    class ChannelWhiteKeyword
    {
    public:
        ChannelWhiteKeyword();
        virtual ~ChannelWhiteKeyword();

        ChannelWhiteKeyword(const ChannelWhiteKeyword& other);

        ChannelWhiteKeyword& operator =(const ChannelWhiteKeyword& other);

    public:
		//t_sms_white_keyword_channel表
        std::string sign;	        //短信签名
        std::string white_keyword; // 白关键字
        UInt32 channelgroup_id;	    //通道组ID
        std::string client_code;	//客户代码
        UInt32 operatorstype;		//通道组对应的运营商类型，0:全网 1:移动 2:联通 3:电信 4:国际
        UInt32 send_type;           //发送类型，0:行业   1:营销
    };

	typedef std::list<ChannelWhiteKeyword> ChannelWhiteKeywordList;
	typedef std::map<string,ChannelWhiteKeywordList> ChannelWhiteKeywordMap;
}

#endif    //__CHANNEL_SEGMENT_H__

