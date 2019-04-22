#include "CMPP3SubmitResp.h"
#include "main.h"

namespace cmpp3
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
		return 8 + 4 + CMPPBase::bodySize();	
	}
    bool CMPPSubmitResp::Unpack(Reader& reader)
    {
        reader(_msgID)(_result);
		//add by fangjinxiong 20161117.
		int ileftByte = _totalLength - bodySize();	//rewrite bodySize
		if(ileftByte > 0)
		{
			LogDebug("ileftByte[%d]", ileftByte);
			string strtmp;
			reader((UInt32)ileftByte, strtmp);
		}
		//add end
		
        return reader;
    }

	bool CMPPSubmitResp::Pack(Writer& writer) const    
	{    	
		CMPPBase::Pack(writer);        
		writer(_msgID)(_result);        
		return writer;    
	}

} /* namespace smsp */
