#include "SMGPDeliverResp.h"
#include <stdio.h>

namespace smsp
{
	SMGPDeliverResp::SMGPDeliverResp()
	{
		_msgId="";
		_result=0;
		requestId_ = SMGP_DELIVER_RESP;
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
	bool SMGPDeliverResp::Unpack(Reader& reader)
    {
        reader((UInt32)10,_msgId)(_result);
        return reader;
    }	
}
