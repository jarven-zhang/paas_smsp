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
        _systemId = "";
        _password = "";
        _systemType = "";

        _version = (UInt8)0x34;
        _addrTon = 0;
        _addrNpi = 0;
        _addrRange = "";
        _buffer = "";
    }


    SMPPConnectReq::~SMPPConnectReq()
    {
    }
    
    bool SMPPConnectReq::readCOctetString(UInt32& position, UInt32 max, char* data)
    {
        const char *src = NULL;
        UInt32 size = 0;
        src = _buffer.data() + position;
        size = _buffer.size() - position;
        UInt32 i = 0;
        for(; i<max; i++)
        {
            if(src[i]==0x00)
            {
                i++;
                break;
            }
        }
        if (i <= size)
        {
            memcpy(data, src, i);
            position += i;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool SMPPConnectReq::Unpack(pack::Reader& reader)
    {
        UInt32 first = (UInt32)_commandLength - SMPPBase::bodySize();

        ////LogNotice("first[%d],reader size[%d].",first,reader.GetSize())

        
        if (reader(first, _buffer))
        {
            UInt32 position=0;
            char systemId[32] = {0};
            char password[32] = {0};
            char systemType[32] = {0};
            char addressRange[50] = {0};

            readCOctetString(position, 16, systemId);
            readCOctetString(position, 9, password);
            readCOctetString(position, 13, systemType);
            readCOctetString(position, 1, reinterpret_cast<char *>(&_version));
            readCOctetString(position, 1, reinterpret_cast<char *>(&_addrTon));
            readCOctetString(position, 1, reinterpret_cast<char *>(&_addrNpi));
            readCOctetString(position, 41, addressRange);

            _systemId = systemId;
            _password = password;
            _systemType = systemType;
            _addrRange = addressRange;

            LogDebug("systemId[%s],length[%d],password[%s],systemType[%s],version[%d],addrton[%d],addrnpi[%d],addrRange[%s]",
                _systemId.data(),_systemId.length(),_password.data(),_systemType.data(),_version,_addrTon,_addrNpi,_addrRange.data());
        }
        else
        {
            LogWarn("smpp connect unpack read is error.");
        }
        
        return reader;
    }

} /* namespace smsp */
