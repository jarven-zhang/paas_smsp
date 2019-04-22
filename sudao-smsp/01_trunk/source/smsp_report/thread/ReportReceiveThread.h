#ifndef C_REPORTRECEIVETHREAD_H_
#define C_REPORTRECEIVETHREAD_H_

#include "Thread.h"
#include <list>
#include <map>

#include "ReportSocketHandler.h"
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
#include "ChannelScheduler.h"
#include "ChannelMng.h"
#include "SnManager.h"


using namespace std;

class THttpServerRequest: public TMsg
{
public:
    string m_strRequest;
    ReportSocketHandler* m_socketHandler;
};


class TReportReciveRequest: public TMsg
{
public:
    string m_strRequest;
    ReportSocketHandler* m_socketHandler;
    string m_strChannel;
};

class ReportReceiveSession
{
public:
    ReportReceiveSession()
    {
        m_webHandler = NULL;
        m_wheelTimer = NULL;
    }

    ReportSocketHandler* m_webHandler;
    CThreadWheelTimer* m_wheelTimer;
};



class  ReportReceiveThread:public CThread
{
    typedef std::map<UInt64, ReportReceiveSession*> SessionMap;
	typedef std::map<int, Channel> ChannelMap;
	typedef std::vector<smsp::StateReport> StateReportVector;
	typedef std::vector<smsp::UpstreamSms> UpstreamSmsVector;
public:
    ReportReceiveThread(const char *name);
    virtual ~ReportReceiveThread();
    bool Init(const std::string ip, unsigned int port);
	UInt32 GetSessionMapSize();
	
private:
	void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);
	void HandleAcceptSocketReqMsg(TMsg* pMsg);
    void HandleReportReciveReqMsg(TMsg* pMsg);
    void HandleReportReciveReturnOverMsg(TMsg* pMsg);
	UInt32 GetChannelIDfromName(string channelname,UInt32& uChannelType, Channel& channel);
	void HandleChannelErrorCodeUpdateMsg(TMsg* pMsg);
	void HttpResponse(UInt64 iSeq, string& response);	//

	InternalServerSocket* m_pServerSocekt;
    std::string data_;
    UInt32 _cons;
    SessionMap m_SessionMap;
    ChannelScheduler* m_chlSchedule;
    ChannelMng* m_pLibMng;
    SnManager* m_pSnManager;
	InternalService* m_pInternalService;
};

#endif /* C_SMSERVERTHREAD_H_ */

