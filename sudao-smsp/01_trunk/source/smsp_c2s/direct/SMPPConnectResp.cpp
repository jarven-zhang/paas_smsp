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
        _commandId = BIND_TRANSCEIVER_RESP;
        _systemId = "";
    }

    SMPPConnectResp::~SMPPConnectResp()
    {
    }

    UInt32 SMPPConnectResp::bodySize() const
    {
        return SMPPBase::bodySize() + (_systemId.length()<16?(_systemId.length()+1):(_systemId.length())); 
    }
    
    bool SMPPConnectResp::Pack(pack::Writer& writer) const
    {   
        char systemId[35] = {0};
        snprintf(systemId,35,"%s",_systemId.data());
        
        SMPPBase::Pack(writer);
        writer((UInt32)(_systemId.length()<16?(_systemId.length()+1):(_systemId.length())), systemId);
        return writer;
    }
} /* namespace smsp */
