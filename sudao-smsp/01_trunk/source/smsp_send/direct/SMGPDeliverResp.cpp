#include "SMGPDeliverResp.h"
#include <stdio.h>
SMGPDeliverResp::SMGPDeliverResp()
{
	requestId_=SMGP_DELIVER_RESP;
	_msgId="";
	_result=0;
}

SMGPDeliverResp::~SMGPDeliverResp() 
{
}

UInt32 SMGPDeliverResp::bodySize() const 
{
	return SMGPBase::bodySize()+14;
}

bool SMGPDeliverResp::Pack(Writer& writer) const 
{
	SMGPBase::Pack(writer);
    char msgId[11] = {0};
    snprintf(msgId,11,"%s",_msgId.data());
    
	writer((UInt32)10, msgId)(_result);
	return writer;
}

