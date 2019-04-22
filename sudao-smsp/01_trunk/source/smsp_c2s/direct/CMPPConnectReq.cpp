#include "CMPPConnectReq.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssl/md5.h"
#include "main.h"

namespace smsp
{

    CMPPConnectReq::CMPPConnectReq()
    {
        _commandId = CMPP_CONNECT;
        _sourceAddr = "000000";
        _password = "000000";
        _version = (Int8) (0x20);
        _timestamp=0;
    }

    void CMPPConnectReq::createAuthSource()
    {
        char str[80];
        timestamp(str);
        _timestamp = atoi(str);
        unsigned char md5[16] = { 0 };
        char target[1024] = { 0 };
        char* pos = target;
        memcpy(pos, _sourceAddr.data(), _sourceAddr.length());
        UInt32 targetLen = _sourceAddr.length() + 9 + _password.size() + 10;
        pos += (_sourceAddr.length() + 9);

        memcpy(pos, _password.data(), _password.size());
        pos += _password.size();
        memcpy(pos, str, 10);

        MD5((const unsigned char*) target, targetLen, md5);
        _authSource.append((const char*) md5);
    }

    CMPPConnectReq::~CMPPConnectReq()
    {
    }

    UInt32 CMPPConnectReq::bodySize() const
    {
        return 6 + 16 + 1 + 4 + CMPPBase::bodySize();
    }

    bool CMPPConnectReq::Pack(Writer& writer) const
    {
        CMPPBase::Pack(writer);
        writer((UInt32) 6, _sourceAddr)((UInt32) 16, _authSource)(_version)(_timestamp);
        return writer;
    }

    bool CMPPConnectReq::Unpack(Reader& reader)
    {
    	if(reader.GetSize() == 0)
			return false;
		
		reader(6, _sourceAddr)(16, _authSource)(_version)(_timestamp);
		if (_commandId == CMPP_CONNECT)
		{
			//LogDebug("unpack CMPP_CONNECT, _sourceAddr[%s]", _sourceAddr.data());
        }
		
        return reader;
    }

} /* namespace smsp */
