#include "SMPPDeliverResp.h"

namespace smsp
{

    SMPPDeliverResp::SMPPDeliverResp()
    {
    }

    SMPPDeliverResp::~SMPPDeliverResp()
    {
    }

    bool SMPPDeliverResp::Unpack(pack::Reader &reader)
    {
        UInt32 first = (UInt32)_commandLength-bodySize();
        std::string strBuffer = "";
        reader(first, strBuffer);

        return reader;
    }

} /* namespace smsp */

