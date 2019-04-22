#include "COverRateThread.h"
#include "TpsAndHoldCount.h"
#include "UrlCode.h"
#include "main.h"
#include "UTFString.h"
#include "Comm.h"
#include "HttpParams.h"
#include "base64.h"
#include "CotentFilter.h"
#include "global.h"

#include "boost/foreach.hpp"

////////////////////////////////////////////////////////////////////////////

OverRateThread* g_pOverRateThread = NULL;

const UInt64 TIMER_ID_CLEAN_OVERRATEMAP = 111;
const int SECONDS_OF_ONE_DAYA = 24*60*60*1000;

////////////////////////////////////////////////////////////////////////////

#define foreach BOOST_FOREACH

#define CheckNextOverRate(fsmstate) \
    pSession->m_eCheckOverRateState = fsmstate; \
    checkOverRate(pSession);

////////////////////////////////////////////////////////////////////////////

bool startOverRateThread()
{
    g_pOverRateThread = new OverRateThread("OverRateThread");
    INIT_CHK_NULL_RET_FALSE(g_pOverRateThread);
    INIT_CHK_FUNC_RET_FALSE(g_pOverRateThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pOverRateThread->CreateThread());
    return true;
}

////////////////////////////////////////////////////////////////////////////

OverRateThread::Session::Session(OverRateThread* pThread)
{
    init();

    m_pThread = pThread;
    m_ulSequence = pThread->m_snManager.getSn();
    pThread->m_mapSession[m_ulSequence] = this;
}

OverRateThread::Session::~Session()
{
    if (0 != m_ulSequence)
    {
        m_pThread->m_mapSession.erase(m_ulSequence);
    }
}

void OverRateThread::Session::init()
{
    m_pThread = NULL;
    m_ulSequence = 0;
    m_eCheckOverRateState = CheckGlobalOverRate;
    m_eOverRateResult = OverRateNone;
    m_iKeyWordOverRateKeyType = -1;
    m_bInOverRateWhiteList = false;
}

void OverRateThread::Session::setOverRateCheckReqMsg(const OverRateCheckReqMsg* pReq)
{
    CHK_NULL_RETURN(pReq);
    m_reqMsg = *pReq;

    LogDebug("[%s:%s:%s] Sign[%s],Content[%s],"
        "SmsType[%s],C2sDate[%s],TemplateType[%s],ChannelId[%u],OverRateCheckType[%d],isoverrateWhite[%d].",
        m_reqMsg.m_strClientId.c_str(), m_reqMsg.m_strSmsId.c_str(), m_reqMsg.m_strPhone.c_str(),
        m_reqMsg.m_strSign.c_str(),
        m_reqMsg.m_strContent.c_str(),
        m_reqMsg.m_strSmsType.c_str(),
        m_reqMsg.m_strC2sDate.c_str(),
        m_reqMsg.m_strTemplateType.c_str(),
        m_reqMsg.m_uiChannelId,
        m_reqMsg.m_eOverRateCheckType,
        m_reqMsg.m_bInOverRateWhiteList);
}

OverRateThread::OverRateThread(const char* name) : CThread(name)
{
    m_chlSelect = NULL;
    m_pOverRate = NULL;
}

OverRateThread::~OverRateThread()
{
}

bool OverRateThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    m_pOverRate = new CoverRate();
    INIT_CHK_NULL_RET_FALSE(m_pOverRate);

    map<string,list<string> > mapListSet;
    g_pRuleLoadThread->getOverRateKeyWordMap(mapListSet);
    initAuditOverKeyWordSearchTree(mapListSet, m_OverRateKeyWordMap);

    m_chlSelect = new ChannelSelect();
    INIT_CHK_NULL_RET_FALSE(m_chlSelect);
    m_chlSelect->initParam();

    m_pCleanOverRateTimer = SetTimer(TIMER_ID_CLEAN_OVERRATEMAP, "TIMER_CLEAN_OVERRATEMAP", getTimeFromNowToMidnight() * 1000);

    return true;
}

void OverRateThread::initAuditOverKeyWordSearchTree(map<string, list<string> >& mapSetIn, map<string, searchTree*>& mapSetOut)
{
    for (map<string, searchTree*>::iterator iterOld = mapSetOut.begin(); iterOld != mapSetOut.end();)
    {
        if (NULL != iterOld->second)
        {
            delete iterOld->second;
        }

        mapSetOut.erase(iterOld++);
    }

    for (map<string, list<string> >::iterator iterNew = mapSetIn.begin(); iterNew != mapSetIn.end(); ++iterNew)
    {
        searchTree* pTree = new searchTree();

        if (NULL == pTree)
        {
            LogFatal("pTree is NULL.");
            return;
        }

        pTree->initTree(iterNew->second);

        mapSetOut.insert(make_pair(iterNew->first, pTree));

        LogDebug("init %s.", iterNew->first.data());

        for (list<string>::iterator iter = iterNew->second.begin(); iter != iterNew->second.end(); ++iter)
        {
            string& strData = *iter;
            LogDebug("==> %s.", strData.data());
        }
    }
}

void OverRateThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (true)
    {
        m_pTimerMng->Click();

        for (int i = 0; i < 10; i++)
        {
            pthread_mutex_lock(&m_mutex);
            TMsg* pMsg = m_msgQueue.GetMsg();
            pthread_mutex_unlock(&m_mutex);

            if (NULL == pMsg)
            {
                usleep(10000);
            }
            else
            {
                HandleMsg(pMsg);
                delete pMsg;
            }
        }
    }
}

void OverRateThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CAdapter::InterlockedIncrement((long*)&m_iCount);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_OVERRATE_CHECK_REQ:
        {
            handleOverRateCheckReqMsg(pMsg);
            break;
        }
        case MSGTYPE_OVERRATE_SET_RSQ:
        {
            handleOverRateSetReqMsg(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            handleRedisRspMsg(pMsg);
            break;
        }
        case  MSGTYPE_RULELOAD_OVERRATEKEYWORD_UPDATE_REQ:
        {
            TUpdateOverRateKeyWordReq* msg = (TUpdateOverRateKeyWordReq*)pMsg;
            initAuditOverKeyWordSearchTree(msg->m_OverRateKeyWordMap, m_OverRateKeyWordMap);
            LogNotice("RuleUpdate OverRateKEYWORD update. ");
            break;
        }
        case MSGTYPE_RULELOAD_CHANNELOVERRATE_UPDATE_REQ:
        {
            TUpdateChannelOverrateReq* msg = (TUpdateChannelOverrateReq*)pMsg;
            m_chlSelect->m_ChannelOverrateMap = msg->m_ChannelOverrateMap;
            m_pOverRate->cleanChannelOverRateMap();
            LogNotice("RuleUpdate channel overrate update. map.size[%d]", msg->m_ChannelOverrateMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_KEYWORDTEMPLATERULE_UPDATE_REQ:
        {
            TUpdateKeyWordTempltRuleReq* msg = (TUpdateKeyWordTempltRuleReq*)pMsg;
            m_chlSelect->m_KeyWordTempRuleMap = msg->m_KeyWordTempRuleMap;
            m_pOverRate->cleanKeyWordOverRateMap();
            LogNotice("RuleUpdate TempRuleMap update. map.size[%d]", msg->m_KeyWordTempRuleMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_TEMPLATERULE_UPDATE_REQ:
        {
            TUpdateTempltRuleReq* msg = (TUpdateTempltRuleReq*)pMsg;
            m_chlSelect->m_TempRuleMap = msg->m_TempRuleMap;
            m_pOverRate->cleanOverRateMap();
            LogNotice("RuleUpdate TempRuleMap update. map.size[%d]", msg->m_TempRuleMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* msg = (TUpdateSysParamRuleReq*)pMsg;

            //modify overRate config
            m_chlSelect->ParseGlobalOverRateSysPara(msg->m_SysParamMap);

            m_chlSelect->ParseOverRateSysPara(msg->m_SysParamMap);
            m_pOverRate->cleanOverRateMap();

            m_chlSelect->ParseKeyWordOverRateSysPara(msg->m_SysParamMap);
            m_pOverRate->cleanKeyWordOverRateMap();

            m_chlSelect->ParseChannelOverRateSysPara(msg->m_SysParamMap);
            m_pOverRate->cleanChannelOverRateMap();

            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            handleTimeout(pMsg);
            break;
        }
        default:
        {
            LogWarn("msgType[0x:%x] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}

void OverRateThread::handleOverRateCheckReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    OverRateCheckReqMsg* pReq = (OverRateCheckReqMsg*)pMsg;

    Session* pSession = new Session(this);
    CHK_NULL_RETURN(pSession);
    pSession->setOverRateCheckReqMsg(pReq);
    pSession->m_bInOverRateWhiteList = pReq->m_bInOverRateWhiteList;
    

    if (pSession->m_bInOverRateWhiteList)
    {
        CheckNextOverRate(Session::CheckOverRateOk);
        return;
    }

    switch (pSession->m_reqMsg.m_eOverRateCheckType)
    {
        case OverRateCheckReqMsg::CommonCheck:
        {
            CheckNextOverRate(Session::CheckGlobalOverRate);
            break;
        }
        case OverRateCheckReqMsg::ChannelOverRateCheck:
        {
            CheckNextOverRate(Session::CheckChannelOverRate);
            break;
        }
        default:
        {
            LogError("Invalid OverRateCheckType[%d].", pSession->m_reqMsg.m_eOverRateCheckType);
            break;
        }
    }
}


void OverRateThread::checkOverRate(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    if (Session::CheckGlobalOverRate == pSession->m_eCheckOverRateState)
    {
        checkGlobalOverRate(pSession);
    }
    else if (Session::CheckKeywordOverRate == pSession->m_eCheckOverRateState)
    {
        checkKeywordOverRate(pSession);
    }
    else if (Session::CheckSmsTypeOverRate == pSession->m_eCheckOverRateState)
    {
        checkSmsTypeOverRate(pSession);
    }
    else if (Session::CheckChannelOverRate == pSession->m_eCheckOverRateState)
    {
        checkChannelOverRate(pSession);
    }
    else if (Session::CheckOverRateOk == pSession->m_eCheckOverRateState)
    {
        sendOverRateCheckRspMsg(pSession);
    }
    else
    {
        LogError("[%s:%s:%s] Invalid CheckOverRateState[%d].", 
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(), 
            pSession->m_eCheckOverRateState);
    }
}

void OverRateThread::checkGlobalOverRate(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    m_chlSelect->getGlobalOverRateRule(pSession->m_globalOverRateRule);

    if (1 == pSession->m_globalOverRateRule.m_enable)
    {
        redisGet("HGETALL SASP_Global_OverRate:" + pSession->m_reqMsg.m_strPhone,
            "SASP_Global_OverRate",
            this,
            pSession->m_ulSequence,
            "check global over rate",
            g_pOverrateRedisThreadPool);
        return;
    }

    CheckNextOverRate(Session::CheckKeywordOverRate);
}

void OverRateThread::checkKeywordOverRate(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    // 获取关键字超频规则:系统级or用户级
    m_chlSelect->getKeyWordOverRateRule(pSession->m_reqMsg.m_strClientId,
                                        pSession->m_reqMsg.m_strSign,
                                        pSession->m_keywordOverRateRule,
                                        pSession->m_iKeyWordOverRateKeyType);

    // 如果找到规则,且规则开启
    if ((-1 != pSession->m_iKeyWordOverRateKeyType) && (1 == pSession->m_keywordOverRateRule.m_enable))
    {
        string strOverRateKeyord;

        // 检查超频关键字
        if (OverRateKeywordRegex(pSession, strOverRateKeyord))
        {
            LogDebug("[%s:%s:%s], content[%s], sign[%s], hit strOverRateKeyord[%s].",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                pSession->m_reqMsg.m_strContent.data(),
                pSession->m_reqMsg.m_strSign.data(),
                strOverRateKeyord.data());

            getKeyWordOverRateKey(pSession);

            if (m_pOverRate->isKeyWordOverRate(pSession->m_strKeyWordOverRateKey))
            {
                // 如果内存中存在超频记录

                LogWarn("[%s:%s:%s] match keyWord[%s] in cache.",
                    pSession->m_reqMsg.m_strClientId.data(),
                    pSession->m_reqMsg.m_strSmsId.data(),
                    pSession->m_reqMsg.m_strPhone.data(),
                    pSession->m_strKeyWordOverRateKey.data());

                sendOverRateCheckRspMsg(pSession);
                return;
            }
            else
            {
                // 如果内存中没有,则查看Redis是否存在超频记录

                LogDebug("[%s:%s:%s] check keyWord OverRate[%s] in redis.",
                    pSession->m_reqMsg.m_strClientId.data(),
                    pSession->m_reqMsg.m_strSmsId.data(),
                    pSession->m_reqMsg.m_strPhone.data(),
                    strOverRateKeyord.data());

                redisGet("GET " + pSession->m_strKeyWordOverRateKey,
                         pSession->m_strKeyWordOverRateKey,
                         this,
                         pSession->m_ulSequence,
                         "check keyword overrate",
                         g_pOverrateRedisThreadPool);
                return;
            }
        }
    }

    CheckNextOverRate(Session::CheckSmsTypeOverRate);
}

void OverRateThread::checkSmsTypeOverRate(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    // 获取短信类型超频规则:系统级or用户级
    m_chlSelect->getOverRateRule(pSession->m_reqMsg.m_strClientId + "_" + pSession->m_reqMsg.m_strSmsType,
                                 pSession->m_smsTypeOverRateRule);

    if (1 == pSession->m_smsTypeOverRateRule.m_enable)
    {
        getSmsTypeOverRateKey(pSession);

        if (m_pOverRate->isOverRate(pSession->m_strSmsTypeOverRateKey))
        {
            // 如果内存中存在超频记录

            LogWarn("[%s:%s] clientid[%s] isOverRate, strOverRateKey[%s].",
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_strSmsTypeOverRateKey.data());

            sendOverRateCheckRspMsg(pSession);
            return;
        }
        else
        {
            // 如果内存中没有,则查看Redis是否存在超频记录

            LogDebug("[%s:%s:%s] check smstype OverRate[%s] in redis.",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                pSession->m_strSmsTypeOverRateKey.data());

            redisGet("GET " + pSession->m_strSmsTypeOverRateKey,
                     "SASP_OverRate",
                     this,
                     pSession->m_ulSequence,
                     "check smstype overrate",
                     g_pOverrateRedisThreadPool);
            return;
        }
    }

    CheckNextOverRate(Session::CheckChannelOverRate);
}

void OverRateThread::checkChannelOverRate(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    // 获取通道超频规则:系统级or用户级
    int keyType = -1;
    m_chlSelect->getChannelOverRateRuleAndKey(to_string(pSession->m_reqMsg.m_uiChannelId),
                                                pSession->m_reqMsg.m_strSmsType,
                                                pSession->m_reqMsg.m_strSign,
                                                pSession->m_reqMsg.m_strPhone,
                                                pSession->m_strChannelOverRateKey,
                                                pSession->m_channelOverRateRule,
                                                keyType);

    if ((keyType != -1) && (1 == pSession->m_channelOverRateRule.m_enable))
    {
        if (m_pOverRate->isChannelOverRate(pSession->m_strChannelOverRateKey, pSession->m_channelOverRateRule))
        {
            // 如果内存中存在超频记录

            LogWarn("[%s:%s:%s] channelid[%d] is OverRate [%s].",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                pSession->m_reqMsg.m_uiChannelId,
                pSession->m_strChannelOverRateKey.data());

            sendOverRateCheckRspMsg(pSession);
            return;
        }
        else
        {
            // 如果内存中没有,则查看Redis是否存在超频记录

            LogDebug("[%s:%s:%s] channelid[%d] is not memory OverRate, search redis.",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                pSession->m_reqMsg.m_uiChannelId);

            redisGet("HGETALL SASP_Channel_Day_OverRate:" + pSession->m_strChannelOverRateKey,
                "SASP_Channel_Day_OverRate",
                this,
                pSession->m_ulSequence,
                "check channel day overrate",
                g_pOverrateRedisThreadPool);

            return;
        }
    }

    CheckNextOverRate(Session::CheckOverRateOk);
}

void OverRateThread::handleRedisRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TRedisResp* pRedisRsp = (TRedisResp*)pMsg;

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pRedisRsp->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    redisReply* pRedisReply = pRedisRsp->m_RedisReply;

    LogDebug("[%s:%s:%s] redis SessionID[%s], redisType[%d].",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        pRedisRsp->m_strSessionID.c_str(),
        pRedisReply->type);

    ///////////////////////////////////////////////////////////////////////////

    if ("check global over rate" == pMsg->m_strSessionID)
    {
        checkRedisGlobalOverRate(pSession, pRedisReply);
    }
    else if ("check keyword overrate" == pMsg->m_strSessionID)
    {
        checkRedisKeywordOverRate(pSession, pRedisReply);
    }
    else if ("check smstype overrate" == pMsg->m_strSessionID)
    {
        checkRedisSmsTypeOverRate(pSession, pRedisReply);
    }
    else if ("check channel day overrate" == pMsg->m_strSessionID)
    {
        checkRedisChannelDayOverRate(pSession, pRedisReply);
    }
    else if ("check channel overrate" == pMsg->m_strSessionID)
    {
        checkRedisChannelOverRate(pSession, pRedisReply);
    }
    else
    {
        LogError("[%s:%s:%s] Invalid redis session[%s].", 
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            pMsg->m_strSessionID.data());
    }

    freeReply(pRedisReply);
}

void OverRateThread::checkRedisGlobalOverRate(Session* pSession, redisReply* pRedisReply)
{
    CHK_NULL_RETURN(pSession);
    CHK_NULL_RETURN(pRedisReply);

    bool retBool = false;

    if ((NULL != pRedisReply)
    && (pRedisReply->type == REDIS_REPLY_ARRAY)
    && (pRedisReply->elements > 0))
    {
        retBool = checkRedisGlobalOverRateEx(pSession, (void**)pRedisReply->element, pRedisReply->elements);
    }
    else
    {
        if (NULL == pRedisReply)
        {
            retBool = true;
        }
        else
        {
            LogNotice("ignore RedisReply, type[%d], size[%u].",
                pRedisReply->type, pRedisReply->elements);
        }
    }

    if (retBool)
    {
        sendOverRateCheckRspMsg(pSession);
        return;
    }

    increaseRedisGlobalOverRate(pSession, 1);
    CheckNextOverRate(Session::CheckKeywordOverRate);
}

bool OverRateThread::checkRedisGlobalOverRateEx(Session* pSession, void** data, int size)
{
    redisReply** reply = (redisReply**)data;
    UInt32 total_count = 0;
    string strDelDate = "";
    string strCurDate = pSession->m_reqMsg.m_strC2sDate;

    if (8 > pSession->m_reqMsg.m_strC2sDate.size())
    {
        strCurDate = Comm::getCurrentDay();
    }

    strCurDate = strCurDate.substr(0, 8);
    strCurDate.append("00:00:00");

    Int64 curTimestamp = Comm::strToTime(const_cast< char* >(strCurDate.data()));

    LogDebug("[%s:%s:%s] redis reply array size: %d, CurTime[ %s, %ld, %ld ]",
        pSession->m_reqMsg.m_strClientId.c_str(),
        pSession->m_reqMsg.m_strSmsId.c_str(),
        pSession->m_reqMsg.m_strPhone.c_str(),
        size,
        strCurDate.c_str(),
        curTimestamp,
        time(NULL));

    const models::TempRule& rule = pSession->m_globalOverRateRule;

    for (int i = 0; i < size; i += 2)
    {
        if (REDIS_REPLY_STRING == reply[i]->type)
        {
            string strDate = reply[i]->str;
            strDate.append("00:00:00");
            Int64 uRedisStamp = Comm::strToTime(const_cast<char*>(strDate.data()));
            Int64 uDiff = curTimestamp - uRedisStamp ;

            if (uDiff >  6 * 24 * 3600)
            {
                strDelDate.append(reply[i]->str);
                strDelDate.append(" ");
            }
            else
            {
                if ((rule.m_overRate_time_h >= 24) && uDiff <= (rule.m_overRate_time_h - 24) * 3600)
                {
                    total_count += atoi(reply[i + 1]->str);
                }
            }
        }
    }

    bool retVal = false;

    if (total_count >= rule.m_overRate_num_h)
    {
        LogWarn("[%s:%s:%s] global_overRate warn, total sended: %u! num_h[%u], time_h[%u]",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data(),
            total_count,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);
        retVal = true;
    }
    else
    {
        LogDebug("[%s:%s:%s] do not reach global_overRate threshold, total sended: %u! num_h[%u], time_h[%u]",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data(),
            total_count,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);

        retVal = false;
    }

    if (!strDelDate.empty())
    {
        string strRedisCmd;
        strRedisCmd.append("HDEL SASP_Global_OverRate:");
        strRedisCmd.append(pSession->m_reqMsg.m_strPhone);
        strRedisCmd.append(" ");
        strRedisCmd.append(strDelDate);

        redisSet(strRedisCmd, "SASP_Global_OverRate", NULL, g_pOverrateRedisThreadPool);
    }

    return retVal;
}

void OverRateThread::checkRedisKeywordOverRate(Session* pSession, redisReply* pRedisReply)
{
    CHK_NULL_RETURN(pSession);

    bool retBool = false;

    if (NULL == pRedisReply)
    {
        LogWarn("[%s:%s] redis reply is NULL.",
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());
        retBool = true;
    }
    else
    {
        // 解析超频数据，并判断是否超频,redis没有数据不处理超频
        if (pRedisReply->type == REDIS_REPLY_STRING)
        {
            std::string strOut = pRedisReply->str;
            std::string strCmp = strOut;

            LogDebug("[%s:%s:%s] redis replay string[%s].",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                strOut.data());

            retBool = checkRedisKeywordOverRateEx(pSession, strOut);

            /* redis数据已经太长需要清理*/
            if (strOut.size() != 0 && strOut.size() < strCmp.size())
            {
                LogDebug("[%s:%s:%s] strOut[%s:%d], strcmp[%s:%d].",
                    pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                    strOut.c_str(),
                    strOut.size(),
                    strCmp.c_str(),
                    strCmp.size());

                setOverRateRedis(pSession->m_strKeyWordOverRateKey, strOut, true);
            }
        }
    }

    if (retBool)
    {
        sendOverRateCheckRspMsg(pSession);
        return;
    }

    CheckNextOverRate(Session::CheckChannelOverRate);
}

bool OverRateThread::checkRedisKeywordOverRateEx(Session* pSession, string& strtimes)
{
    long curtime = time(NULL);
    const models::TempRule& rule = pSession->m_keywordOverRateRule;

    //ruleconfig parma check
    if ((rule.m_overRate_time_h != 0 && rule.m_overRate_time_s != 0)
    && (rule.m_overRate_num_s != 0 && rule.m_overRate_num_h != 0))
    {
        if ((rule.m_overRate_time_h * 60 * 60 < rule.m_overRate_time_s)
        || (rule.m_overRate_num_h < rule.m_overRate_num_s))
        {
            LogWarn("[%s:%s:%s] key word overRate parma error. time_h[%d], time_s[%d], num_h[%d], num_s[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_time_h,
                rule.m_overRate_time_s,
                rule.m_overRate_num_h,
                rule.m_overRate_num_s);
            return false;
        }
    }

    /* 超过超频redis限制的最大长度*/
    if (strtimes.length() > SMS_OVERRATE_REDIS_STRING_MAX_SIZE)
    {
        m_pOverRate->setKeyWordOverRatePer(pSession->m_strKeyWordOverRateKey, curtime + rule.m_overRate_time_h * 60 * 60);

        LogWarn("[%s:%s:%s] key word overRate record too large."
            "strtimes.length[%d],overRate_h warn! num_s[%d],time_s[%d],num_m[%d],time_m[%d],num_h[%d],time_h[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            strtimes.length(), rule.m_overRate_num_s,
            rule.m_overRate_time_s,
            rule.m_overRate_num_m,
            rule.m_overRate_time_m,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);
        return true;    //超频了
    }

    //strtimes transfer to timelist
    std::vector<string> timeslist;

    if (!strtimes.empty())
    {
        string strTemp = strtimes;
        Comm::split(strTemp, "&", timeslist);
    }

    LogDebug("[%s:%s:%s] timestr from redis[%s] num_s[%d], time_s[%d],num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        strtimes.c_str(),
        rule.m_overRate_num_s,
        rule.m_overRate_time_s,
        rule.m_overRate_num_m,
        rule.m_overRate_time_m,
        rule.m_overRate_num_h,
        rule.m_overRate_time_h);

    //sum not enough
    if ((timeslist.size() < rule.m_overRate_num_h)
    && (timeslist.size() < rule.m_overRate_num_m)
    && (timeslist.size() < rule.m_overRate_num_s))
    {
        return false;
    }

    //sum too large
    UInt32 LargerNum = (rule.m_overRate_num_h > rule.m_overRate_num_s ? rule.m_overRate_num_h : rule.m_overRate_num_s);

    LargerNum = (LargerNum > rule.m_overRate_num_m ? LargerNum : rule.m_overRate_num_m);

    while (timeslist.size() > LargerNum)
    {
        LogNotice("[%s:%s:%s] list size over large. num_h[%d],num_m[%d],num_s[%d],list size[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            rule.m_overRate_num_h,
            rule.m_overRate_num_m,
            rule.m_overRate_num_s,
            timeslist.size());

        timeslist.erase(timeslist.begin());
    }

    strtimes = "";

    for (std::vector<string>::iterator ittime = timeslist.begin(); ittime != timeslist.end(); ittime++)
    {
        strtimes  += *ittime + "&" ;
    }

    LogDebug("[%s:%s] strtimes[%s]",
        pSession->m_reqMsg.m_strSmsId.data(),
        pSession->m_reqMsg.m_strPhone.data(),
        strtimes.data());

    //check if overRate_h
    if (rule.m_overRate_num_h != 0 && rule.m_overRate_time_h != 0 && timeslist.size() >= rule.m_overRate_num_h)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_h);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_h * 60 * 60)
        {
            //warn out, and save map
            m_pOverRate->setKeyWordOverRatePer(pSession->m_strKeyWordOverRateKey, atoi(starttime.data()) + rule.m_overRate_time_h * 60 * 60);

            LogWarn("[%s:%s:%s] key word over rate overRate_h warn! "
                "num_s[%d], time_s[%d], num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    //check if overRate_m
    if (rule.m_overRate_num_m != 0 && rule.m_overRate_time_m != 0 && timeslist.size() >= rule.m_overRate_num_m)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_m);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_m * 60)
        {
            //warn out, and save map
            m_pOverRate->setKeyWordOverRatePer(pSession->m_strKeyWordOverRateKey, atoi(starttime.data()) + rule.m_overRate_time_m * 60);

            LogWarn("[%s:%s:%s] key word over rate overRate_m warn! "
                "num_s[%d], time_s[%d], num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    //check if overRate_s
    if (rule.m_overRate_num_s != 0 && rule.m_overRate_time_s != 0 && timeslist.size() >= rule.m_overRate_num_s)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_s);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_s)
        {
            //warn out, and save map
            m_pOverRate->setKeyWordOverRatePer(pSession->m_strKeyWordOverRateKey, atoi(starttime.data()) + rule.m_overRate_time_s);

            LogWarn("[%s:%s:%s] key word overRate_s warn! "
                "num_s[%d], time_s[%d], num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    return false;
}

void OverRateThread::checkRedisSmsTypeOverRate(Session* pSession, redisReply* pRedisReply)
{
    CHK_NULL_RETURN(pSession);
    CHK_NULL_RETURN(pRedisReply);

    bool retBool = false;

    if (NULL == pRedisReply)
    {
        LogWarn("[%s:%s:%s] redis reply is NULL.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());

        retBool = true;
    }
    else
    {
        LogDebug("[%s:%s:%s] redis reply not NULL.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());

        // 解析超频数据，并判断是否超频,redis没有数据不处理超频
        if (pRedisReply->type == REDIS_REPLY_STRING)
        {
            std::string strOut = pRedisReply->str;
            std::string strCmp = strOut;

            LogDebug("[%s:%s:%s] redis replay string[%s].",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                strOut.data());

            retBool = checkRedisSmsTypeOverRateEx(pSession, strOut);

            // redis数据已经太长需要清理
            if (strOut.size() != 0 && strOut.size() < strCmp.size())
            {
                LogDebug("[%s:%s:%s] strOut[%s:%u], strcmp[%s:%u].",
                    pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                    strOut.data(),
                    strOut.size(),
                    strCmp.data(),
                    strCmp.size());

                setOverRateRedis(pSession->m_strSmsTypeOverRateKey, strOut, true);
            }
        }
    }

    if (retBool)
    {
        sendOverRateCheckRspMsg(pSession);
        return;
    }

    CheckNextOverRate(Session::CheckChannelOverRate);
}

bool OverRateThread::checkRedisSmsTypeOverRateEx(Session* pSession, string& strtimes)
{
    CHK_NULL_RETURN_FALSE(pSession);

    long curtime = time(NULL);
    const models::TempRule& rule = pSession->m_smsTypeOverRateRule;

    //ruleconfig parma check
    if ((rule.m_overRate_time_h != 0 && rule.m_overRate_time_s != 0)
        && (rule.m_overRate_num_s != 0 && rule.m_overRate_num_h != 0))
    {
        if ((rule.m_overRate_time_h * 60 * 60 < rule.m_overRate_time_s)
        || (rule.m_overRate_num_h < rule.m_overRate_num_s))
        {
            LogWarn("[%s:%s:%s] overRate parma error. time_h[%d], time_s[%d], num_h[%d], num_s[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_time_h,
                rule.m_overRate_time_s,
                rule.m_overRate_num_h,
                rule.m_overRate_num_s);
            return false;
        }
    }

    /* 超过超频限制的字符串长度认为超频*/
    if (strtimes.length() > SMS_OVERRATE_REDIS_STRING_MAX_SIZE)
    {
        string str = "SASP_OverRate:"
                   + pSession->m_reqMsg.m_strClientId
                   + "&"
                   + pSession->m_reqMsg.m_strPhone
                   + "&"
                   + pSession->m_reqMsg.m_strSmsType;

        m_pOverRate->setOverRatePer(str, curtime + rule.m_overRate_time_h * 60 * 60);

        LogWarn("[%s:%s:%s] overRate record too large.strtimes."
            "length[%d],overRate_h warn! num_s[%d], time_s[%d], num_h[%d], time_h[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            strtimes.length(),
            rule.m_overRate_num_s,
            rule.m_overRate_time_s,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);
        return true;
    }

    //strtimes transfer to timelist
    std::vector<string> timeslist;

    if (!strtimes.empty())
    {
        string strTemp = strtimes ;
        Comm::split(strTemp, "&", timeslist);
    }

    LogDebug("[%s:%s:%s] timestr from redis[%s] num_s[%d], time_s[%d], num_m[%d], time_m[%d],num_h[%d], time_h[%d]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        strtimes.data(),
        rule.m_overRate_num_s,
        rule.m_overRate_time_s,
        rule.m_overRate_num_m,
        rule.m_overRate_time_m,
        rule.m_overRate_num_h,
        rule.m_overRate_time_h);

    //sum not enough
    if (timeslist.size() < rule.m_overRate_num_h
        && timeslist.size() < rule.m_overRate_num_s
        && timeslist.size() < rule.m_overRate_num_m)
    {
        return false;
    }

    //sum too large
    UInt32 largerNum = 0;
    largerNum = (rule.m_overRate_num_h > rule.m_overRate_num_m ? rule.m_overRate_num_h : rule.m_overRate_num_m);
    largerNum = (largerNum > rule.m_overRate_num_s ? largerNum : rule.m_overRate_num_s);

    /* 删除多余的数据*/
    while (timeslist.size() > largerNum)
    {
        LogNotice("[%s:%s:%s] list size over large. num_h[%d],  num_m[%d],  num_s[%d], list size[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            rule.m_overRate_num_h,
            rule.m_overRate_num_m,
            rule.m_overRate_num_s,
            timeslist.size());
        timeslist.erase(timeslist.begin());
    }

    strtimes = "";

    for (std::vector<string>::iterator ittime = timeslist.begin(); ittime != timeslist.end(); ittime++)
    {
        strtimes += *ittime + "&" ;
    }

    LogDebug("[%s:%s:%s] strtimes[%s]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        strtimes.data());

    //check if overRate_h
    if (rule.m_overRate_num_h != 0 && rule.m_overRate_time_h != 0 && timeslist.size() >= rule.m_overRate_num_h)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_h);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_h * 60 * 60)
        {
            //warn out, and save map
            string str = "SASP_OverRate:"
                       + pSession->m_reqMsg.m_strClientId
                       + "&"
                       + pSession->m_reqMsg.m_strPhone
                       + "&"
                       + pSession->m_reqMsg.m_strSmsType;

            m_pOverRate->setOverRatePer(str, atoi(starttime.data()) + rule.m_overRate_time_h * 60 * 60);

            LogWarn("[%s:%s:%s] overRate_h warn! num_s[%d], time_s[%d], num_m[%d],time_m[%d],num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    //check if overRate_m
    if (rule.m_overRate_num_m != 0 && rule.m_overRate_time_m != 0 && timeslist.size() >= rule.m_overRate_num_m)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_m);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_m * 60)
        {
            //warn out, and save map
            string str = "SASP_OverRate:"
                       + pSession->m_reqMsg.m_strClientId
                       + "&"
                       + pSession->m_reqMsg.m_strPhone
                       + "&"
                       + pSession->m_reqMsg.m_strSmsType;

            m_pOverRate->setOverRatePer(str, atoi(starttime.data()) + rule.m_overRate_time_m * 60);

            LogWarn("[%s:%s:%s] overRate_m warn! num_s[%d], time_s[%d], num_m[%d],time_m[%d],num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    //check if overRate_s
    if (rule.m_overRate_num_s != 0 && rule.m_overRate_time_s != 0 && timeslist.size() >= rule.m_overRate_num_s)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_s);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_s)
        {
            //warn out, and save map
            string str = "SASP_OverRate:"
                       + pSession->m_reqMsg.m_strClientId
                       + "&"
                       + pSession->m_reqMsg.m_strPhone
                       + "&"
                       + pSession->m_reqMsg.m_strSmsType;

            m_pOverRate->setOverRatePer(str, atoi(starttime.data()) + rule.m_overRate_time_s);

            LogWarn("[%s:%s:%s] overRate_s warn! num_s[%d], time_s[%d], num_m[%d],time_m[%d],num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_h,
                rule.m_overRate_time_h);
            return true;    //超频了
        }
    }

    return false;
}

void OverRateThread::checkRedisChannelDayOverRate(Session* pSession, redisReply* pRedisReply)
{
    CHK_NULL_RETURN(pSession);
    CHK_NULL_RETURN(pRedisReply);

    bool retBool = false;

    string strCurDate;
    strCurDate = Comm::getCurrentDay();
    strCurDate = strCurDate.substr(0, 8);
    pSession->m_strCurrentDay = strCurDate;

    string ChannelDayOverRate = "SASP_Channel_Day_OverRate:" + pSession->m_strChannelOverRateKey;
    pSession->m_strChannelOverRateKeyall.append("&channelOverrateDate=");
    pSession->m_strChannelOverRateKeyall.append(ChannelDayOverRate + " " + strCurDate);
    pSession->m_strChannelOverRateKeyall.append("&oriChannelid=");
    pSession->m_strChannelOverRateKeyall.append(Comm::int2str(pSession->m_reqMsg.m_uiChannelId));

    if ((pRedisReply != NULL)
    && (pRedisReply->type == REDIS_REPLY_ARRAY)
    && (0 < pRedisReply->elements))
    {
        retBool = checkRedisChannelDayOverRateEx(pSession, (void**)pRedisReply->element, pRedisReply->elements);
    }
    else
    {
        if (pRedisReply == NULL)
        {
            retBool = true;
        }
        else
        {
            LogNotice("ignore RedisReply, type[%d], size[%u].",
                pRedisReply->type, pRedisReply->elements);
        }
    }

    if (retBool)
    {
        sendOverRateCheckRspMsg(pSession);
        return;
    }

    redisGet("GET SASP_Channel_OverRate:" + pSession->m_strChannelOverRateKey,
        "SASP_Channel_OverRate",
        this,
        pSession->m_ulSequence,
        "check channel overrate",
        g_pOverrateRedisThreadPool);
}

bool OverRateThread::checkRedisChannelDayOverRateEx(Session* pSession, void** data, int size)
{
    redisReply** reply = (redisReply**)data;
    UInt32 total_count = 0;
    Int64 curTimestamp;
    string strDelDate  = "";
    string strCurDate  = "";
    Comm comm;
    bool retVal = false;

    strCurDate = pSession->m_strCurrentDay;
    strCurDate.append("00:00:00");

    curTimestamp = comm.strToTime(const_cast< char* >(strCurDate.data()));

    LogDebug("[%s:%s:%s] redis reply array size[%d], CurTime[%s,%ld,%ld]",
        pSession->m_reqMsg.m_strClientId.c_str(),
        pSession->m_reqMsg.m_strSmsId.c_str(),
        pSession->m_reqMsg.m_strPhone.c_str(),
        size,
        strCurDate.data(),
        curTimestamp,
        time(NULL));

    const models::TempRule& rule = pSession->m_channelOverRateRule;

    std::map<std::string, std::string> channelDayDataMap;

    for (int i = 0; i < size; i += 2)
    {
        if (REDIS_REPLY_STRING == reply[i]->type)
        {
            string strDate = reply[i]->str;
            strDate.append("00:00:00");
            Int64 uRedisStamp = comm.strToTime(const_cast<char*>(strDate.data()));
            Int64 uDiff = curTimestamp - uRedisStamp ;

            if (uDiff >=  7 * 24 * 3600)
            {
                strDelDate.append(reply[i]->str);
                strDelDate.append(" ");
            }
            else
            {
                if ((rule.m_overRate_time_h >= 24) && uDiff <= (rule.m_overRate_time_h - 24) * 3600)
                {
                    total_count += atoi(reply[i + 1]->str);
                    channelDayDataMap[reply[i]->str] = reply[i + 1]->str;
                }
            }
        }
    }

    if (total_count >= rule.m_overRate_num_h)
    {
        LogWarn("[%s:%s:%s] channel_day_overRate warn, total sended[%u], num_h[%u], time_h[%u]",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data(),
            total_count,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);

        retVal = true;
        m_pOverRate->setChannelDayOverRatePer(pSession->m_strChannelOverRateKey, channelDayDataMap);
    }
    else
    {
        LogDebug("[%s:%s:%s] do not reach channel_day_overRate threshold, total sended: %u! num_h[%u], time_h[%u]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            total_count,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);

        retVal = false;
    }

    if (!strDelDate.empty())
    {
        TRedisReq* pRedisReq = new TRedisReq();
        pRedisReq->m_strKey = "SASP_Channel_Day_OverRate:" + pSession->m_strChannelOverRateKey;
        pRedisReq->m_RedisCmd = " HDEL ";
        pRedisReq->m_RedisCmd.append(pRedisReq->m_strKey);
        pRedisReq->m_RedisCmd.append(" " + strDelDate);
        pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;
        LogNotice("[ChannelDayOverRate] Redis cmd:[%s]", pRedisReq->m_RedisCmd.data());
        SelectRedisThreadPoolIndex(g_pOverrateRedisThreadPool, pRedisReq);
    }

    return retVal;
}

void OverRateThread::checkRedisChannelOverRate(Session* pSession, redisReply* pRedisReply)
{
    CHK_NULL_RETURN(pSession);
    CHK_NULL_RETURN(pRedisReply);

    bool retBool = false;

    if (NULL == pRedisReply)
    {
        LogWarn("[%s:%s:%s] redis reply is NULL.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());

        retBool = true;
    }
    else
    {
        LogDebug("[%s:%s:%s] redis reply not NULL.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());

        // 解析超频数据,并判断是否超频,redis没有数据不处理超频
        if (pRedisReply->type == REDIS_REPLY_STRING)
        {
            std::string strOut = pRedisReply->str;
            std::string strCmp = strOut;

            LogDebug("[%s:%s:%s] redis replay string[%s].",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                strOut.data());

            retBool = checkRedisChannelOverRateEx(pSession, strOut);

            // redis数据已经太长需要清理
            if (strOut.size() != 0 && strOut.size() < strCmp.size())
            {
                LogDebug("[%s:%s:%s] strOut:%s(%d), strcmp:%s(%d)",
                    pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                    strOut.c_str(),
                    strOut.size(),
                    strCmp.c_str(),
                    strCmp.size());

                checkRedisChannelOverRateSetRedis(pSession, strOut, true);
            }
        }
    }

    if (retBool)
    {
        sendOverRateCheckRspMsg(pSession);
        return;
    }

    CheckNextOverRate(Session::CheckOverRateOk);
}

bool OverRateThread::checkRedisChannelOverRateEx(Session* pSession, string& strtimes)
{
    long curtime = time(NULL);
    const models::TempRule& rule = pSession->m_channelOverRateRule;

    if (rule.m_enable == 0)
    {
        return false;
    }

    //ruleconfig param check
    if ((rule.m_overRate_time_m != 0 && rule.m_overRate_time_s != 0)
    && (rule.m_overRate_num_s != 0 && rule.m_overRate_num_m != 0))
    {
        if ((rule.m_overRate_time_m * 60 < rule.m_overRate_time_s)
        || (rule.m_overRate_num_m < rule.m_overRate_num_s))
        {
            LogWarn("[%s:%s:%s] channel overRate parma error. time_h[%d], time_s[%d], num_h[%d], num_s[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_time_m,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_num_s);
            return false;
        }
    }

    /* 超过超频redis限制的最大长度*/
    if (strtimes.length() > SMS_OVERRATE_REDIS_STRING_MAX_SIZE)
    {
        m_pOverRate->setChannelOverRatePer(pSession->m_strChannelOverRateKey, curtime + rule.m_overRate_time_m * 60);

        LogWarn("[%s:%s:%s] channel overRate record too large."
            "strtimes.length[%d],overRate_h warn! num_s[%d], time_s[%d], num_m[%d], time_m[%d],num_h[%d], time_h[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            strtimes.length(), rule.m_overRate_num_s,
            rule.m_overRate_time_s,
            rule.m_overRate_num_m,
            rule.m_overRate_time_m,
            rule.m_overRate_num_h,
            rule.m_overRate_time_h);

        return true;    //超频了
    }

    //strtimes transfer to timelist
    std::vector<string> timeslist;

    if (!strtimes.empty())
    {
        string strTemp = strtimes;
        Comm::split(strTemp, "&", timeslist);
    }

    LogDebug("[%s:%s:%s] timestr from redis[%s] num_s[%d], time_s[%d],num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        strtimes.data(), rule.m_overRate_num_s,
        rule.m_overRate_time_s,
        rule.m_overRate_num_m,
        rule.m_overRate_time_m,
        rule.m_overRate_num_h,
        rule.m_overRate_time_h);

    //sum not enough
    if (timeslist.size() < rule.m_overRate_num_m && timeslist.size() < rule.m_overRate_num_s)
    {
        return false;
    }

    //sum too large
    UInt32 LargerNum = (rule.m_overRate_num_m > rule.m_overRate_num_s ? rule.m_overRate_num_m : rule.m_overRate_num_s);

    while (timeslist.size() > LargerNum)
    {
        LogNotice("[%s:%s:%s] list size over large.num_m[%d],  num_s[%d], list size[%d]",
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
            rule.m_overRate_num_m,
            rule.m_overRate_num_s,
            timeslist.size());

        timeslist.erase(timeslist.begin());
    }

    strtimes = "";

    for (std::vector<string>::iterator ittime = timeslist.begin(); ittime != timeslist.end(); ittime++)
    {
        strtimes  += *ittime + "&" ;
    }

    LogDebug("[%s:%s:%s] strtimes[%s]",
        pSession->m_reqMsg.m_strClientId.data(),
        pSession->m_reqMsg.m_strSmsId.data(),
        pSession->m_reqMsg.m_strPhone.data(),
        strtimes.data());

    //check if overRate_m
    if (rule.m_overRate_num_m != 0 && rule.m_overRate_time_m != 0 && timeslist.size() >= rule.m_overRate_num_m)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_m);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_m * 60)
        {
            //warn out, and save map
            m_pOverRate->setChannelOverRatePer(pSession->m_strChannelOverRateKey, atoi(starttime.data()) + rule.m_overRate_time_m * 60);

            LogWarn("[%s:%s:%s] channel over rate overRate_m warn! "
                "num_s[%d], time_s[%d], num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.data(),
                pSession->m_reqMsg.m_strSmsId.data(),
                pSession->m_reqMsg.m_strPhone.data(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m);

            return true;    //超频了
        }
    }

    //check if overRate_s
    if (rule.m_overRate_num_s != 0 && rule.m_overRate_time_s != 0 && timeslist.size() >= rule.m_overRate_num_s)
    {
        std::string starttime = timeslist.at(timeslist.size() - rule.m_overRate_num_s);

        if (curtime - atoi(starttime.data()) <= rule.m_overRate_time_s)
        {
            //warn out, and save map
            m_pOverRate->setChannelOverRatePer(pSession->m_strChannelOverRateKey, atoi(starttime.data()) + rule.m_overRate_time_s);

            LogWarn("[%s:%s:%s] channel overRate_s warn! "
                "num_s[%d], time_s[%d], num_m[%d], time_m[%d], num_h[%d], time_h[%d]",
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                rule.m_overRate_num_s,
                rule.m_overRate_time_s,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m,
                rule.m_overRate_num_m,
                rule.m_overRate_time_m);

            return true;    //超频了
        }
    }

    return false;
}

void OverRateThread::checkRedisChannelOverRateSetRedis(Session* pSession, cs_t redisStr, bool bReset)
{
    long curtime = time(NULL);
    struct tm tblock = {0};
    localtime_r(&curtime, &tblock);
    int h = tblock.tm_hour;
    int cleantime = (24 - h) * 60 * 60 - tblock.tm_min * 60 - tblock.tm_sec ;

    string strRedisCmdKey;
    strRedisCmdKey.append("SASP_Channel_OverRate:");
    strRedisCmdKey.append(pSession->m_strChannelOverRateKey);

    string strRedisCmd;
    if (bReset)
    {
        strRedisCmd.append("SET ");
        strRedisCmd.append(strRedisCmdKey);
        strRedisCmd.append(" ");
        strRedisCmd.append(redisStr);
    }
    else
    {
        strRedisCmd.append("APPEND ");
        strRedisCmd.append(strRedisCmdKey);
        strRedisCmd.append(" ");
        strRedisCmd.append(to_string(curtime));
        strRedisCmd.append("&");
    }

    LogDebug("[%s:%s:%s] Postmsg to redis, save channel overRate timestring, cmd[%s].",
        pSession->m_reqMsg.m_strClientId.data(),
        pSession->m_reqMsg.m_strSmsId.data(),
        pSession->m_reqMsg.m_strPhone.data(),
        strRedisCmd.data());

    redisSet(strRedisCmd, "SASP_Channel_OverRate", NULL, g_pOverrateRedisThreadPool);
    redisSetTTL(strRedisCmdKey, cleantime, "SASP_Channel_OverRate", g_pOverrateRedisThreadPool);
}

void OverRateThread::increaseRedisGlobalOverRate(Session* pSession, int value)
{
    static UInt32 uiTimeOut = 3600 * 24 * 7;
    string strRedisCmdKey = "SASP_Global_OverRate:" + pSession->m_reqMsg.m_strPhone;

    string strRedisCmd;
    strRedisCmd.append("HINCRBY ");
    strRedisCmd.append(strRedisCmdKey);
    strRedisCmd.append(" ");
    strRedisCmd.append(pSession->m_reqMsg.m_strC2sDate.substr(0, 8));
    strRedisCmd.append(" ");
    strRedisCmd.append(to_string(value));

    redisSet(strRedisCmd, "SASP_Global_OverRate", NULL, g_pOverrateRedisThreadPool);
    redisSetTTL(strRedisCmdKey, uiTimeOut, "SASP_Global_OverRate", g_pOverrateRedisThreadPool);
}

void OverRateThread::decreaseRedisGlobalOverRate(Session* pSession)
{
    if (1 == pSession->m_globalOverRateRule.m_enable)
    {
        LogWarn("[%s:%s:%s] rollback global over rate count.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());

        increaseRedisGlobalOverRate(pSession, -1);
    }
}

bool OverRateThread::OverRateKeywordRegex(Session* pSession, string& strOut)
{
    CHK_NULL_RETURN_FALSE(pSession);

    string strContentTemp = pSession->m_reqMsg.m_strContent;
    string strSignData;

    if (pSession->m_reqMsg.m_bIncludeChinese)
    {
        static const string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
        static const string strRight = http::UrlCode::UrlDecode("%e3%80%91");

        strSignData = strLeft + pSession->m_reqMsg.m_strSign + strRight;
    }
    else
    {
        strSignData = "[" + pSession->m_reqMsg.m_strSign + "]";
    }

    Comm::ToLower(strSignData);
    Comm::ToLower(strContentTemp);

    LogDebug("[%s:%s:%s] strContentTemp[%s], strSignData[%s].",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        strContentTemp.data(),
        strSignData.data());

    map<string, searchTree*>::iterator itrClient = m_OverRateKeyWordMap.find(pSession->m_reqMsg.m_strClientId);
    if (m_OverRateKeyWordMap.end() != itrClient)
    {
        LogDebug("Find %s.", pSession->m_reqMsg.m_strClientId.data());

        searchTree* pTree = itrClient->second;

        if (pTree->searchSign(strSignData, strOut))
            return true;

        if (pTree->search(strContentTemp, strOut))
            return true;
    }

    map<string, searchTree*>::iterator itrPlatform = m_OverRateKeyWordMap.find("*");
    if (m_OverRateKeyWordMap.end() != itrPlatform)
    {
        LogDebug("Find *.");

        searchTree* pTree = itrPlatform->second;

        if (pTree->searchSign(strSignData, strOut))
            return true;

        if (pTree->search(strContentTemp, strOut))
            return true;
    }

    LogDebug("not find.");

    return false;
}

void OverRateThread::handleOverRateSetReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    OverRateSetReqMsg* pReq = (OverRateSetReqMsg*)pMsg;

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    if (pSession->m_bInOverRateWhiteList)
    {
        LogDebug("[%s:%s:%s] InOverRateWhiteList, no need set over rate.",
            pSession->m_reqMsg.m_strClientId.data(),
            pSession->m_reqMsg.m_strSmsId.data(),
            pSession->m_reqMsg.m_strPhone.data());
    }
    else
    {
        if (0 == pReq->m_uiState)
        {
            // 后续逻辑成功时，需要设置超频计数
            setKeywordOverRateRedis(pSession);
            setSmsTypeOverRateRedis(pSession);
            setChannelOverRateRedis(pSession);
        }
        else
        {
            // 后续逻辑失败时，需要回滚全局超频计数
            decreaseRedisGlobalOverRate(pSession);
        }
    }

    // 删除会话
    SAFE_DELETE(pSession);
}

void OverRateThread::setKeywordOverRateRedis(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    LogDebug("[%s:%s:%s] m_strKeyWordOverRateKey[%s], m_enable[%u]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        pSession->m_strKeyWordOverRateKey.c_str(),
        pSession->m_keywordOverRateRule.m_enable);

    if (!pSession->m_strKeyWordOverRateKey.empty())
    {
        if ((-1 != pSession->m_iKeyWordOverRateKeyType) && (1 == pSession->m_keywordOverRateRule.m_enable))
        {
            setOverRateRedis(pSession->m_strKeyWordOverRateKey, "", false);
        }
    }
}

void OverRateThread::setSmsTypeOverRateRedis(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    LogDebug("[%s:%s:%s] m_strSmsTypeOverRateKey[%s], m_enable[%u]",
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        pSession->m_strSmsTypeOverRateKey.data(),
        pSession->m_smsTypeOverRateRule.m_enable);

    if (!pSession->m_strSmsTypeOverRateKey.empty())
    {
        if (1 == pSession->m_smsTypeOverRateRule.m_enable)
        {
            setOverRateRedis(pSession->m_strSmsTypeOverRateKey, "", false);
        }
    }
}

void OverRateThread::setChannelOverRateRedis(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    if (1 == pSession->m_channelOverRateRule.m_enable)
    {
        int cleantime = 3600 * 24 * 7;

        string strRedisCmdKey;
        strRedisCmdKey.append("SASP_Channel_Day_OverRate:");
        strRedisCmdKey.append(pSession->m_strChannelOverRateKey);

        string strRedisCmd;
        strRedisCmd.append("HINCRBY ");
        strRedisCmd.append(strRedisCmdKey);
        strRedisCmd.append(" ");
        strRedisCmd.append(pSession->m_strCurrentDay);
        strRedisCmd.append(" 1");

        redisSet(strRedisCmd, "SASP_Channel_Day_OverRate:", NULL, g_pOverrateRedisThreadPool);
        redisSetTTL(strRedisCmdKey, cleantime, "SASP_Channel_Day_OverRate:", g_pOverrateRedisThreadPool);

        checkRedisChannelOverRateSetRedis(pSession, "", false);
    }
}

void OverRateThread::setOverRateRedis(cs_t strRedisCmdKey, cs_t redisStr, bool bReset)
{
    long curtime = time(NULL);
    struct tm tblock = {0};
    localtime_r(&curtime, &tblock);
    int h = tblock.tm_hour;
    int cleantime = (24 - h) * 60 * 60 - tblock.tm_min * 60 - tblock.tm_sec ;

    string strRedisCmd;
    if (bReset == true)
    {
        strRedisCmd.append("SET ");
        strRedisCmd.append(strRedisCmdKey);
        strRedisCmd.append(" ");
        strRedisCmd.append(redisStr);
    }
    else
    {
        strRedisCmd.append("APPEND ");
        strRedisCmd.append(strRedisCmdKey);
        strRedisCmd.append(" ");
        strRedisCmd.append(to_string(curtime));
        strRedisCmd.append("&");
    }

    redisSet(strRedisCmd, strRedisCmdKey, NULL, g_pOverrateRedisThreadPool);
    redisSetTTL(strRedisCmdKey, cleantime, strRedisCmdKey, g_pOverrateRedisThreadPool);
}

void OverRateThread::smsChannelDayOverRateDescRedis(string strKey, int value)
{
	int cleantime = 3600 * 24 * 7;

	string key;
	std::size_t i = strKey.find(" ");
	if (i != std::string::npos)
		key = strKey.substr(0, i);

	TRedisReq* req = new TRedisReq();
	req->m_iMsgType = MSGTYPE_REDIS_REQ;
	req->m_strKey = key;
	req->m_RedisCmd = "HINCRBY ";
	req->m_RedisCmd.append(strKey);
	req->m_RedisCmd.append(" " + Comm::int2str(value));
	SelectRedisThreadPoolIndex(g_pOverrateRedisThreadPool, req);
	LogDebug("redis cmd: %s", req->m_RedisCmd.data());
	
	
	// 更新老化时间
	TRedisReq* reqtime = new TRedisReq();
	reqtime->m_strKey = key ;
	reqtime->m_RedisCmd = "EXPIRE ";
	reqtime->m_RedisCmd.append(key);
	reqtime->m_RedisCmd.append(" " + Comm::int2str(cleantime));
	reqtime->m_iMsgType = MSGTYPE_REDIS_REQ;
	SelectRedisThreadPoolIndex(g_pOverrateRedisThreadPool,reqtime);	
	
}

void OverRateThread::handleTimeout(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    if (TIMER_ID_CLEAN_OVERRATEMAP == pMsg->m_iSeq)
    {
        LogNotice("release overRate cache.");

        m_pOverRate->cleanOverRateMap();
        m_pOverRate->cleanKeyWordOverRateMap();
        m_pOverRate->cleanChannelOverRateMap();

        SAFE_DELETE(m_pCleanOverRateTimer);
        m_pCleanOverRateTimer = SetTimer(TIMER_ID_CLEAN_OVERRATEMAP, "TIMER_CLEAN_OVERRATEMAP", SECONDS_OF_ONE_DAYA);
    }
}

void OverRateThread::getKeyWordOverRateKey(Session* pSession)
{
    CHK_NULL_RETURN(pSession);
    
    unsigned char md5sign[16] = {0};
    unsigned char md5star[16] = {0};
    unsigned char star[2] = "*";
    string key;
    std::string HEX_CHARS = "0123456789abcdef";
    string strMd5Sign;

    if (pSession->m_iKeyWordOverRateKeyType <= OVERRATE_KEYWORD_RULE_STAR_SIGN)
    {
        MD5((const unsigned char*) pSession->m_reqMsg.m_strSign.c_str(), pSession->m_reqMsg.m_strSign.length(), md5sign);

        for (int i = 0; i < 16; i++)
        {
            strMd5Sign.append(1, HEX_CHARS.at(md5sign[i] >> 4 & 0x0F));
            strMd5Sign.append(1, HEX_CHARS.at(md5sign[i] & 0x0F));
        }
    }

    if (OVERRATE_KEYWORD_RULE_USER_SIGN == pSession->m_iKeyWordOverRateKeyType)
    {
        key = "SASP_KeyWord_OverRate:"
            + pSession->m_reqMsg.m_strClientId
            + "&"
            + strMd5Sign
            + "&"
            + pSession->m_reqMsg.m_strPhone;
    }
    else if (OVERRATE_KEYWORD_RULE_STAR_SIGN == pSession->m_iKeyWordOverRateKeyType)
    {
        key = "SASP_KeyWord_OverRate:"
            + string("*")
            + "&"
            + strMd5Sign
            + "&"
            + pSession->m_reqMsg.m_strPhone;
    }
    else if (OVERRATE_KEYWORD_RULE_USER_STAR == pSession->m_iKeyWordOverRateKeyType)
    {
        MD5((const unsigned char*) star, strlen((char*)star), md5star);
        string strMd5Star;

        for (int i = 0; i < 16; i++)
        {
            strMd5Star.append(1, HEX_CHARS.at(md5star[i] >> 4 & 0x0F));
            strMd5Star.append(1, HEX_CHARS.at(md5star[i] & 0x0F));
        }

        key = "SASP_KeyWord_OverRate:"
            + pSession->m_reqMsg.m_strClientId
            + "&"
            + strMd5Star
            + "&"
            + pSession->m_reqMsg.m_strPhone;
    }

    pSession->m_strKeyWordOverRateKey = key;

    LogDebug("KeyWordOverRateKey[%s].", pSession->m_strKeyWordOverRateKey.data());
}

void OverRateThread::getSmsTypeOverRateKey(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    pSession->m_strSmsTypeOverRateKey = "SASP_OverRate:"
                                        + pSession->m_reqMsg.m_strClientId
                                        + "&"
                                        + pSession->m_reqMsg.m_strPhone
                                        + "&"
                                        + pSession->m_reqMsg.m_strSmsType;

    LogDebug("SmsTypeOverRateKey[%s].", pSession->m_strSmsTypeOverRateKey.data());
}

void OverRateThread::sendOverRateCheckRspMsg(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    LogDebug("[%s:%s:%s] CheckOverRateState[%d].", 
        pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
        pSession->m_eCheckOverRateState);

    switch (pSession->m_eCheckOverRateState)
    {
        case Session::CheckGlobalOverRate:
        {
            pSession->m_eOverRateResult = OverRateGlobal;
            pSession->m_strErrorCode = GLOBAL_OVER_RATE;
            break;
        }
        case Session::CheckKeywordOverRate:
        {
            pSession->m_eOverRateResult = OverRateKeyword;
            pSession->m_strErrorCode = KEYWORD_OVER_RATE;
            break;
        }
        case Session::CheckSmsTypeOverRate:
        {
            pSession->m_eOverRateResult = OverRateSmsType;
            pSession->m_strErrorCode = OVER_RATE;
            break;
        }
        case Session::CheckChannelOverRate:
        {
            pSession->m_eOverRateResult = OverRateChannel;
            pSession->m_strErrorCode = CHANNEL_OVER_RATE;
            break;
        }
        case Session::CheckOverRateOk:
        {
            pSession->m_eOverRateResult = OverRateNone;
            break;
        }
        default:
        {
            LogError("[%s:%s:%s] Invalid CheckOverRateState[%d].", 
                pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str(),
                pSession->m_eCheckOverRateState);
            break;
        }
    }

    sendOverRateCheckRspMsgEx(pSession);

    if (OverRateNone != pSession->m_eOverRateResult)
    {
        // 除全局超频，其他超频时均需要roll back全局超频计数
        if (OverRateGlobal != pSession->m_eOverRateResult)
        {
            decreaseRedisGlobalOverRate(pSession);
        }

        LogDebug("[%s:%s:%s] delete session.",  
            pSession->m_reqMsg.m_strClientId.c_str(), pSession->m_reqMsg.m_strSmsId.c_str(), pSession->m_reqMsg.m_strPhone.c_str());
        
        // 超频时，删除会话
        SAFE_DELETE(pSession);

    }
}

void OverRateThread::sendOverRateCheckRspMsgEx(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    OverRateCheckRspMsg* pRsp = new OverRateCheckRspMsg();
    CHK_NULL_RETURN(pRsp);

    pRsp->m_iMsgType = MSGTYPE_OVERRATE_CHECK_RSP;
    pRsp->m_iSeq = pSession->m_reqMsg.m_iSeq;
    pRsp->m_eOverRateResult = pSession->m_eOverRateResult;
    pRsp->m_strErrCode = pSession->m_strErrorCode;
    pRsp->m_ulOverRateSessionId = pSession->m_ulSequence;
    pRsp->m_strChannelOverRateKeyall = pSession->m_strChannelOverRateKeyall;
    g_pSessionThread->PostMsg(pRsp);
}

