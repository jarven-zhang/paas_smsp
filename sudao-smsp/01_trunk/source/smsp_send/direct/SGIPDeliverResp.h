#ifndef SGIPDELIVERRESP__
#define SGIPDELIVERRESP__
#include "SGIPBase.h"

class SGIPDeliverResp: public SGIPBase
{
public:

	SGIPDeliverResp();
	SGIPDeliverResp(const SGIPBase& other)
	{
		this->packetLength_=other.packetLength_;
		this->requestId_=other.requestId_;
		this->sequenceIdNode_=other.sequenceIdNode_;
		this->sequenceIdTime_=other.sequenceIdTime_;
		this->sequenceId_=other.sequenceId_;
	}

	virtual UInt32 bodySize() const;
	virtual ~SGIPDeliverResp();
public:
	virtual bool Pack(Writer &writer) const;
public:
	//SGIP_DELIVER_RESP
	UInt8 result_;          ////1
	std::string reserve_;   ////8
};
#endif ////SGIPDELIVERRESP__
