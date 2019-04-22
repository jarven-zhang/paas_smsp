#ifndef C_SGIPSERVERTHREAD_H_
#define C_SGIPSERVERTHREAD_H_

#include <list>
#include <map>
#include <vector>

#include "Thread.h"
#include "SGIPSocketHandler.h"
#include "internalservice.h"
#include "internalsocket.h"
#include "eventtype.h"
#include "address.h"
#include "sockethandler.h"
#include "inputbuffer.h"
#include "outputbuffer.h"
#include "internalserversocket.h"
#include "ibuffermanager.h"
#include "epollselector.h"
#include "internalserversocket.h"
#include "SnManager.h"
#include "SmsAccount.h"
#include "Comm.h"
////#include "PhoneDao.h"
using namespace std;



#define EXPIRE_TIMER_ID		1
#define EXPIRE_TIMER_TIMEOUT 30*1000  ////30s

class SGIPLinkManager
{
public:
    SGIPLinkManager()
    {
        m_SGIPHandler    = NULL;
        m_ExpireTimer = NULL;
		m_uNoHBRespCount = 0;
    }
    SGIPSocketHandler* m_SGIPHandler;
	CThreadWheelTimer* m_ExpireTimer;
	string	m_strAccount;
	UInt8   m_uNoHBRespCount;
};

class  SGIPServiceThread:public CThread
{
	typedef std::map<string, SGIPLinkManager*> SGIPLinkMap;
	typedef std::map<string, SmsAccount> AccountMap;
	
public:
    SGIPServiceThread(const char *name);
    virtual ~SGIPServiceThread();
    bool Init(const std::string ip, unsigned int port); 
	UInt32 GetSessionMapSize();

private:
	void MainLoop();
    InternalService* m_pInternalService;
    virtual void HandleMsg(TMsg* pMsg);
	void HandleSGIPAcceptSocketMsg(TMsg* pMsg);
	void HandleTimerOutMsg(TMsg* pMsg);
	void HandleSGIPLinkCloseReq(TMsg* pMsg);
	
	/**
		process cunitythread send loginresp msg
	*/
	void HandleLoginResp(TMsg* pMsg);

	/**
		process submit resp msg
	*/
	void HandleSubmitResp(TMsg* pMsg);
	
private:
    InternalServerSocket* m_pServerSocekt;
    SGIPLinkMap m_LinkMap;
    SnManager* m_pSnManager;
	AccountMap m_AccountMap;
};


#endif ////C_SGIPSERVERTHREAD_H_

