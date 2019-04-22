#include "CMPP3DeliverResp.h"

namespace cmpp3
{

    CMPPDeliverResp::CMPPDeliverResp()
    {
        _commandId=CMPP_DELIVER_RESP;
        _msgId=0;
        _result=0;
    }

    CMPPDeliverResp::~CMPPDeliverResp()
    {
        // TODO Auto-generated destructor stub
    }

	UInt32 CMPPDeliverResp::bodySize() const
    {
        return CMPPBase::bodySize()+12;
    }
	bool CMPPDeliverResp::Pack(Writer& writer) const
    {
        CMPPBase::Pack(writer);
        writer(_msgId)(_result);
        return writer;
    }
	bool CMPPDeliverResp::Unpack(Reader& reader)    
	{        
		reader(_msgId)(_result);        
		return reader;    
	}
} /* namespace smsp */
