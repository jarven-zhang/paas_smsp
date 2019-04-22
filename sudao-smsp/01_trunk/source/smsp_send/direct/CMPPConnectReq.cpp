#include "CMPPConnectReq.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "ssl/md5.h"
namespace smsp
{
    char * timestamp(char *buf)
    {
        time_t tt;
        struct tm now = {0};

        time(&tt);
        localtime_r(&tt,&now);
        snprintf(buf, 64,"%02d%02d%02d%02d%02d", now.tm_mon + 1, now.tm_mday,
                now.tm_hour, now.tm_min, now.tm_sec);
        return buf;
    }

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
        char sourceAddr[7] = {0};
        snprintf(sourceAddr,7,"%s",_sourceAddr.data());

        char authSource[17] = {0};
        snprintf(authSource,17,"%s",_authSource.data());
        
        writer((UInt32)6, sourceAddr)((UInt32)16, authSource)(_version)(_timestamp);
        return writer;
    }

    bool CMPPConnectReq::Unpack(Reader& reader)
    {
        return false;
    }

} /* namespace smsp */
