#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <set>
#include <unistd.h>
#include <time.h>

////#include "SmsAccount.h"
////#include "../public/models/Channel.h"
#include "../public/models/SysUser.h"
#include "../public/models/SysNotice.h"
#include "../public/models/SysNoticeUser.h"
#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"

#include "../public/models/MqConfig.h"
#include "../public/models/MiddleWareConfig.h"
#include "../public/models/ComponentRefMq.h"
#include "../public/models/ComponentConfig.h"



using namespace models;

enum ConsumerRloadBlacklist
{
	CONSUMER_RELOAD_BLACKLIST = 0,
	CONSUMER_NOT_RELOAD_BLACKLIST = 1,
};
enum BlacklistSwitch
{
	COMPONENT_BLACKLIST_CLOSE = 0,
	COMPONENT_BLACKLIST_OPEN = 1,
};
#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070
#define GET_DB_BLACK_LIST_NUM           500000
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

////5.0
class TUpdateMqConfigReq:public TMsg
{
public:
	map<UInt64,MqConfig> m_mapMqConfig;
};

class TUpdateComponentRefMqReq:public TMsg
{
public:
	map<string,ComponentRefMq> m_mapComponentRefMq;
};


class CRuleLoadThread:public CThread
{
public:
    CRuleLoadThread(const char *name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname,UInt32 uRedisSwitch);
    void getSysParamMap(std::map<std::string, std::string>& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);

	////5.0
	void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	void getComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	void getMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	void getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	//5.8 add by liaojialin
	Int32 getDBChannelBlackListMap(map<string, string>& ChannelBlackListMap);
	Int32 getDBBlackList(map<string,string>& blackLists);
    Int32 getDBUnsubBlackList(map<string, string> &blackLists);
	Int32 getDBBlankPhone(set<string>& blankPhone);
	bool ReloadDbBlackList2Redis(map<string,string>& BlackListMap,const string& strSessionID);
	bool ReloadDbBlankPhone2Redis(set<string>& Blackphone,const string& strSessionID);
	void ReloadRedisBlacklistFromDb(string strSessionID);
	void FlushReidsBlacklist();
	void HandleRedisRespMsg(TMsg* pMsg);
	bool getComponentSwitch(UInt32 uComponentId);
private:
    virtual void HandleMsg(TMsg* pMsg);
	std::string& trim(std::string& s);
    void checkDbUpdate();
    bool isTableUpdate(std::string tableName, std::string& lastUpdateTime);
    std::string getTableLastUpdateTime(std::string tableName);

    void getAllTableUpdateTime();
    bool getAllParamFromDB();

    bool getDBSysParamMap(std::map<std::string, std::string>& sysParamMap);
    bool getDBSysUserMap(std::map<int, SysUser>& sysUserMap);
    bool getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    bool getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);

	////5.0
	bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	bool getDBComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);

	bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
	bool DBPing();
	CThreadWheelTimer *m_pDbTimer;
	std::string m_DBHost;
	std::string m_DBUser;
	std::string m_DBPwd;
	std::string m_DBName;
	int m_DBPort;
	UInt32 m_uBlackListSwitch;
	UInt32 m_uRedisSwitch;//0:handle redis,1:not handle redis
	UInt32 m_uReloadTotalNum;// 加载黑名单总条数
    MYSQL* m_DBConn;
    CThreadWheelTimer *m_pTimer;

    std::map<std::string, std::string> m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;

	///5.0
	map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
	map<UInt64,MqConfig> m_mqConfigInfo;
	map<string,ComponentRefMq> m_componentRefMqInfo;
	UInt32             m_uBlistTotalNum;  //channel and sys all
	UInt64             m_uBlackListIndex;
	UInt32             m_uBlackListCount;
	UInt32             m_uBlistDBIndex;
	map<string,string> m_Sysblacklist;
	map<string,string> m_ChannelBlacklistInfo;
	map<string,string> m_UnsubBlacklistInfo;
	set<string>  m_BlankPhonelist;
    std::string  m_SysParamLastUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;

	////5.0
	std::string  m_mqConfigInfoUpdateTime;
	std::string  m_componentRefMqUpdateTime;
	std::string  m_componentConfigInfoUpdateTime;
	std::string  m_strmiddleWareUpdateTime;
};

#endif


