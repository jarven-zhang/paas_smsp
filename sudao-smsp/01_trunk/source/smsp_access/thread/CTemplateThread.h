#ifndef __C_TEMPLATE_THREAD_H__
#define __C_TEMPLATE_THREAD_H__

#include "Thread.h"
#include "SnManager.h"
#include "SmsContext.h"
#include "HttpSender.h"
#include "PhoneDao.h"
#include "SignExtnoGw.h"
#include "boost/regex.hpp"
#include <regex.h>
#include "SmspUtils.h"
#include "AgentInfo.h"
#include "SmsAccount.h"
#include "ComponentConfig.h"
#include "ComponentRefMq.h"
#include "PhoneSection.h"
#include "searchTree.h"
#include "agentAccount.h"
#include "CChineseConvert.h"
#include "KeyWordCheck.h"
#include "keywordClassify.h"
#include "sysKeywordClassify.h"
#include "CommClass.h"
#include "SmsTemplate.h"
#include "enum.h"


bool startTemplateThread();


enum AutoTempateCheckPhase
{
    AUTO_TEMPLATE_CHECK_PHASE_1_GLOBAL_SIGN = 0,
    AUTO_TEMPLATE_CHECK_PHASE_2_GLOBAL_NO_SIGN = 1,
    AUTO_TEMPLATE_CHECK_PHASE_3_CLIENT_SIGN = 2,
    AUTO_TEMPLATE_CHECK_PHASE_END = 3,
};

// 智能模板请求
class TSmsTemplateReq :public TMsg
{
public:
    TSmsTemplateReq()  {}
    ~TSmsTemplateReq() {}

public:
    string          m_strSmsid;
    string          m_strPhone;
    string          m_strClientId;                  // 客户ID
    string          m_strSmsType;                   // 短信类型
    string          m_strContent;                   // 内容
    string          m_strSign;                      // 签名
    string          m_strSignSimple;                // 转换为简体中文的签名, 后续所有检查都用简体中文
    string          m_strContentSimple;             // 转换为简体中文的短信内容, 后续所有检查都用简体中文
    bool            m_bTestChannel;                 // 是否是测试通道
    bool            m_bIncludeChinese;              // 是否为中文
};

// 智能模板回应
class TSmsTemplateResp :public TMsg
{
public:
    TSmsTemplateResp()
    {
       nMatchResult = NoMatch ;
       nTemplateId  = 0;
    }
    ~TSmsTemplateResp(){}

public:
    SmartTemplateMatchResult nMatchResult;          // 模板匹配状态
    int                      nTemplateId;           // 模板ID

    string          m_strErrorCode;                 // 入DB errorcode 描述
    string          m_strYZXErrCode;                // 透传 report 描述
    string          m_strKeyWord;                   // 包含的系统关键字
};

//变量模板
typedef std::list<StTemplateInfo> vartemp_list_t;
typedef std::map<std::string, vartemp_list_t> vartemp_to_user_map_t;

//固定模板
typedef std::set<StTemplateInfo> fixedtemp_set_t;
typedef std::map<std::string, fixedtemp_set_t> fixedtemp_to_user_map_t;

// 智能白模板
class TUpdateAutoWhiteTemplateReq:public TMsg
{
public:
    vartemp_to_user_map_t       m_mapAutoWhiteTemplate;
    fixedtemp_to_user_map_t m_mapFixedAutoWhiteTemplate;
};

// 智能黑模板
class TUpdateAutoBlackTemplateReq:public TMsg
{
public:
    vartemp_to_user_map_t       m_mapAutoBlackTemplate;
    fixedtemp_to_user_map_t m_mapFixedAutoBlackTemplate;
};

class CTemplateThread : public CThread
{
public:
    CTemplateThread(const char *name);
    virtual ~CTemplateThread();

public:
    // 初始化
    bool Init();

private:
    virtual void MainLoop();
    virtual void HandleMsg(TMsg* pMsg);

    // 处理session发送过来的消息
    void HandleSeesionSendTemplateMsg(TMsg* pMsg);
    void proSendTemplateResp(TSmsTemplateResp* pSmsTmpResp, UInt64 llSeq = 0);

    // 校验签名中是否包括系统关键字
    bool CheckSysKeywordInSign(string strClientID,string& strKeyWord,string& strSign,string& strKey);

    // 校验是否包含系统关键字 【true 不包含系统关键字，false包含系统关键字】
    bool CheckSysKeyword(string strClientID, string& strContent, string& strKeyWord, string& strSign, string& strKey, bool bIncludeChinese);

    // ------------------> 模板匹配相关 <-------------------------
    // 智能白模板匹配入口函数
    bool CheckAutoWhiteTemplate(const TSmsTemplateReq* pInfo, TSmsTemplateResp* pSmsTmpResp, vector <string>& vecContentAfterCheckAutoTemplate, bool& bMatchGlobalNoSignAutoTemplate);
    // 智能黑模板匹配入口函数
    bool CheckAutoBlackTemplate(const TSmsTemplateReq* pInfo, TSmsTemplateResp* pSmsTmpResp, vector <string>& vecContentAfterCheckAutoTemplate);
    // 调用检查固定/变量模板的方法
    bool CheckAutoTemplateByKey(const std::string& Key,                             // 三种key[ClientId + smsType + sign/"*" + smsType/"*" + smsType + sign]
                                const TSmsTemplateReq* pInfo,                       // 短信消息
                                const fixedtemp_to_user_map_t& fixedAutoTemplateMap,// 固定模板
                                const vartemp_to_user_map_t& varAutoTemplateMap,    // 变量模板
                                const std::string& strLogTip,                       // 提示
                                StTemplateInfo* & ptrMatchedTempInfo,               // 模板信息
                                vector <string>& vecContentAfterCheckAutoTemplate);

    // 固定智能模板检查公共方法
    bool CheckAutoFixedTemplate(const std::string& Key,
                                const TSmsTemplateReq* pInfo,
                                const fixedtemp_to_user_map_t& fixedAutoTemplateMap,
                                const std::string& strLogTip,                       // 提示
                                StTemplateInfo* & ptrMatchedTempInfo,               // 模板信息
                                vector <string>& vecContentAfterCheckAutoTemplate);
    // 变量智能模板检查公共方法
    bool CheckAutoVariableTemplate(const std::string& Key,
                                   const TSmsTemplateReq* pInfo,
                                   const vartemp_to_user_map_t& varAutoTemplateMap,
                                   const std::string& strLogTip,                    // 提示
                                   StTemplateInfo* & ptrMatchedTempInfo,            // 模板信息
                                   vector <string>& vecContentAfterCheckAutoTemplate);
    // 使用strContent去一一匹配setFixedTemplate中的固定模板，
    // 并把匹配到的模板存储到ptrMatchedTempInfo指针中
    bool MatchFixedTemplate(const std::string& strContent,
                            const fixedtemp_set_t& setFixedTemplate,
                            StTemplateInfo* & ptrMatchedTempInfo);
    // 使用strContent去一一匹配listVariableTemplate中的变量模板，
    // 并把匹配到的模板存储到ptrMatchedTempInfo指针中，
    // 同时在vectorRemainContent中保存所有变量
    bool MatchVariableTemplate(const std::string& strContent,
                              const vartemp_list_t& listVariableTemplate,
                              StTemplateInfo* & ptrMatchedTempInfo,
                              vector<string>& vectorRemainContent);
    // 把短信的SmsType转换成模板对应的SmsType，以便匹配
    std::string ConvertToTemplateSmsType(const std::string& strSmsType);
    std::string GetTemplateInfoOutput(const StTemplateInfo* ptrMatchedTempInfo);

    void InitKeyWordRegex(vector<string> vectorRegex);
    void FreeKeyWordRegex();
    bool CheckRemainContentRegex(vector<string> strRemainSmsContent, vector<string> & ContentMatch, vector<string> & RegexMatch);

    void GetSysPara(const STL_MAP_STR& mapSysPara);

    // 校验正则表达式
    bool CheckRegex(UInt32 uChannelId, string& strData, string& strOut, string strSign = "");

    // 初始化通道关键字
    void initChannelKeyWordSearchTree(map<UInt32, list<string> >& mapSetIn, map<UInt32, searchTree*>& mapSetOut);

private:

    CChineseConvert     *m_pCChineseConvert;                // 中文简体转换

    KeyWordCheck        *m_KeyWordCheck;                    // 关键字校验
    sysKeywordClassify  *m_sysKeywordCheck;                 // 系统关键字校验

    //system keyword convert regular
    int                 m_iSysKeywordCovRegular;            // 是否将系统关键字转换为简体
    int                 m_iAuditKeywordCovRegular;          // 审核关键字转换

    list<SystemParamRegex>  m_listKeyWordRegex;

    STL_MAP_STR         m_mapSystemErrorCode;               // 系统错误代码

    USER_GW_MAP         m_userGwMap;                        //

    //智能模板
    vartemp_to_user_map_t       m_mapAutoWhiteTemplate;     // 白模板变量模板
    fixedtemp_to_user_map_t     m_mapFixedAutoWhiteTemplate;// 白模板固定模板

    vartemp_to_user_map_t       m_mapAutoBlackTemplate;     // 黑模板变量模板
    fixedtemp_to_user_map_t     m_mapFixedAutoBlackTemplate;// 黑模板固定模板

};


extern CTemplateThread* g_pTemplateThread;


#endif
