#ifndef _CMPPSOCKETHANDLER_H_
#define _CMPPSOCKETHANDLER_H_

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"		
#include "httpresponse.h"		
#include "internalsocket.h"

#include "CMPPBase.h"
#include "CMPPConnectReq.h"
#include "CMPPConnectResp.h"
#include "CMPPHeartbeatReq.h"
#include "CMPPHeartbeatResp.h"
#include "CmppSubmitReq.h"
#include "CMPPSubmitResp.h"
#include "CMPPDeliverReq.h"
#include "CMPPDeliverResp.h"
#include "CMPPHeartbeatResp.h"
#include "CMPPTerminateResp.h"
#include "CMPPTerminateReq.h"
#include "SMGPTerminateReq.h"
#include "SMPPTerminateReq.h"

using namespace std;

enum LinkState
{
    LINK_INIT  = 0,
    LINK_CONNECTED,
    LINK_CLOSE,
};

class CThread;

class CMPPSocketHandler : public SocketHandler
{
	
public:
    CMPPSocketHandler(CThread* pThread, std::string strLinkId);
    virtual ~CMPPSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);

	InternalSocket *m_socket;
	UInt32 m_iLinkState;
	string m_strClientIP;
	
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    void OnData();
    bool ParseCMPPData();
	
private:
	//UInt64 m_uSeq;
	string m_strLinkId;
	CThread* m_pThread;
	string m_strAccount;
};



#endif /* _CMPPSOCKETHANDLER_H_ */

