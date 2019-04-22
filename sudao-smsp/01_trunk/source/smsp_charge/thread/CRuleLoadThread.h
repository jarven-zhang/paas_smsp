#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include <map>
#include <list>
#include <unistd.h>
#include <time.h>

#include "SysUser.h"
#include "SysNotice.h"
#include "SysNoticeUser.h"

#include "CustomerOrder.h"
#include "OEMClientPool.h"
#include "MiddleWareConfig.h"
#include "ComponentConfig.h"
#include "ListenPort.h"


#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "CLogThread.h"
#include "Comm.h"



using namespace models;
using namespace std;

#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070

#define CHECK_T_SMS_CLIENTORDER_UPDATA_TIMER_MSGID        6071
#define CHECK_T_SMS_CLIENTORDER_UPDATE_TIME_OUT     6000

enum ProductType
{
    PRODUCT_TYPE_UNKNOW = -1,
    PRODUCT_TYPE_HANGYE = 0,
    PRODUCT_TYPE_YINGXIAO,
    PRODUCT_TYPE_GUOJI,
    PRODUCT_TYPE_YANZHENGMA = 3,
    PRODUCT_TYPE_TONGZHI = 4,
    PRODUCT_TYPE_USSD,
    PRODUCT_TYPE_SHANXIN,
    PRODUCT_TYPE_GUAJIDUANXIN,
};

typedef list<CustomerOrder> CustomerOrderLIST;

typedef list<OEMClientPool> OEMClientPoolLIST;


class TUpdateCustomerOrderReq: public TMsg
{
public:
    std::map<string, CustomerOrderLIST> m_CustomerOrderMap;     //key sub_id
    std::set<UInt64> m_CustomerOrderSetKey;
};

class TUpdateOEMClientPoolReq: public TMsg
{
public:
    std::map<string, OEMClientPoolLIST> m_OEMClientPoolMap;
    std::set<UInt64> m_OEMClientPoolSetKey;
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

class TUpdateSysUserReq: public TMsg
{
public:
    std::map<int, SysUser> m_SysUserMap;
};

class TUpdateSysParamRuleReq: public TMsg
{
public:
    std::map<std::string, std::string> m_SysParamMap;
};




class CRuleLoadThread: public CThread
{

public:
    CRuleLoadThread(const char* name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);

    void getCustomerOrderMap(std::map<string, CustomerOrderLIST>& customerOrders, std::set<UInt64>& customerOrdersKey);
    void getSysParamMap(std::map<std::string, std::string>& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
    void getOEMClientPoolsMap(std::map<string, OEMClientPoolLIST>& OEMClientPools, std::set<UInt64>& OEMClientPoolsKey);
    void getMiddleWareConfig(map<UInt32, MiddleWareConfig>& middleWareConfigInfo);
    void getComponentConfig(ComponentConfig& componentConfigInfo);
    void getListenPort(map<string, ListenPort>& listenPortInfo);


private:
    virtual void HandleMsg(TMsg* pMsg);
    void checkDbUpdate();
    bool isTableUpdate(std::string tableName, std::string& lastUpdateTime);
    std::string getTableLastUpdateTime(std::string tableName);

    void getAllTableUpdateTime();
    bool getAllParamFromDB();

    bool getDBCustomerOrderMap(std::map<string, CustomerOrderLIST>& customerOrders, std::set<UInt64>& customerOrdersKey);
    bool getDBSysParamMap(std::map<std::string, std::string>& sysParamMap);
    bool getDBSysUserMap(std::map<int, SysUser>& sysUserMap);
    bool getDBSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    bool getDBSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
    bool getDBOEMClientPoolsMap(std::map<string, OEMClientPoolLIST>& OEMClientPools, std::set<UInt64>& OemOrdersKey);
    bool getDBMiddleWareConfig(map<UInt32, MiddleWareConfig>& middleWareConfigInfo);
    bool getDBComponentConfig(ComponentConfig& componentConfigInfo);
    bool getDBListenPort(map<string, ListenPort>& listenPortInfo);

    bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
    bool DBPing();
    CThreadWheelTimer* m_pDbTimer;
    std::string m_DBHost;
    std::string m_DBUser;
    std::string m_DBPwd;
    std::string m_DBName;
    int m_DBPort;

    UInt32 m_uOrderReadIntval;

    string& trim(std::string& s);
    MYSQL* m_DBConn;
    CThreadWheelTimer* m_pTimer;

    CThreadWheelTimer* m_pClientOrderTimer;

    std::map<string, CustomerOrderLIST> m_CustomerOrderMap;     //key client_producttype
    std::set<UInt64> m_CustomerOrderSetKey;

    std::map<std::string, std::string> m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;

    map<string, OEMClientPoolLIST> m_OEMClientPoolMap;
    set<UInt64> m_OEMClientPoolSetKey;

    map<UInt32, MiddleWareConfig> m_middleWareConfigInfo;
    ComponentConfig m_componentConfigInfo;
    map<string, ListenPort> m_listenPortInfo;


    std::string  m_CustomerOrderUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
    std::string  m_SysParamLastUpdateTime;
    std::string  m_OEMClientPoolUpdateTime;
};

extern CRuleLoadThread* g_pRuleLoadThread;

#endif
