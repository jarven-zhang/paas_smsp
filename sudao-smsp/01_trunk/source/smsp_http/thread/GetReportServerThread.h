#ifndef C_GETREPORTSERVERTHREAD_H_
#define C_GETREPORTSERVERTHREAD_H_

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


using namespace std;
typedef set<string> DATES;

//GET REPORT ERROR CODE
const int ERR_CODE_GETREPORT_RETURN_OK						= 0;
const int ERR_CODE_GETREPORT_RETURN_AUTHENTICATIONFAIL		= -1;

const int ERR_CODE_GETREPORT_RETURN_FORBBIDEN				= -3;
const int ERR_CODE_GETREPORT_RETURN_INTERNALERROR			= -14;
const int ERR_CODE_GETREPORT_RETURN_IPNOTALLOW				= -5;
const int ERR_CODE_GETREPORT_RETURN_PARAMERROR				= -6;
const int ERR_CODE_GETREPORT_RETURN_NOTALLOWTOGETREPORT 	= -11;
const int ERR_CODE_GETREPORT_RETURN_TOO_OFFTEN				= -12;
const int ERR_CODE_GETREPORT_RETURN_NOTALLOWTOGET_MO		= -32;

//GET REPORT ERROR MESSAGE
const string ERR_MSG_GETREPORT_RETURN_OK					= "成功";
const string ERR_MSG_GETREPORT_RETURN_AUTHENTICATIONFAIL	= "鉴权失败";

const string ERR_MSG_GETREPORT_RETURN_FORBBIDEN				= "账户已注销";
const string ERR_MSG_GETREPORT_RETURN_INTERNALERROR			= "内部错误";
const string ERR_MSG_GETREPORT_RETURN_IPNOTALLOW			= "IP鉴权失败";
const string ERR_MSG_GETREPORT_RETURN_PARAMERROR			= "参数错误";
const string ERR_MSG_GETREPORT_RETURN_NOTALLOWTOGETREPORT	= "不允许拉取状态报告";
const string ERR_MSG_GETREPORT_RETURN_TOO_OFFTEN			= "访问频率过快";
const string ERR_MSG_GETREPORT_RETURN_NOTALLOWTOGET_MO 		= "不允许拉取上行";


typedef enum GET_REPORT_TYPE
{
    GET_REPORT = 0,
    GET_MO     = 1
} getReportType;

struct ReportSession
{
    AccessHttpSocketHandler 	*m_webHandler;
    CThreadWheelTimer 			*m_wheelTimer;
    string 						strDate;	//query redisList date
    string 						m_strGustIP;

    ReportSession()
    {
        m_webHandler = NULL;
        m_wheelTimer = NULL;
        strDate		 = "";
        m_strGustIP	 = "";
    }

    ReportSession &operator=(const ReportSession &other)
    {
        if (this == &other)
        {
            return *this;
        }

        this->m_webHandler = other.m_webHandler;
        this->m_wheelTimer = other.m_wheelTimer;
        this->strDate	   = other.strDate;
        this->m_strGustIP  = other.m_strGustIP;

        return *this;
    }

};


class  GetReportServerThread: public CThread
{
    typedef std::map<string, UInt64>		 StrUint64Map;
    typedef std::map<UInt64, ReportSession *> SessionMap;
    typedef std::map<string, SmsAccount> 	 AccountMap;

public:
    GetReportServerThread(const char *name);
    virtual ~GetReportServerThread();
    bool Init();

private:
    virtual void HandleMsg(TMsg *pMsg);

    /*
    	uGetType: 0 is get report, 1 is get mo
    */
    void HandleGetReportOrMoReqMsg(TMsg *pMsg, UInt32 uGetType);

    bool checkEmptyDayMap(string strClientID, string strDate);
    void HandleRedisListReportMsg(TMsg *pMsg);
    void RedisListReq(string strClientID, UInt32 uGetType, UInt64 uSeq, UInt32 maxSize);

    //get report response
    void Response(UInt64 iSeq, int uCode, string strMsg, string strData);

private:
    SnManager 		*m_pSnManager;
    AccountMap 		m_AccountMap;
    StrUint64Map 	m_mapClientReportReqTimes;	// get report clientid, times
    StrUint64Map 	m_mapClientMoReqTimes;		// get mo clientid, times

    //save no data redisKey. everyDay can only generat currDay-1 or currDay-2 record
    map<string, DATES>	m_emptyRedisLists;		// clientid  <==>  "20160918" "20160919"

};

extern GetReportServerThread           *g_pGetReportServerThread;

#endif /* C_GETREPORTSERVERTHREAD_H_ */

