#include "SGIPDeliverResp.h"


SGIPDeliverResp::SGIPDeliverResp()
{
    result_ = 0;
    reserve_ = "";
    requestId_ = SGIP_DELIVER_RESP;
}

SGIPDeliverResp::~SGIPDeliverResp()
{

}

UInt32 SGIPDeliverResp::bodySize() const
{
    return 9 + SGIPBase::bodySize();
}

bool SGIPDeliverResp::Pack(Writer &writer) const
{   
    /*
    SGIPBase::Pack(writer);
    char reserve[9]={0};
    snprintf(reserve,9,"%s",reserve_.data());
    
    writer(result_)((UInt32)8, reserve);
	return writer;
	*/

    return false;
}

bool SGIPDeliverResp::Unpack(Reader &reader)
{
    reader(result_)((UInt32)8,reserve_);

    return reader;
}