#include "SMPPConnectReq.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssl/md5.h"
#include "main.h"
namespace smsp
{

    SMPPConnectReq::SMPPConnectReq()
    {

        _commandId=BIND_TRANSCEIVER;

        _systemId = "";
        _password = "";
        _systemType = "";

        _version = (UInt8)0x34;
        _addrTon = 0;
        _addrNpi = 0;
        _addrRange = "";

    }


    SMPPConnectReq::~SMPPConnectReq()
    {
    }

    UInt32 SMPPConnectReq::bodySize() const
    {
        /////return _systemId.length()+1 + (UInt32)_password.length()+1 + _systemType.length()+1 + 1 + 1 + 1 + _addrRange.length()+1 + SMPPBase::bodySize();
		return (_systemId.length()<16?(_systemId.length()+1):(_systemId.length())) +
        (_password.length()<9?(_password.length()+1):(_password.length())) +
        (_systemType.length()<13?(_systemType.length()+1):(_systemType.length())) + 3 +
        (_addrRange.length()<41?(_addrRange.length()+1):(_addrRange.length())) +
        SMPPBase::bodySize();
    }

    bool SMPPConnectReq::Pack(pack::Writer& writer) const
    {
        SMPPBase::Pack(writer);
        char systemId[32] = {0};
        char password[32] = {0};
        char systemType[32] = {0};
        char addrRange[64] = {0};
        snprintf(systemId,32,"%s",_systemId.data());
        snprintf(password,32,"%s",_password.data());
        snprintf(systemType,32,"%s",_systemType.data());
        snprintf(addrRange,64,"%s",_addrRange.data());

        LogDebug("systemId[%s],password[%s]",systemId,password);
        writer((UInt32)(_systemId.length()<16?(_systemId.length()+1):(_systemId.length())), systemId)
        ((UInt32)(_password.length()<9?(_password.length()+1):(_password.length())), password)
        ((UInt32)(_systemType.length()<13?(_systemType.length()+1):(_systemType.length())), systemType)
        (_version)(_addrTon)(_addrNpi)
        ((UInt32)(_addrRange.length()<41?(_addrRange.length()+1):(_addrRange.length())), addrRange);
        return writer;
    }

    bool SMPPConnectReq::Unpack(pack::Reader& reader)
    {
        return false;
    }

} /* namespace smsp */
