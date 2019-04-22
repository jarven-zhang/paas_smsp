#include "CMPP3QueryReq.h"

namespace cmpp3
{
    
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
    	std::string time;
		reader((UInt32) 8, _time)(_queryType)((UInt32) 10, _queryCode)((UInt32) 8, _reserve);
        return reader;
    }

} /* namespace smsp */
