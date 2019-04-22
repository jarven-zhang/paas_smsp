#ifndef _CMPP3SocketHandler_H_
#define _CMPP3SocketHandler_H_

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"		
#include "httpresponse.h"		
#include "internalsocket.h"

#include "CMPP3Base.h"
#include "CMPP3ConnectReq.h"
#include "CMPP3ConnectResp.h"
#include "CMPP3HeartbeatReq.h"
#include "CMPP3HeartbeatResp.h"
#include "CMPP3SubmitReq.h"
#include "CMPP3SubmitResp.h"
#include "CMPP3DeliverReq.h"
#include "CMPP3DeliverResp.h"
#include "CMPP3TerminateResp.h"




using namespace std;
using namespace cmpp3;

class CThread;

class CMPP3SocketHandler : public SocketHandler
{
	
public:
    CMPP3SocketHandler(CThread* pThread, std::string strLinkId);
    virtual ~CMPP3SocketHandler();
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



#endif /* _CMPP3SocketHandler_H_ */

