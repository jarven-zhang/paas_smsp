#ifndef _SMGPSOCKETHANDLER_H_
#define _SMGPSOCKETHANDLER_H_

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"		
#include "httpresponse.h"		
#include "internalsocket.h"

#include "SMGPBase.h"
#include "SMGPConnectReq.h"
#include "SMGPConnectResp.h"
#include "SMGPHeartbeatReq.h"
#include "SMGPHeartbeatResp.h"
#include "SMGPSubmitReq.h"
#include "SMGPSubmitResp.h"
#include "SMGPDeliverReq.h"
#include "SMGPDeliverResp.h"
#include "SMGPTerminateResp.h"

using namespace std;

/*enum LinkState
{
    LINK_INIT  = 0,
    LINK_CONNECTED,
    LINK_CLOSE,
};*/

class CThread;

class SMGPSocketHandler : public SocketHandler
{
	
public:
    SMGPSocketHandler(CThread* pThread, std::string strLinkId);
    virtual ~SMGPSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);

	InternalSocket *m_socket;
	UInt32 m_iLinkState;
	string m_strClientIP;
	
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    void OnData();
    bool ParseSMGPData();
	
private:
	//UInt64 m_uSeq;
	string m_strLinkId;
	CThread* m_pThread;
	string m_strAccount;
};



#endif /* _CMPPSOCKETHANDLER_H_ */

