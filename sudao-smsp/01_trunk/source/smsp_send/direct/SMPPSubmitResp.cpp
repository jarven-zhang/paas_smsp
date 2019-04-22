#include "SMPPSubmitResp.h"
#include "main.h"
namespace smsp
{

    SMPPSubmitResp::SMPPSubmitResp()
    {
        _msgID="";

    }

    SMPPSubmitResp::~SMPPSubmitResp()
    {
        // TODO Auto-generated destructor stub
    }

    bool SMPPSubmitResp::Unpack(pack::Reader& reader)
    {
        char msgid[128] = {0};
        reader((UInt32)_commandLength-bodySize(),msgid);
        LogDebug("_commandLength[%d],bodySize[%d],msgid[%s]",_commandLength,bodySize(),msgid);
        _msgID = msgid;
        return reader;
    }

} /* namespace smsp */
