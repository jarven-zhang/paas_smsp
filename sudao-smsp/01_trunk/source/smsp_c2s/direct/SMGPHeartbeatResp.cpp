
#include "SMGPHeartbeatResp.h"

namespace smsp
{
	SMGPHeartbeatResp::SMGPHeartbeatResp()
	{
	    requestId_=SMGP_ACTIVE_TEST_RESP;
	}

	SMGPHeartbeatResp::~SMGPHeartbeatResp()
	{
	}

	bool SMGPHeartbeatResp::Unpack(Reader& reader)
	{
	    return reader;
	}
}
