#ifndef __C_ROUTER_THREAD_H__
#define __C_ROUTER_THREAD_H__

#include "Thread.h"
#include "ChannelMng.h"
#include "HttpSender.h"
#include "PhoneDao.h"
#include "SignExtnoGw.h"
#include "ChannelSelect.h"
#include "ChannelScheduler.h"
#include "SmspUtils.h"
#include "ChannelGroup.h"
#include "AgentInfo.h"
#include "SmsAccount.h"
#include "ComponentConfig.h"
#include "ComponentRefMq.h"
#include "ChannelSegment.h"
#include "ChannelWhiteKeyword.h"
#include "PhoneSection.h"
#include "CBaseRouter.h"
#include "searchTree.h"
#include "agentAccount.h"
#include "ChannelGroupWeight.h"
#include "KeyWordCheck.h"
#include "keywordClassify.h"
#include "sysKeywordClassify.h"
#include "SnManager.h"
#include "CoverRate.h"
#include "CommClass.h"


bool startRouterThread();


class SMSSmsInfo;

#define     SMS_TYPE_NOTICE_FLAG             0x1
#define     SMS_TYPE_VERIFICATION_CODE_FLAG  0x2
#define     SMS_TYPE_MARKETING_FLAG          0x4
#define     SMS_TYPE_ALARM_FLAG              0x8
#define     SMS_TYPE_USSD_FLAG               0x10
#define     SMS_TYPE_FLUSH_SMS_FLAG          0x20

class ChannelPropertyLog
{
public:
    ChannelPropertyLog():m_dChannelCostPrice(0.0){};
    string m_strProperty;
    double m_dChannelCostPrice;
    string m_strEffectDate;
};
typedef boost::shared_ptr<ChannelPropertyLog> channelPropertyLog_ptr_t;


class UserPriceLog
{
public:
    UserPriceLog():m_dUserPrice(0.0){};
    string m_strClientID;
    string m_strSmsType;
    string m_strEffectDate;
    double m_dUserPrice;
};
typedef boost::shared_ptr<UserPriceLog> userPriceLog_ptr_t;

class UserPropertyLog
{
public:
    UserPropertyLog()
    {
        m_dydPrice = 0.0;
        m_dltPrice = 0.0;
        m_ddxPrice = 0.0;
        m_dfjDiscount = 10;
    }
    string m_strClientID;
    string m_strEffectDate;
    double m_dydPrice;
    double m_dltPrice;
    double m_ddxPrice;
    double m_dfjDiscount;
};
typedef boost::shared_ptr<UserPropertyLog> userPropertyLog_ptr_t;

typedef struct CHANNEL_FLOW_T
{
    UInt64 m_uTime;
    UInt32 m_uNum;

    CHANNEL_FLOW_T()
    {
        memset(this, 0, sizeof(CHANNEL_FLOW_T));
    }
}channelFlow;

// 路由请求
class TSmsRouterReq : public TMsg
{
public:
    TSmsRouterReq()
    {
        /*need init val */
        m_uFromSmsReback = 0;
        m_bInOverRateWhiteList = false;
        m_iFailedResendTimes = 0;
        m_fCostFee = 0.0;
        m_dSaleFee = 0.0;
        m_iOverCount = 0;
        m_uOverTime = 0;
        m_iClientSendLimitCtrlTypeFlag = 0;
    }
    ~TSmsRouterReq() {}

public:
    string          m_strSmsId;                     // 消息ID
    string          m_strSid;
    string          m_strClientId;                  // 客户ID
    string          m_strPhone;                     // 手机号
    UInt32          m_uOperater;                    // 通道运营商
    UInt32          m_uSmsFrom;
    string          m_strSmsType;                   // 短信类型
    string          m_strContent;                   // 内容
    string          m_strSign;                      // 签名
    string          m_strTemplateId;                // 模板ID
    string          m_strUcpaasPort;
    string          m_strExtpendPort;               // user extend port
    string          m_strSignPort;                  // 签名端口
    UInt32          m_uSendType;                    // 0:行业 1:营销

    string          m_strFailedResendChannels;      // 通道失败重新发送
    string          m_strChannelOverRateKey;        // 通道超频 ？
    string          m_strTestChannelId;             // 测试通道号
    UInt32          m_uChannleId;                   // 通道ID
    string          m_strChannelTemplateId;         // 通道模板ID

    float           m_fCostFee;                     // 成本价
    double          m_dSaleFee;                     // 销售价

    int             m_iFailedResendTimes;           // 失败从发次数
    bool            m_bInOverRateWhiteList;         // 白名单超频列表
    bool            m_bIncludeChinese;              // 内容或签名是否包含中文
    bool            m_bSelectChannelAgain;          // 是否再次选择通道

    UInt32          m_uPhoneOperator;               // 号码所属运营商
    UInt32          m_uFreeChannelKeyword;          // 0:check channel keyword, 1:not check channel keyword;
    UInt32          m_uSelectChannelNum;            // 所选通道号
    set<UInt32>     m_FailedResendChannelsSet;      // 通道失败重新发送表
    set<string>     m_phoneBlackListInfo;           // eg: value:0:3 the phone is in sysblacklist ,value:2154 the phone is in channel 2154 blacklist
    UInt8           m_uFromSmsReback;
    int             m_iOverCount;
    UInt64          m_uOverTime;   //订单超时时间
    set<UInt32>     m_channelOverrateResendChannelsSet;
    int  m_iClientSendLimitCtrlTypeFlag;
};

// 路由回应
class TSmsRouterResp : public TMsg
{
public:
    TSmsRouterResp()
    {
        m_uChannleId = 0;
        nRouterResult = 0;
        m_uSelectChannelNum = 0;
        m_fCostFee = 0.0;
        m_dSaleFee = 0.0;
        m_uIsSendToReback = 0;
    }
    ~TSmsRouterResp() {}
public:
    int             nRouterResult;                  // 路由结果
    UInt32          m_uChannleId;                   // 通道ID
    UInt32          m_uSelectChannelNum;            // 所选通道号
    float           m_fCostFee;                     // 成本价
    double          m_dSaleFee;                     // 销售价
    string          m_strUcpaasPort;                // 平台端口
    string          m_strExtpendPort;               // user extend port
    string          m_strSignPort;                  // 签名端口

    string          m_strErrorCode;                 // 入DB errorcode 描述
    string          m_strYZXErrCode;                // 透传 report 描述
    UInt8           m_uIsSendToReback;
};


class CBaseRouter;
class CRouterThread : public CThread
{
public:
    CRouterThread(const char* name);
    virtual ~CRouterThread();

    // 初始化
    bool Init();
    UInt32 GetSessionMapSize();

public:

    //////////////////////////////////////////////////////////////////////////
    // 外部接口

    // 通道关键字校验
    bool CheckRegex(UInt32 uChannelId, string& strData, string& strOut, string strSign = "");

    // 校验通道是否 超频 true 超频了，false 没有超频
    bool checkChannelOverrate(TSmsRouterReq* pInfo, Channel smsChannel);

    // 校验通道是否流控 true 流控了，false 没有流控
    bool CheckOverFlow(TSmsRouterReq* pInfo, Channel smsChannel);

    // 获取通道登录状态
    bool GetChannelLoginStatus(Int32 channelid);

    std::map<UInt32, UInt32>& GetChannelQueueSizeMap();

    std::map<std::string, PhoneSection>& GetPhoneSectionMap();

    // 获取发送通道组
    bool GetReSendChannelGroup(std::string strKey, std::vector<std::string>& vectorChannelGroups);

    //////////////////////////////////////////////////////////////////////////

    ChannelSelect* GetChannelSelect();
    ChannelScheduler* GetChannelScheduler();
    std::map<UInt32, UInt64Set>&  GetChannelWhiteListMap();
    STL_MAP_STR& GetChannelTemplateMap();
    stl_map_str_ChannelSegmentList& GetChannelSegmentMap();
    ChannelWhiteKeywordMap& GetChannelWhiteKeywordListMap();
    std::map<std::string, stl_list_str>& GetClientSegmentMap();
    std::map<std::string, set<std::string> >& GetClientidSegmentMap();
    std::map<UInt32, stl_list_int32>& GetChannelSegCodeMap();
    AccountMap& GetAccountMap();
    map<UInt32,ChannelGroup>& GetChannelGroupMap();


    /* >>>>>>>>>>>>>>>>>>>> 路由权重分配相关 BEGIN <<<<<<<<<<<<<<<<<<<<<< */

    // 获取通道属性权重配置信息
    channel_attribute_weight_config_t& GetChannelAttributeWeightConfig();

    // 获取通道实时权重相关信息
    channels_realtime_weightinfo_t& GetChannelsRealtimeWeightInfo();

    // 获取多个通道组分别对应的通道组权重映射
    channelgroups_weight_t& GetChannelGroupsWeightMap();

    // 获取系统配置的通道组使用权重因子配置
    std::vector<UInt32>& GetChannelGroupWeightRatio();

    // 获取通道池策略配置 (自动路由)
    channel_pool_policies_t& GetChannelPoolPolicyConf();
    /* >>>>>>>>>>>>>>>>>>>> 路由权重分配相关 END <<<<<<<<<<<<<<<<<<<<<< */

private:
    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);

    void HandleSeesionSendRouterMsg(TMsg *pMsg);
    void proSendRouterResp(TSmsRouterResp* pSmsRouterResp, UInt64 llSeq = 0);


    void HandleTimeOut(TMsg* pMsg);

    // 获取测试通道路由
    void smsGetTestChannelRouter(TSmsRouterReq* pInfo, TSmsRouterResp *pRouterResp);

    // 获取通道路由
    void smsGetChannelRouter(TSmsRouterReq* pInfo, TSmsRouterResp *pRouterResp);


    // 初始化通道关键字列表
    void initChannelKeyWordSearchTree(map<UInt32,list<string> >& mapSetIn,map<UInt32,searchTree*>& mapSetOut);

    // 初始化通道组权重
    void initChannelGroupWeightMap(map<UInt32,ChannelGroup>& channelGroupMap, channelgroups_weight_t& channelGroupsWeight);

    // 获取通道面审关键字开关
    UInt32 GetFreeChannelKeyword(TSmsRouterReq* pInfo);


    // 获取系统参数
    void GetSysPara(const STL_MAP_STR& mapSysPara);
    bool CheckRebackSendTimes(TSmsRouterReq * pInfo);

private:

    SnManager*   m_pSnMng;

    UInt64 m_uAccessId;

    UInt32 m_uSelectChannelMaxNum;

    CoverRate *m_pOverRate;

    ChannelSelect *m_chlSelect;
    ChannelScheduler *m_chlSchedule;
    KeyWordCheck *m_KeyWordCheck;
    sysKeywordClassify *m_sysKeywordCheck;
    SmspUtils m_SmspUtils;
    std::map<UInt32,string> m_mapSendIpId;
    CThreadWheelTimer* m_pCheckDBWaitToSendTimer;                   // ontimer to delete overrateMap

    map<UInt32,ChannelGroup> m_ChannelGroupMap;                     // 通道组

    map<UInt32,searchTree*> m_ChannelKeyWordMap;                    // 通道关键字
    map<UInt32, UInt64Set> m_ChannelWhiteListMap;                   // 通道白关键字
    map<UInt32,channelFlow>m_ChannelFlowInfo;                       // 通道超频

    AccountMap m_AccountMap;                                        // 用户信息

    std::map<std::string, models::UserGw> m_userGwMap;              // KEY: userid_smstype

    map<string, ComponentConfig> m_componentConfigInfoMap;          // componentConfigInfoMap.[componentConfigInfo.m_uComponentId] = componentConfigInfo;

    STL_MAP_STR m_mapChannelTemplate;                               // key: 平台模板id+"_"+通道id; value: 通道模板id
    map<string,string> m_mapSystemErrorCode;                        // 系统错误代码
    map<UInt32, UInt32> m_mapGetChannelMqSize;                      // 获取通道mq大小
    std::map<std::string, PhoneSection> m_mapPhoneSection;          // 手机号段表 t_sms_phone_section, key: phone_section


    stl_map_str_ChannelSegmentList m_mapChannelSegment;             // 强制路由通道表 t_sms_segment_channel, key:phone_segment
    ChannelWhiteKeywordMap m_mapChannelWhiteKeyword;                // 白关键字强制路由 t_sms_white_keyword_channel

    std::map<std::string, stl_list_str> m_mapClientSegment;
    std::map<std::string, set<std::string> > m_mapClientidSegment;
    std::map<UInt32, stl_list_int32> m_mapChannelSegCode;

    CBaseRouter* m_pRouter;                                         // 通道路由指针
    CBaseRouter* m_pForeignRouter;                                  // 国际路由
    CBaseRouter* m_pDomesticRouter;                                 // 国内路由
    CBaseRouter* m_pDomesticNumberRouter;                           // 国内号码

    set<Int32>                  m_setLoginChannels;                 // 登录通道信息

    /* >>>>>>>>>>>>>>> 路由权重相关 BEGIN <<<<<<<<<<<<< */
    std::vector<UInt32> m_channelGroupWeightRatio;                  // 系统配置通道权重因子
    channelgroups_weight_t m_channelGroupsWeight;                   // 多通道组的权重
    channel_pool_policies_t m_channelPoolPolicies;                  // 通道池策略配置表
    channels_realtime_weightinfo_t m_channelsRealtimeWeightInfo;    // 通道实时更新的权重相关信息
    channel_attribute_weight_config_t m_channelAttrWeightConfig;    // 通道属性权重配置
    /* >>>>>>>>>>>>>>> 路由权重相关 END <<<<<<<<<<<<< */
    set<string> m_overRateWhiteList;
    UInt64 m_uOverTime;   //订单超时时间
    int m_iOverCount;  //订单失效次数
};

class TUpdateLoginChannelsReq:public TMsg
{
public:
    std::set<Int32>         m_setLoginChannels;
};

class TUpdateChannelTemplateReq : public TMsg
{
public:
    STL_MAP_STR m_mapChannelTemplate;
};

class TUpdateChannelSegmentReq : public TMsg
{
public:
    stl_map_str_ChannelSegmentList m_mapChannelSegment;

};

class TUpdateChannelWhiteKeywordReq : public TMsg
{
public:
    ChannelWhiteKeywordMap m_mapChannelWhiteKeyword;

};

class TUpdateClientSegmentReq : public TMsg
{
public:
    map<std::string, StrList> m_mapClientSegment;
    map<std::string, set<std::string> > m_mapClientidSegment;
};


class TUpdateChannelSegCodeReq : public TMsg
{
public:
    map<UInt32, stl_list_int32> m_mapChannelSegCode;
};

class TUpdateChannelRealtimeWeightInfo : public TMsg
{
public:
    channels_realtime_weightinfo_t m_channelsRealtimeWeightInfo;
};

class TUpdateChannelAttrWeightConfig : public TMsg
{
public:
    channel_attribute_weight_config_t m_channelAttrWeightConfig;
};

class TUpdateChannelPoolPolicyConfig : public TMsg
{
public:
    channel_pool_policies_t m_channelPoolPolicies;
};

class TUpdateChannelWhiteListReq: public TMsg
{
public:
    map<UInt32, UInt64Set> m_ChannelWhiteListMap;
};


class TUpdateChannelBlackListReq: public TMsg
{
public:
    map<UInt32, UInt64Set> m_ChannelBlackListMap;
};

class TUpdateChannelKeyWordReq: public TMsg
{
public:
    map<UInt32,list<string> > m_ChannelKeyWordMap;
};

class TUpdateChannelOverrateReq: public TMsg
{
public:
    std::map<string ,TempRule> m_ChannelOverrateMap;
};

class TUpdateChannelGroupReq: public TMsg
{
public:
    std::map<UInt32,ChannelGroup> m_ChannelGroupMap;
};


extern CRouterThread* g_pRouterThread;


#endif
