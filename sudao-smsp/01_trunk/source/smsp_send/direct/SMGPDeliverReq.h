#ifndef SMGPDELIVERREQ_H_
#define SMGPDELIVERREQ_H_
#include "SMGPBase.h"

class SMGPDeliverReq: public SMGPBase
{
public:
	SMGPDeliverReq();
	virtual ~SMGPDeliverReq();
public:
	virtual bool Unpack(Reader &reader);
public:
	// 10 ,1,1,14,21,21,1, len 8
	std::string _msgId;
	UInt8 _isReport;
	UInt8 _msgFmt;
	std::string _recvTime;
	std::string _srcTermId;
	std::string _destTermId;
	
	UInt8 _msgLength;
	std::string _msgContent;
	std::string _reserve;
	UInt32 _status;
	std::string _remark;

	std::string m_strErrorCode;		//err:***      m_strErrorCode=***
};

#endif /* SMGPDELIVERREQ_H_ */
