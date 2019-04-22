#ifndef SGIPDELIVERREQ__
#define SGIPDELIVERREQ__
#include "SGIPBase.h"

class SGIPDeliverReq: public SGIPBase
{
public:
	SGIPDeliverReq();
	virtual ~SGIPDeliverReq();
public:
	virtual bool Unpack(Reader &reader);
public:
	//SGIP_DELIVER
	std::string m_strPhone; ///21
	std::string m_strSpNum; ///21
	UInt8 m_uTpPid;         ///1
	UInt8 m_uTpUdhi;        ///1  
	UInt8 m_uMsgCoding;     ///1
	UInt32 m_uMsgLength;    ///4
	std::string m_strMsgContent;
	std::string m_strReserve;///8
};
#endif ////SGIPDELIVERREQ__
