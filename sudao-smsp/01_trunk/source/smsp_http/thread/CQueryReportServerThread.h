#ifndef C_QUERY_REPORT_SERVER_THREAD_H_
#define C_QUERY_REPORT_SERVER_THREAD_H_

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

const int ERR_CODE_QUERYREPORT_RETURN_OK                    = 0;
const int ERR_CODE_QUERYREPORT_RETURN_AUTHENTICATIONFAIL    = -1;
const int ERR_CODE_QUERYREPORT_RETURN_FORBBIDEN             = -3;
const int ERR_CODE_QUERYREPORT_RETURN_IPNOTALLOW            = -5;
const int ERR_CODE_QUERYREPORT_RETURN_PARAMERROR            = -6;
const int ERR_CODE_QUERYREPORT_RETURN_TOO_OFFTEN            = -12;
const int ERR_CODE_QUERYREPORT_RETURN_INTERNALERROR         = -14;
const int ERR_CODE_QUERYREPORT_RETURN_NO_RESULT             = -31;

const string ERR_MSG_QUERYREPORT_RETURN_OK                  = "成功";
const string ERR_MSG_QUERYREPORT_RETURN_AUTHENTICATIONFAIL  = "鉴权失败";
const string ERR_MSG_QUERYREPORT_RETURN_FORBBIDEN           = "账户被禁用";
const string ERR_MSG_QUERYREPORT_RETURN_INTERNALERROR       = "内部错误";
const string ERR_MSG_QUERYREPORT_RETURN_IPNOTALLOW          = "IP鉴权失败";
const string ERR_MSG_QUERYREPORT_RETURN_PARAMERROR          = "参数错误";
const string ERR_MSG_QUERYREPORT_RETURN_TOO_OFFTEN          = "访问频率过快";
const string ERR_MSG_QUERYREPORT_RETURN_NO_RESULT           = "无查询结果";

const string ERR_MSG_QUERYREPORT_RETURN_UNKNOWN             = "未知";

class CQueryReportServerThread : public CThread
{
    typedef struct
    {
        int         code;
        std::string msg;
        std::string sid;
        std::string uid;
        std::string mobile;
        std::string report_status;
        std::string desc;
        std::string user_receive_time;
    } TQueryReportResponseInfo;

    typedef struct
    {
        UInt64 uCurrTimeStamp;  // 当前时间戳(精确到秒)
        UInt32 uQueryCount;     // 查询次数
    } TQueryFrequency;

    typedef std::map<string, SmsAccount>            AccountMap;
    typedef std::set<int>                           stl_set_int;
    typedef std::map<std::string, stl_set_int>      stl_map_str_set_int;
    typedef std::map<std::string, TQueryFrequency>  stl_map_query_frequency;

public:
    CQueryReportServerThread(const char *name);
    virtual ~CQueryReportServerThread();
    bool Init();
    UInt32 GetSessionMapSize();

private:
    virtual void HandleMsg(TMsg *pMsg);
    void HandleAcceptSocketReqMsg(TMsg *pMsg);
    void HandleReportReciveReqMsg(TMsg *pMsg);

    // http response
    void Response(UInt64 iSeq, const TQueryReportResponseInfo &response);

    // db response
    void HandleDBResponseMsg(TMsg *pMsg);

    void GetSysPara(const std::map<std::string, std::string> &mapSysPara);
private:
    SnManager               *m_pSnManager;
    AccountMap              m_AccountMap;

    UInt32                  m_uQueryReportTimeIntva;// 每秒查询次数

    stl_map_query_frequency m_ClientReqTimes;		// clientid, times

    stl_set_int             m_setStateUnknown;      // 保存未知状态值
    stl_set_int             m_setStateSuccess;      // 保存成功状态值
    stl_set_int             m_setStateFail;         // 保存失败状态值
    stl_map_str_set_int     m_mapReportStatus;      // 保存各种状态对应的repost_status字段描述
};

extern CQueryReportServerThread        *g_pQueryReportServerThread;

#endif /* C_QUERY_REPORT_SERVER_THREAD_H_ */

