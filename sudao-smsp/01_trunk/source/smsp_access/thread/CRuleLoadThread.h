#ifndef __C_RULE_LOAD_THREAD_H__
#define __C_RULE_LOAD_THREAD_H__

#include "Channel.h"
#include "TemplateGw.h"
#include "TemplateTypeGw.h"
#include "SignExtnoGw.h"
#include "TempRule.h"
#include "SysUser.h"
#include "SysNotice.h"
#include "SysNoticeUser.h"
#include "SmsAccount.h"
#include "SmsSendNode.h"
#include "SmsAccountState.h"
#include "AgentInfo.h"
#include "MiddleWareConfig.h"
#include "ListenPort.h"
#include "SmsTemplate.h"
#include "Thread.h"
#include "CThreadSQLHelper.h"
#include "PhoneSection.h"
#include "ChannelSegment.h"
#include "ChannelWhiteKeyword.h"
#include "agentAccount.h"
#include "ChannelGroup.h"
#include "ChannelWeight.h"
#include "CChineseConvert.h"
#include "ClientPriority.h"
#include "BlackList.h"

#include "CRouterThread.h"
#include "CTemplateThread.h"
#include "COverRateThread.h"
#include "CChargeThread.h"

using namespace models;
using namespace boost;


const string AUTO_TEMPLATE_SEPARATOR = "{}";
typedef std::map<std::string, UserGw>       USER_GW_MAP;
typedef std::map<std::string, std::string>  STL_MAP_STR;
typedef list<string>                        StrList;


#define MSGTYPE_RULELOAD_TIMEOUT    0X80020000
#define CHECK_TABLE_UPDATE_TIME_OUT     6000
#define CHECK_UPDATA_TIMER_MSGID        6070


typedef list<regex> RegList;
typedef list<string> StrList;
typedef list<Int32> stl_list_int32;




class TUpdateChannelPropertyLogReq : public TMsg
{
public:
    map<UInt64, list<channelPropertyLog_ptr_t> > m_channelPropertyLogMap;
};

class TUpdateUserPriceLogReq : public TMsg
{
public:
    map<string, list<userPriceLog_ptr_t> > m_userPriceLogMap;
};

class TUpdateUserPropertyLogReq : public TMsg
{
public:
    map<string, list<userPropertyLog_ptr_t> > m_userPropertyLogMap;
};

class TUpdateChannelReq: public TMsg
{
public:
    std::map<int,Channel> m_ChannelMap;
};

class TUpdateUserGwReq: public TMsg
{
public:
    USER_GW_MAP m_UserGwMap;
};
class TUpdateBlackListReq: public TMsg
{
public:
    set<UInt64> m_blackLists;
};
class TUpdateSignextnoGwReq: public TMsg
{
public:
    std::map<std::string, SignExtnoGw> m_SignextnoGwMap;
};

class TUpdateChannelExtendPortReq:public TMsg
{
public:
    STL_MAP_STR m_ChannelExtendPortTable;
};
class TUpdateSmppPriceReq: public TMsg
{
public:
    STL_MAP_STR m_PriceMap;
};
class TUpdateSmppSalePriceReq: public TMsg
{
public:
    STL_MAP_STR m_salePriceMap;
};

class TUpdateOperaterSegmentReq: public TMsg
{
public:
    map<string, UInt32> m_OperaterSegmentMap;
};


class TUpdateSysParamRuleReq: public TMsg
{
public:
    STL_MAP_STR m_SysParamMap;
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

class TUpdateSmsSendIDIpReq: public TMsg
{
public:
    std::list<SmsSendNode> m_SmsSendIDipList;
};

class TUpdateKeyWordReq: public TMsg
{
public:
    TUpdateKeyWordReq():m_uDefaultGroupId(0){};
    map<UInt32,list<string> > m_keyWordMap;  //categoryID + keyword
    map<UInt32,set<UInt32> >  m_keywordGrpRefCategoryMap;//keywordGrpID + categoryID
    map<UInt32,UInt32>        m_clientGrpRefKeywordGrpMap;//clientGrpID<->keywordGrpID
    UInt32                    m_uDefaultGroupId;//in t_sms_sys_client_group
    map<string,UInt32>        m_clientGrpRefClientMap;//clientGrpID<->client
};


class TUpdateClientIdAndSignReq: public TMsg
{
public:
    std::map<std::string, ClientIdSignPort> m_ClientIdAndSignMap;
};

class TUpdatePhoneAreaReq: public TMsg
{
public:
    std::map<std::string, PhoneSection> m_PhoneAreaMap;
};

class TUpdateAgentInfoReq: public TMsg
{
public:
    std::map<UInt64, AgentInfo> m_AgentInfoMap;
};

class TupdateAgentAcctInfoReq : public TMsg
{
public:
    std::map<UInt64, AgentAccount> m_AgentAcctMap;
};

class TUpdateMqConfigReq:public TMsg
{
public:
    map<UInt64,MqConfig> m_mapMqConfig;
};

class CGetChannelMqSizeReqMsg :public TMsg
{
public:
    map<UInt32, UInt32> m_mapGetChannelMqSize;
};

class TUpdateComponentRefMqReq:public TMsg
{
public:
    map<string,ComponentRefMq> m_componentRefMqInfo;
};

class TUpdateComponentConfigReq:public TMsg
{
public:
    map<string,ComponentConfig> m_componentConfigInfoMap;
};

class TUpdateSystemErrorCodeReq:public TMsg
{
public:
    map<string,string> m_mapSystemErrorCode;
};

class TUpdateTempltRuleReq: public TMsg
{
public:
    std::map<string ,TempRule> m_TempRuleMap;
};

class TUpdateKeyWordTempltRuleReq: public TMsg
{
public:
    std::map<string ,TempRule> m_KeyWordTempRuleMap;
};

class TUpdateOverRateWhiteListReq:public TMsg
{
public:
    set<string> m_overRateWhiteList;
};


class TUpdateOverRateKeyWordReq: public TMsg
{
public:
    map<string,list<string> > m_OverRateKeyWordMap;
};


class TUpdateAuidtKeyWordReq: public TMsg
{
public:
    map<string,set<string> >  m_IgnoreAuditKeyWordMap;
    map<UInt32,list<string> > m_AuditKeyWordMap;
    map<UInt32,set<UInt32> >  m_AuditKgroupRefCategoryMap;
    map<UInt32,UInt32>        m_AuditClientGroupMap;
    UInt32                    m_uDefaultGroupId;
    map<string,UInt32>        m_AuditCGroupRefClientMap;
};

class TUpdateNoAuidtKeyWordReq: public TMsg
{
public:
    map<string,list<string> > m_NoAuditKeyWordMap;
};


class TUpdateBlGrpRefCatConfigReq : public TMsg
{
public:
    map<UInt32,UInt64> m_uBgroupRefCategoryMap;
};


class TUpdateBlUserConfigGrpReq : public TMsg
{
public:
    map<string,UInt32> m_uBlacklistGrpMap;
};

class TUpdateClientPriorityReq : public TMsg
{
public:
    client_priorities_t m_clientPriorities;
};



class CRuleLoadThread:public CThread
{

public:
    CRuleLoadThread(const char *name);
    ~CRuleLoadThread();
    bool Init(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
    void getSysParamMap(STL_MAP_STR& sysParamMap);
    void getSysUserMap(std::map<int, SysUser>& sysUserMap);
    void getSysNoticeMap(std::map<int, SysNotice>& sysNoticeMap);
    void getSysNoticeUserList(std::list<SysNoticeUser>& sysNoticeUserList);
    void getSmsAccountMap(std::map<string, SmsAccount>& SmsAccountMap);
    void getNoAuditKeyWordMap(map<string,list<string> >& NoAuditKeyWordMap);
    void getIgnoreAuditKeyWordMap(map<string,set<string> >& IgnoreAuditKeyWordMap);
    void getAuditKeyWordMap(map<UInt32,list<string> >& AuditKeyWordMap); ////smsp4.5
    void getAuditKgroupRefCategoryMap(map<UInt32,set<UInt32> >& AuditKgroupRefCategoryMap);
    void getAuditClientGroupMap(map<UInt32,UInt32>& AuditClientGroupMap,UInt32& uDefaultGroupId);
    void getAuditCGroupRefClientMap(map<string,UInt32>& AuditCGroupRefClientMap);
    void getOverRateKeyWordMap(map<string,list<string> >& OverRateKeyWordMap);

    void getSmsClientAndSignMap(std::map<string, ClientIdSignPort>& smsClientIdAndSignMap);
    void getSmsClientAndSignPortMap(std::map<string, ClientIdSignPort>& smsClientIdAndSignPortMap);

    bool getBlackList(set<UInt64>& m_blackLists);
    void getSystemErrorCode(map<string,string>& systemErrorCode);

    /////send copy to access
    void getChannelGroupMap(std::map<UInt32,ChannelGroup>& ChannelGroupMap);
    void getChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
    void getChannelKeyWordMap(map<UInt32,list<string> >& ChannelKeyWordMap);
    void getChannelWhiteListMap(map<UInt32, UInt64Set>& ChannelWhiteLists);
    void getOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap);
    void getOverRateWhiteList(set<string>& overRateWhiteList);
    void getSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap);
    void getSmppPriceMap(STL_MAP_STR& priceMap);
    void getSmppSalePriceMap(STL_MAP_STR& salePriceMap);
    void getAllTempltRule(std::map<string ,TempRule> &tempRuleMap);
    void getAllKeyWordTempltRule(std::map<string ,TempRule> &KeyWordtempRuleMap);
    void getChannelOverrateRule(std::map<string ,TempRule> &channelOverrate);

    void getPhoneAreaMap(std::map<std::string, PhoneSection>& phoneAreaMap);
    void getChannlelMap(std::map<int , Channel>& channelmap);
    void getUserGwMap(USER_GW_MAP& userGwMap);
    void getSendIpIdMap(std::map<UInt32,string>& sendIpId);
    void getChannelExtendPortTableMap(STL_MAP_STR& channelExtendPortTable);
    void getAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);

    ////smsp5.0
    void getMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
    void getComponentConfig(map<string, ComponentConfig>& componentConfigInfoMap);
    void getListenPort(map<string,ListenPort>& listenPortInfo);
    void getMqConfig(map<UInt64, MqConfig>& mqConfigInfo);
    void getComponentRefMq(map<string, ComponentRefMq>& componentRefMqInfo);

    void getSmsTemplateMap(std::map<std::string, SmsTemplate>& smsTemplateMap);
    void getChannelTemplateMap(std::map<std::string, std::string>& channelTemplateMap);

    void getChannelSegmentMap(stl_map_str_ChannelSegmentList& channelSegmentMap);
    void getChannelWhiteKeywordListMap(ChannelWhiteKeywordMap& channelWhiteKeywordlistmap);

    void getClientSegmentMap(map<std::string, StrList>& clientSegmentMap,map<string,set<std::string> >& clientidSegmentMap);
    void getChannelSegCodeMap(std::map<UInt32, stl_list_int32>& channelSegCodeMap);
    void getAgentAccount ( map<UInt64, AgentAccount>& agentAcctMap );
    bool getComponentSwitch(UInt32 uComponentId);
    void getAutoWhiteTemplateMap(vartemp_to_user_map_t & mapAutoWhiteTemplate, fixedtemp_to_user_map_t & mapFixedAutoWhiteTemplate);
    void getAutoBlackTemplateMap(vartemp_to_user_map_t & mapAutoBlackTemplate, fixedtemp_to_user_map_t & mapFixedAutoBlackTemplate);
    void getLoginChannels(std::set<Int32> & setLoginChannels);
    void getChannelsRealtimeWeightInfo( channels_realtime_weightinfo_t&);
    void getChannelAttributeWeightConfig( channel_attribute_weight_config_t& );
    void getChannelPoolPolicies(channel_pool_policies_t& channelPoolPolicies);
    bool getKeywordFilterRegular(std::map<std::string, std::string>& sysParamMap);

    /******************************************system keyword classify begin***********************************************/
    void getSysKeywordMap(map<UInt32,list<string> >& keywordMap);
    void getSysKgroupRefCategoryMap(map<UInt32,set<UInt32> >& kgroupRefCategoryMap);
    void getSysClientGroupMap(map<UInt32,UInt32>& clientGroupMap,UInt32& uDefaultGroupId);
    void getSysCGroupRefClientMap(map<string,UInt32>& cGroupRefClientMap);
    /*******************************************system keyword classify end***********************************************/

    void getClientPriorities( client_priorities_t& clientPriorities);
    void getChannlelUnusualMap(std::map<int , Channel>& channelmap);
    void getChannelPropertyLog(map<UInt64, list<channelPropertyLog_ptr_t> > &channelPropertyLogMap);
    void getUserPriceLog(map<string, list<userPriceLog_ptr_t> > &userPriceLogMap);
    void getUserPropertyLog(map<string, list<userPropertyLog_ptr_t> > &userPropertyLogMap);
    
    void getBlacklistGrp(map<string,UInt32> &blacklistGrpMap);
    void getBlacklistCatAll(map<UInt32,UInt64> &blacklistCatMap);

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
    bool getDBNoAuditKeyWordMap(map<string,list<string> >& NoAuditKeyWordMap);
    bool getDBOverRateKeyWordMap(map<string, list<string> >& OverRateKeyWordMap);
    bool getDBIgnoreAuditKeyWordMap(map<string, set<string> >& IgnoreAuditKeyWordMap);
    bool getDBAuditKeyWordMap(map<UInt32,list<string> >& AuditKeyWordMap); ////smsp4.5
    bool getDBAuditKgroupRefCategoryMap(map<UInt32,set<UInt32> >& AuditKgroupRefCategoryMap);
    bool getDBAuditClientGroupMap(map<UInt32,UInt32>& AuditClientGroupMap,UInt32& uDefaultGroupId);
    bool getDBAuditCGroupRefClientMap(map<string,UInt32>& AuditCGroupRefClientMap);
    bool getDBClientAndSignMap(std::map<std::string,ClientIdSignPort>& smsClientIdAndSign,std::map<std::string,ClientIdSignPort>& smsClientIdAndSignPort);
    bool getDBChannelExtendPortTableMap(std::map<std::string,std::string>& channelExtendPortTable);
    bool getDBSystemErrorCode(map<string,string>& systemErrorCode);



    ////send copy to access
    bool getDBChannelGroupMap(map<UInt32,ChannelGroup>& ChannelGroupMap);
    bool getDBChannelRefChannelGroupMap();
    bool getDBChannelBlackListMap(map<UInt32, UInt64Set>& ChannelBlackLists);
    bool getDBChannelKeyWordMap(map<UInt32,list<string> >& ChannelKeyWordMap);
    bool getDBChannelWhiteListMap(map<UInt32, UInt64Set>& ChannelWhiteLists);
    bool getDBOperaterSegmentMap(map<string, UInt32>& PhoneHeadMap);
    bool getDBOverRateWhiteList(set<string>& overRateWhiteList);
    bool getDBSendIpIdMap(std::map<UInt32,string>& sendIpId);
    bool getDBSignExtnoGwMap(std::map<std::string,SignExtnoGw>& signextnoGwMap);
    bool getDBSmppPriceMap(std::map<std::string, std::string>& priceMap);
    bool getDBSmppSalePriceMap(std::map<std::string, std::string>& salePriceMap);
    bool getDBAllTempltRule(std::map<string ,TempRule>& tempRuleMap,std::map<string ,TempRule>& KeyWordtempRuleMap);
    bool getDBChannelOverrate(std::map<string ,TempRule>& channelOverrate);

    bool getDBChannlelMap(std::map<int , Channel>& channelmap);
    bool getDBUserGwMap(std::map<std::string,UserGw>& userGwMap);
    bool getDBPhoneAreaMap(std::map<std::string, PhoneSection>& phoneAreaMap);
    bool getDBAgentInfo(map<UInt64, AgentInfo>& agentInfoMap);
    ////smsp5.0
    bool getDBMiddleWareConfig(map<UInt32,MiddleWareConfig>& middleWareConfigInfo);
    bool getDBComponentConfig(map<string, ComponentConfig>& componentConfigInfoMap);
    bool getDBListenPort(map<string,ListenPort>& listenPortInfo);
    bool getDBMqConfig(map<UInt64,MqConfig>& mqConfigInfo);
    bool getDBComponentRefMq(map<string,ComponentRefMq>& componentRefMqInfo);

    bool getDBChannelTemplateMap(std::map<std::string, std::string>& channelTemplateMap);

    bool getDBChannelSegmentMap(stl_map_str_ChannelSegmentList& channelSegmentMultimap);
    bool getDBChannelWhiteKeywordMap(ChannelWhiteKeywordMap& channelWhiteKeywordmap);
    bool getDBClientSegmentMap(map<std::string, StrList>& clientSegmentMap,map<std::string,set<std::string> >& clientidSegmentMap);
    bool getDBChannelSegCodeMap(map<UInt32, stl_list_int32>& channelSegCodeMap);

    bool getDBChannelRealtimeWeightInfo(channels_realtime_weightinfo_t& channelRealtimeWeightInfo);
    bool getDBChannelAttributeWeightConfig(channel_attribute_weight_config_t& channelAttrWeightConfig);
    bool getDBChannelPoolPolicies(channel_pool_policies_t& channelPoolPolicies);
    bool getDBClientPriorities(client_priorities_t& clientPriorities);
    bool getDBClientPrioritiesConfig(client_priorities_t& clientPriorities);

    bool DBConnect(const std::string dbhost, unsigned int dbport, const std::string dbuser, const std::string dbpwd, const std::string dbname);
    bool DBPing();

    bool TimeRegCheck(string strTime);  //eg 21:53 will return ture;


    bool getDBAgentAccount( map<UInt64, AgentAccount>& agentAcctMap );

    bool getDBAutoTemplateMap(SMSTemplateBusType templateBusType, vartemp_to_user_map_t & mapTemplate, fixedtemp_to_user_map_t & mapFixedTemplate);

    bool getDBLoginChannels(std::set<Int32> & setLoginChannels);


    /*********keyword classification,now is system keyword begin***************/
    bool getDBKeyWordMap(map<UInt32, list<string> >& KeyWordMap, const string& tableName, int keywordConvertRegular);
    bool getDBKgroupRefCategoryMap(map<UInt32,set<UInt32> >& kgroupRefCategoryMap, const string& kgrpRefCatTable, const string& keywordCatTable);
    bool getDBClientGroupMap(map<UInt32,UInt32>& clientGroupMap,UInt32& uDefaultGroupId, const string& clientGrpTable, const string& keywordGrpTable);
    bool getDBCGroupRefClientMap(map<string,UInt32>& cGroupRefClientMap, const string& cGrRefClientTable);
    /*********keyword classification,now is system keyword end***************/
    bool getDBChannlelUnusualMap(std::map<int , Channel>& channelmap);

    bool getDBChannelPropertyLogMap(map<UInt64, list<channelPropertyLog_ptr_t> >& ChannelPropertyLogMap);
    bool getDBUserPriceLogMap(map<string, list<userPriceLog_ptr_t> >& userPriceLogMap);
    bool getDBUserPropertyLogMap(map<string, list<userPropertyLog_ptr_t> >& userPropertyLogMap);
    void setUserPropertyLogMap(userPropertyLog_ptr_t userPropertyLogMap,string strProperty, string strSet);

    bool getDBbgroupRefCategoryMap(map<UInt32,UInt64 >& kgroupRefCategoryMap);
    bool getDBBlacklistGroupMap(map<string,UInt32>& blacklistGrpMap);

    std::set<Int32>             m_setLoginChannels;
    std::string                 m_setLoginChannelsUpdateTime;

    vartemp_to_user_map_t       m_mapAutoWhiteTemplate;         //变量白模板
    fixedtemp_to_user_map_t     m_mapFixedAutoWhiteTemplate;         //固定白模板
    std::string             m_autoWhiteTemplateLastUpdateTime;

    vartemp_to_user_map_t       m_mapAutoBlackTemplate;         //变量黑模板
    fixedtemp_to_user_map_t     m_mapFixedAutoBlackTemplate;    //固定黑模板
    std::string             m_autoBlackTemplateLastUpdateTime;

    MYSQL* m_DBConn;
    CThreadWheelTimer *m_pTimer;

    CThreadWheelTimer *m_pDbTimer;
    std::string m_DBHost;
    std::string m_DBUser;
    std::string m_DBPwd;
    std::string m_DBName;
    int m_DBPort;

    regex m_regTime;        //eg 21:53

    CChineseConvert  *m_pCChineseConvert;

    STL_MAP_STR m_SysParamMap;
    std::map<int, SysUser> m_SysUserMap;
    std::map<int, SysNotice> m_SysNoticeMap;
    std::list<SysNoticeUser> m_SysNoticeUserList;
    std::map<string, SmsAccount> m_SmsAccountMap;
    std::map<string, SmsAccountState> m_SmsAccountStateMap;
    std::list<SmsSendNode> m_SmsSendIDipList;
    map<string,list<string> > m_NoAuditKeyWordMap;
    map<string,set<string> >  m_IgnoreAuditKeyWordMap;
    map<UInt32,list<string> > m_AuditKeyWordMap; ////smsp4.5
    map<UInt32,set<UInt32> >  m_AuditKgroupRefCategoryMap;
    map<UInt32,UInt32>        m_AuditClientGroupMap;
    UInt32                    m_uDefaultGroupId;
    map<string,UInt32>        m_AuditCGroupRefClientMap;

    map<string,list<string> > m_OverRateKeyWordMap;

    std::map<std::string, ClientIdSignPort> m_ClientIdAndSignMap;
    std::map<std::string, ClientIdSignPort> m_ClientIdAndSignPortMap;

    ////list<regex> m_listKeyWordReg;
    set<UInt64> m_blackLists;
    std::map<int, Channel> m_ChannelMap;
    std::map<int, Channel> m_ChannelUnusualMap;
    USER_GW_MAP m_UserGwMap;
    std::map<std::string, SignExtnoGw> m_SignextnoGwMap;
    STL_MAP_STR m_ChannelExtendPortTable;
    STL_MAP_STR m_PriceMap;
    STL_MAP_STR m_SalePriceMap;
    std::map<string ,TempRule> m_TempRuleMap;       //KEY:userID_templetID
    std::map<string ,TempRule> m_KeyWordTempRuleMap; ///key client id
    std::map<string ,TempRule> m_ChannelOverrateMap;

    std::map<std::string, PhoneSection> m_PhoneAreaMap;
    map<UInt32,ChannelGroup> m_ChannelGroupMap;
    map<UInt32, UInt64Set> m_ChannelBlackListMap;
    map<UInt32,list<string> > m_ChannelKeyWordMap;
    set<string> m_overRateWhiteList;        //userid_phone
    map<UInt32, UInt64Set> m_ChannelWhiteListMap;
    map<string, UInt32> m_OperaterSegmentMap;       //eg   134--1;
    std::map<UInt32,string> m_sendIpId;
    std::map<UInt64, AgentInfo> m_AgentInfoMap;     //agentid - agentInfo
    std::map<UInt64, AgentAccount> m_AgentAcctMap;  //agentid - agentAccount

    ////smsp5.0
    map<UInt32,MiddleWareConfig> m_middleWareConfigInfo;
    map<string, ComponentConfig> m_componentConfigInfoMap;
    ComponentConfig m_componentConfigInfo;
    map<string,ListenPort> m_listenPortInfo;
    map<UInt64,MqConfig> m_mqConfigInfo;
    map<string,ComponentRefMq> m_componentRefMqInfo;
    map<string,string> m_mapSystemErrorCode;
    STL_MAP_STR m_mapChannelTemplate;

    stl_map_str_ChannelSegmentList m_mapChannelSegment;
    ChannelWhiteKeywordMap m_mapChannelWhiteKeyword;
    map<std::string, StrList> m_mapClientSegment;
    map<std::string,set<std::string> > m_mapClientidSegment;
    map<UInt32, stl_list_int32> m_mapChannelSegCode;

    std::string  m_SysParamLastUpdateTime;
    std::string  m_SysNoticeLastUpdateTime;
    std::string  m_SysNoticeUserLastUpdateTime;
    std::string  m_SysUserLastUpdateTime;
    std::string  m_SmsAccountLastUpdateTime;
    std::string  m_SmsAccountStateLastUpdateTime;
    std::string  m_SmsSendIdIpLastUpdateTime;
    std::string  m_SmsClientidSignPortUpdateTime;
    std::string  m_NoAuditKeyWordUpdateTime;
    std::string  m_componentConfigInfoUpdateTime;

    std::string  m_IgnoreAuditKeyWordUpdateTime;
    std::string  m_AuditKeyWordUpdateTime; ////smsp4.5
    std::string  m_AuditCGroupRefClientUpdateTime;
    std::string  m_AuditClientGroupUpdateTime;
    std::string  m_AuditKgroupRefCategoryUpdateTime;
    std::string  m_OverRateKeyWordUpdateTime;
    std::string  m_ChannelLastUpdateTime;
    std::string  m_UserGwLastUpdateTime;
    std::string  m_BlackListLastUpdateTime;
    std::string  m_SysKeywordLastUpdateTime;
    std::string  m_SysCGroupRefClientUpdateTime;
    std::string  m_SysClientGroupUpdateTime;
    std::string  m_SysKgroupRefCategoryUpdateTime;
    std::string  m_SignextnoGwLastUpdateTime;
    std::string  m_ChannelExtendPortTableUpdateTime;
    std::string  m_PriceLastUpdateTime;
    std::string  m_strSalePriceLastUpdateTime;
    std::string  m_TemplateOverrateLastUpdateTime;
    std::string  m_ChannelOverrateLastUpdateTime;
    std::string  m_PhoneAreaLastUpdateTime;
    std::string  m_ChannelGroupUpdateTime;
    std::string  m_ChannelRefChlGroupUpdateTime;
    std::string  m_ChannelBlackListUpdateTime;
    std::string  m_ChannelKeyWordUpdateTime;
    std::string  m_ChannelWhiteListUpdateTime;
    std::string  m_OperaterSegmentUpdateTime;
    std::string  m_OverRateWhiteListUpdateTime;
    std::string  m_AgentInfoUpdateTime;
    std::string  m_strChannelTemplateUpdateTime;

    /*t_sms_agent_account*/
    std::string m_strAgentAccountUpdateTime;

    ////smsp5.0
    string  m_mqConfigInfoUpdateTime;
    string  m_componentRefMqUpdateTime;
    std::string  m_systemErrorCodeUpdateTime;

    std::string m_ChannelSegmentUpdateTime;
    std::string m_ChannelWhiteKeywordUpdateTime;
    std::string m_ClientSegmentUpdateTime;
    std::string m_ChannelSegCodeUpdateTime;
    std::string m_strmiddleWareUpdateTime;

    string m_strAvoidBlackConfigUpdateTime;
    string m_strChannelPropertyLogUpdateTime;
    string m_strUserPriceLogUpdateTime;
    string m_strUserPropertyLogUpdateTime;

    string m_strBLGrpRefCategoryUpdateTime;
    string m_strBLUserConfigUpdateTime;

    /****************system keyword convert regular*******************/
    int m_iSysKeywordCovRegular;
    int m_iSysKeywordCovRegularOld;
    int m_iAuditKeywordCovRegular;
    int m_iAuditKeywordCovRegularOld;
    /*****************************************************************/

    /* >>>>>>>>>>>>>>>>>>>> 通道路由权重相关 BEGIN <<<<<<<<<<<<<<<<<<< */
    // 1. 通道实时权重信息
    channels_realtime_weightinfo_t m_channelsRealtimeWeightInfo;
    std::string m_ChannelWeightInfoUpdateTime;

    // 2. 通道权重信息配置
    channel_attribute_weight_config_t m_channelAttrWeightConfig;
    std::string m_ChannelAttrWeightConfigUpdateTime;

    // 3. 通道池策略配置
    channel_pool_policies_t m_channelPoolPolicies;
    std::string m_ChannelPoolPolicyUpdateTime;
    /* >>>>>>>>>>>>>>>>>>>> 通道路由权重相关 END <<<<<<<<<<<<<<<<<<< */

    /*********keyword classification,now is system keyword begin***************/
    map<UInt32,list<string> > m_sysKeyWordMap;  //categoryID + keyword
    map<UInt32,set<UInt32> >  m_syskeywordGrpRefCategoryMap;//keywordGrpID + categoryID
    map<UInt32,UInt32>        m_sysclientGrpRefKeywordGrpMap;//clientGrpID<->keywordGrpID
    UInt32                    m_uSysDefaultGroupId;//in t_sms_sys_client_group
    map<string,UInt32>        m_sysClientGrpRefClientMap;//clientGrpID<->client
    /*********keyword classification,now is system keyword end***************/

    std::string m_clientPrioritiesUpdateTime;
    client_priorities_t m_clientPriorities;

    map<UInt64, list<channelPropertyLog_ptr_t> > m_channelPropertyLogMap;
    map<string, list<userPriceLog_ptr_t> > m_userPriceLogMap;
    map<string, list<userPropertyLog_ptr_t> > m_userPropertyLogMap;

    map<UInt32,UInt64> m_uBgroupRefCategoryMap;
    map<string,UInt32> m_uBlacklistGrpMap;
};

#endif


