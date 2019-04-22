#ifndef SGIPSUBMITRESP_H_
#define SGIPSUBMITRESP_H_
#include "SGIPBase.h"

class SGIPSubmitResp:public SGIPBase
{
public:
	SGIPSubmitResp();
	SGIPSubmitResp(const SGIPBase& other)
	{
		this->packetLength_=other.packetLength_;
		this->requestId_=other.requestId_;
		this->sequenceIdNode_=other.sequenceIdNode_;
		this->sequenceIdTime_=other.sequenceIdTime_;
		this->sequenceId_=other.sequenceId_;
	}

	virtual ~SGIPSubmitResp();
	virtual UInt32 bodySize() const;
public:
	virtual bool Unpack(Reader &reader);
	virtual bool Pack(Writer &writer) const;
public:
	UInt8 _result;
	std::string _reserve;
	std::string _msgId;
};
#endif /* SGIPSUBMITRESP_H_ */
