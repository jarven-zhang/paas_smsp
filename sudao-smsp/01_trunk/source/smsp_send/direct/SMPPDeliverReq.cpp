#include "SMPPDeliverReq.h"

namespace smsp
{

    SMPPDeliverReq::SMPPDeliverReq()
    {
        _commandId=DELIVER_SM_RESP;
    }

    SMPPDeliverReq::~SMPPDeliverReq()
    {
        // TODO Auto-generated destructor stub
    }

    UInt32 SMPPDeliverReq::bodySize() const
    {
        return SMPPBase::bodySize()+1;
    }

    bool SMPPDeliverReq::Pack(pack::Writer& writer) const
    {
        SMPPBase::Pack(writer);
        char msgId[1]= {0};
        writer((UInt32)1,msgId);
        return writer;
    }

} /* namespace smsp */
