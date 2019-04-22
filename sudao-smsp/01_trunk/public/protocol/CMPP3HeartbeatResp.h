#ifndef CMPP3HEARTBEATRESP_H_
#define CMPP3HEARTBEATRESP_H_
#include "CMPP3Base.h"
namespace cmpp3
{

    class CMPPHeartbeatResp : public CMPPBase
    {
    public:
        CMPPHeartbeatResp();
        virtual ~CMPPHeartbeatResp();
		virtual UInt32 bodySize() const;
        virtual bool Unpack(Reader &reader);
		virtual bool Pack(Writer &writer) const;
    };

} /* namespace smsp */
#endif /* CMPPHEARTBEATRESP_H_ */
