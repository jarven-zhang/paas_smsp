#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <unistd.h>
#include <time.h>

#include "../public/models/Channel.h"
#include "../public/models/UserGw.h"
#include "../public/models/TemplateGw.h"
#include "../public/models/TemplateTypeGw.h"
#include "../public/models/SignExtnoGw.h"
#include "../public/models/TempRule.h"
////#include "../public/models/ChannelWarnInfo.h"
#include "../public/models/SysUser.h"
#include "../public/models/SysNotice.h"
#include "../public/models/SysNoticeUser.h"
#include "../public/models/SmsAccount.h"
#include "../public/models/SmsSendNode.h"
#include "../public/models/SmsAccountState.h"
#include "AgentInfo.h"
#include "MiddleWareConfig.h"
#include "ComponentConfig.h"
#include "ListenPort.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "SmsTemplate.h"

#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"
#include "boost/regex.hpp"


using namespace models;
using namespace boost;

#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070


typedef list<regex> RegList;
typedef list<string> StrList;


class TUpdateUserGwReq: public TMsg
{
public:
    std::map<std::string, UserGw> m_UserGwMap;
};

class TUpdateBlackListReq: public TMsg
{
public:
	set<UInt64> m_blackLists;
	
};

class TUpdateKeyWordListReq: public TMsg
{
public:
	TUpdateKeyWordListReq()
	{
	}
	list<regex> m_listKeyWordReg;
};

class TUpdateSignextnoGwReq: public TMsg
{
public:
    std::map<std::string, SignExtnoGw> m_SignextnoGwMap;
};

class TUpdateSmppPriceReq: public TMsg
{
public:
    std::map<std::string, std::string> m_PriceMap;
};

class TUpdateSmppSalePriceReq: public TMsg
{
public:
    std::map<std::string, std::string> m_salePriceMap;
};


class TUpdateTempltRuleReq: public TMsg
{
public:
    std::map<string ,TempRule> m_TempRuleMap;
};

class TUpdateOperaterSegmentReq: public TMsg
{
public:
    map<string, UInt32> m_OperaterSegmentMap;
};

class TUpdateOverRateWhiteListReq:public TMsg
{
public:
	set<string> m_overRateWhiteList;
};

class TUpdateSysParamRuleReq: public TMsg
{
public:
    std::map<std::string, std::string> m_SysParamMap;
};


class TUpdateSysUserReq: public TMsg
{
public:
    std::map<int, SysUser> m_SysUserMap;
};

class TUpdateSysNoticeReq: public TMsg
{
public:
    std::map<int, SysNotice> m_SysNoticeMap;
};

class TUpdateSysNoticeUserReq: public TMsg
{
public:
    std::list<SysNoticeUser> m_SysNoticeUserList;
};

class TUpdateSmsAccontReq: public TMsg
{
public:
    std::map<string, SmsAccount> m_SmsAccountMap;
};

class TUpdateSmsAccontStatusReq: public TMsg
{
public:
    std::map<string, SmsAccountState> m_SmsAccountStateMap;
};

class TUpdateClientIdAndSignReq: public TMsg
{
public:
    std::map<std::string, ClientIdSignPort> m_ClientIdAndSignMap;
};

class TUpdatePhoneAreaReq: public TMsg
{
public:
    std::map<std::string, int> m_PhoneAreaMap;
};

class TUpdateAgentInfoReq: public TMsg
{
public:
    std::map<UInt64, AgentInfo> m_AgentInfoMap;
};

class TUpdateMqConfigReq:public TMsg
{
public:
	map<UInt64,MqConfig> m_mapMqConfig;
};

class TUpdateComponentRefMqReq:public TMsg
{
public:
	map<string,ComponentRefMq> m_componentRefMqInfo;
};

class TUpdateComponentConfigReq:public TMsg
{
public:
	ComponentConfig m_componentConfig;
};

class TUpdateSmsTemplateReq : public TMsg
{
public:
    std::map<std::string, SmsTemplate> m_mapSmsTemplate;
};

class TUpdateSystemErrorCodeReq:public TMsg
{
public:
	map<string,string> m_mapSystemErrorCode;
};

class TUpdateClientForceExitReq: public TMsg
{
public:
    std::map<string, int> m_clientForceExitMap;
};

class CRuleLoadThread:public CThread
{

public:
    CRuleLoadThread(const char *name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
    void getSysParamMap(std::map<std::string, std::string>& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	void getSmsAccountMap(std::map<string, SmsAccount>& SmsAccountMap);
	void getNoAuditKeyWordMap(map<string,RegList>& NoAuditKeyWordMap);

	void getSmsClientAndSignMap(std::map<string, ClientIdSignPort>& smsClientIdAndSignMap);
	void getSmsClientAndSignPortMap(std::map<string, ClientIdSignPort>& smsClientIdAndSignPortMap);

	bool getBlackList(set<UInt64>& m_blackLists);
    bool getKeyWordList(list<regex> &listKeyWordReg);

	/////send copy to access
	
	void getOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap);
	void getOverRateWhiteList(set<string>& overRateWhiteList);
	void getSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap); 
   	void getSmppPriceMap(std::map<std::string, std::string>& priceMap);
	void getSmppSalePriceMap(std::map<std::string, std::string>& salePriceMap);
   	void getAllTempltRule(std::map<string ,TempRule> &tempRuleMap);
   	void getPhoneAreaMap(std::map<std::string, int>& phoneAreaMap);
    void getUserGwMap(std::map<std::string,UserGw>& userGwMap);
	void getSendIpIdMap(std::map<UInt32,string>& sendIpId);
	void getAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);

	////smsp5.0
	void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	void getComponentConfig(ComponentConfig& componentConfigInfo);
	void getListenPort(map<string,ListenPort>& listenPortInfo);
	void getMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	void getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	
	void getSmsTemplateMap(std::map<std::string, SmsTemplate>& smsTemplateMap);

	void getSystemErrorCode(map<string,string>& systemErrorCode);
	
    void getClientForceExit(map<string, int>& clientForceExitMap);
    bool setDBClientForceExitStop();
private:
    virtual void HandleMsg(TMsg* pMsg);
    void checkDbUpdate();
    bool isTableUpdate(std::string tableName, std::string& lastUpdateTime);
    std::string getTableLastUpdateTime(std::string tableName);
    void getAllTableUpdateTime();
	
    bool getAllParamFromDB();
    bool getDBSysParamMap(std::map<std::string, std::string>& sysParamMap);
    bool getDBSysUserMap(std::map<int, SysUser>& sysUserMap);
    bool getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    bool getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	bool getDBSmsAccontMap(std::map<string, SmsAccount>& smsAccountMap);
	bool getDBSmsAccontStatusMap(std::map<string, SmsAccountState>& smsAccountStateMap);
	bool getDBSmsSendIDIpList(std::list<SmsSendNode>& smsSendIDipList);
	bool getDBClientAndSignMap(std::map<std::string,ClientIdSignPort>& smsClientIdAndSign,std::map<std::string,ClientIdSignPort>& smsClientIdAndSignPort);
    bool getDBKeyWordList(std::list<std::string>& list);

	///bool getDBChannelWhiteListMap(map<UInt32, UInt64Set>& ChannelWhiteLists);
	bool getDBOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap);
	bool getDBOverRateWhiteList(set<string>& overRateWhiteList);
	bool getDBSendIpIdMap(std::map<UInt32,string>& sendIpId);
	bool getDBSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap);
    bool getDBSmppPriceMap(std::map<std::string, std::string>& priceMap);
	bool getDBSmppSalePriceMap(std::map<std::string, std::string>& salePriceMap);
    bool getDBAllTempltRule(std::map<string ,TempRule>& tempRuleMap);
    bool getDBUserGwMap(std::map<std::string,UserGw>& userGwMap);
	bool getDBPhoneAreaMap(std::map<std::string, int>& phoneAreaMap);
	bool getDBAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);
	////smsp5.0
	bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	bool getDBComponentConfig(ComponentConfig& componentConfigInfo);
	bool getDBListenPort(map<string,ListenPort>& listenPortInfo);
	bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	
	bool getDBSmsTemplateMap(std::map<std::string, SmsTemplate>& smsTemplateMap);

	bool getDBSystemErrorCode(map<string,string>& systemErrorCode);

	bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
	bool DBPing();

	bool TimeRegCheck(string strTime);	//eg 21:53 will return ture; 
    bool getDBClientForceExit(map<string, int>& clientForceExitMap);
	
    MYSQL* m_DBConn;
    CThreadWheelTimer *m_pTimer;

	CThreadWheelTimer *m_pDbTimer;
	std::string m_DBHost;
	std::string m_DBUser;
	std::string m_DBPwd;
	std::string m_DBName;
	int m_DBPort;

	regex m_regTime;		//eg 21:53

    std::map<std::string, std::string> m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;
	std::map<string, SmsAccount> m_SmsAccountMap;
	std::map<string, SmsAccountState> m_SmsAccountStateMap;
	std::list<SmsSendNode> m_SmsSendIDipList;
	map<string,RegList> m_NoAuditKeyWordMap;
	std::map<std::string, ClientIdSignPort> m_ClientIdAndSignMap;
	std::map<std::string, ClientIdSignPort> m_ClientIdAndSignPortMap;

	list<regex> m_listKeyWordReg;
	set<UInt64> m_blackLists; 
	std::list<std::string> m_List;
	std::map<int, Channel> m_ChannelMap;
    std::map<std::string,UserGw> m_UserGwMap;
	std::map<std::string, SignExtnoGw> m_SignextnoGwMap;
	std::map<std::string,std::string> m_ChannelExtendPortTable;
	std::map<std::string, std::string> m_PriceMap;
	std::map<std::string,std::string> m_SalePriceMap;
	std::map<string ,TempRule> m_TempRuleMap;		//KEY:userID_templetID  
	std::map<std::string, int> m_PhoneAreaMap;
	//map<UInt32,ChannelGroup> m_ChannelGroupMap;
	//map<UInt32, UInt64Set> m_ChannelBlackListMap;
	map<UInt32,RegList> m_ChannelKeyWordMap;
	set<string> m_overRateWhiteList;		//userid_phone
	//map<UInt32, UInt64Set> m_ChannelWhiteListMap;
	map<string, UInt32> m_OperaterSegmentMap;		//eg   134--1;
	std::map<UInt32,string> m_sendIpId;
	std::map<UInt64, AgentInfo> m_AgentInfoMap;		//agentid - agentInfo

	////smsp5.0
	map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
	ComponentConfig m_componentConfigInfo;;
	map<string,ListenPort> m_listenPortInfo;
	map<UInt64,MqConfig> m_mqConfigInfo;
	map<string,ComponentRefMq> m_componentRefMqInfo;
	
	std::map<std::string, SmsTemplate> m_mapSmsTemplate;
	map<string,string> m_mapSystemErrorCode;
    map<string, int> m_clientForceExitMap;

    std::string  m_SysParamLastUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
	std::string  m_SmsAccountLastUpdateTime;
	std::string  m_SmsAccountStateLastUpdateTime;
	std::string  m_SmsSendIdIpLastUpdateTime;
	std::string  m_SmsClientidSignPortUpdateTime;
	std::string  m_NoAuditKeyWordUpdateTime;

	std::string  m_ChannelLastUpdateTime;
    std::string  m_UserGwLastUpdateTime;
    std::string  m_BlackListLastUpdateTime;
    std::string  m_KeywordLastUpdateTime;
    std::string  m_SignextnoGwLastUpdateTime;
	std::string  m_ChannelExtendPortTableUpdateTime;
    std::string  m_PriceLastUpdateTime;
	std::string  m_strSalePriceLastUpdateTime;
    std::string  m_TemplateOverrateLastUpdateTime;
	std::string  m_PhoneAreaLastUpdateTime;
	std::string  m_ChannelGroupUpdateTime;
	std::string  m_ChannelRefChlGroupUpdateTime;
	std::string  m_ChannelBlackListUpdateTime;
	std::string  m_ChannelKeyWordUpdateTime;
	std::string  m_ChannelWhiteListUpdateTime;
	std::string  m_OperaterSegmentUpdateTime;
	std::string  m_OverRateWhiteListUpdateTime;
	std::string  m_AgentInfoUpdateTime;
	
	////smsp5.0
	string  m_mqConfigInfoUpdateTime;
	string  m_componentRefMqUpdateTime;
	string  m_componentConfigUpdateTime;
	
	std::string  m_strSmsTemplateUpdateTime;

	std::string  m_systemErrorCodeUpdateTime;
	std::string  m_strmiddleWareUpdateTime;

	std::string  m_clientForceExitUpdateTime;
};

#endif


