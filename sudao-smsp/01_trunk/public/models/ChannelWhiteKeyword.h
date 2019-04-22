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
		//t_sms_white_keyword_channel��
        std::string sign;	        //����ǩ��
        std::string white_keyword; // �׹ؼ���
        UInt32 channelgroup_id;	    //ͨ����ID
        std::string client_code;	//�ͻ�����
        UInt32 operatorstype;		//ͨ�����Ӧ����Ӫ�����ͣ�0:ȫ�� 1:�ƶ� 2:��ͨ 3:���� 4:����
        UInt32 send_type;           //�������ͣ�0:��ҵ   1:Ӫ��
    };

	typedef std::list<ChannelWhiteKeyword> ChannelWhiteKeywordList;
	typedef std::map<string,ChannelWhiteKeywordList> ChannelWhiteKeywordMap;
}

#endif    //__CHANNEL_SEGMENT_H__

