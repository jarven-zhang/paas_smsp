#ifndef CSGIPREPORTTHREAD__
#define CSGIPREPORTTHREAD__

#include <list>
#include <map>
#include "SnManager.h"
#include "Thread.h"
#include "SgipReportSocketHandler.h"
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
#include "models/Channel.h"
#include "models/channelErrorDesc.h"


using namespace std;

class  CSgipReportThread:public CThread
{
public:
	typedef std::map<int, models::Channel> ChannelMap;
    CSgipReportThread(const char *name);
    virtual ~CSgipReportThread();
    bool Init(const std::string ip, unsigned int port);
	void getErrorCode(string strDircet,UInt32 uChannelId,string strIn,string& strDesc,string& strReportDesc);
private:
	void MainLoop();
    InternalService* m_pInternalService;
    virtual void HandleMsg(TMsg* pMsg);
	void HandleSgipReportAcceptSocketMsg(TMsg* pMsg);
	void HandleSgipReportOverMsg(TMsg* pMsg);
	void HandleChannelUpdate(TMsg* pMsg);
private:
    InternalServerSocket* m_pServerSocekt;
	SnManager m_SnManager;
	map<UInt64,SgipReportSocketHandler*> m_mapSession;
public:
	ChannelMap m_mapChannelInfo;

	map<string,channelErrorDesc> m_mapChannelErrorCode;
};
#endif ////CSGIPREPORTTHREAD__

