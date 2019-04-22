#ifndef SMPPHEARTBEATREQ_H_
#define SMPPHEARTBEATREQ_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPHeartbeatReq : public SMPPBase
    {
    public:
        SMPPHeartbeatReq();
        virtual ~SMPPHeartbeatReq();
    };

} /* namespace smsp */
#endif /* SMPPHEARTBEATREQ_H_ */
