#include "CMPPHeartbeatResp.h"
#include "main.h"

namespace smsp
{

    CMPPHeartbeatResp::CMPPHeartbeatResp()
    {
        _commandId = CMPP_ACTIVE_TEST_RESP;

    }

    CMPPHeartbeatResp::~CMPPHeartbeatResp()
    {
        // TODO Auto-generated destructor stub
    }

    bool CMPPHeartbeatResp::Unpack(Reader& reader)
    {
        UInt8 reserve=0;
		if(_totalLength > CMPPBase::bodySize())\
		{
	        reader(reserve);

			//add by fangjinxiong 20161117.
			int ileftByte = _totalLength -1 - CMPPBase::bodySize();
			if(ileftByte > 0)
			{
				LogDebug("ileftByte[%d]", ileftByte);
				string strtmp;
				reader((UInt32)ileftByte, strtmp);
			}
		}
		//add end
        return reader;
    }

    UInt32 CMPPHeartbeatResp::bodySize() const
    {
        return CMPPBase::bodySize()+1;
    }

    bool CMPPHeartbeatResp::Pack(Writer &writer) const
    {
        UInt8 reserve=0;
        CMPPBase::Pack(writer);
        writer(reserve);
        return writer;
    }

} /* namespace smsp */
