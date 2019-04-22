#ifndef C_REPORTRECEIVETHREAD_H_
#define C_REPORTRECEIVETHREAD_H_

#include "Thread.h"
#include <list>
#include <map>
#include "BlackList.h"
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
#include "SnManager.h"
#include "SignExtnoGw.h"
#include "SmsAccount.h"
#include "Comm.h"




using namespace std;
using namespace models;

class TReportReciveRequest: public TMsg
{
public:
    string m_strRequest;
};

class ReportReceiveSession
{
public:
    ReportReceiveSession()
    {
        m_wheelTimer = NULL;
    }

    CThreadWheelTimer *m_wheelTimer;
};



class  ReportReceiveThread: public CThread
{
    typedef std::map<UInt64, ReportReceiveSession *> SessionMap;
    typedef std::map<UInt64, BlacklistInfo *> BlacklistMap;
public:
    ReportReceiveThread(const char *name);
    virtual ~ReportReceiveThread();
    bool Init();
    UInt32 GetSessionMapSize();

private:
    void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);
    void HandleAcceptSocketReqMsg(TMsg* pMsg);
    void HandleReportReciveReqMsg(TMsg* pMsg);
    void HandleReportReciveReturnOverMsg(TMsg* pMsg);
    void HandleMqRtAndMoMsg(TMsg* pMsg);
    void InsertMoRecord(TMsg* pMsg);
    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);
    void HandleRedisResp(TMsg* pMsg);
    void InsertPhoneToBlackList(std::string &strPhone, std::string &strClient, std::string &oldStr, int smstype, int isSet,int isSetClass, UInt64 uClass);

    void HandleBlacklistRedisResp(redisReply* pRedisReply, UInt64 uSeq);

    void processHttpsReportRePush(map<string, string>& m);

    InternalServerSocket* m_pServerSocekt;
    std::string data_;
    UInt32 _cons;
    SessionMap m_SessionMap;
    SnManager* m_pSnManager;
    InternalService* m_pInternalService;

    map<string, SmsAccount>         m_mapAccount;
    map<string, ClientIdSignPort>   m_mapClientSignPort;
    string                          m_strUnsubscribe;
    BlacklistMap                    m_mapBlacklist;
	set<string> m_setBlackList; //for连续回复的上行

};

#endif /* C_SMSERVERTHREAD_H_ */

