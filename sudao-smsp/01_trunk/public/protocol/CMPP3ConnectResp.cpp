#include "CMPP3ConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
namespace cmpp3
{

    CMPPConnectResp::CMPPConnectResp()
    {
         _isOK = false;        
		 _status = 0;        
		 _version = (UInt8)0x30;		
		 _commandId = CMPP_CONNECT_RESP;
    }

    CMPPConnectResp::~CMPPConnectResp()
    {
    }

	UInt32 CMPPConnectResp::bodySize() const 
	{
		return 4 + 16 + 1 + CMPPBase::bodySize();
	}

    bool CMPPConnectResp::Pack(Writer& writer) const
    {
        CMPPBase::Pack(writer);		
		char authISMG[16]= {0};	//todo...not null.        
		writer(_status)((UInt32)16, authISMG)(_version);        
		return writer;
    }

    bool CMPPConnectResp::Unpack(Reader& reader)
    {
        char authISMG[17]= {0};
		
        reader(_status)(16, authISMG)(_version);
        if (_status == 0)
        {
            _isOK = true;
        }
        else
        {
            LogWarn("Cmpp3.0 connect state[%d].",_status);
        }

		//add by fangjinxiong 20161117.
		int ileftByte = _totalLength -4 -16 -1 - CMPPBase::bodySize();
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
