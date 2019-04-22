#include "CMPPSubmitResp.h"
#include "main.h"

namespace smsp
{

    CMPPSubmitResp::CMPPSubmitResp()
    {
        _msgID=0;
        _result=0;

    }

    CMPPSubmitResp::~CMPPSubmitResp()
    {
        // TODO Auto-generated destructor stub
    }

    bool CMPPSubmitResp::Unpack(Reader& reader)
    {
        reader(_msgID)(_result);

		//add by fangjinxiong 20161117.
		int ileftByte = _totalLength -8 -1 -CMPPBase::bodySize();	//rewrite bodySize
		if(ileftByte > 0)
		{
			LogDebug("ileftByte[%d]", ileftByte);
			string strtmp;
			reader((UInt32)ileftByte, strtmp);
		}
		//add end
		
        return reader;
    }

} /* namespace smsp */
