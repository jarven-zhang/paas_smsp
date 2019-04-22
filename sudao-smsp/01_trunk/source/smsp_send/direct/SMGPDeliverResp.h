#ifndef SMGPDELIVERRESP_H_
#define SMGPDELIVERRESP_H_
#include "SMGPBase.h"

class SMGPDeliverResp: public SMGPBase
{
public:
	SMGPDeliverResp();
	virtual ~SMGPDeliverResp();
public:
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
public:
	std::string _msgId;
	UInt32 _result;
};

#endif /* SMGPDELIVERRESP_H_ */
