#include "CMPPQueryReq.h"

namespace smsp
{
    char * timestampYear(char *buf)
    {
        time_t tt;
        struct tm now = {0};

        time(&tt);
        localtime_r(&tt,&now);
        snprintf(buf,64, "%04d%02d%02d", now.tm_year + 1900, now.tm_mon + 1,now.tm_mday);
        return buf;
    }
    CMPPQueryReq::CMPPQueryReq()
    {
        _commandId = CMPP_QUERY;
    }

    CMPPQueryReq::~CMPPQueryReq()
    {

    }

    UInt32 CMPPQueryReq::bodySize() const
    {
        return 27 + CMPPBase::bodySize();
    }

    bool CMPPQueryReq::Pack(Writer& writer) const
    {
        char time[80];
        timestampYear(time);
        UInt8 queryType = 0;
        char queryCode[10] = { 0 };
        char reserve[8] = { 0 };
        CMPPBase::Pack(writer);
        writer((UInt32) 8, time)(queryType)((UInt32) 10, queryCode)((UInt32) 8,
                reserve);
        return writer;
    }

    bool CMPPQueryReq::Unpack(Reader& reader)
    {
        return false;
    }

} /* namespace smsp */
