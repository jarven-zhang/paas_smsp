#ifndef SGIPCONNECTRESP_H_
#define SGIPCONNECTRESP_H_
#include "SGIPBase.h"

class SGIPConnectResp: public SGIPBase
{
public:
	SGIPConnectResp();
	virtual ~SGIPConnectResp();
	SGIPConnectResp(const SGIPBase& other)
	{
		this->packetLength_=other.packetLength_;
		this->requestId_=other.requestId_;
		this->sequenceIdNode_=other.sequenceIdNode_;
		this->sequenceIdTime_=other.sequenceIdTime_;
		this->sequenceId_=other.sequenceId_;
	}
	virtual UInt32 bodySize() const;
public:
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);
	bool isLogin()
	{
		return isOK_;
	}

public:
	//SGIP_BIND_RESP
	UInt8 result_;          ////1
	std::string reserve_;   ////8
	bool isOK_;
};

#endif
