#ifndef C_HTTPSERVERTHREAD_H_
#define C_HTTPSERVERTHREAD_H_

#include "platform.h"
#include "Thread.h"
#include "SnManager.h"
#include "HttpSocketHandler.h"
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

#include <map>

using std::string;
using std::map;


class THttpRequest : public TMsg
{
public:
    string m_strRequest;
    HttpSocketHandler* m_socketHandle;
};

class THttpQueryResp : public TMsg
{
public:
    THttpQueryResp()
    {
        m_uiReturn = 0;
        m_dSlaePrice = 0;
        m_dProductCost = 0;
        m_uiProductType = INVALID_UINT32;
    }

    UInt32 m_uiReturn;   //0 OK, 1:not enough 2:can find product 3:parma error
    string m_strSubID;
    double m_dSlaePrice;
    double m_dProductCost;
    string m_strSmsID;
    string m_strPhone;
    UInt32 m_uiProductType;
};

class HttpServerThread: public CThread
{
class Session
{
public:
    Session(HttpServerThread* pThread, UInt64 ulSequence=0);
    ~Session();

    bool parseReq(cs_t strData);

public:
    HttpServerThread* m_pThread;
    UInt64 m_ulSequence;

    HttpSocketHandler* m_webHandler;
    CThreadWheelTimer* m_wheelTimer;

    string m_strClientid;
    UInt32 m_uiAgentType;
    string m_strAgendId;
    UInt32 m_uiPayType;
    string m_strProductType;
    UInt32 m_uiOperaterCode;
    UInt32 m_uiArea;
    UInt32 m_uiCount;
    double m_dFee;
    string m_strSmsID;
    string m_strPhone;
    UInt32 m_uiClientType;
};

typedef map<UInt64, Session*> SessionMap;
typedef map<UInt64, Session*>::iterator SessionMapIter;

public:
    HttpServerThread(const char* name);
    ~HttpServerThread();

    static bool start();

    bool Init();
    UInt32 GetSessionMapSize() {return m_mapSession.size();}

private:
    virtual void MainLoop();
    virtual void initWorkFuncMap();

    void handleAcceptSocketReqMsg(TMsg* pMsg);
    void handleHttpServiceReqMsg(TMsg* pMsg);
    void handleHttpRspMsg(TMsg* pMsg);
    void handleReturnOverMsg(TMsg* pMsg);
    void HandleTimerOutMsg(TMsg* pMsg);

private:
    InternalService* m_pInternalService;
    InternalServerSocket* m_pServerSocekt;

    SessionMap m_mapSession;

    SnManager m_snManager;
};

extern HttpServerThread* g_pHttpServiceThread;

#endif
