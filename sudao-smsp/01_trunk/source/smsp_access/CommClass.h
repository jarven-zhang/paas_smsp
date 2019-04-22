#ifndef __ACCESS_COMM_CLASS_H__
#define __ACCESS_COMM_CLASS_H__

#include <map>
#include <list>
#include <set>
#include <time.h>
#include <sys/time.h>
#include "Comm.h"
#include "agentAccount.h"
#include "SmsAccount.h"
#include "AgentInfo.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "ComponentConfig.h"
#include "AgentInfo.h"
#include "boost/regex.hpp"
#include <regex.h>
#include "UserGw.h"
#include "protocol.h"
#include "ErrorCode.h"
#include "SmspUtils.h"
#include "Adapter.h"
#include "LogMacro.h"
#include "Fmt.h"
#include "PhoneSection.h"

using namespace std;
using namespace models;
using namespace boost;

using std::string;
using std::map;
using models::UserGw;


//////////////////////////////////////////////////////////////////////////

#define     SMS_TYPE_NOTICE_FLAG             0x1
#define     SMS_TYPE_VERIFICATION_CODE_FLAG  0x2
#define     SMS_TYPE_MARKETING_FLAG          0x4
#define     SMS_TYPE_ALARM_FLAG              0x8
#define     SMS_TYPE_USSD_FLAG               0x10
#define     SMS_TYPE_FLUSH_SMS_FLAG          0x20


//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
typedef list<string>                        StrList;
typedef set<UInt64>                         UInt64Set;
typedef vector<string>                      VecString;
typedef std::list<Int32>                    STL_LIST_INT32;
typedef std::list<std::string>              STL_LIST_STR;
typedef std::map<UInt32, UInt32>            STL_MAP_UINT32;
typedef std::map<std::string, std::string>  STL_MAP_STR;

//////////////////////////////////////////////////////////////////////////

//access module compile switch
#define ACCESS_MOD_SWI_CHARGE 1
#define ACCESS_MOD_SWI_BLACKLIST 1
#define ACCESS_MOD_SWI_AUTO_TEMPLATE 1
#define ACCESS_MOD_SWI_AUDIT  0
#define ACCESS_MOD_SWI_OVERRATE 0
#define ACCESS_MOD_SWI_ROUTE  1
#define ACCESS_MOD_DISPATCH  0

//access module compile switch
typedef enum{
   STATE_INIT,
   STATE_BLACKLIST,
   STATE_TEMPLATE,
   STATE_AUDIT,
   STATE_ROUTE,
   STATE_OVERRATE,
   STATE_CHARGE,
   STATE_DONE,
   STATE_MAX
}core_state_t;

#define     SMS_TYPE_NOTICE_FLAG             0x1
#define     SMS_TYPE_VERIFICATION_CODE_FLAG  0x2
#define     SMS_TYPE_MARKETING_FLAG          0x4
#define     SMS_TYPE_ALARM_FLAG              0x8
#define     SMS_TYPE_USSD_FLAG               0x10
#define     SMS_TYPE_FLUSH_SMS_FLAG          0x20


typedef enum
{
    ACCESS_ERROR_NONE,
    ACCESS_ERROR_PARAM_NULL,
    ACCESS_ERROR_COMM_POST,
    ACCESS_ERROR_ACCOUNT_NOT_EXSIT,
    ACCESS_ERROR_AGENT_ERROR,
    ACCESS_ERROR_SESSION_NOT_EXSIT,
    ACCESS_ERROR_SESSION_STAT_WRONG,
    ACCESS_ERROR_SESSION_TIMEOUT,
    ACCESS_ERROR_PHONE_BLACK,
    ACCESS_ERROR_PHONE_FORMAT,
    ACCESS_ERROR_MQ_NOT_FIND,
    ACCESS_ERROR_MQ_ROUTER_EMPTY,
    ACCESS_ERROR_CHARGE,
    ACCESS_ERROR_CHARGE_POST,
    ACCESS_ERROR_CHARGE_URL,
    ACCESS_ERROR_CHARGE_SENDER,
    ACCESS_ERROR_DISPATCH_STAT_EXSIT,

    ACCESS_ERROR_TEMPLATE_PARAM,
    ACCESS_ERROR_TEMPLATE_MATCH_BLACK,
    ACCESS_ERROR_TEMPLATE_MATCH_KEYWORD,

    ACCESS_ERROR_AUDIT_DONE,
    ACCESS_ERROR_AUDIT_FAIL,

    ACCESS_ERROR_ROUTER_FAIL,
    ACCESS_ERROR_ROUTER_PARAM,
    ACCESS_ERROR_ROUTER_NO_CHANNEL,
    ACCESS_ERROR_ROUTER_OVERFLOW,

    ACCESS_ERROR_OVERRATE_CHANNEL,
    ACCESS_ERROR_OVERRATE_CHARGE, //超频计费
    ACCESS_ERROR_OVERRATE_OTHER,
    ACCESS_ERROR_NEXT_STATE,

}sms_err_code_t;

typedef std::map< std::string, SmsAccount> accountMap;
typedef std::map< std::string, ComponentConfig > compoentMap;
typedef std::map<UInt64, AgentInfo> agentInfoMap;
typedef std::map<UInt64, AgentAccount> agentAcctMap;
typedef std::map<std::string,ComponentRefMq > componentRefMQMap;
typedef std::map< UInt64, MqConfig > mqConfigMap;
typedef const std::map<std::string, std::string > constSysParamMap;

typedef std::list<boost::regex> RegList;
typedef std::map<string, SmsAccount> AccountMap;

typedef std::map<std::string, UserGw> USER_GW_MAP;

typedef std::map<std::string, PhoneSection> phoneAreaMap;


typedef struct SystemParamRegex
{
    regex_t _regex;     //编译后的正则表达式
    string  _pattern;   //编译前的正则表达式
}_SystemParamRegex;


extern SmsAccount* GetAccountInfo( string &clientid, accountMap &mapAccount );
extern MqConfig* GetMQConfig( UInt32 mqId, mqConfigMap &conifgMQMap );
extern MqConfig* GetMQConfig( string &strKey, componentRefMQMap &refMQMap, mqConfigMap &conifgMQMap );
extern ComponentConfig* getComponentConfig( string &strKeyId, compoentMap &compMap );
extern bool GetAgentInfo( UInt64 AgentId, AgentInfo & agent );
extern UInt32 GetSendType( string& strSmsType );
extern UInt32 GetSmsTypeFlag(string& strSmsType);
extern UInt64 getCurrentTimeMS();
extern string state2String( UInt32 uState );
extern string error2Str( UInt32 uError );
extern UInt32 GetPhoneArea( cs_t strPhone, phoneAreaMap& areaPhoneMap );

extern bool CommPostMsg( UInt32 uState, TMsg *pMsg );

class CSessionThread;
extern CSessionThread *g_pSessionThread;

class CRouterThread;
extern CRouterThread * g_pRouterThread;

class CTemplateThread;
extern CTemplateThread* g_pTemplateThread;

#endif
