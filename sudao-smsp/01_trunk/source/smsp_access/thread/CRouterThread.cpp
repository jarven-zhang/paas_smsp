#include "UTFString.h"
#include "HttpParams.h"
#include "base64.h"
#include "CDomesticRouter.h"
#include "CForeignRouter.h"
#include "CDomesticNumberRouter.h"
#include "CotentFilter.h"
#include "CRouterThread.h"
#include "propertyutils.h"
#include "main.h"


CRouterThread* g_pRouterThread = NULL;


bool startRouterThread()
{
    g_pRouterThread = new CRouterThread("RouterThread");
    INIT_CHK_NULL_RET_FALSE(g_pRouterThread);
    INIT_CHK_FUNC_RET_FALSE(g_pRouterThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pRouterThread->CreateThread());
    return true;
}


CRouterThread::CRouterThread(const char *name) : CThread(name)
{
    m_chlSelect     = NULL;
    m_chlSchedule   = NULL;
    m_pRouter       = NULL;
    m_uAccessId     = 0;

    m_uSelectChannelMaxNum = 3;

    // CHANNELGTOUP_WEIGHT_RATIO default: 100|0
    m_channelGroupWeightRatio.push_back(100);
    m_channelGroupWeightRatio.push_back(0);

    m_pForeignRouter = new CForeignRouter();
    m_pDomesticRouter = new CDomesticRouter();
    m_pDomesticNumberRouter = new CDomesticNumberRouter();
    m_pSnMng = new SnManager();
    m_pOverRate = new CoverRate();

    // REBACK_TIME_OVER default: 86400;20
    m_uOverTime = 86400;
    m_iOverCount = 20;
}

CRouterThread::~CRouterThread()
{
    SAFE_DELETE(m_pDomesticNumberRouter);
    SAFE_DELETE(m_pDomesticRouter);
    SAFE_DELETE(m_pForeignRouter);
    SAFE_DELETE(m_pOverRate);
}

bool CRouterThread::Init()
{
    if (NULL == m_pForeignRouter || NULL == m_pDomesticRouter || NULL == m_pDomesticNumberRouter || NULL == m_pSnMng || NULL == m_pOverRate)
    {
        printf("m_pForeignRouter, m_pDomesticRouter, m_pDomesticNumberRouter, m_pOverRate or m_pSnMng is NULL.");
        return false;
    }

    if (!CThread::Init())
    {
        printf("CThread::Init is failed.\n");
        return false;
    }

    // 获取系统错误代码
    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    // 获取用户信息
    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    // 获取通道模板
    m_mapChannelTemplate.clear();
    g_pRuleLoadThread->getChannelTemplateMap(m_mapChannelTemplate);

    // 获取路由通道信息
    m_mapChannelSegment.clear();
    g_pRuleLoadThread->getChannelSegmentMap(m_mapChannelSegment);

    // 获取通道白关键字信息
    m_mapChannelWhiteKeyword.clear();
    g_pRuleLoadThread->getChannelWhiteKeywordListMap(m_mapChannelWhiteKeyword);

    // 获取
    m_mapClientSegment.clear();
    m_mapClientidSegment.clear();
    g_pRuleLoadThread->getClientSegmentMap(m_mapClientSegment, m_mapClientidSegment);

    m_mapChannelSegCode.clear();
    g_pRuleLoadThread->getChannelSegCodeMap(m_mapChannelSegCode);

    m_pLinkedBlockPool = new LinkedBlockPool();
    m_uAccessId = g_uComponentId;

    m_chlSelect = new ChannelSelect();
    m_chlSelect->initParam();

    m_chlSchedule = new ChannelScheduler();
    m_chlSchedule->init();

    m_KeyWordCheck = new KeyWordCheck();
    m_KeyWordCheck->initParam();

    m_sysKeywordCheck = new sysKeywordClassify("sys");
    m_sysKeywordCheck->initParam();

    g_pRuleLoadThread->getSendIpIdMap(m_mapSendIpId);

    g_pRuleLoadThread->getChannelGroupMap(m_ChannelGroupMap);
    initChannelGroupWeightMap(m_ChannelGroupMap, m_channelGroupsWeight);

    map<UInt32, list<string> > mapChannelKeySet;
    g_pRuleLoadThread->getChannelKeyWordMap(mapChannelKeySet);
    initChannelKeyWordSearchTree(mapChannelKeySet, m_ChannelKeyWordMap);

    g_pRuleLoadThread->getChannelWhiteListMap(m_ChannelWhiteListMap);
    g_pRuleLoadThread->getComponentConfig(m_componentConfigInfoMap);

    // t_sms_channel_login_status
    m_setLoginChannels.clear();
    g_pRuleLoadThread->getLoginChannels(m_setLoginChannels);

    m_userGwMap.clear();
    g_pRuleLoadThread->getUserGwMap(m_userGwMap);

    STL_MAP_STR sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    // 号段->号段信息
    m_mapPhoneSection.clear();
    g_pRuleLoadThread->getPhoneAreaMap(m_mapPhoneSection);

    return true;

}

// 通道关键字校验
bool CRouterThread::CheckRegex(UInt32 uChannelId, string &strData, string &strOut, string strSign)	//strOut: matched words
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

    if (strTempData.empty())
    {
        LogWarn("CheckRegex() failed.  strContent[%s] is null", strTempData.data());
        return false;
    }

    map<UInt32, searchTree *>::iterator itrKey = m_ChannelKeyWordMap.find(uChannelId);
    if (m_ChannelKeyWordMap.end() != itrKey)
    {
        searchTree *pTree = itrKey->second;

        Comm::ToLower(strSignData);
        if (true == pTree->searchSign(strSignData, strOut))
        {
            return true;
        }
        Comm::ToLower(strContentTemp);
        if (true == pTree->search(strContentTemp, strOut))
        {
            return true;
        }
    }

    return false;
}

// 校验通道是否 超频 true 超频了，false 没有超频
bool CRouterThread::checkChannelOverrate( TSmsRouterReq *pInfo, Channel smsChannel )
{
    LogDebug("[%s:%s:%s] smsChannel:%d, content: %s, sign: %s, m_bInOverRateWhiteList:%d",
             pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
             smsChannel.channelID, pInfo->m_strContent.c_str(), pInfo->m_strSign.c_str(), pInfo->m_bInOverRateWhiteList);

    if (true != pInfo->m_bInOverRateWhiteList)
    {
        /* 查看内存是否存在超频记录*/
        // user which key
        int keyType = -1;
        models::TempRule ChannelTempRule;
        char strChannelid[128] = { 0 };
        sprintf(strChannelid, "%d", smsChannel.channelID);
        m_chlSelect->getChannelOverRateRuleAndKey(strChannelid, pInfo, ChannelTempRule, keyType);
        if (keyType != -1)//-1: 什么也没找到
        {
            if (m_pOverRate->isChannelOverRate(pInfo->m_strChannelOverRateKey, ChannelTempRule))
            {
                LogWarn("[%s:%s:%s] channelid[%d] is channel OverRate [%s].",
                        pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                        smsChannel.channelID, pInfo->m_strChannelOverRateKey.c_str());
                return true;
            }
        }
    }
    else
    {
        LogDebug("[%s:%s:%s] clientid is in t_sms_overrate_white_list",
                 pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
    }
    return false;
}

// 校验通道是否流控 true 流控了，false 没有流控
bool CRouterThread::CheckOverFlow(TSmsRouterReq *pInfo, Channel smsChannel)
{
    map<UInt32, channelFlow>::iterator itr = m_ChannelFlowInfo.find(smsChannel.channelID);
    if (itr == m_ChannelFlowInfo.end())
    {
        channelFlow flowInfo;
        flowInfo.m_uTime = time(NULL);
        flowInfo.m_uNum = 1;

        m_ChannelFlowInfo.insert(make_pair(smsChannel.channelID, flowInfo));

        return false;
    }
    else
    {
        UInt64 uTime = time(NULL);

        if (itr->second.m_uTime != uTime)
        {
            itr->second.m_uTime = uTime;
            itr->second.m_uNum = 1;
            return false;
        }
        else
        {
            if (itr->second.m_uNum < smsChannel.m_uAccessSpeed)
            {
                itr->second.m_uNum += 1;
                return false;
            }
            else
            {
                itr->second.m_uNum += 1;
                return true;
            }
        }
    }
}

// 获取通道登录状态
bool CRouterThread::GetChannelLoginStatus(Int32 channelid)
{
    return m_setLoginChannels.find(channelid) != m_setLoginChannels.end();
}

std::map<UInt32, UInt32> &CRouterThread::GetChannelQueueSizeMap()
{
    return m_mapGetChannelMqSize;
}

std::map<std::string, PhoneSection> &CRouterThread::GetPhoneSectionMap()
{
    return m_mapPhoneSection;
}


// 获取系统参数
void CRouterThread::GetSysPara(const STL_MAP_STR &mapSysPara)
{
    string strSysPara;
    STL_MAP_STR::const_iterator iter;

    do
    {
        strSysPara = "SELECT_CHANNEL_NUM";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;
        std::string::size_type pos = strTmp.find_last_of(";");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                     strSysPara.c_str(), strTmp.c_str());
            break;
        }

        int nTmp1 = atoi(strTmp.substr(0, pos).c_str());
        int nTmp2 = atoi(strTmp.substr(pos + 1).c_str());
        if (0 > nTmp1 || 0 > nTmp2)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                     strSysPara.c_str(), strTmp.c_str());
            break;
        }
        m_uSelectChannelMaxNum = nTmp1;
    }
    while (0);
    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uSelectChannelMaxNum);

    do
    {
        strSysPara = "CHANNELGTOUP_WEIGHT_RATIO";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;

        std::size_t pos = strTmp.find_last_of("|");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                     strSysPara.c_str(), strTmp.c_str());
            break;
        }

        int nTmp1 = atoi(strTmp.substr(0, pos).data());
        int nTmp2 = atoi(strTmp.substr(pos + 1).data());

        if ((0 > nTmp1) || (0 > nTmp2))
        {
            LogError("Invalid system parameter(%s) value(%s, %d, %d).",
                     strSysPara.c_str(),
                     strTmp.c_str(),
                     nTmp1,
                     nTmp2);
            break;
        }

        m_channelGroupWeightRatio.clear();
        m_channelGroupWeightRatio.push_back(nTmp1);
        m_channelGroupWeightRatio.push_back(nTmp2);
    }
    while (0);

    LogNotice("System parameter(%s) value size(%u).",
              strSysPara.c_str(), m_channelGroupWeightRatio.size());

    do
    {
        strSysPara = "REBACK_TIME_OVER";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;

        std::size_t pos = strTmp.find_last_of(";");
        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                     strSysPara.c_str(), strTmp.c_str());
            break;
        }

        UInt64 iOverTime = atoi(strTmp.substr(0, pos).data());
        int iOverCount = atoi(strTmp.substr(pos + 1).data());

        if ((0 > iOverTime) || (0 > iOverCount))
        {
            LogError("Invalid system parameter(%s) value(%s, %u, %d).",
                     strSysPara.c_str(),
                     strTmp.c_str(),
                     iOverTime,
                     iOverCount);
            break;
        }

        m_uOverTime = iOverTime;
        m_iOverCount = iOverCount;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u, %d).",
              strSysPara.c_str(),
              m_uOverTime,
              m_iOverCount);

}

// 初始化通道关键字列表
void CRouterThread::initChannelKeyWordSearchTree(map<UInt32, list<string> > &mapSetIn, map<UInt32, searchTree *> &mapSetOut)
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
        if (NULL == pTree)
        {
            LogFatal("pTree is NULL.");
            return;
        }
        pTree->initTree(iterNew->second);
        mapSetOut.insert(make_pair(iterNew->first, pTree));
    }
}

void CRouterThread::MainLoop()
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

            if(pMsg == NULL)
            {
                usleep(g_uSecSleep);
            }
            else if (NULL != pMsg)
            {
                HandleMsg(pMsg);
                SAFE_DELETE( pMsg );
            }
        }
    }
}

void CRouterThread::HandleMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CAdapter::InterlockedIncrement((long *)&m_iCount);

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_SEESION_TO_ROUTER_REQ:
        {
            HandleSeesionSendRouterMsg( pMsg );
            break;
        }
        case MSGTYPE_GET_CHANNEL_MQ_SIZE_REQ:
        {
            CGetChannelMqSizeReqMsg *pReq = (CGetChannelMqSizeReqMsg *)pMsg;
            if (NULL != pReq)
            {
                m_mapGetChannelMqSize.clear();
                m_mapGetChannelMqSize = pReq->m_mapGetChannelMqSize;
                LogNotice("update CGetChannelMqSizeReqMsg size:%d.", m_mapGetChannelMqSize.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
        {
            TUpdateSystemErrorCodeReq *pReq = (TUpdateSystemErrorCodeReq *)pMsg;
            if (NULL != pReq)
            {
                m_mapSystemErrorCode.clear();
                m_mapSystemErrorCode = pReq->m_mapSystemErrorCode;
                LogNotice("update t_sms_system_error_desc size:%d.", m_mapSystemErrorCode.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            TUpdateSmsAccontReq *pReq = (TUpdateSmsAccontReq *)pMsg;
            if (NULL != pReq)
            {
                m_AccountMap.clear();
                m_AccountMap = pReq->m_SmsAccountMap;
                LogNotice("RuleUpdate account update!");
            }
            break;
        }

        /***********************************system keyword update begin****************************************/
        case MSGTYPE_RULELOAD_SYS_KEYWORDLIST_UPDATE_REQ:
        {
            TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
            if (NULL != pReq && NULL != m_sysKeywordCheck)
            {
                m_sysKeywordCheck->initKeywordSearchTree(pReq->m_keyWordMap);
                LogNotice("RuleUpdate system keyword list update. ");
            }
            break;
        }
        case MSGTYPE_RULELOAD_SYS_CGROUPREFCLIENT_UPDATE_REQ:
        {
            TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
            if (NULL != pReq && NULL != m_sysKeywordCheck)
            {
                m_sysKeywordCheck->setClientGrpRefClient(pReq->m_clientGrpRefClientMap);
                LogNotice("RuleUpdate sysCGroupRefClient update. size[%d]", pReq->m_clientGrpRefClientMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SYS_CLIENTGROUP_UPDATE_REQ:
        {
            TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
            if (NULL != pReq && NULL != m_sysKeywordCheck)
            {
                m_sysKeywordCheck->setCgrpRefKeywordGrp(pReq->m_clientGrpRefKeywordGrpMap, pReq->m_uDefaultGroupId);
                LogNotice("RuleUpdate sysClientGroup update. size[%d]", pReq->m_clientGrpRefKeywordGrpMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SYS_KGROUPREFCATEGORY_UPDATE_REQ:
        {
            TUpdateKeyWordReq *pReq = (TUpdateKeyWordReq *)pMsg;
            if (NULL != pReq && NULL != m_sysKeywordCheck)
            {
                m_sysKeywordCheck->setkeyGrpRefCategory(pReq->m_keywordGrpRefCategoryMap);
                LogNotice("RuleUpdate sysKGroupRefCategory update. size[%d]", pReq->m_keywordGrpRefCategoryMap.size());
            }

            break;
        }
        /***********************************system keyword update end****************************************/

        case MSGTYPE_RULELOAD_CHANNELGROUP_UPDATE_REQ:
        {
            TUpdateChannelGroupReq *pReq = (TUpdateChannelGroupReq *)pMsg;
            if (NULL != pReq)
            {
                m_ChannelGroupMap.clear();
                m_ChannelGroupMap = pReq->m_ChannelGroupMap;
                initChannelGroupWeightMap(m_ChannelGroupMap, m_channelGroupsWeight);
                LogNotice("RuleUpdate ChannelGroupMap update. map.size[%d]", pReq->m_ChannelGroupMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            TUpdateChannelReq *pReq = (TUpdateChannelReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSchedule->m_ChannelMap.clear();
                m_chlSchedule->m_ChannelMap = pReq->m_ChannelMap;
                LogNotice("RuleUpdate channelMap update. map.size[%d]", pReq->m_ChannelMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            TUpdateUserGwReq *pReq = (TUpdateUserGwReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSelect->m_userGwMap.clear();
                m_chlSelect->m_userGwMap = pReq->m_UserGwMap;
                m_userGwMap.clear();
                m_userGwMap = pReq->m_UserGwMap;
                LogNotice("RuleUpdate userGwMap update. map.size[%d]", pReq->m_UserGwMap.size());
            }
            break;
        }
        case  MSGTYPE_RULELOAD_CHANNELKEYWORD_UPDATE_REQ:
        {
            TUpdateChannelKeyWordReq *pReq = (TUpdateChannelKeyWordReq *)pMsg;
            if (NULL != pReq)
            {
                initChannelKeyWordSearchTree(pReq->m_ChannelKeyWordMap, m_ChannelKeyWordMap);
                LogNotice("RuleUpdate CHANNELKEYWORD update. ");
            }
            break;
        }
        case  MSGTYPE_RULELOAD_CHANNELWHITELIST_UPDATE_REQ:
        {
            TUpdateChannelWhiteListReq *pReq = (TUpdateChannelWhiteListReq *)pMsg;
            if (NULL != pReq)
            {
                m_ChannelWhiteListMap = pReq->m_ChannelWhiteListMap;
                LogNotice("RuleUpdate CHANNELWHITELIST update.");
            }
            break;
        }
        case MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ:
        {
            TUpdateSignextnoGwReq *pReq = (TUpdateSignextnoGwReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSelect->m_SignextnoGwMap.clear();
                m_chlSelect->m_SignextnoGwMap = pReq->m_SignextnoGwMap;
                LogNotice("RuleUpdate SignExtnoGwMap update. map.size[%d]", pReq->m_SignextnoGwMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SMPPPRICE_UPDATE_REQ:
        {
            TUpdateSmppPriceReq *pReq = (TUpdateSmppPriceReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSelect->m_PriceMap.clear();
                m_chlSelect->m_PriceMap = pReq->m_PriceMap;
                LogNotice("RuleUpdate SmppPrice update. map.size[%d]", pReq->m_PriceMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SMPPSALEPRICE_UPDATE_REQ:
        {
            TUpdateSmppSalePriceReq *pReq = (TUpdateSmppSalePriceReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSelect->m_SalePriceMap.clear();
                m_chlSelect->m_SalePriceMap = pReq->m_salePriceMap;
                LogNotice("RuleUpdate SmppSalePrice update. map.size[%d]", pReq->m_salePriceMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNELOVERRATE_UPDATE_REQ://channel overrate
        {
            TUpdateChannelOverrateReq *pReq = (TUpdateChannelOverrateReq *)pMsg;
            if (NULL != pReq)
            {
                m_chlSelect->m_ChannelOverrateMap.clear();
                m_chlSelect->m_ChannelOverrateMap = pReq->m_ChannelOverrateMap;
                LogNotice("RuleUpdate channel overrate update. map.size[%d]", pReq->m_ChannelOverrateMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq *pReq = (TUpdateSysParamRuleReq *)pMsg;
            if (NULL != pReq)
            {
                GetSysPara(pReq->m_SysParamMap);
                //modify overRate config
                m_chlSelect->ParseGlobalOverRateSysPara(pReq->m_SysParamMap);
            }
            break;
        }
        case MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ:
        {
            TUpdatePhoneAreaReq *pReq = (TUpdatePhoneAreaReq *)pMsg;
            if (pReq)
            {
                m_chlSelect->m_PhoneAreaMap.clear();
                m_chlSelect->m_PhoneAreaMap = pReq->m_PhoneAreaMap;	//手机号段信息
                LogNotice("RuleUpdate PhoneArea update. map.size[%d]", pReq->m_PhoneAreaMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ:
        {
            TUpdateChannelExtendPortReq *pReq = (TUpdateChannelExtendPortReq *)pMsg;
            if (pReq)
            {
                m_chlSelect->m_ChannelExtendPortTable.clear();
                m_chlSelect->m_ChannelExtendPortTable = pReq->m_ChannelExtendPortTable;
                LogNotice("RuleUpdate channel_extendport update. map.size[%d]", pReq->m_ChannelExtendPortTable.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNELTEMPLATE_UPDATE_REQ:
        {
            TUpdateChannelTemplateReq *pReq = (TUpdateChannelTemplateReq *)pMsg;
            if (pReq)
            {
                m_mapChannelTemplate.clear();
                m_mapChannelTemplate = pReq->m_mapChannelTemplate;
                LogNotice("RuleUpdate Channel Template update.. map.size[%d]", pReq->m_mapChannelTemplate.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            //m_componentConfigInfoMap
            TUpdateComponentConfigReq *pReq = (TUpdateComponentConfigReq *)(pMsg);
            if (pReq)
            {
                m_componentConfigInfoMap.clear();
                m_componentConfigInfoMap = pReq->m_componentConfigInfoMap;
                LogNotice("update t_sms_component size:%d", m_componentConfigInfoMap.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_SEGMENT_UPDATE_REQ:
        {
            TUpdateChannelSegmentReq *pReq = (TUpdateChannelSegmentReq *)(pMsg);
            if (pReq)
            {
                m_mapChannelSegment.clear();
                m_mapChannelSegment = pReq->m_mapChannelSegment;
                LogNotice("update t_sms_segment_channel map size:%d", m_mapChannelSegment.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_WHITEKEYWORD_UPDATE_REQ:
        {
            TUpdateChannelWhiteKeywordReq *pReq = (TUpdateChannelWhiteKeywordReq *)(pMsg);
            if (pReq)
            {
                m_mapChannelWhiteKeyword.clear();
                m_mapChannelWhiteKeyword = pReq->m_mapChannelWhiteKeyword;
                LogNotice("update t_sms_white_keyword_channel map size:%d", m_mapChannelWhiteKeyword.size());
            }
            break;
        }
        case MSGTYPE_RULELOAD_CLIENT_SEGMENT_UPDATE_REQ:
        {
            TUpdateClientSegmentReq *pReq = (TUpdateClientSegmentReq *)(pMsg);
            if (pReq)
            {
                m_mapClientSegment.clear();
                m_mapClientidSegment.clear();
                m_mapClientSegment = pReq->m_mapClientSegment;
                m_mapClientidSegment = pReq->m_mapClientidSegment;
                LogNotice("update t_sms_segment_client size:%d", m_mapClientSegment.size());
            }
            break;

        }
        case MSGTYPE_RULELOAD_CHANNEL_SEG_CODE_UPDATE_REQ:
        {
            TUpdateChannelSegCodeReq *pReq = (TUpdateChannelSegCodeReq *)(pMsg);
            if (pReq)
            {
                m_mapChannelSegCode.clear();
                m_mapChannelSegCode = pReq->m_mapChannelSegCode;
                LogNotice("update t_sms_segcode_channel size:%d", m_mapChannelSegCode.size());
            }
            break;

        }
        // 更新通道登录状态
        case MSGTYPE_RULELOAD_CHANNEL_LOGIN_UPDATE_REQ:
        {
            TUpdateLoginChannelsReq *pReq = (TUpdateLoginChannelsReq *)pMsg;
            if (pReq)
            {
                m_setLoginChannels.clear();
                m_setLoginChannels = pReq->m_setLoginChannels;
            }
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_REALTIME_WEIGHTINFO_REQ:
        {
            TUpdateChannelRealtimeWeightInfo *pReq = (TUpdateChannelRealtimeWeightInfo *) pMsg;
            if (pReq)
            {
                m_channelsRealtimeWeightInfo = pReq->m_channelsRealtimeWeightInfo;
                LogNotice("update t_sms_channel_attribute_realtime_weight");
            }
            break;
        }
        // 通道权重属性配置
        case MSGTYPE_RULELOAD_CHANNEL_ATTRIBUTE_WEIGHT_CONFIG_REQ:
        {
            TUpdateChannelAttrWeightConfig *pReq = (TUpdateChannelAttrWeightConfig *) pMsg;
            if (pReq)
            {
                m_channelAttrWeightConfig = pReq->m_channelAttrWeightConfig;
                LogNotice("update t_sms_channel_attribute_weight_config");
            }
            break;
        }
        // 通道策略池
        case MSGTYPE_RULELOAD_CHANNEL_POOL_POLICY_REQ:
        {
            TUpdateChannelPoolPolicyConfig *pReq = (TUpdateChannelPoolPolicyConfig *) pMsg;
            if (pReq)
            {
                m_channelPoolPolicies = pReq->m_channelPoolPolicies;
                LogNotice("update t_sms_channel_pool_policy");
            }
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOut(pMsg);
            break;
        }
        default:
        {
            LogWarn("msgType[0x:%x] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}

void CRouterThread::HandleSeesionSendRouterMsg(TMsg *pMsg)
{
    TSmsRouterReq *pRouterReq = (TSmsRouterReq *)pMsg;
    CHK_NULL_RETURN(pRouterReq);

    TSmsRouterResp *pRouterResp = new TSmsRouterResp();
    CHK_NULL_RETURN(pRouterResp);

    pRouterResp->m_iSeq = pRouterReq->m_iSeq;
    pRouterResp->m_strSessionID = pRouterReq->m_strSessionID;
    pRouterResp->m_pSender = pRouterReq->m_pSender;

    // 如果存在测试通道ID直接走测试通道流程 否则走正常流程
    if (!pRouterReq->m_strTestChannelId.empty())
    {
        smsGetTestChannelRouter(pRouterReq, pRouterResp);
    }
    else
    {
        smsGetChannelRouter(pRouterReq, pRouterResp);
    }

    proSendRouterResp(pRouterResp);
}

void CRouterThread::proSendRouterResp(TSmsRouterResp* pSmsRouterResp, UInt64 llSeq)
{
    if (pSmsRouterResp)
    {
        pSmsRouterResp->m_iMsgType = MSGTYPE_ROUTER_RESP_TO_SEESION;
        pSmsRouterResp->m_pSender->PostMsg( pSmsRouterResp );
    }
}

// 获取测试通道路由
void CRouterThread::smsGetTestChannelRouter(TSmsRouterReq *pInfo, TSmsRouterResp *pRouterResp)
{
    Int32 channel_get_ret = -1;     // 通道返回状态码

    CHK_NULL_RETURN(pInfo);
    CHK_NULL_RETURN(pRouterResp);

    models::Channel smsChannel;

    // 国内路由
    m_pRouter = m_pDomesticRouter;
    if (NULL == m_pRouter)
    {
        LogWarn("[%s:%s:%s] m_pRouter is NULL",
                pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
        channel_get_ret = CHANNEL_GET_FAILURE_OTHER;
        goto CHECK_RET;
    }

    pInfo->m_uChannleId = atoi(pInfo->m_strTestChannelId.c_str());

    if ((false == m_pRouter->Init(this, pInfo)))
    {
        LogWarn("[%s:%s:%s] TestRoute Init failure", pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
        channel_get_ret = CHANNEL_GET_FAILURE_OTHER;
        goto CHECK_RET;
    }

    channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_TEST);

CHECK_RET:
    pRouterResp->m_fCostFee = pInfo->m_fCostFee;
    pRouterResp->m_dSaleFee = pInfo->m_dSaleFee;
    pRouterResp->m_uSelectChannelNum = pInfo->m_uSelectChannelNum;
    pRouterResp->m_strUcpaasPort = pInfo->m_strUcpaasPort;
    pRouterResp->m_strExtpendPort = pInfo->m_strExtpendPort;
    pRouterResp->m_strSignPort = pInfo->m_strSignPort;
    pRouterResp->nRouterResult = channel_get_ret;
    switch (channel_get_ret)
    {
    /* 通道选择成功 */
    case CHANNEL_GET_SUCCESS:
    {
        pRouterResp->m_uChannleId = smsChannel.channelID;
        break;
    }
    /* 没有找到通道组 */
    case CHANNEL_GET_FAILURE_NO_CHANNELGROUP:
    {
        pRouterResp->m_strErrorCode = NO_CHANNEL_GROUPS;
        pRouterResp->m_strYZXErrCode = NO_CHANNEL_GROUPS;
        break;
    }
    /* 流速限制 */
    case CHANNEL_GET_FAILURE_LOGINSTATUS:
    case CHANNEL_GET_FAILURE_OVERFLOW:
    {
        pRouterResp->m_uSelectChannelNum++;
        LogNotice("[%s:%s:%s] selectchannelnum:%d,is overflower or channel login fail",
                  pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                  pRouterResp->m_uSelectChannelNum);

        if ( pInfo->m_uSelectChannelNum >= m_uSelectChannelMaxNum )
        {
            LogError("[%s:%s:%s] selectChannelNum[%d] is over SelectChannelMaxNum.",
                     pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                     pRouterResp->m_uSelectChannelNum);

            pRouterResp->m_uChannleId = smsChannel.channelID;
            pRouterResp->m_strErrorCode = OVER_SELECT_CHANNEL_MAX_NUM;
            pRouterResp->m_strYZXErrCode = OVER_SELECT_CHANNEL_MAX_NUM;
        }
        break;
    }
    default:
    {
        pRouterResp->m_uChannleId = smsChannel.channelID;
        pRouterResp->m_strErrorCode = NO_CHANNEL;
        pRouterResp->m_strYZXErrCode = NO_CHANNEL;
    }
    }

    if (CHANNEL_GET_SUCCESS != channel_get_ret)
    {
        LogError("[%s:%s:%s] getChannel failed!", pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
    }

    LogDebug("[%s:%s:%s] get channel complete,channelName[%s],channelId[%d].",
             pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
             smsChannel.channelname.data(), smsChannel.channelID);
}

bool CRouterThread::CheckRebackSendTimes(TSmsRouterReq *pInfo)
{
    do
    {
        if(pInfo->m_iOverCount >= m_iOverCount)
        {
            LogWarn("[%s:%s:%s] reback times[%d] arrived setting reback times[%d], Stop sending!",
                    pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                    pInfo->m_iOverCount,
                    m_iOverCount);

            break;
        }

        long now = time(NULL);
        long iDiff = now - (long)pInfo->m_uOverTime;
        if((iDiff > 0) && (iDiff >= (long)m_uOverTime))
        {
            LogWarn("[%s:%s:%s] time difference [%d - %d] = [%d] arrived setting time difference [%d], send fail!",
                    pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                    now,
                    pInfo->m_uOverTime,
                    iDiff,
                    m_uOverTime);

            break;
        }

        return true;
    }
    while (0);

    return false;
}

// 获取通道路由
void CRouterThread::smsGetChannelRouter(TSmsRouterReq *pInfo, TSmsRouterResp *pRouterResp)
{
    Int32 channel_get_ret = -1;     // 通道路由返回结果

    CHK_NULL_RETURN(pInfo);
    CHK_NULL_RETURN(pRouterResp);

    models::Channel smsChannel;
   
    //get free_channel_keyword
    GetFreeChannelKeyword(pInfo);

    if (BusType::FOREIGN == pInfo->m_uOperater)
    {
        m_pRouter = m_pForeignRouter;   // 国际路由
        m_pRouter->m_uRouterType = 1;
    }
    else
    {
        m_pRouter = m_pDomesticNumberRouter;    // 国内先走号码强制路由
    }

    if (NULL == m_pRouter)
    {
        LogWarn("[%s:%s:%s] m_pRouter is NULL",
            pInfo->m_strClientId.c_str(),
            pInfo->m_strSmsId.c_str(),
            pInfo->m_strPhone.c_str());

        channel_get_ret = CHANNEL_GET_FAILURE_OTHER;
    }
    else
    {
        if (!m_pRouter->Init(this, pInfo))
        {
            LogWarn("[%s:%s:%s] Init %s failure",
                pInfo->m_strClientId.c_str(),
                pInfo->m_strSmsId.c_str(),
                pInfo->m_strPhone.c_str(),
                (BusType::FOREIGN == pInfo->m_uOperater) ? "m_pForeignRouter" : "m_pDomesticNumberRouter");

            channel_get_ret = CHANNEL_GET_FAILURE_OTHER;
        }
        else
        {
            if (pInfo->m_iClientSendLimitCtrlTypeFlag > NOT_CONTROL)
            {
                channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_SEND_LIMIT);

                LogDebug("[%s:%s:%s] STRATEGY_SEND_LIMIT, channel_get_ret=%d",
                    pInfo->m_strClientId.c_str(),
                    pInfo->m_strSmsId.c_str(),
                    pInfo->m_strPhone.c_str(),
                    channel_get_ret);
            }

            if (pInfo->m_uFromSmsReback && pInfo->m_iFailedResendTimes > 0)
            {
                channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_RESEND);

                LogDebug("[%s:%s:%s] STRATEGY_RESEND, channel_get_ret=%d",
                    pInfo->m_strClientId.c_str(),
                    pInfo->m_strSmsId.c_str(),
                    pInfo->m_strPhone.c_str(),
                    channel_get_ret);
            }

            if (channel_get_ret != CHANNEL_GET_SUCCESS)
            {
                if (CHANNEL_GET_SUCCESS != (channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_FORCE)))
                {
                    LogDebug("[%s:%s:%s] STRATEGY_FORCE %s route fail!!!",
                        pInfo->m_strClientId.c_str(),
                        pInfo->m_strSmsId.c_str(),
                        pInfo->m_strPhone.c_str(),
                        (BusType::FOREIGN == pInfo->m_uOperater) ? "foreign section" : "domestic number");

                    if (BusType::FOREIGN == pInfo->m_uOperater)  // 国际
                    {
                        //失败后再尝试白关键字路由
                        if (CHANNEL_GET_SUCCESS != (channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_WHITE_KEYWORD)))
                        {
                            LogDebug("[%s:%s:%s] WhiteKeyword  section route fail!!!",
                                pInfo->m_strClientId.c_str(),
                                pInfo->m_strSmsId.c_str(),
                                pInfo->m_strPhone.c_str());

                            // 失败后走智能路由
                            channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_INTELLIGENCE);
                        }
                    }
                    else// 国内
                    {
                        // 失败先走号段强制路由
                        m_pRouter = m_pDomesticRouter;

                        if ((!m_pRouter->Init(this, pInfo)))
                        {
                            LogWarn("[%s:%s:%s] Init m_pDomesticRouter failure",
                                pInfo->m_strClientId.c_str(),
                                pInfo->m_strSmsId.c_str(),
                                pInfo->m_strPhone.c_str());

                            channel_get_ret = CHANNEL_GET_FAILURE_OTHER;
                        }
                        else
                        {
                            if (CHANNEL_GET_SUCCESS != (channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_FORCE)))
                            {
                                LogDebug("[%s:%s:%s] STRATEGY_FORCE domestic section route fail!!!",
                                    pInfo->m_strClientId.c_str(),
                                    pInfo->m_strSmsId.c_str(),
                                    pInfo->m_strPhone.c_str());

                                if (CHANNEL_GET_SUCCESS != (channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_WHITE_KEYWORD)))
                                {
                                    LogDebug("[%s:%s:%s] WhiteKeyword  section route fail!!",
                                        pInfo->m_strClientId.c_str(),
                                        pInfo->m_strSmsId.c_str(),
                                        pInfo->m_strPhone.c_str());

                                    // 再次失败就走智能路由
                                    channel_get_ret = m_pRouter->GetRoute(smsChannel, STRATEGY_INTELLIGENCE);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    pRouterResp->m_fCostFee = pInfo->m_fCostFee;
    pRouterResp->m_dSaleFee = pInfo->m_dSaleFee;
    pRouterResp->m_uSelectChannelNum = pInfo->m_uSelectChannelNum;
    pRouterResp->m_strUcpaasPort = pInfo->m_strUcpaasPort;
    pRouterResp->m_strExtpendPort = pInfo->m_strExtpendPort;
    pRouterResp->m_strSignPort = pInfo->m_strSignPort;
    pRouterResp->nRouterResult = channel_get_ret;
    switch (channel_get_ret)
    {
    /* 通道选择成功 */
    case CHANNEL_GET_SUCCESS:
    {
        pRouterResp->m_uChannleId = smsChannel.channelID;
        break;
    }
    /* 没有找到通道组 */
    case CHANNEL_GET_FAILURE_NO_CHANNELGROUP:
    {
        pRouterResp->m_uChannleId = smsChannel.channelID;
        pRouterResp->m_strErrorCode = NO_CHANNEL_GROUPS;
        pRouterResp->m_strYZXErrCode = NO_CHANNEL_GROUPS;
        break;
    }
    /* 流速限制 */
    case CHANNEL_GET_FAILURE_LOGINSTATUS:
    case CHANNEL_GET_FAILURE_OVERFLOW:
    {
        if (pInfo->m_uFromSmsReback == 1)
        {
            if (!CheckRebackSendTimes(pInfo))
            {
                LogElk("[%s:%s:%s] reback over time or over count",
                       pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
            }
            else
            {
                pRouterResp->m_uIsSendToReback = 1;
            }
            pRouterResp->m_uChannleId = smsChannel.channelID;
        }
        else if (pInfo->m_uFromSmsReback == 0)
        {
            pRouterResp->m_uSelectChannelNum++;
            LogNotice("[%s:%s:%s] selectchannelnum:%d,is overflower or channel login fail",
                      pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                      pRouterResp->m_uSelectChannelNum);

            if ( pInfo->m_uSelectChannelNum >= m_uSelectChannelMaxNum )
            {
                LogElk("[%s:%s:%s] selectChannelNum[%d] is over SelectChannelMaxNum.",
                       pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                       pRouterResp->m_uSelectChannelNum);

                pRouterResp->m_uChannleId = smsChannel.channelID;
                pRouterResp->m_strErrorCode = OVER_SELECT_CHANNEL_MAX_NUM;
                pRouterResp->m_strYZXErrCode = OVER_SELECT_CHANNEL_MAX_NUM;
            }
        }
        else
        {
            LogElk("[%s:%s:%s] pInfo->m_uFromSmsReback[%u] value is error",
                   pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                   pInfo->m_uFromSmsReback);
            pRouterResp->m_uChannleId = smsChannel.channelID;
            pRouterResp->m_strErrorCode = OVER_SELECT_CHANNEL_MAX_NUM;
            pRouterResp->m_strYZXErrCode = OVER_SELECT_CHANNEL_MAX_NUM;
        }

        break;
    }
    default:
    {
        pRouterResp->m_uChannleId = smsChannel.channelID;
        pRouterResp->m_strErrorCode = NO_CHANNEL;
        pRouterResp->m_strYZXErrCode = NO_CHANNEL;
    }
    }

    if (CHANNEL_GET_SUCCESS != channel_get_ret)
    {
        LogWarn("[%s:%s:%s] getChannel failed!", pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str());
    }

    LogDebug("[%s:%s:%s] get channel complete,channelName[%s],channelId[%d].",
             pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
             smsChannel.channelname.c_str(), smsChannel.channelID);
}

bool CRouterThread::GetReSendChannelGroup(std::string strKey, std::vector<std::string> &vectorChannelGroups)
{
    std::map<std::string, models::UserGw>::iterator iter = m_chlSelect->m_userGwMap.find(strKey);

    if(iter != m_chlSelect->m_userGwMap.end())
    {
        Comm::splitExVectorSkipEmptyString(iter->second.m_strFailedReSendChannelID, ",", vectorChannelGroups);
    }

    return vectorChannelGroups.size();
}

//what to do with timeout, alarm or record it??
void CRouterThread::HandleTimeOut(TMsg *pMsg)
{
    return;
}

UInt32 CRouterThread::GetSessionMapSize()
{
    return 0;
}

/* 获取通道面审关键字开关*/
UInt32 CRouterThread::GetFreeChannelKeyword(TSmsRouterReq *pInfo)
{
    if (!pInfo)
    {
        LogError("pInfo is NULL");
        return 0;
    }

    string strKey = pInfo->m_strClientId + "_" + pInfo->m_strSmsType;
    std::map<std::string, models::UserGw>::iterator itrGW = m_userGwMap.find(strKey);
    if (itrGW == m_userGwMap.end())
    {
        LogWarn("[%s:%s:%s] key[%s] is not find in table userGw.",
                pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                strKey.c_str());
    }
    else
    {
        //0:check channel keyword; 1:don't check channel keyword. by liaojialin 20170804
        pInfo->m_uFreeChannelKeyword = itrGW->second.m_uFreeChannelKeyword;
        LogDebug("[%s:%s:%s] key[%s] is freeKeyword[%d].",
                 pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                 strKey.c_str(), pInfo->m_uFreeChannelKeyword);
        return pInfo->m_uFreeChannelKeyword;
    }
    return 0;
}

ChannelSelect *CRouterThread::GetChannelSelect()
{
    return m_chlSelect;
}

ChannelScheduler *CRouterThread::GetChannelScheduler()
{
    return m_chlSchedule;
}

std::map<UInt32, UInt64Set>  &CRouterThread::GetChannelWhiteListMap()
{
    return m_ChannelWhiteListMap;
}

ChannelWhiteKeywordMap  &CRouterThread::GetChannelWhiteKeywordListMap()
{
    return m_mapChannelWhiteKeyword;
}

STL_MAP_STR &CRouterThread::GetChannelTemplateMap()
{
    return m_mapChannelTemplate;
}

stl_map_str_ChannelSegmentList &CRouterThread::GetChannelSegmentMap()
{
    return m_mapChannelSegment;
}

std::map<std::string, stl_list_str> &CRouterThread::GetClientSegmentMap()
{
    return m_mapClientSegment;
}
std::map<std::string, set<std::string> > &CRouterThread::GetClientidSegmentMap()
{
    return m_mapClientidSegment;
}

std::map<UInt32, stl_list_int32> &CRouterThread::GetChannelSegCodeMap()
{
    return m_mapChannelSegCode;
}

std::map<string, SmsAccount> &CRouterThread::GetAccountMap()
{
    return m_AccountMap;
}

map<UInt32, ChannelGroup> &CRouterThread::GetChannelGroupMap()
{
    return m_ChannelGroupMap;
}

/* >>>>>>>>>>>>>>>>>>>>>>>>> 通道组权重 BEGIN <<<<<<<<<<<<<<<<<<<<<<<<< */
channel_attribute_weight_config_t &CRouterThread::GetChannelAttributeWeightConfig()
{
    return m_channelAttrWeightConfig;
}

channels_realtime_weightinfo_t &CRouterThread::GetChannelsRealtimeWeightInfo()
{
    return m_channelsRealtimeWeightInfo;
}

channelgroups_weight_t &CRouterThread::GetChannelGroupsWeightMap()
{
    return m_channelGroupsWeight;
}

channel_pool_policies_t &CRouterThread::GetChannelPoolPolicyConf()
{
    return m_channelPoolPolicies;
}

std::vector<UInt32> &CRouterThread::GetChannelGroupWeightRatio()
{
    return m_channelGroupWeightRatio;
}

void CRouterThread::initChannelGroupWeightMap(map<UInt32, ChannelGroup> &channelGroupMap, channelgroups_weight_t &channelGroupsWeight)
{
    channelGroupsWeight.clear();

    for (map<UInt32, ChannelGroup>::iterator cgIter = channelGroupMap.begin();
            cgIter != channelGroupMap.end();
            ++cgIter)
    {
        UInt32 ChlGrpID = cgIter->first;
        const ChannelGroup &cg = cgIter->second;
        ChannelGroupWeight cgw(ChlGrpID);

        for (UInt32 i = 0; i < CHANNEL_GROUP_MAX_SIZE; ++i)
        {
            if (cg.m_szChannelIDs[i] != 0)
            {
                cgw.addChannelWeightInfo(cg.m_szChannelIDs[i], cg.m_szChannelsWeight[i]);
            }
        }
        channelGroupsWeight.insert(make_pair(ChlGrpID, cgw));
    }
}
// #endif
