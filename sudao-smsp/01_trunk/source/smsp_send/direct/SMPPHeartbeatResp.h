#ifndef SMPPHEARTBEATRESP_H_
#define SMPPHEARTBEATRESP_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPHeartbeatResp : public SMPPBase
    {
    public:
        SMPPHeartbeatResp();
        virtual ~SMPPHeartbeatResp();
        virtual bool Unpack(pack::Reader &reader);
    };

} /* namespace smsp */
#endif /* SMPPHEARTBEATRESP_H_ */
