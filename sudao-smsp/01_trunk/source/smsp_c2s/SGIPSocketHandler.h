#ifndef __SGIPSOCKETHANDLE__
#define __SGIPSOCKETHANDLE__

#include <string>
#include "ThreadMacro.h"
#include "eventtype.h"
#include "sockethandler.h"		
#include "inputbuffer.h"		
#include "outputbuffer.h"		
#include "httprequest.h"		
#include "httpresponse.h"		
#include "internalsocket.h"

#include "SGIPBase.h"
#include "SGIPConnectReq.h"
#include "SGIPConnectResp.h"
#include "SGIPSubmitReq.h"
#include "SGIPSubmitResp.h"
#include "SGIPUnBindReq.h"
#include "SGIPUnBindResp.h"


using namespace std;

////enum LinkState
////{
////    LINK_INIT  = 0,
////    LINK_CONNECTED,
////    LINK_CLOSE,
////};

class CThread;

class SGIPSocketHandler : public SocketHandler
{
	
public:
    SGIPSocketHandler(CThread* pThread, std::string strLinkId);
    virtual ~SGIPSocketHandler();
	bool Init(InternalService *service, int socket, const Address &address);

	InternalSocket *m_socket;
	UInt32 m_iLinkState;
	string m_strClientIP;
	
protected:
    virtual void OnEvent(int type, InternalSocket *socket);
    void OnData();
    bool ParseCMPPData();
	
private:
	string m_strLinkId;
	CThread* m_pThread;
public:
	string m_strAccount;
};










#endif ////SGIPSOCKETHANDLE__
