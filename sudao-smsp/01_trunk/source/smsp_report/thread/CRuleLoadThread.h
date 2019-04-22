#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <unistd.h>
#include <time.h>

#include "SmsAccount.h"
#include "SysAuthentication.h"

#include "../public/models/Channel.h"
#include "../public/models/SysUser.h"
#include "../public/models/SysNotice.h"
#include "../public/models/SysNoticeUser.h"
#include "AgentInfo.h"
#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"

#include "../public/models/MqConfig.h"
#include "../public/models/MiddleWareConfig.h"
#include "../public/models/ListenPort.h"
#include "../public/models/ComponentRefMq.h"
#include "../public/models/ComponentConfig.h"
#include "../public/models/channelErrorDesc.h"
#include "../public/models/SignExtnoGw.h"



using namespace models;


#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070

typedef set<UInt64> UInt64Set;


class TUpdateChannelReq: public TMsg
{
public:
    std::map<int , Channel> m_ChannelMap;
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

class TUpdateSignextnoGwReq: public TMsg
{
public:
    std::map<std::string, vector<SignExtnoGw> > m_SignextnoGwMap;
};

class TUpdateChannelBlackListReq: public TMsg
{
public:
    map<UInt32, UInt64Set> m_ChannelBlackListMap;
};

class TUpdateSmsAccontReq: public TMsg
{
public:
    std::map<string, SmsAccount> m_SmsAccountMap;
};

class TUpdateAgentInfoReq: public TMsg
{
public:
    std::map<UInt64, AgentInfo> m_AgentInfoMap;
};


////5.0
class TUpdateMqConfigReq:public TMsg
{
public:
	map<UInt64,MqConfig> m_mapMqConfig;
};

class TUpdateComponentConfigReq:public TMsg
{
public:
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
};

class TUpdateChannelErrorCodeReq:public TMsg
{
public:
	map<string,channelErrorDesc> m_mapChannelErrorCode;
};

class ChannelExtendPort
{
public:
	string m_strClientId;
	string m_strExtendPort;
};

class TUpdateChannelExtendPortReq:public TMsg
{
public:
	std::multimap<UInt32,ChannelExtendPort> m_ChannelExtendPortTable;
};
class TUpdateComponentRefMqConfigReq:public TMsg
{
public:
	map<string,ComponentRefMq> m_componentRefMqInfo;
};

template<typename T>
class DbUpdateReq : public TMsg
{
public:
    T m_map;
};

class CRuleLoadThread:public CThread
{
public:
    CRuleLoadThread(const char *name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);

    void getChannlelMap(std::map<int , Channel>& channelmap);
    void getSysParamMap(std::map<std::string, std::string>& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	void getChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode);
	void getClientIdAndAppend(std::map<std::string,vector<SignExtnoGw> >& signextnoGwMap);
	void getChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
	void getSmsAccountMap(std::map<string, SmsAccount>& SmsAccountMap);
	void getAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);

	////smsp5.0
	void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	void getComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	void getListenPort(map<string,ListenPort>& listenPortInfo);
	void getMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	void getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	void getChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable);
	bool getComponentSwitch(UInt32 uComponentId);
	void getSysAuthentication(SysAuthenticationMap& mapTmp);

private:
    virtual void HandleMsg(TMsg* pMsg);
	std::string& trim(std::string& s);
    void checkDbUpdate();
    bool isTableUpdate(std::string tableName, std::string& lastUpdateTime);
    std::string getTableLastUpdateTime(std::string tableName);

    void getAllTableUpdateTime();
    bool getAllParamFromDB();

    bool getDBChannlelMap(std::map<int , Channel>& channelmap);
    bool getDBSysParamMap(std::map<std::string, std::string>& sysParamMap);
    bool getDBSysUserMap(std::map<int, SysUser>& sysUserMap);
    bool getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    bool getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	bool getDBChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode);
	bool getDBSignExtnoGwMap(std::map<std::string,vector<SignExtnoGw> >& clientIdAndAppend);
	bool getDBChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
	bool getDBSmsAccontMap(std::map<string, SmsAccount>& smsAccountMap);
	bool getDBAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);

	////smsp5.0
	bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	bool getDBComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	bool getDBListenPort(map<string,ListenPort>& listenPortInfo);
	bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	bool getDBChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable);

    bool getDBSysAuthentication();

	bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
	bool DBPing();
	CThreadWheelTimer *m_pDbTimer;
	std::string m_DBHost;
	std::string m_DBUser;
	std::string m_DBPwd;
	std::string m_DBName;
	int m_DBPort;


    MYSQL* m_DBConn;
    CThreadWheelTimer *m_pTimer;

    std::map<int, Channel> m_ChannelMap;
    std::map<std::string, std::string> m_SysParamMap;
	map<UInt32, UInt64Set> m_ChannelBlackListMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;
	map<string,channelErrorDesc> m_mapChannelErrorCode;
	std::map<std::string, vector<SignExtnoGw> > m_ClientIdAndAppendid;
	std::map<string, SmsAccount> m_SmsAccountMap;
	std::map<UInt64, AgentInfo> m_AgentInfoMap;		//agentid - agentInfo

	////smsp5.0
	map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
	map<string,ListenPort> m_listenPortInfo;
	map<UInt64,MqConfig> m_mqConfigInfo;
	map<string,ComponentRefMq> m_componentRefMqInfo;
	SysAuthenticationMap m_mapSysAuthentication;

    std::string  m_ChannelLastUpdateTime;
    std::string  m_SysParamLastUpdateTime;
	std::string  m_ChannelBlackListUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
	std::string  m_strChannelErrorCodeUpdateTime;
	std::string  m_SignextnoGwLastUpdateTime;
	std::string  m_SmsAccountLastUpdateTime;
	std::string  m_AgentInfoUpdateTime;

	////smsp5.0
	std::string  m_mqConfigInfoUpdateTime;
	std::string  m_componentConfigInfoUpdateTime;
	std::multimap<UInt32,ChannelExtendPort> m_ChannelExtendPortTable;
	std::string  m_ChannelExtendPortTableUpdateTime;
	std::string  m_componentRefMqConfigUpdateTime;
	std::string  m_strmiddleWareUpdateTime;
    string m_strSysAuthenticationUpdateTime;

};

#endif


