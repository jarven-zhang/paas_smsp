#include "SMPPHeartbeatReq.h"

namespace smsp
{

    SMPPHeartbeatReq::SMPPHeartbeatReq()
    {
        _commandId=ENQUIRE_LINK;
    }

    SMPPHeartbeatReq::~SMPPHeartbeatReq()
    {
    }

} /* namespace smsp */
