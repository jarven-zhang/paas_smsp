#include "SMPPHeartbeatResp.h"

namespace smsp
{

    SMPPHeartbeatResp::SMPPHeartbeatResp()
    {
        // TODO Auto-generated constructor stub
        _commandId=ENQUIRE_LINK_RESP;
    }

    SMPPHeartbeatResp::~SMPPHeartbeatResp()
    {
        // TODO Auto-generated destructor stub
    }

    bool SMPPHeartbeatResp::Unpack(pack::Reader& reader)
    {
        return reader;
    }

} /* namespace smsp */
