#ifndef SMGPHEARTBEATRESP_H_
#define SMGPHEARTBEATRESP_H_
#include "SMGPBase.h"

class SMGPHeartbeatResp : public SMGPBase
{
public:
	SMGPHeartbeatResp();
	virtual ~SMGPHeartbeatResp();
	virtual bool Unpack(Reader &reader);
};
#endif /* SMGPHEARTBEATRESP_H_ */
