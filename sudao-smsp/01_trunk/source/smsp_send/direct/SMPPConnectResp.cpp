#include "SMPPConnectResp.h"
#include "ssl/md5.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
namespace smsp
{

    SMPPConnectResp::SMPPConnectResp()
    {
        _isOK = false;
        _systemId = "";
    }

    SMPPConnectResp::~SMPPConnectResp()
    {
    }

    bool SMPPConnectResp::Pack(pack::Writer& writer) const
    {
        return false;
    }

    bool SMPPConnectResp::Unpack(pack::Reader& reader)
    {
        reader(_commandLength-bodySize(),_systemId);
        if (_commandStatus==0)
        {
            _isOK = true;
        }
        else
        {
        	_isOK = false;
            LogWarn("Smpp connect commandStatus[%d] _isOK[%d].",_commandStatus, _isOK);
        }
        return reader;
    }

} /* namespace smsp */
