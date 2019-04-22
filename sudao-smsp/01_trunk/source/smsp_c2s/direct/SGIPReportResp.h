#ifndef SGIPREPORTRESP_H_
#define SGIPREPORTRESP_H_
#include "SGIPBase.h"

class SGIPReportResp : public SGIPBase 
{
public:
	SGIPReportResp();
	virtual ~SGIPReportResp();
public:
	virtual UInt32 bodySize() const;
	virtual bool Pack(Writer &writer) const;
	virtual bool Unpack(Reader &reader);
public:
	UInt8 _result;
	std::string _reserve;
};

#endif /* SGIPDELIVERRESP_H_ */
