#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <unistd.h>
#include <time.h>
#include "../public/models/SysUser.h"
#include "../public/models/SysNotice.h"
#include "../public/models/SysNoticeUser.h"
#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"
#include "Comm.h"
#include "MiddleWareConfig.h"
#include "ComponentConfig.h"
#include "ListenPort.h"
#include "MqConfig.h"
#include "ComponentRefMq.h"
#include "CAuditServiceThread.h"


using namespace models;


#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070



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
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
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
	void getNeedAuditMap(std::map<std::string, NeedAudit>& NeedAuditMap);
	void getAuditIdArray(std::vector<UInt64>& AuditIdArray);
	void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	void getComponentConfig(map<UInt64, ComponentConfig>& componentConfigInfo);
	void getMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	void getComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	bool getComponentSwitch(UInt32 uComponentId);


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
	bool getDBNeedAuditMap(std::map<string, NeedAudit>& mapNeedAudit);
	bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
	bool getDBComponentConfig(map<UInt64,ComponentConfig>& componentConfigInfo);
	bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);
	bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
	string& trim(std::string& s);
	bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
	bool DBPing();

    MYSQL* m_DBConn;
    CThreadWheelTimer *m_pTimer;
	CThreadWheelTimer *m_pDbTimer;
	std::string m_DBHost;
	std::string m_DBUser;
	std::string m_DBPwd;
	std::string m_DBName;
	int m_DBPort;

    std::map<std::string, std::string> m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;
	std::map<string, NeedAudit> m_NeedAuditMap;
	std::vector<UInt64> m_AuditIdArray;
	map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
	map<UInt64,ComponentConfig> m_mapComponentConfigInfo;
	map<UInt64,MqConfig> m_mqConfigInfo;
	map<string,ComponentRefMq> m_componentRefMqInfo;

    std::string  m_SysParamLastUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
	std::string  m_strmiddleWareUpdateTime;
	////smsp5.0
	string  m_mqConfigInfoUpdateTime;
	string  m_componentRefMqUpdateTime;
	string  m_componentConfigUpdateTime;

};

#endif


