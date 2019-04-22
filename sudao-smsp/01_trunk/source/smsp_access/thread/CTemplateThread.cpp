#include <mysql.h>
#include "UrlCode.h"
#include "main.h"
#include "UTFString.h"
#include "Comm.h"
#include "HttpParams.h"
#include "base64.h"
#include "SMSSmsInfo.h"
#include "CTemplateThread.h"
#include "CotentFilter.h"


CTemplateThread* g_pTemplateThread = NULL;


bool startTemplateThread()
{
    g_pTemplateThread = new CTemplateThread("TemplateThread");
    INIT_CHK_NULL_RET_FALSE(g_pTemplateThread);
    INIT_CHK_FUNC_RET_FALSE(g_pTemplateThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pTemplateThread->CreateThread());
    return true;
}


CTemplateThread::CTemplateThread(const char *name) : CThread(name)
{
    m_pCChineseConvert	= NULL;
    m_KeyWordCheck		= NULL;
    m_sysKeywordCheck	= NULL;
    m_iSysKeywordCovRegular = 0;
    m_iAuditKeywordCovRegular = 0;
}

CTemplateThread::~CTemplateThread()
{
}

bool CTemplateThread::Init( )
{
    if (!g_pRuleLoadThread)
    {
        LogError("CTemplateThread::Init ==>> g_pRuleLoadThread Init Error");
        return false;
    }

    // 初始化线程
    if (false == CThread::Init())
    {
        printf("CTemplateThread::Init ==>> CThread::Init is failed.\n");
        return false;
    }

    // 初始化中文简体转化
    m_pCChineseConvert = new CChineseConvert();
    if (!m_pCChineseConvert->Init())
    {
        LogError("CTemplateThread::Init ==>>m_pCChineseConvert Init Error");
        return false;
    }

    // 获取系统错误码
    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    // 关键字校验
    m_KeyWordCheck = new KeyWordCheck();
    m_KeyWordCheck->initParam();

    // 系统关键字校验
    m_sysKeywordCheck = new sysKeywordClassify("sys");
    m_sysKeywordCheck->initParam();

    //智能白模板
    m_mapAutoWhiteTemplate.clear();
    m_mapFixedAutoWhiteTemplate.clear();
    g_pRuleLoadThread->getAutoWhiteTemplateMap(m_mapAutoWhiteTemplate, m_mapFixedAutoWhiteTemplate);
    LogNotice("m_mapAutoWhiteTemplate size[%d], m_mapFixedAutoWhiteTemplate size[%d]",
              m_mapAutoWhiteTemplate.size(), m_mapFixedAutoWhiteTemplate.size());

    //智能黑模板
    m_mapAutoBlackTemplate.clear();
    m_mapFixedAutoBlackTemplate.clear();
    g_pRuleLoadThread->getAutoBlackTemplateMap(m_mapAutoBlackTemplate, m_mapFixedAutoBlackTemplate);
    LogNotice("m_mapAutoBlackTemplate size[%d], m_mapFixedAutoBlackTemplate size[%d]",
              m_mapAutoBlackTemplate.size(), m_mapFixedAutoBlackTemplate.size());

    // 获取用户信息
    m_userGwMap.clear();
    g_pRuleLoadThread->getUserGwMap(m_userGwMap);

    // 获取系统配置信息
    STL_MAP_STR sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;

}

void CTemplateThread::GetSysPara(const STL_MAP_STR &mapSysPara)
{
    string strSysPara = "";
    STL_MAP_STR::const_iterator iter;

    do
    {
        strSysPara = "KEYWORD_CONVERT_REGULAR";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }
        STL_MAP_STR mapSet;
        Comm::splitMap(strTmp, "|", ";", mapSet);
        m_iSysKeywordCovRegular = atoi(mapSet["1"].data());
        m_iAuditKeywordCovRegular = atoi(mapSet["2"].data());
        m_KeyWordCheck->initKeyWordRegular(m_iAuditKeywordCovRegular);
    }
    while (0);

    LogNotice("System parameter(%s) value size(m_iSysKeywordCovRegular[%d], m_iAuditKeywordCovRegular[%d]).",
              strSysPara.c_str(),
              m_iSysKeywordCovRegular,
              m_iAuditKeywordCovRegular );

    do
    {
        strSysPara = "KEYWORD_REGULAR";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;

        string strKeywordRegexSimple = "";
        m_pCChineseConvert->ChineseTraditional2Simple(strTmp, strKeywordRegexSimple);

        vector<string> vec;
        Comm::splitExVectorSkipEmptyString(strKeywordRegexSimple, ";", vec);
        FreeKeyWordRegex();
        InitKeyWordRegex(vec);
    }
    while (0);

}

void CTemplateThread::InitKeyWordRegex(vector<string> vectorRegex)
{
    for(vector<string>::iterator itor = vectorRegex.begin(); itor != vectorRegex.end(); itor++)
    {
        regex_t _regex;
        string strRegex = *itor;
        if(0 == regcomp(&_regex, strRegex.data(), REG_EXTENDED))
        {
            LogNotice("regcomp %s success!", strRegex.data());
            SystemParamRegex sysRegex;
            sysRegex._regex = _regex;
            sysRegex._pattern = strRegex;
            m_listKeyWordRegex.push_back(sysRegex);
        }
        else
        {
            LogError("regcomp %s fail!", strRegex.data());
        }
    }

    LogNotice("m_listKeyWordRegex init over, size[%d]", m_listKeyWordRegex.size());


}

void CTemplateThread::FreeKeyWordRegex()
{
    LogNotice("Free m_listKeyWordRegex, old size[%d]", m_listKeyWordRegex.size());
    for(list<SystemParamRegex>::iterator itor = m_listKeyWordRegex.begin(); itor != m_listKeyWordRegex.end(); )
    {
        SystemParamRegex sysRegex = *itor;
        sysRegex._pattern.clear();
        regfree(&sysRegex._regex);
        itor = m_listKeyWordRegex.erase(itor);
    }
}

// 校验内容
bool CTemplateThread::CheckRemainContentRegex(vector<string> strRemainSmsContent,
        vector<string> &ContentMatch, vector<string> &RegexMatch)
{
    const size_t nmatch = 1;

    for(vector<string>::iterator itor = strRemainSmsContent.begin(); itor != strRemainSmsContent.end(); itor++)
    {
        string strPart = *itor;

        for(list<SystemParamRegex>::iterator it = m_listKeyWordRegex.begin(); it != m_listKeyWordRegex.end(); it++)
        {
            regmatch_t pmatch[1];
            SystemParamRegex sysRegex = *it;
            regex_t _regex = sysRegex._regex;
            if(0 == regexec(&_regex, strPart.data(), nmatch, pmatch, 0))
            {
                RegexMatch.push_back(sysRegex._pattern);
                ContentMatch.push_back(strPart.substr(pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so));
            }
        }
    }
    return !RegexMatch.empty();
}

void CTemplateThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        for(int i = 0; i < 10; i++)
        {
            pthread_mutex_lock(&m_mutex);
            TMsg *pMsg = m_msgQueue.GetMsg();
            pthread_mutex_unlock(&m_mutex);

            if (NULL == pMsg)
            {
                usleep(g_uSecSleep);
            }
            else if (NULL != pMsg)
            {
                HandleMsg(pMsg);
                SAFE_DELETE( pMsg ) ;
            }
        }
    }
}

void CTemplateThread::HandleMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CAdapter::InterlockedIncrement((long *)&m_iCount);

    switch(pMsg->m_iMsgType)
    {
    case MSGTYPE_SEESION_TO_TEMPLATE_REQ:
    {
        HandleSeesionSendTemplateMsg(pMsg);
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
    {
        TUpdateSystemErrorCodeReq *pHttp = (TUpdateSystemErrorCodeReq *)pMsg;
        if (pHttp)
        {
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pHttp->m_mapSystemErrorCode;
            LogNotice("CTemplateThread::HandleMsg ==>> update t_sms_system_error_desc size:%d.", m_mapSystemErrorCode.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
    {
        TUpdateUserGwReq *pReq = (TUpdateUserGwReq *)pMsg;
        if (pReq)
        {
            m_userGwMap.clear();
            m_userGwMap = pReq->m_UserGwMap;
            LogNotice("RuleUpdate userGwMap update. map.size[%d]", pReq->m_UserGwMap.size());
        }
        break;
    }
    // 智能白模板
    case MSGTYPE_RULELOAD_AUTO_TEMPLATE_UPDATE_REQ:
    {
        TUpdateAutoWhiteTemplateReq *pReq = (TUpdateAutoWhiteTemplateReq *)pMsg;
        if (pReq)
        {
            m_mapAutoWhiteTemplate.clear();
            m_mapFixedAutoWhiteTemplate.clear();
            m_mapAutoWhiteTemplate = pReq->m_mapAutoWhiteTemplate;
            m_mapFixedAutoWhiteTemplate = pReq->m_mapFixedAutoWhiteTemplate;
            LogNotice("CTemplateThread::HandleMsg ==>> update t_sms_auto_template, m_mapAutoWhiteTemplate.size[%d], m_mapFixedAutoWhiteTemplate.size[%d]",
                      m_mapAutoWhiteTemplate.size(), m_mapFixedAutoWhiteTemplate.size());
        }
        break;
    }
    // 智能黑模板
    case MSGTYPE_RULELOAD_AUTO_BLACK_TEMPLATE_UPDATE_REQ:
    {
        TUpdateAutoBlackTemplateReq *pReq = (TUpdateAutoBlackTemplateReq *)pMsg;
        if (pReq)
        {
            m_mapAutoBlackTemplate.clear();
            m_mapFixedAutoBlackTemplate.clear();
            m_mapAutoBlackTemplate = pReq->m_mapAutoBlackTemplate;
            m_mapFixedAutoBlackTemplate = pReq->m_mapFixedAutoBlackTemplate;
            LogNotice("CTemplateThread::HandleMsg ==>> update t_sms_auto_black_template, m_mapAutoBlackTemplate.size[%d], m_mapFixedAutoBlackTemplate.size[%d]",
                      m_mapAutoBlackTemplate.size(), m_mapFixedAutoBlackTemplate.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
    {
        TUpdateSysParamRuleReq *pReq = (TUpdateSysParamRuleReq *)pMsg;
        if (pReq)
        {
            GetSysPara(pReq->m_SysParamMap);
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ:
    {
        TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
        if (pReq && m_sysKeywordCheck)
        {
            m_sysKeywordCheck->initKeywordSearchTree(pReq->m_keyWordMap);
            LogNotice("RuleUpdate system keyword list update. ");
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYS_CGROUPREFCLIENT_UPDATE_REQ:
    {
        TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
        if (pReq && m_sysKeywordCheck)
        {
            m_sysKeywordCheck->setClientGrpRefClient(pReq->m_clientGrpRefClientMap);
            LogNotice("RuleUpdate sysCGroupRefClient update. size[%d]", pReq->m_clientGrpRefClientMap.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYS_CLIENTGROUP_UPDATE_REQ:
    {
        TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
        if (pReq && m_sysKeywordCheck)
        {
            m_sysKeywordCheck->setCgrpRefKeywordGrp(pReq->m_clientGrpRefKeywordGrpMap, pReq->m_uDefaultGroupId);
            LogNotice("RuleUpdate sysClientGroup update. size[%d]", pReq->m_clientGrpRefKeywordGrpMap.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYS_KGROUPREFCATEGORY_UPDATE_REQ:
    {
        TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
        if (pReq && m_sysKeywordCheck)
        {
            m_sysKeywordCheck->setkeyGrpRefCategory(pReq->m_keywordGrpRefCategoryMap);
            LogNotice("RuleUpdate sysKGroupRefCategory update. size[%d]", pReq->m_keywordGrpRefCategoryMap.size());
        }
        break;
    }
    default:
    {
        LogWarn("CTemplateThread::HandleMsg ==>> msgType[0x:%x] is invalid.", pMsg->m_iMsgType);
        break;
    }
    }
}

void CTemplateThread::HandleSeesionSendTemplateMsg(TMsg *pMsg)
{
    bool bRet = false;
    bool bCheckSysKeyword = false;  // 是否校验系统关键字

    TSmsTemplateReq *pTemplateReq = (TSmsTemplateReq *)pMsg;
    CHK_NULL_RETURN(pTemplateReq);

    TSmsTemplateResp *pSmsTmpResp = new TSmsTemplateResp();
    CHK_NULL_RETURN(pSmsTmpResp);

    pSmsTmpResp->m_iSeq = pTemplateReq->m_iSeq;
    pSmsTmpResp->m_strSessionID = pTemplateReq->m_strSessionID;
    pSmsTmpResp->m_pSender = pTemplateReq->m_pSender;

    // 判断是否为测试通道,若为测试通道则不过黑白模板，直接走系统关键字
    if (!pTemplateReq->m_bTestChannel)
    {
        vector <string> vecContentAfterCheckAutoTemplate;  //智能模板匹配过剩下的短信内容，可能被砍成几段
        vecContentAfterCheckAutoTemplate.clear();

        // 匹配黑模板，则返回
        if (CheckAutoBlackTemplate(pTemplateReq, pSmsTmpResp, vecContentAfterCheckAutoTemplate))
        {
            // 错误代码在CheckAutoBlackTemplate中设置
            pSmsTmpResp->nMatchResult = MatchBlack;
            bRet = true;
            goto GO_EXIT;
        }

        // 匹配智能变量白模板
        // 是否匹配上的是全局空签名智能模板
        bool bMatchGlobalNoSignAutoTemplate = false;
        vecContentAfterCheckAutoTemplate.clear();
        if (CheckAutoWhiteTemplate(pTemplateReq, pSmsTmpResp, vecContentAfterCheckAutoTemplate, bMatchGlobalNoSignAutoTemplate))
        {
            // 匹配上智能变量白模板
            // 且是未配置签名的全局白模板，则需要检查签名是否含有系统关键字
            if (bMatchGlobalNoSignAutoTemplate)
            {
                string strKey = pTemplateReq->m_strClientId + "_" + pTemplateReq->m_strSmsType;
                string strTempSign = pTemplateReq->m_strSign;
                if (m_iSysKeywordCovRegular & 0x01)
                {
                    strTempSign = pTemplateReq->m_strSignSimple;
                }
                // 校验签名中是否包含系统签名
                if (!CheckSysKeywordInSign(pTemplateReq->m_strClientId, pSmsTmpResp->m_strKeyWord, strTempSign, strKey))
                {
                    pSmsTmpResp->m_strErrorCode = BLACKLIST_KEYWORD;
                    pSmsTmpResp->m_strYZXErrCode = BLACKLIST_KEYWORD;
                    pSmsTmpResp->nMatchResult = MatchKeyWord;
                    bRet = true;
                    goto GO_EXIT;
                }
            }

            LogDebug("vecContentAfterCheckAutoTemplate size %d", vecContentAfterCheckAutoTemplate.size());

            /* 模板变量部分正则检测 */
            if (!vecContentAfterCheckAutoTemplate.empty())
            {
                // 对剩余部分进行关键字正则匹配
                // 系统参数KEYWORD_REGULAR正则匹配
                vector<string> strContentMatch;
                vector<string> strRegexMatch;
                strContentMatch.clear();
                strRegexMatch.clear();
                if (CheckRemainContentRegex(vecContentAfterCheckAutoTemplate, strContentMatch, strRegexMatch))
                {
                    for (size_t i = 0; i < strContentMatch.size(); i++)
                    {
                        LogElk("[%s:%s:%s] sms content contain[%s] match regex[%s]",
                               pTemplateReq->m_strClientId.c_str(), pTemplateReq->m_strSmsid.c_str(), pTemplateReq->m_strPhone.c_str(),
                               strContentMatch[i].c_str(), strRegexMatch[i].c_str());
                    }
                    pSmsTmpResp->nMatchResult = MatchVariable_MatchRegex;
                    bRet = true;
                }
                else
                {
                    LogNotice("[%s:%s:%s] MatchVariable_NoMatchRegex Pass",
                              pTemplateReq->m_strClientId.c_str(), pTemplateReq->m_strSmsid.c_str(), pTemplateReq->m_strPhone.c_str());
                    pSmsTmpResp->nMatchResult = MatchVariable_NoMatchRegex;
                    bRet = true;
                }
            }
            else
            {
                LogNotice("[%s:%s:%s] MatchConstant Pass",
                          pTemplateReq->m_strClientId.c_str(), pTemplateReq->m_strSmsid.c_str(), pTemplateReq->m_strPhone.c_str());
                pSmsTmpResp->nMatchResult = MatchConstant;
                bRet = true;
            }
        }
        else // 智能模板匹配失败
        {
            bCheckSysKeyword = true;
        }
    }
    else
    {
        bCheckSysKeyword = true;
    }

    // 检查短信内容是否含有系统关键字
    if (bCheckSysKeyword)
    {
        string strKey = pTemplateReq->m_strClientId + "_" + pTemplateReq->m_strSmsType;
        string strTempSign = pTemplateReq->m_strSign, strTempContent = pTemplateReq->m_strContent;
        if (m_iSysKeywordCovRegular & 0x01)
        {
            strTempSign = pTemplateReq->m_strSignSimple;
            strTempContent = pTemplateReq->m_strContentSimple;
        }
        if (false == CheckSysKeyword(pTemplateReq->m_strClientId, strTempContent, pSmsTmpResp->m_strKeyWord, strTempSign, strKey, pTemplateReq->m_bIncludeChinese))
        {
            LogElk("[%s:%s:%s] Content[%s] or sign[%s] contain Keyword[%s]",
                   pTemplateReq->m_strClientId.c_str(), pTemplateReq->m_strSmsid.c_str(), pTemplateReq->m_strPhone.c_str(),
                   pTemplateReq->m_strContent.c_str(), pTemplateReq->m_strSign.c_str(), pSmsTmpResp->m_strKeyWord.c_str());
            pSmsTmpResp->m_strErrorCode = BLACKLIST_KEYWORD;
            pSmsTmpResp->m_strYZXErrCode = BLACKLIST_KEYWORD;
            pSmsTmpResp->nMatchResult = MatchKeyWord;
            bRet = true;
        }
    }

GO_EXIT:

    if( false == bRet )
    {
        LogNotice("[%s:%s:%s] AutoTemplate Pass !!!!! ",
                  pTemplateReq->m_strClientId.c_str(), pTemplateReq->m_strSmsid.c_str(), pTemplateReq->m_strPhone.c_str());
    }

    proSendTemplateResp(pSmsTmpResp);

}

void CTemplateThread::proSendTemplateResp( TSmsTemplateResp *pSmsTmpResp, UInt64 llSeq )
{
    if ( pSmsTmpResp )
    {
        pSmsTmpResp->m_iMsgType = MSGTYPE_TEMPLATE_RESP_TO_SEESION;
        pSmsTmpResp->m_pSender->PostMsg( pSmsTmpResp );
    }
}

// 把短信的SmsType转换成模板对应的SmsType，以便匹配
std::string CTemplateThread::ConvertToTemplateSmsType(const std::string &strSmsType)
{
    string smsType = strSmsType;

    /*模板只分行业(10)和会员营销(11)了*/
    if(smsType == "4" || smsType == "0")
    {
        //验证码(4)和通知(0)匹配行业(10)
        smsType = "10";
    }
    else if(smsType == "5")
    {
        //营销(5)匹配会员营销(11)
        smsType = "11";
    }
    else
    {
        ;
    }

    return smsType;
}

std::string CTemplateThread::GetTemplateInfoOutput(const StTemplateInfo *ptrMatchedTempInfo)
{
    char szMatchedTempInfo[128] = {0};
    int n = 0;

    if (ptrMatchedTempInfo)
    {
        n = snprintf(szMatchedTempInfo, sizeof(szMatchedTempInfo) / sizeof(szMatchedTempInfo[0]),
                     "[temp-level:%s]-[temp-type:%s]-[temp-id:%d]",
                     ptrMatchedTempInfo->iTemplateLevel == AUTO_TEMPLATE_LEVEL_GLOBAL ? "GLOBAL" : "CLIENT",
                     ptrMatchedTempInfo->iTemplateType == AUTO_TEMPLATE_TYPE_FIXED ? "FIXED" : "VARIABLE",
                     ptrMatchedTempInfo->iTemplateId);
    }
    else
    {
        n = snprintf(szMatchedTempInfo, sizeof(szMatchedTempInfo) / sizeof(szMatchedTempInfo[0]),
                     "[unknown-temp-level]-[unknown-temp-type]-[unknown-temp-tempid]");
    }

    return std::string(szMatchedTempInfo, n);
}

bool CTemplateThread::CheckAutoBlackTemplate( const TSmsTemplateReq *pInfo, TSmsTemplateResp *pSmsTmpResp, vector <string> &vecContentAfterCheckAutoTemplate )
{
    std::string smsType = ConvertToTemplateSmsType(pInfo->m_strSmsType);
    std::string sign = pInfo->m_strSignSimple;
    Comm::ConvertPunctuations(sign);
    Comm::Sbc2Dbc(sign);

    // 1. 检查带签名的全局黑模板
    // 2. 检查不带签名的全局黑模板
    // 3. 检查带签名的用户黑模板
    for (int checkPhase = (int)AUTO_TEMPLATE_CHECK_PHASE_1_GLOBAL_SIGN;
            checkPhase < (int)AUTO_TEMPLATE_CHECK_PHASE_END; ++checkPhase)
    {
        std::string Key = "", strLogTip = "";
        switch (static_cast<AutoTempateCheckPhase>(checkPhase))
        {
        // 设置查找模板集合使用的Key，
        // 并预设错误码，减少switch判断
        case AUTO_TEMPLATE_CHECK_PHASE_1_GLOBAL_SIGN:
            Key = "*" + smsType + sign;
            pSmsTmpResp->m_strErrorCode = GLOBAL_BLACK_TEMPLATE;
            pSmsTmpResp->m_strYZXErrCode = GLOBAL_BLACK_TEMPLATE;
            strLogTip = "Global-Black-Sign";
            break;
        case AUTO_TEMPLATE_CHECK_PHASE_2_GLOBAL_NO_SIGN:
            Key = "*" + smsType;
            pSmsTmpResp->m_strErrorCode = GLOBAL_BLACK_TEMPLATE;
            pSmsTmpResp->m_strYZXErrCode = GLOBAL_BLACK_TEMPLATE;
            strLogTip = "Global-Black-No-Sign";
            break;
        case AUTO_TEMPLATE_CHECK_PHASE_3_CLIENT_SIGN:
            Key = pInfo->m_strClientId + smsType + sign;
            pSmsTmpResp->m_strErrorCode = CLIENT_BLACK_TEMPLATE;
            pSmsTmpResp->m_strYZXErrCode = CLIENT_BLACK_TEMPLATE;
            strLogTip = "Client-Black-Sign";
            break;
        default:
            LogWarn("[%s:%s:%s] CTemplateThread::CheckAutoBlackTemplate ==>> Unknown Template-check-phase: [%d]",
                    pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
                    (int)checkPhase);
            continue;
        }

        StTemplateInfo *ptrMatchedTempInfo = NULL;
        // 匹配到加签名的全局黑模板，返回错误
        if (CheckAutoTemplateByKey(Key, pInfo, m_mapFixedAutoBlackTemplate, m_mapAutoBlackTemplate,
                                   strLogTip, ptrMatchedTempInfo, vecContentAfterCheckAutoTemplate))
        {
            if (ptrMatchedTempInfo != NULL)
            {
                pSmsTmpResp->nTemplateId = ptrMatchedTempInfo->iTemplateId;
            }
            LogElk("[%s:%s:%s] Content[%s] SmsType[%s] Sign[%s] Match black-template %s %s!!!",
                   pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
                   pInfo->m_strContent.c_str(), pInfo->m_strSmsType.c_str(), pInfo->m_strSign.c_str(), strLogTip.c_str(),
                   GetTemplateInfoOutput(ptrMatchedTempInfo).c_str());

            return true;
        }
    }

    pSmsTmpResp->m_strErrorCode = "";
    pSmsTmpResp->m_strYZXErrCode = "";

    LogDebug("[%s:%s:%s] Content[%s] SmsType[%s] Sign[%s] NOT Matched any Auto-Black-Template.",
             pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
             pInfo->m_strContent.c_str(), pInfo->m_strSmsType.c_str(), pInfo->m_strSign.c_str());

    return false;
}

// 智能白模板匹配入口函数
bool CTemplateThread::CheckAutoWhiteTemplate(const TSmsTemplateReq *pInfo,
        TSmsTemplateResp *pSmsTmpResp, vector <string> &vecContentAfterCheckAutoTemplate, bool &bMatchGlobalNoSignAutoTemplate)
{
    string smsType = ConvertToTemplateSmsType(pInfo->m_strSmsType);

    std::string sign = pInfo->m_strSignSimple;
    Comm::ConvertPunctuations(sign);
    Comm::Sbc2Dbc(sign);

    for (int checkPhase = (int)AUTO_TEMPLATE_CHECK_PHASE_1_GLOBAL_SIGN;
            checkPhase < (int)AUTO_TEMPLATE_CHECK_PHASE_END; ++checkPhase)
    {
        string Key = "", strLogTip = "";
        switch(static_cast<AutoTempateCheckPhase>(checkPhase))
        {
        // 设置查找模板集合使用的Key，
        case AUTO_TEMPLATE_CHECK_PHASE_1_GLOBAL_SIGN:
            Key = "*" + smsType + sign;
            strLogTip = "Global-White-Sign";
            break;
        case AUTO_TEMPLATE_CHECK_PHASE_2_GLOBAL_NO_SIGN:
            Key = "*" + smsType;
            strLogTip = "Global-White-No-Sign";
            break;
        case AUTO_TEMPLATE_CHECK_PHASE_3_CLIENT_SIGN:
            Key = pInfo->m_strClientId + smsType + sign;
            strLogTip = "Client-White-Sign";

            break;
        default:
            LogWarn("[%s:%s:%s] Unknown Template-check-phase: [%d]",
                    pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
                    (int)checkPhase);
            continue;
        }

        // 匹配到指定白模板，则可以返回
        StTemplateInfo *ptrMatchedTempInfo = NULL;
        if (true == CheckAutoTemplateByKey(Key, pInfo, m_mapFixedAutoWhiteTemplate,
                                           m_mapAutoWhiteTemplate, strLogTip, ptrMatchedTempInfo, vecContentAfterCheckAutoTemplate))
        {
            if ( (int)AUTO_TEMPLATE_CHECK_PHASE_2_GLOBAL_NO_SIGN == checkPhase)
            {
                bMatchGlobalNoSignAutoTemplate = true;
            }

            if ( ptrMatchedTempInfo != NULL )
            {
                pSmsTmpResp->nTemplateId = ptrMatchedTempInfo->iTemplateId;
            }

            LogElk("[%s:%s:%s] Content[%s] SmsType[%s] Sign[%s] Match white-template %s %s!!!!!!!!",
                   pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
                   pInfo->m_strContent.c_str(), pInfo->m_strSmsType.c_str(),  pInfo->m_strSign.c_str(),
                   strLogTip.c_str(), GetTemplateInfoOutput(ptrMatchedTempInfo).c_str());

            return true;
        }
    }

    LogDebug("[%s:%s:%s] Content[%s] SmsType[%s] Sign[%s] NOT Matched any Auto-White-Template.",
             pInfo->m_strClientId.c_str(), pInfo->m_strSmsid.c_str(), pInfo->m_strPhone.c_str(),
             pInfo->m_strContent.c_str(), pInfo->m_strSmsType.c_str(), pInfo->m_strSign.c_str());

    return false;

}

bool CTemplateThread::CheckAutoTemplateByKey(const std::string &Key,
        const TSmsTemplateReq *pInfo,
        const fixedtemp_to_user_map_t &fixedAutoTemplateMap,
        const vartemp_to_user_map_t &varAutoTemplateMap,
        const std::string &strLogTip,
        StTemplateInfo *&ptrMatchedTempInfo,
        vector <string> &vecContentAfterCheckAutoTemplate)
{
    // 先检查固定模板
    if (CheckAutoFixedTemplate(Key, pInfo, fixedAutoTemplateMap, strLogTip, ptrMatchedTempInfo, vecContentAfterCheckAutoTemplate))
    {
        return true;
    }

    // 再检查可变模板
    if (CheckAutoVariableTemplate(Key, pInfo, varAutoTemplateMap, strLogTip, ptrMatchedTempInfo, vecContentAfterCheckAutoTemplate))
    {
        return true;
    }

    return false;
}

// 检查固定模板
bool CTemplateThread::CheckAutoFixedTemplate(const std::string &Key,
        const TSmsTemplateReq *pInfo,
        const fixedtemp_to_user_map_t &fixedAutoTemplateMap,
        const std::string &strLogTip,
        StTemplateInfo *&ptrMatchedTempInfo,               // 模板信息
        vector <string> &vecContentAfterCheckAutoTemplate)
{
    fixedtemp_to_user_map_t::const_iterator itor = fixedAutoTemplateMap.find(Key);
    if(itor == fixedAutoTemplateMap.end())
    {
        LogDebug("[%s:%s:%s] SmsType[%s] Sign[%s] "
            "sms content[%s] not configure [%s-Fixed] AutoTemplate",
            pInfo->m_strClientId.c_str(),
            pInfo->m_strSmsid.c_str(),
            pInfo->m_strPhone.c_str(),
            pInfo->m_strSmsType.c_str(),
            pInfo->m_strSign.c_str(),
            pInfo->m_strContent.c_str(),
            strLogTip.c_str());

        return false;
    }

    return MatchFixedTemplate(pInfo->m_strContentSimple, itor->second, ptrMatchedTempInfo);
}

// 检查可变模板
bool CTemplateThread::CheckAutoVariableTemplate(const std::string &Key,
        const TSmsTemplateReq *pInfo,
        const vartemp_to_user_map_t &varAutoTemplateMap,
        const std::string &strLogTip,
        StTemplateInfo *&ptrMatchedTempInfo,               // 模板信息
        vector <string> &vecContentAfterCheckAutoTemplate)
{

    vartemp_to_user_map_t::const_iterator itor = varAutoTemplateMap.find(Key);

    if(itor == varAutoTemplateMap.end())
    {
        char szLogPrefix[128] = { 0 };

        snprintf(szLogPrefix, sizeof(szLogPrefix) / sizeof(szLogPrefix[0]),
                 "[%s:%s:%s] SmsType[%s] Sign[%s]",
                 pInfo->m_strClientId.c_str(),
                 pInfo->m_strSmsid.c_str(),
                 pInfo->m_strPhone.c_str(),
                 pInfo->m_strSmsType.c_str(),
                 pInfo->m_strSign.c_str());

        LogDebug("%s sms content[%s] not configure [%s-Variable] AutoTemplate",
            szLogPrefix,
            pInfo->m_strContent.c_str(),
            strLogTip.c_str());

        return false;
    }

    return MatchVariableTemplate(pInfo->m_strContentSimple,
                                itor->second,
                                ptrMatchedTempInfo,
                                vecContentAfterCheckAutoTemplate);
}

bool CTemplateThread::MatchFixedTemplate(const std::string &strOrigContent,
        const fixedtemp_set_t &setFixedTemplate,
        StTemplateInfo *&ptrMatchedTempInfo)
{
    std::string strContent = strOrigContent;
    Comm::ConvertPunctuations(strContent);

    ptrMatchedTempInfo = NULL;

    // 循环遍历，直到找到与短信内容一致的模板
    fixedtemp_set_t::iterator it = setFixedTemplate.begin();
    for (; it != setFixedTemplate.end(); ++it )
    {
        if (it->fixedTempContent == strContent)
        {
            ptrMatchedTempInfo = const_cast<StTemplateInfo *>(&(*it));
            return true;
        }
    }
    return false;
}

bool CTemplateThread::MatchVariableTemplate(const std::string &strOrigContent,
        const vartemp_list_t &listVariableTemplate,
        StTemplateInfo *&ptrMatchedTempInfo,
        vector<string> &vectorRemainContent
                                           )
{
    ptrMatchedTempInfo = NULL;
    string strConvertedContent = strOrigContent;
    Comm::ConvertPunctuations(strConvertedContent);
    vartemp_list_t::const_iterator it = listVariableTemplate.begin();
    for( ; it != listVariableTemplate.end(); it++)
    {
        const StTemplateInfo &stTempInfo = *it;
        const vartemp_elements_vec_t &varTempElements = stTempInfo.varTempElements;
        vectorRemainContent.clear();
        std::string strContent = strConvertedContent;

        vartemp_elements_vec_t::const_iterator itr = varTempElements.begin();
        for(; itr != varTempElements.end(); itr++)
        {
            const string &_part = *itr;
            std::size_t pos = strContent.find(_part);
            if(pos == std::string::npos)
            {
                break;
            }

            // 模板的第一个元素不是完全匹配到内容的开头，则算没匹配上
            if (itr == varTempElements.begin()
                    && !stTempInfo.bHasHeaderSeperator
                    && pos != 0)
            {
                break;
            }

            if(pos > 0)
            {
                LogDebug("remain content:%s", strContent.substr(0, pos).c_str());
                vectorRemainContent.push_back(strContent.substr(0, pos));
            }
            strContent = strContent.substr(pos + _part.length());
        }

        if(itr == varTempElements.end())
        {
            if (!strContent.empty())
            {
                if (stTempInfo.bHasLastSeperator)
                {
                    LogDebug("remain content:%s", strContent.c_str());
                    vectorRemainContent.push_back(strContent);
                }
                else
                {
                    return false;
                }
            }
            // 往上层传递匹配上的模板的指针
            ptrMatchedTempInfo = const_cast<StTemplateInfo *>(&(*it));

            return true;
        }
    }
    return false;
}

bool CTemplateThread::CheckRegex(UInt32 uChannelId, string &strData, string &strOut, string strSign)	//strOut: matched words
{
    string strTempData = "";
    string strSignData = "";
    string strContentTemp = strData;
    if (strSign.empty())
    {
        strTempData.append(strSign);
        strTempData.append(strData);
    }
    else
    {
        string strLeft;
        string strRight;
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";

        strLeft = http::UrlCode::UrlDecode(strLeft);

        strRight = http::UrlCode::UrlDecode(strRight);
        strTempData.append(strLeft);
        strTempData.append(strSign);
        strTempData.append(strRight);
        strSignData.append(strTempData);
        strTempData.append(strData);
    }

    if(strTempData.empty())
    {
        LogWarn("CheckRegex() failed.  strContent[%s] is null", strTempData.data());
        return false;
    }

    return false;
}

bool CTemplateThread::CheckSysKeywordInSign(string strClientID, string &strKeyWord, string &strSign, string &strKey)
{
    if(strSign.empty())
    {
        LogWarn("sign[%s] is null!!", strSign.c_str());
        return true;
    }

    LogDebug("clientid:%s, Check sys-keyword in sign (%s)", strClientID.c_str(), strSign.c_str());
    std::string strTempData = "";
    USER_GW_MAP::iterator itrGW = m_userGwMap.find(strKey);
    if (itrGW == m_userGwMap.end())
    {
        LogWarn("clientid:%s, key[%s] is not find in table userGw.", strClientID.c_str(), strKey.c_str());
    }
    else
    {
        //0:check system keyword; 1:don't check system keyword. by liaojialin 20170706
        if (1 == itrGW->second.m_uFreeKeyword)
        {
            LogNotice("clientid:%s, key[%s] is freeKeyword.", strClientID.c_str(), strKey.c_str());
            return true;
        }
    }

    if ( true == m_sysKeywordCheck->keywordCheck(strClientID,
            string(), strSign, strKeyWord, m_iSysKeywordCovRegular))
    {
        return false;
    }

    return true;
}

// 检验系统关键字
bool CTemplateThread::CheckSysKeyword(string strClientID, string &strContent,
                                      string &strKeyWord, string &strSign, string &strKey, bool bIncludeChinese)
{
    string strTempData = "";
    if(strContent.empty() || strSign.empty())
    {
        LogWarn("clientid:%s, strContent[%s] or sign[%s] is null!!", strClientID.c_str(), strContent.c_str(), strSign.c_str());
        return true;
    }
    USER_GW_MAP::iterator itrGW = m_userGwMap.find(strKey);
    if (itrGW == m_userGwMap.end())
    {
        LogWarn("clientid:%s key[%s] is not find in table userGw.", strClientID.c_str(), strKey.c_str());
    }
    else
    {
        //0:check system keyword; 1:don't check system keyword. by liaojialin 20170706
        if (1 == itrGW->second.m_uFreeKeyword)
        {
            LogNotice("clientid:%s, key[%s] is freeKeyword.", strClientID.c_str(), strKey.c_str());
            return true;
        }
    }

    if ( true == m_sysKeywordCheck->keywordCheck(strClientID,
            strContent, strSign, strKeyWord, m_iSysKeywordCovRegular, bIncludeChinese))
    {
        return false;
    }
    return true;
}

// 初始化通道关键字
void CTemplateThread::initChannelKeyWordSearchTree(map<UInt32, list<string> > &mapSetIn, map<UInt32, searchTree *> &mapSetOut)
{
    map<UInt32, searchTree *>::iterator iterChannel = mapSetOut.begin();
    for (; iterChannel != mapSetOut.end();)
    {
        if (NULL != iterChannel->second)
        {
            delete iterChannel->second;
        }

        mapSetOut.erase(iterChannel++);
    }

    for (map<UInt32, list<string> >::iterator iterNew = mapSetIn.begin(); iterNew != mapSetIn.end(); ++iterNew)
    {
        searchTree *pTree = new searchTree();
        CHK_NULL_RETURN(pTree);

        pTree->initTree(iterNew->second);

        mapSetOut.insert(make_pair(iterNew->first, pTree));
    }

}

