#ifndef C_GETBALANCESERVERTHREAD_H_
#define C_GETBALANCESERVERTHREAD_H_

#include "Thread.h"
#include <list>
#include <map>
#include <set>
#include "AccessHttpSocketHandler.h"
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
#include "ChannelMng.h"
#include "SnManager.h"
#include "SmsAccount.h"
#include "AgentInfo.h"
#include "SysAuthentication.h"


using namespace std;
using namespace models;

typedef set<string> DATES;

// GET BALANCE ERROR CODE
const int ERR_CODE_GETBALANCE_RETURN_OK					= 0;
const int ERR_CODE_GETBALANCE_RETURN_AUTHENTICATIONFAIL	= -1;
const int ERR_CODE_GETBALANCE_RETURN_FORBBIDEN			= -3;
const int ERR_CODE_GETBALANCE_RETURN_INTERNALERROR		= -14;
const int ERR_CODE_GETBALANCE_RETURN_IPNOTALLOW			= -5;
const int ERR_CODE_GETBALANCE_RETURN_PARAMERROR			= -6;
const int ERR_CODE_GETBALANCE_RETURN_TOO_OFFTEN			= -12;
const int ERR_CODE_GETBALANCE_RETURN_NOFEEINFO			= -13;


// GET BALANCE ERROR MESSAGE
const string ERR_MSG_GETBALANCE_RETURN_OK					= "成功";
const string ERR_MSG_GETBALANCE_RETURN_AUTHENTICATIONFAIL	= "鉴权失败";

const string ERR_MSG_GETBALANCE_RETURN_FORBBIDEN			= "账户被禁用";
const string ERR_MSG_GETBALANCE_RETURN_INTERNALERROR		= "内部错误";
const string ERR_MSG_GETBALANCE_RETURN_IPNOTALLOW			= "IP鉴权失败";
const string ERR_MSG_GETBALANCE_RETURN_PARAMERROR			= "参数错误";
const string ERR_MSG_GETBALANCE_RETURN_TOO_OFFTEN			= "访问频率过快";
const string ERR_MSG_GETBALANCE_RETURN_NOFEEINFO			= "您的费用信息不存在";      	// code:-9

struct ReqSession
{

    AccessHttpSocketHandler* m_webHandler;
    CThreadWheelTimer* 		 m_wheelTimer;
    string 					 m_strGustIP;
	
    ReqSession()
    {
        m_webHandler = NULL;
        m_wheelTimer = NULL;
		m_strGustIP  = "";
    }

	ReqSession& operator=(const ReqSession& other)
    {
        if (this == &other)
        {
            return *this;
        }

		this->m_webHandler = other.m_webHandler;
		this->m_wheelTimer = other.m_wheelTimer;
		this->m_strGustIP = other.m_strGustIP;
		
        return *this;
    }
};

class  GetBalanceServerThread:public CThread
{
	typedef std::map<string, UInt64>		StrUint64Map;
    typedef std::map<UInt64, ReqSession*> 	SessionMap;
    typedef std::map<string, SmsAccount> 	AccountMap;

public:
    GetBalanceServerThread(const char *name);
    virtual ~GetBalanceServerThread();
    bool Init();

private:
    virtual void HandleMsg(TMsg* pMsg);
    void HandleAcceptSocketReqMsg(TMsg* pMsg);
    void HandleReportReciveReqMsg(TMsg* pMsg);

    // http response
    void Response(UInt64 iSeq, int iCode, string strMsg, UInt32 uHANGYE = 0, UInt32 uYINGXIAO = 0, float fGUOJI = 0,UInt32 uIdCode = 0,UInt32 uNotice = 0,
        int agentType = 0, UInt32 uUssd_Remain = 0, UInt32 uShanXin_Remain = 0, UInt32 uGuaJiDuanXin_Remain = 0);

    // db response
    void HandleDBResponseMsg(TMsg* pMsg);

    int getAgentTypeByID(UInt64 uAgentID);

    void GetSysPara(const std::map<std::string, std::string>& mapSysPara);

    void handleAuthentication(TMsg* pMsg);

    bool processAuthentication(TMsg* pMsg, string& strExpireTimestamp, string& strPubKey);

private:
    SnManager* 				m_pSnManager;
    AccountMap 				m_AccountMap;
    AgentInfoMap			m_AgentInfoMap;

    // for get balance
    UInt32 					m_uGetBlcTimeIntva;     // system param

    StrUint64Map 			m_mapClientReqTimes;		// clientid, times

    string 					m_strSmspRsaPriKey;

    SysAuthenticationMap 	m_mapSysAuthentication;
};

extern GetBalanceServerThread          *g_pGetBalanceServerThread;

#endif /* C_GETBALANCESERVERTHREAD_H_ */

