#ifndef _SMPPSOCKETHANDLER_H_
#define _SMPPSOCKETHANDLER_H_

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"		
#include "httpresponse.h"		
#include "internalsocket.h"

#include "SMPPBase.h"
#include "SMPPConnectReq.h"
#include "SMPPConnectResp.h"
#include "SMPPHeartbeatReq.h"
#include "SMPPHeartbeatResp.h"
#include "SMPPSubmitReq.h"
#include "SMPPSubmitResp.h"
#include "SMPPDeliverReq.h"
#include "SMPPDeliverResp.h"
#include "SMPPHeartbeatReq.h"
#include "SMPPHeartbeatResp.h"
#include "SMPPGeneric.h"

using namespace std;

////enum LinkState
////{
////    LINK_INIT  = 0,
////    LINK_CONNECTED,
////    LINK_CLOSE,
////};

class CThread;

class SMPPSocketHandler : public SocketHandler
{
	
public:
    SMPPSocketHandler(CThread* pThread, std::string strLinkId);
    virtual ~SMPPSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);

	InternalSocket *m_socket;
	UInt32 m_iLinkState;
	string m_strClientIP;
	
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    void OnData();
    bool ParseSMPPData();
	
private:
	//UInt64 m_uSeq;
	string m_strLinkId;
	CThread* m_pThread;
	string m_strAccount;
};

#endif ////_SMPPSOCKETHANDLER_H_

