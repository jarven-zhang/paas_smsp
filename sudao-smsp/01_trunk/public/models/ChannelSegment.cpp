#include "ChannelSegment.h"
namespace models
{

    ChannelSegment::ChannelSegment()
    {
    
    }

    ChannelSegment::~ChannelSegment()
    {

    }

    ChannelSegment::ChannelSegment(const ChannelSegment& other)
    {
        this->phone_segment = other.phone_segment;
		this->channel_id = other.channel_id;
		this->client_code = other.client_code;
		this->type = other.type;
		this->operatorstype = other.operatorstype;
		this->area_id = other.area_id;
		this->route_type = other.route_type;
		this->send_type = other.send_type;
    }

    ChannelSegment& ChannelSegment::operator =(const ChannelSegment& other)
    {
        this->phone_segment = other.phone_segment;
		this->channel_id = other.channel_id;
		this->client_code = other.client_code;
		this->type = other.type;
		this->operatorstype = other.operatorstype;
		this->area_id = other.area_id;
		this->route_type = other.route_type;
		this->send_type = other.send_type;
        return *this;
    }
} /* namespace models */

