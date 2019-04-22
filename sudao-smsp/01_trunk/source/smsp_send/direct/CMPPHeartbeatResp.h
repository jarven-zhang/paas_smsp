#ifndef CMPPHEARTBEATRESP_H_
#define CMPPHEARTBEATRESP_H_
#include "CMPPBase.h"
namespace smsp
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
