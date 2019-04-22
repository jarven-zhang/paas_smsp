#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <set>
#include <unistd.h>
#include <time.h>
#include "../public/models/Channel.h"
#include "../public/models/UserGw.h"
#include "../public/models/SignExtnoGw.h"
#include "../public/models/SysUser.h"
#include "../public/models/SysNotice.h"
#include "../public/models/SysNoticeUser.h"

#include "../public/models/MqConfig.h"
#include "../public/models/MiddleWareConfig.h"
#include "../public/models/ListenPort.h"
#include "../public/models/ComponentRefMq.h"
#include "../public/models/ComponentConfig.h"
#include "../public/models/channelErrorDesc.h"


#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"
#include "boost/regex.hpp"
#include "../public/models/AccessToken.h"
#include "SmsAccount.h"


using namespace models;
using namespace std;
using namespace boost;

#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070
#define CHECK_UPDATA_TIMER_MSGID_INIT   6071


typedef set<UInt64> UInt64Set;



class TUpdateChannelReq: public TMsg
{
public:
    std::map<int , Channel> m_ChannelMap;
};

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

class TUpdateComponentRefMqConfigReq:public TMsg
{
public:
	map<string,ComponentRefMq> m_componentRefMqInfo;
};

/*class TUpdateUserGwReq: public TMsg
{
public:
    std::map<std::string, UserGw> m_UserGwMap;
};*/

class TUpdateAccountReq: public TMsg
{
public:
    std::map<std::string, SmsAccount> m_accountMap;
};

class TUpdateSignextnoGwReq: public TMsg
{
public:
    std::map<std::string, vector<SignExtnoGw> > m_SignextnoGwMap;
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
class TUpdateChannelBlackListReq: public TMsg
{
public:
    map<UInt32, UInt64Set> m_ChannelBlackListMap;
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

class TUpdateChannelErrorCodeReq:public TMsg
{
public:
	map<string,channelErrorDesc> m_mapChannelErrorCode;
};

class TUpdateSystemErrorCodeReq:public TMsg
{
public:
	map<string,string> m_mapSystemErrorCode;
};

class TUpdateAccessTokenReq : public TMsg
{
public:
    std::map<UInt32, AccessToken> m_mapAccessToken;
};

class CRuleLoadThread:public CThread
{
	typedef list<string> StrList;
	typedef set<UInt64> UInt64Set;
	
public:
    CRuleLoadThread(const char *name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);

    void getChannlelMap(std::map<int , Channel>& channelmap);
	void getSgipChannlelMap(std::map<int , Channel>& mapSgipChannel);

	
    void getChannlelMapDel(std::map<int , Channel>& channelmap);

	
    //void getUserGwMap(std::map<std::string,UserGw>& userGwMap);
    void getAccountMap(std::map<std::string,SmsAccount>& sAccount);	
	void getClientIdAndAppend(std::map<std::string,vector<SignExtnoGw> >& signextnoGwMap);
    void getSysParamMap(std::map<std::string, std::string>& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	void getChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode);
	void getChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
	void getChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable);

	////smsp5.0
	void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	void getComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	void getListenPort(map<string,ListenPort>& listenPortInfo);
	void getMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	void getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	void getSystemErrorCode(map<string,string>& systemErrorCode);
	
	void getChannelAccessToken(std::map<UInt32, AccessToken>& mapAccessToken);

	bool getComponentSwitch(UInt32 uComponentId);
private:
    virtual void HandleMsg(TMsg* pMsg);
    void checkDbUpdate();
    bool isTableUpdate(std::string tableName, std::string& lastUpdateTime);
    std::string getTableLastUpdateTime(std::string tableName);

    void getAllTableUpdateTime();
    bool getAllParamFromDB();

    bool getDBChannlelMap(std::map<int , Channel>& channelmap);
	bool getDBSgipChannlelMap(std::map<int , Channel>& channelmap);
	bool getDBAccountMap(std::map<std::string,SmsAccount>& accountMap);    
    bool getDBSignExtnoGwMap(std::map<std::string,vector<SignExtnoGw> >& clientIdAndAppend);
   
    bool getDBSysParamMap(std::map<std::string, std::string>& sysParamMap);

    bool getDBSysUserMap(std::map<int, SysUser>& sysUserMap);
    bool getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    bool getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
	bool getDBChannelErrorCode(map<string,channelErrorDesc>& channelErrorCode);
	bool getDBChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
	bool getDBChannelExtendPortTableMap(std::multimap<UInt32,ChannelExtendPort>& channelExtendPortTable);

	////smsp5.0
	bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	bool getDBComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	bool getDBListenPort(map<string,ListenPort>& listenPortInfo);
	bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	bool getDBSystemErrorCode(map<string,string>& systemErrorCode);
	
	bool getDBChannelAccessTokenMap(std::map<UInt32, AccessToken>& mapAccessToken);

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
    std::map<int, Channel> m_mapSgipChannel;

	/* 新增失效的channel */	
    std::map<int, Channel> m_ChannelMapDeleted;

	
	std::map<std::string,SmsAccount> m_accountMap;
    std::map<std::string, vector<SignExtnoGw> > m_SignextnoGwMap;
	std::map<std::string, vector<SignExtnoGw> > m_ClientIdAndAppendid;
    std::map<std::string, std::string> m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;
	map<UInt32, UInt64Set> m_ChannelBlackListMap;
	map<string,channelErrorDesc> m_mapChannelErrorCode;
	std::multimap<UInt32,ChannelExtendPort> m_ChannelExtendPortTable;

	////smsp5.0
	map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
	map<string,ListenPort> m_listenPortInfo;
	map<UInt64,MqConfig> m_mqConfigInfo;
	map<string,ComponentRefMq> m_componentRefMqInfo;
	map<string,string> m_mapSystemErrorCode;
	
	std::map<UInt32, AccessToken> m_mapAccessToken;


	///times begin///
    std::string  m_ChannelLastUpdateTime;
    std::string  m_UserGwLastUpdateTime;
	std::string  m_AccountUpdateTime;
    std::string  m_SignextnoGwLastUpdateTime;
    std::string  m_SysParamLastUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
	std::string  m_strChannelErrorCodeUpdateTime;
	std::string  m_ChannelExtendPortTableUpdateTime;
	std::string  m_ChannelBlackListUpdateTime;
    std::string  m_strAccessTokenUpdateTime;
	///times end///

	////smsp5.0
	std::string  m_mqConfigInfoUpdateTime;
	std::string  m_componentConfigInfoUpdateTime;
	std::string  m_systemErrorCodeUpdateTime;
	std::string  m_componentRefMqConfigUpdateTime;

	std::string  m_strmiddleWareUpdateTime;

};

#endif


