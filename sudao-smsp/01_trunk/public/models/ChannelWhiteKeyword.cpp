#include "ChannelWhiteKeyword.h"
namespace models
{

    ChannelWhiteKeyword::ChannelWhiteKeyword()
    {
    
    }

    ChannelWhiteKeyword::~ChannelWhiteKeyword()
    {

    }

    ChannelWhiteKeyword::ChannelWhiteKeyword(const ChannelWhiteKeyword& other)
    {
        this->sign = other.sign;
		this->white_keyword = other.white_keyword;
		this->channelgroup_id = other.channelgroup_id;
		this->client_code = other.client_code;
		this->operatorstype = other.operatorstype;
		this->send_type = other.send_type;
    }

    ChannelWhiteKeyword& ChannelWhiteKeyword::operator =(const ChannelWhiteKeyword& other)
    {
        this->sign = other.sign;
		this->white_keyword = other.white_keyword;
		this->channelgroup_id = other.channelgroup_id;
		this->client_code = other.client_code;
		this->operatorstype = other.operatorstype;
		this->send_type = other.send_type;
        return *this;
    }
} /* namespace models */

