#include "CMPPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
namespace smsp
{

    CMPPConnectResp::CMPPConnectResp()
    {
        _isOK = false;
        _status = 0;
        _version = 0;
		_commandId = CMPP_CONNECT_RESP;
    }

    CMPPConnectResp::~CMPPConnectResp()
    {
    }

	UInt32 CMPPConnectResp::bodySize() const 
	{
		return 1 + 16 + 1 + CMPPBase::bodySize();
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
        char authISMG[16]= {0};
        reader(_status)((UInt32)16, authISMG)(_version);
        if (_status == 0)
        {
            _isOK = true;
        }
        else
        {
            LogWarn("Cmpp connect state[%d].",_status);
        }
        return reader;
    }

} /* namespace smsp */
