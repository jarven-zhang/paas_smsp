#include "CMPPHeartbeatResp.h"

namespace smsp
{

    CMPPHeartbeatResp::CMPPHeartbeatResp()
    {
        _commandId = CMPP_ACTIVE_TEST_RESP;

    }

    CMPPHeartbeatResp::~CMPPHeartbeatResp()
    {
        // TODO Auto-generated destructor stub
    }

    bool CMPPHeartbeatResp::Unpack(Reader& reader)
    {
        UInt8 reserve=0;
		UInt32 iBodySize = CMPPBase::bodySize();
		if(_totalLength > iBodySize)
        	reader(reserve);
        return reader;
    }

    UInt32 CMPPHeartbeatResp::bodySize() const
    {
        return CMPPBase::bodySize()+1;
    }

    bool CMPPHeartbeatResp::Pack(Writer &writer) const
    {
        UInt8 reserve=0;
        CMPPBase::Pack(writer);
        writer(reserve);
        return writer;
    }

} /* namespace smsp */
