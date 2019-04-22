#include "CMPPSubmitResp.h"

namespace smsp
{

    CMPPSubmitResp::CMPPSubmitResp()
    {
        _msgID=0;
        _result=0;
		_commandId = CMPP_SUBMIT_RESP;

    }

    CMPPSubmitResp::~CMPPSubmitResp()
    {
        // TODO Auto-generated destructor stub
    }

	UInt32 CMPPSubmitResp::bodySize() const 
	{
		return 8 + 1 + CMPPBase::bodySize();
	}

    bool CMPPSubmitResp::Unpack(Reader& reader)
    {
        reader(_msgID)(_result);
        ////printf("msgID:[%lu],result:[%d]\n",_msgID,_result);
        return reader;
    }

	bool CMPPSubmitResp::Pack(Writer& writer) const
    {
    	CMPPBase::Pack(writer);
        writer(_msgID)(_result);
        return writer;
    }
	
} /* namespace smsp */
