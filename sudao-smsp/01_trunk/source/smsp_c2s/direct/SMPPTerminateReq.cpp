#include "SMPPTerminateReq.h"

namespace smsp
{

    SMPPTerminateReq::SMPPTerminateReq()
    {
        _commandId=UNBIND;
    }

    SMPPTerminateReq::~SMPPTerminateReq()
    {
    }

} /* namespace smsp */
