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

    }

    CMPPConnectResp::~CMPPConnectResp()
    {
    }


    bool CMPPConnectResp::Pack(Writer& writer) const
    {
        return false;
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
        	_isOK = false;
            LogWarn("Cmpp connect state[%d].",_status);
        }

		//add by fangjinxiong 20161117.
		int ileftByte = _totalLength -1 -16 -1 - CMPPBase::bodySize();
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
