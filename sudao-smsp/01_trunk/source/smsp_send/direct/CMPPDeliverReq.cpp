#include "CMPPDeliverReq.h"

namespace smsp
{

    CMPPDeliverReq::CMPPDeliverReq()
    {
        _commandId=CMPP_DELIVER_RESP;
        _msgId=0;
        _result=0;
    }

    CMPPDeliverReq::~CMPPDeliverReq()
    {
        // TODO Auto-generated destructor stub
    }

    UInt32 CMPPDeliverReq::bodySize() const
    {
        return CMPPBase::bodySize()+9;
    }

    bool CMPPDeliverReq::Pack(Writer& writer) const
    {
        CMPPBase::Pack(writer);
        writer(_msgId)(_result);
        return writer;
    }

} /* namespace smsp */
