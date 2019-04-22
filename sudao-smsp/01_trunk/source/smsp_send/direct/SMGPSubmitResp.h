#ifndef SMGPSUBMITRESP_H_
#define SMGPSUBMITRESP_H_
#include "SMGPBase.h"

class SMGPSubmitResp:public SMGPBase
{
public:
	SMGPSubmitResp();
	virtual ~SMGPSubmitResp();
public:
	virtual bool Unpack(Reader &reader);
public:
	std::string msgId_;
	UInt32 status_;

};
#endif /* SMGPSUBMITRESP_H_ */
