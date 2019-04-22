#include "UrlCode.h"
#include "UTFString.h"
#include "Comm.h"
#include "base64.h"
#include "CotentFilter.h"
#include "CSessionThread.h"
#include "LogMacro.h"
#include "CChargeThread.h"
#include "CBlackListThread.h"
#include "Channellib.h"
#include "CRedisThread.h"
#include "CMQProducerThread.h"
#include "main.h"
#include "HttpParams.h"
#include "BusTypes.h"
#include "global.h"
#include "RuleLoadThread.h"


using namespace BusType;


CSessionThread* g_pSessionThread = NULL;


enum PushBackToMQType
{
    MQ_PUSH_REPORT = 0,
    MQ_PUSH_OVERFLOW = 1,         //通道限速
    MQ_PUSH_AUDITPASS = 2,        //审核通过
    MQ_PUSH_IOREBACK = 3,
};

//TODO check input msg
#define CHECK_PARAM_NULL_RETURN( pmsg ) \
    do \
    {  \
        if( NULL == pmsg ) \
        { \
            LogWarn("%s,Param Error\n", #pmsg );\
            return ACCESS_ERROR_PARAM_NULL; \
        }\
    }\
    while (0)


bool startSessionThread()
{
    g_pSessionThread = new CSessionThread("SessionThread");
    INIT_CHK_NULL_RET_FALSE(g_pSessionThread);
    INIT_CHK_FUNC_RET_FALSE(g_pSessionThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pSessionThread->CreateThread());
    return true;
}

CSessionThread::CSessionThread(const char* name) : CThread(name)
{
    m_pThreadTimer = NULL;
    m_mapAccount.clear();
    m_componentConfigInfo.clear();
    m_mqConfigInfo.clear();
    m_uCurSessionSize = 0;
    m_iOverCount = 20;
    m_uOverTime = 86400;
}

CSessionThread::~CSessionThread()
{
}

bool CSessionThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());


    m_pCChineseConvert = new CChineseConvert();
    INIT_CHK_NULL_RET_FALSE(m_pCChineseConvert);
    INIT_CHK_FUNC_RET_FALSE(m_pCChineseConvert->Init());


    m_honeDao.Init();

    m_pSnMng = new SnManager();
    INIT_CHK_NULL_RET_FALSE(m_pSnMng);


    m_uAccessId = g_uComponentId;
    m_pBlackListThread = new CBlackListThread("CBlackList");

    if (false == static_cast<CBlackListThread*>(m_pBlackListThread)->Init())
    {
        printf("Init CBlackListThread failed");
        return false;
    }

    m_pChargeThread = new CChargeThread("CCharge");

    if (false == static_cast< CChargeThread*>(m_pChargeThread)->Init())
    {
        printf("Init ChargeThread failed ");
        return false;
    }

    /*计费http响应内存池*/
    m_pLinkedBlockPool = new LinkedBlockPool();

    m_uSleepTime = g_uSecSleep;

    g_pRuleLoadThread->getSmsAccountMap(m_mapAccount);
    g_pRuleLoadThread->getComponentConfig(m_componentConfigInfo);
    g_pRuleLoadThread->getMqConfig(m_mqConfigInfo);
    g_pRuleLoadThread->getComponentRefMq(m_mqConfigRef);
    g_pRuleLoadThread->getChannlelMap(m_mapChannelInfo);
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);
    g_pRuleLoadThread->getPhoneAreaMap(m_mapPhoneArea);
    g_pRuleLoadThread->getClientPriorities(m_clientPriorities);
    g_pRuleLoadThread->getChannelPropertyLog(m_channelPropertyLogMap);
    g_pRuleLoadThread->getUserPriceLog(m_userPriceLogMap);
    g_pRuleLoadThread->getUserPropertyLog(m_userPropertyLogMap);
    g_pRuleLoadThread->getOverRateWhiteList(m_setOverRateWhiteList);

    GET_RULELOAD_DATA_MAP(t_sms_client_send_limit, m_mapClientSendLimitCfg);
    GET_RULELOAD_DATA_VECMAP(t_sms_user_property_log, m_mapVecUserProperty);

    m_pThreadTimer = SetTimer(0, ACCESS_STATISTIC_TIMER_ID, ACCESS_STATISTIC_TIMEOUT);
    return true;
}

bool CSessionThread::checkOverRateWhiteList(session_pt pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    ////clientid_
    string strClientKey = "";
    strClientKey.assign(pSession->m_strClientId);
    strClientKey.append("_");

    set<string>::iterator itrClient = m_setOverRateWhiteList.find(strClientKey);

    if (itrClient != m_setOverRateWhiteList.end())
    {
        LogDebug("[%s:%s:%s] Find in the over rate white list.",
                 pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
        pSession->m_bInOverRateWhiteList = true;

        return true;
    }

    ////client_phone
    string strClientPhone;
    strClientPhone.assign(pSession->m_strClientId);
    strClientPhone.append("_");
    strClientPhone.append(pSession->m_strPhone);

    set<string>::iterator itrPhone = m_setOverRateWhiteList.find(strClientPhone);

    if (itrPhone != m_setOverRateWhiteList.end())
    {
        LogDebug("[%s:%s:%s] Find in the over rate white list.",
                 pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
        pSession->m_bInOverRateWhiteList = true;

        return true;
    }

    LogDebug("[%s:%s:%s] Not find in the over rate white list.",
             pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());

    return false;
}

bool CSessionThread::checkSendControlWhiteList(session_pt pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    ////client_phone
    string strClientPhone;
    strClientPhone.assign(pSession->m_strClientId);
    strClientPhone.append("_");
    strClientPhone.append(pSession->m_strPhone);

    set<string>::iterator itrPhone = m_setOverRateWhiteList.find(strClientPhone);

    if (itrPhone != m_setOverRateWhiteList.end())
    {
        LogDebug("[%s:%s:%s] Find in the over rate white list.",
                 pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
        pSession->m_bSendControlOverRateWhiteList = true;

        return true;
    }

    LogDebug("[%s:%s:%s] Not find in the over rate white list.",
             pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());

    return false;
}

void CSessionThread::initGetSysPara(constSysParamMap& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "REBACK_TIME_OVER";
        iter = mapSysPara.find(strSysPara);

        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        std::size_t pos = strTmp.find_last_of(";");

        if (std::string::npos == pos)
        {
            LogError("Invalid system parameter(%s) value(%s).",
                strSysPara.c_str(), strTmp.c_str());
            break;
        }

        int iOverTime = atoi(strTmp.substr(0, pos).data());
        int iOverCount = atoi(strTmp.substr(pos + 1).data());

        if ((0 > iOverTime) || (0 > iOverCount))
        {
            LogError("Invalid system parameter(%s) value(%s, %d, %d).",
                strSysPara.c_str(),
                strTmp.c_str(),
                iOverTime,
                iOverCount);
            break;
        }

        m_uOverTime = iOverTime;
        m_iOverCount = iOverCount;
    } while (0);

    LogNotice("System parameter(%s) value(%d, %d).",
        strSysPara.c_str(),
        m_uOverTime,
        m_iOverCount);

}

void CSessionThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (true)
    {
        m_pTimerMng->Click();
        bool bSelect = static_cast< CChargeThread*>(m_pChargeThread)->smsChargeSelect();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if (pMsg == NULL && bSelect == false)
        {
            usleep(m_uSleepTime);
        }
        else if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            SAFE_DELETE(pMsg);
        }
    }
}

void CSessionThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CAdapter::InterlockedIncrement((long*)&m_iCount);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_MQ_GETMQMSG_REQ:
            HandleMQReqMsg(pMsg);
            break;

        case MSGTYPE_MQ_GETMQREBACKMSG_REQ:
            HandleMQRebackReqMsg(pMsg);
            break;

        case MSGTYPE_HTTPRESPONSE:
            HandleStatusRpt(pMsg, STATE_CHARGE);
            break;

        case MSGTYPE_REDIS_RESP:
            HandleStatusRpt(pMsg, STATE_BLACKLIST);
            break;

        case MSGTYPE_TEMPLATE_RESP_TO_SEESION:
            HandleStatusRpt(pMsg,  STATE_TEMPLATE);
            break;

        case MSGTYPE_ROUTER_RESP_TO_SEESION:
            HandleStatusRpt(pMsg,  STATE_ROUTE);
            break;

        case MSGTYPE_AUDIT_RSP:
            HandleStatusRpt(pMsg,  STATE_AUDIT);
            break;

        case MSGTYPE_OVERRATE_CHECK_RSP:
            HandleStatusRpt(pMsg,  STATE_OVERRATE);
            break;

        case MSGTYPE_TIMEOUT:
            HandleTimeOut(pMsg);
            break;

        case MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ:
        {
            TUpdateOperaterSegmentReq* pOperater = (TUpdateOperaterSegmentReq*)pMsg;
            m_honeDao.m_OperaterSegmentMap.clear();
            m_honeDao.m_OperaterSegmentMap = pOperater->m_OperaterSegmentMap;
            LogNotice("RuleUpdate t_sms_operater_segment update. size[%d]", m_honeDao.m_OperaterSegmentMap.size());
            break;
        }

        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            TUpdateSmsAccontReq* pAccountUpdateReq = (TUpdateSmsAccontReq*)pMsg;
            m_mapAccount.clear();
            m_mapAccount = pAccountUpdateReq->m_SmsAccountMap;
            LogNotice("RuleUpdate account update size:%u!", m_mapAccount.size());

            //更新计费模块信息
            static_cast<CChargeThread*>(m_pChargeThread)->HandleMsg(pMsg);
            break;
        }

        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)(pMsg);
            m_mqConfigInfo.clear();
            m_mqConfigInfo = pMqCon->m_mapMqConfig;
            LogNotice("update t_sms_mq_configure size:%d.", m_mqConfigInfo.size());
            break;
        }

        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            TUpdateComponentConfigReq* pComponetConfig = (TUpdateComponentConfigReq*)(pMsg);
            m_componentConfigInfo.clear();
            m_componentConfigInfo = pComponetConfig->m_componentConfigInfoMap;
            LogNotice("update t_sms_component size:%d", m_componentConfigInfo.size());
            break;
        }

        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ :
        {
            TUpdateComponentRefMqReq* pMqConfigRef = (TUpdateComponentRefMqReq*)(pMsg);
            m_mqConfigRef = pMqConfigRef->m_componentRefMqInfo ;
            LogNotice("update t_sms_component_ref_mq size:%d", m_mqConfigRef.size());
            break;
        }

        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            TUpdateChannelReq* msg = (TUpdateChannelReq*)pMsg;
            m_mapChannelInfo.clear();
            m_mapChannelInfo = msg->m_ChannelMap;
            LogNotice("RuleUpdate channelMap update. map.size[%d]", msg->m_ChannelMap.size());
            break;
        }

        case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
        {
            TUpdateSystemErrorCodeReq* pSysError = (TUpdateSystemErrorCodeReq*)pMsg;
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pSysError->m_mapSystemErrorCode;
            LogNotice("RuleUpdate SysErroCode update. map.size[%d]", pSysError->m_mapSystemErrorCode.size());
            break;
        }

        //系统参数检查
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            static_cast<CBlackListThread*>(m_pBlackListThread)->HandleMsg(pMsg);
            break;
        }

        case MSGTYPE_RULELOAD_BLGRP_REF_CATEGORY_UPDATE_REQ:
        case MSGTYPE_RULELOAD_BLUSER_CONFIG_GROUP_UPDATE_REQ:
        {
            static_cast<CBlackListThread*>(m_pBlackListThread)->HandleMsg(pMsg);
            break;
        }

        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            static_cast<CBlackListThread*>(m_pBlackListThread)->HandleMsg(pMsg);
            break;
        }

        //计费模块处理的配置变更信息
        case MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ:
        case MSGTYPE_RULELOAD_AGENT_ACCOUNT_UPDATE_REQ:
            static_cast<CChargeThread*>(m_pChargeThread)->HandleMsg(pMsg);
            break;

        case MSGTYPE_RULELOAD_PHONE_AREA_UPDATE_REQ :
        {
            TUpdatePhoneAreaReq* pMsgSession = (TUpdatePhoneAreaReq*)pMsg;
            m_mapPhoneArea.clear();
            m_mapPhoneArea = pMsgSession->m_PhoneAreaMap;
            LogNotice("RuleUpdate m_mapPhoneArea update. map.size[%d]", m_mapPhoneArea.size());
            break;
        }

        case MSGTYPE_RULELOAD_CLIENT_PRIORITY_UPDATE_REQ:
        {
            TUpdateClientPriorityReq* pReq = (TUpdateClientPriorityReq*) pMsg;
            m_clientPriorities = pReq->m_clientPriorities;
            LogNotice("update t_sms_client_priority");
            break;
        }

        case MSGTYPE_RULELOAD_CHANNEL_PROPERTY_LOG_UPDATE_REQ:
        {
            TUpdateChannelPropertyLogReq* pReq = (TUpdateChannelPropertyLogReq*) pMsg;
            m_channelPropertyLogMap = pReq->m_channelPropertyLogMap;

            LogNotice("update t_sms_channel_property_log");
            break;
        }

        case MSGTYPE_RULELOAD_USER_PRICE_LOG_UPDATE_REQ:
        {
            TUpdateUserPriceLogReq* pReq = (TUpdateUserPriceLogReq*) pMsg;
            m_userPriceLogMap = pReq->m_userPriceLogMap;

            LogNotice("update t_sms_user_price_log");
            break;
        }

        case MSGTYPE_RULELOAD_USER_PROPERTY_LOG_UPDATE_REQ:
        {
            TUpdateUserPropertyLogReq* pReq = (TUpdateUserPropertyLogReq*) pMsg;
            m_userPropertyLogMap = pReq->m_userPropertyLogMap;
            LogNotice("update t_sms_user_property_log");
            break;
        }

        case MSGTYPE_RULELOAD_OVERRATEWHITELIST_UPDATE_REQ:
        {
            TUpdateOverRateWhiteListReq* msg = (TUpdateOverRateWhiteListReq*)pMsg;
            m_setOverRateWhiteList = msg->m_overRateWhiteList;
            LogNotice("RuleUpdate OVERRATEWHITELIST update. set.size[%u]", msg->m_overRateWhiteList.size());
            break;
        }

        case MSGTYPE_RULELOAD_DB_UPDATE_REQ:
        {
            handleDbUpdateReq(pMsg);
            break;
        }

        default:
            LogWarn("msgType[0x:%x] is invalid.", pMsg->m_iMsgType);
            break;
    }
}

void CSessionThread::handleDbUpdateReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    DbUpdateReq* pReq = (DbUpdateReq*)pMsg;

    switch (pReq->m_uiTableId)
    {
        case t_sms_client_send_limit:
        {
            clearSendLimit(pReq->m_mapAppData);
            break;
        }

        CASE_UPDATE_TABLE_VECMAP(t_sms_user_property_log, m_mapVecUserProperty);

        default:
        {
            LogError("Invalid TableId[%u].", pReq->m_uiTableId);
            break;
        }
    }
}

UInt32 CSessionThread::nextState(session_pt pSession)
{
    //已经审核完成，无需再过黑名单，智能模板，审核
    if ((pSession->m_uSessionState < STATE_ROUTE) && (1 == pSession->m_uAuditFlag))
    {
        LogNotice("[%s:%s:%s] [%s-->%s] been audited before !!!",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            state2String(pSession->m_uSessionState).c_str(),
            state2String(STATE_ROUTE).c_str());

        return STATE_ROUTE;
    }

    //测试短信过系统关键字，不过审核
    if (STATE_TEMPLATE == pSession->m_uSessionState)
    {
        if (!pSession->m_strTestChannelId.empty())
        {
            return STATE_ROUTE;
        }
    }

    if (STATE_BLACKLIST == pSession->m_uSessionState && pSession->m_iClientSendLimitCtrlTypeFlag == BLACKLIST_CONTROL)
    {
        LogDebug("after get blacklist, will audit");
        return STATE_AUDIT;
    }
    if (STATE_ROUTE == pSession->m_uSessionState && pSession->m_iClientSendLimitCtrlTypeFlag > NOT_CONTROL)
    {
        LogDebug("after get route, will get charge");
        return STATE_CHARGE;
    }

    if (STATE_BLACKLIST == pSession->m_uSessionState && pSession->m_uFromSmsReback == 1)
    {
        LogDebug("from reback,after get blacklist, will get route");
        return STATE_ROUTE;
    }

    if (STATE_OVERRATE == pSession->m_uSessionState && pSession->m_uFromSmsReback == 1)
    {
        LogDebug("from reback,after get blacklist, will get route");
        return STATE_DONE;
    }

    return pSession->m_uSessionState + 1 ;

}

UInt32 CSessionThread::HandleStatusRpt(TMsg* pMsg, UInt32 uState)
{
    CHECK_PARAM_NULL_RETURN(pMsg);

    UInt32 uRet = ACCESS_ERROR_NONE;

    session_pt pSession = GetSessionBySeqID(pMsg->m_iSeq);

    if (NULL == pSession)
    {
        if (STATE_CHARGE == uState)
        {
            THttpResponseMsg* pHttpResponse = (THttpResponseMsg*)pMsg;

            if (THttpResponseMsg::HTTP_CLOSED == pHttpResponse->m_uStatus)
            {
                return ACCESS_ERROR_SESSION_NOT_EXSIT;
            }
        }

        LogWarn("status rpt session[ msgtype:0x%x, iseq:%u, session:%s, State:%s ] not find ",
            pMsg->m_iMsgType,
            pMsg->m_iSeq,
            pMsg->m_strSessionID.data(),
            state2String(uState).data());

        return ACCESS_ERROR_SESSION_NOT_EXSIT;
    }

    pSession->m_apiTrace->End(uState);

    switch (uState)
    {
        case STATE_BLACKLIST:
            uRet = sessionBlackListReport(pSession,  pMsg);
            break;

        case STATE_TEMPLATE:
            uRet = sessionTemplateReport(pSession, pMsg);
            break;

        case STATE_AUDIT:
            uRet = sessionAuditReport(pSession, pMsg);
            break;

        case STATE_ROUTE:
            uRet = sessionRouterReport(pSession, pMsg);
            break;

        case STATE_OVERRATE:
            uRet = sessionOverRateReport(pSession, pMsg);
            break;

        case STATE_CHARGE:
            uRet = sessionChargeReport(pSession, pMsg);
            break;

        case STATE_INIT:
        case STATE_DONE:
            uRet = ACCESS_ERROR_SESSION_STAT_WRONG;
            break;

        default:
            uRet = ACCESS_ERROR_SESSION_STAT_WRONG;
            break;
    }

    LogInfo("[%s:%s:%s] [%s] [ SessionId:%lu, Ret:%u ]",
        pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
        state2String(uState).c_str(), pSession->m_uSessionId, uRet);

    pSession->m_apiTrace->setError(uState, uRet);

    switch (uRet)
    {
        //成功继续走下一流程
        case ACCESS_ERROR_NONE:
            uRet = sessionStateProc(pSession, nextState(pSession));
            break;

        //通道超频需要重新去选通道
        case ACCESS_ERROR_OVERRATE_CHANNEL:
            pSession->m_uChannleId = 0;
            uRet = sessionStateProc(pSession, STATE_ROUTE);
            break;

        case ACCESS_ERROR_OVERRATE_CHARGE:
            uRet = sessionStateProc(pSession, STATE_CHARGE);
            break;

        //其余失败流程结束
        default:
            uRet = sessionStateProc(pSession, STATE_DONE);
    }

    return uRet ;

}


UInt32 CSessionThread::sessionStateProc(session_pt pSession, UInt32 uState)
{
    UInt32 uRet = ACCESS_ERROR_NONE;

    CHECK_PARAM_NULL_RETURN(pSession);

    string stateStr = state2String(uState);
    string preState = state2String(pSession->m_uSessionState);

    LogInfo("[%s:%s:%s] [%s-->%s] [SessionId:%lu].",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        preState.c_str(),
        stateStr.c_str(),
        pSession->m_uSessionId);

    pSession->m_uSessionPreState = pSession->m_uSessionState;
    pSession->m_uSessionState = uState;
    pSession->m_apiTrace->Begin(uState);

    switch (uState)
    {
        case STATE_INIT:
            uRet = sessionPreProcess(pSession);
            break;

        case STATE_BLACKLIST:
            uRet = sessionBlackList(pSession);
            break;

        case STATE_TEMPLATE:
            uRet = sessionTemplate(pSession);
            break;

        case STATE_AUDIT:
            uRet = sessionAudit(pSession);
            break;

        case STATE_ROUTE:
            uRet = sessionRouter(pSession);
            break;

        case STATE_OVERRATE:
            uRet = sessionOverRate(pSession, OVERRATE_CHECK);
            break;

        case STATE_CHARGE:
            uRet = sessionCharge(pSession);
            break;

        case STATE_DONE:
            uRet = sessionDone(pSession);
            break;

        default:
            LogWarn("[ session state ] unKown State %d ", uState);

    }

    LogDebug("SessionId[%lu], uRet[%u].", pSession->m_uSessionId, uRet);

    if (ACCESS_ERROR_NEXT_STATE == uRet)
    {
        return sessionStateProc(pSession, nextState(pSession));
    }

    if (ACCESS_ERROR_NONE != uRet)
    {
        pSession->m_apiTrace->setError(uState, uRet);
        return sessionStateProc(pSession, STATE_DONE);
    }

    return ACCESS_ERROR_NONE;

}

void CSessionThread::setSessionSendUrl(session_pt pSession, string& strSendUrl)
{
    strSendUrl.assign("phone=");
    strSendUrl.append(pSession->m_strPhone);
    strSendUrl.append("&clientId=");
    strSendUrl.append(pSession->m_strClientId);
    strSendUrl.append("&smsId=");
    strSendUrl.append(pSession->m_strSmsId);
    strSendUrl.append("&userName=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strUserName));
    strSendUrl.append("&content=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strContent));
    strSendUrl.append("&sid=");
    strSendUrl.append(pSession->m_strSid);

    strSendUrl.append("&smsfrom=");
    strSendUrl.append(Comm::int2str(pSession->m_uSmsFrom));

    strSendUrl.append("&smsType=");
    strSendUrl.append(pSession->m_strSmsType);

    strSendUrl.append("&paytype=");
    strSendUrl.append(Comm::int2str(pSession->m_uPayType));

    strSendUrl.append("&sign=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strSign));

    strSendUrl.append("&userextpendport=");
    strSendUrl.append(pSession->m_strExtpendPort);
    strSendUrl.append("&signextendport=");
    strSendUrl.append(pSession->m_strSignPort);

    strSendUrl.append("&showsigntype=");
    strSendUrl.append(Comm::int2str(pSession->m_uShowSignType));
    strSendUrl.append("&accessid=");
    strSendUrl.append(Comm::int2str(m_uAccessId));
    strSendUrl.append("&csid=");
    strSendUrl.append(pSession->m_strCSid);
    strSendUrl.append("&csdate=");
    strSendUrl.append(pSession->m_strCSdate);

    strSendUrl.append("&area=");
    char cArea[10] = {0};
    snprintf(cArea, 10, "%d", pSession->m_uArea);
    strSendUrl.append(cArea);
    strSendUrl.append("&channelid=");
    char cChannelId[25] = {0};
    snprintf(cChannelId, 25, "%d", pSession->m_uChannleId);
    strSendUrl.append(cChannelId);
    strSendUrl.append("&salefee=");
    char cSaleFee[25] = {0};
    snprintf(cSaleFee, 25, "%lf", pSession->m_uSaleFee);
    strSendUrl.append(cSaleFee);
    strSendUrl.append("&costfee=");
    char cCostFee[25] = {0};
    snprintf(cCostFee, 25, "%f", pSession->m_fCostFee);
    strSendUrl.append(cCostFee);
    strSendUrl.append("&ucpaasport=");
    strSendUrl.append(pSession->m_strUcpaasPort);
    strSendUrl.append("&operater=");
    char cOperater[25] = {0};
    snprintf(cOperater, 25, "%d", pSession->m_uOperater);
    strSendUrl.append(cOperater);
    strSendUrl.append("&signport_ex=");
    strSendUrl.append(pSession->m_strSignPort);
    strSendUrl.append("&userextno_ex=");
    strSendUrl.append(pSession->m_strExtpendPort);
    char cShowSignTypeEx[25] = {0};
    snprintf(cShowSignTypeEx, 25, "%u", pSession->m_uShowSignType);
    strSendUrl.append("&showsigntype_ex=");
    strSendUrl.append(cShowSignTypeEx);
    strSendUrl.append("&ids=");
    strSendUrl.append(pSession->m_strIds);
    strSendUrl.append("&process_times=1");
    strSendUrl.append("&oauth_url=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strHttpOauthUrl));////bast64
    strSendUrl.append("&oauth_post_data=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strHttpOauthData));////bast64
    strSendUrl.append("&templateid=").append(pSession->m_strTemplateId);
    strSendUrl.append("&channel_tempid=").append(pSession->m_strChannelTemplateId);
    strSendUrl.append("&temp_params=").append(pSession->m_strTemplateParam);

    if (!pSession->m_strChannelOverRateKeyall.empty())
    {
        strSendUrl.append(pSession->m_strChannelOverRateKeyall);
    }
}

void CSessionThread::setSendUrlFromReback2Send(session_pt pSession, string& strSendUrl)
{
    CHK_NULL_RETURN(pSession);
    strSendUrl.append(pSession->m_strSendUrl);
    strSendUrl.append("&showsigntype_ex=");
    char cShowSignType[25] = {0};
    snprintf(cShowSignType, 25, "%u", pSession->m_uShowSignType);
    strSendUrl.append(cShowSignType);
    ////ucpaasport
    strSendUrl.append("&ucpaasport=");
    strSendUrl.append(pSession->m_strUcpaasPort);
    /////te su chuli signport and userextendport
    ////signport_ex
    strSendUrl.append("&signport_ex=");
    strSendUrl.append(pSession->m_strSignPort);
    ////userextno_ex
    strSendUrl.append("&userextno_ex=");
    strSendUrl.append(pSession->m_strExtpendPort);

    ////accessid
    strSendUrl.append("&accessid=");
    strSendUrl.append(Comm::int2str(g_uComponentId));
    ////area
    strSendUrl.append("&area=");
    strSendUrl.append(Comm::int2str(pSession->m_uArea));
    ////channelid
    strSendUrl.append("&channelid=");
    char cChannelId[25] = {0};
    snprintf(cChannelId, 25, "%d", pSession->m_uChannleId);
    strSendUrl.append(cChannelId);
    ////salefee
    strSendUrl.append("&salefee=");
    char cSaleFee[25] = {0};
    snprintf(cSaleFee, 25, "%lf", pSession->m_uSaleFee);
    strSendUrl.append(cSaleFee);
    ////costfee
    strSendUrl.append("&costfee=");
    char cCostFee[25] = {0};
    snprintf(cCostFee, 25, "%f", pSession->m_fCostFee);
    strSendUrl.append(cCostFee);

    ////httpoauthurl
    strSendUrl.append("&oauth_url=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strHttpOauthUrl));////bast64
    ////httpoauthdata
    strSendUrl.append("&oauth_post_data=");
    strSendUrl.append(smsp::Channellib::encodeBase64(pSession->m_strHttpOauthData));////bast64

    strSendUrl.append("&channel_tempid=").append(pSession->m_strChannelTemplateId);

    if (!pSession->m_strChannelOverRateKeyall.empty())
    {
        strSendUrl.append(pSession->m_strChannelOverRateKeyall);
    }

}

UInt32 CSessionThread::sessionPreProcess(session_pt pSession)
{
    UInt32 itype = OTHER;

    UInt32 uRet = ACCESS_ERROR_NONE;

    CHECK_PARAM_NULL_RETURN(pSession);

    utils::UTFString utfHelper;
    int iContentL = utfHelper.getUtf8Length(pSession->m_strContent);
    int iSignL = utfHelper.getUtf8Length(pSession->m_strSign);

    SmsAccount* account = GetAccountInfo(pSession->m_strClientId, m_mapAccount);

    if (NULL == account)
    {
        pSession->m_strErrorCode = INTERNAL_ERROR;
        pSession->m_strYZXErrCode = INTERNAL_ERROR;
        uRet = ACCESS_ERROR_ACCOUNT_NOT_EXSIT;
        goto GO_EXIT;
    }

    itype = m_honeDao.getPhoneType(pSession->m_strPhone);

    if (itype == BusType::OTHER)
    {
        pSession->m_strErrorCode = PHONE_SEGMENT_ERROR;
        pSession->m_strYZXErrCode = PHONE_SEGMENT_ERROR;
        uRet = ACCESS_ERROR_PHONE_FORMAT;
        goto GO_EXIT;
    }

    pSession->m_uOperater = itype;
    pSession->m_uIdentify = account->m_uIdentify;
    pSession->m_uNeedAudit = account->m_uNeedaudit;
    pSession->m_uAgentId = account->m_uAgentId;
    pSession->m_uPayType = account->m_uPayType;
    pSession->m_strSid = account->m_strSid;
    pSession->m_strUserName = account->m_strname;
    pSession->m_uOverRateCharge = account->m_uOverRateCharge;
    pSession->m_uNeedSignExtend = account->m_uNeedSignExtend;
    pSession->m_uClientAscription = account->m_uClientAscription;
    pSession->m_uSendType = GetSendType(pSession->m_strSmsType);
    pSession->m_uArea = GetPhoneArea(pSession->m_strPhone, m_mapPhoneArea);
    pSession->m_bIncludeChinese = Comm::IncludeChinese(pSession->m_strSign.data()) || Comm::IncludeChinese(pSession->m_strContent.data());

    ////count charge num
    if ((iContentL + iSignL + 2) <= 70)
    {
        pSession->m_uSmsNum = 1;
    }
    else
    {
        pSession->m_uSmsNum = ((iContentL + iSignL + 2) + 66) / 67;
    }

    if (ACCESS_ERROR_NONE != sessionCharge(pSession, CHARGE_POST))
    {
        uRet = ACCESS_ERROR_CHARGE_POST ;
        goto GO_EXIT;
    }

    LogDebug("[SESSION_INIT ][%s:%s:%s] ===> sessionID:%lu, Size:%u",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_uSessionId,
        m_mapSMSSession.size());
    checkOverRateWhiteList(pSession);
    checkSendControlWhiteList(pSession);

    return ACCESS_ERROR_NEXT_STATE;

GO_EXIT:

    LogElk("[SESSION_INIT][%s:%s:%s] ===> Error [%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        error2Str(uRet).c_str());

    if (pSession->m_uState != SMS_STATUS_SEND_FAIL)
    {
        pSession->m_uState  = SMS_STATUS_GW_SEND_HOLD;
    }

    return uRet;
}

UInt32 CSessionThread::sessionBlackList(session_pt pSession)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    LogDebug("[STATE_BLACKLIST REQ][%s:%s:%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str());

    return static_cast<CBlackListThread*>(m_pBlackListThread)->GetRedisBlackListInfo(pSession);
}

UInt32 CSessionThread::sessionBlackListReport(session_pt pSession, TMsg* pMsg)
{
    UInt32 uRet = ACCESS_ERROR_NONE;

    TRedisResp* pRedisResp = (TRedisResp*)pMsg;

    CHECK_PARAM_NULL_RETURN(pSession);
    CHECK_PARAM_NULL_RETURN(pMsg);
        
    if (static_cast<CBlackListThread* >(m_pBlackListThread)->
        CheckBlacklistRedisResp(pRedisResp->m_RedisReply, pSession))
    {
        pSession->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
        uRet =  ACCESS_ERROR_PHONE_BLACK;
        if (BLACKLIST == pSession->m_strErrorCode || USER_BLACKLIST == pSession->m_strErrorCode)
        {
            checkSendLimit(pSession, BLACKLIST_CONTROL);
            if (pSession->m_iClientSendLimitCtrlTypeFlag != BLACKLIST_CONTROL)
                refreshSendLimit(pSession, BLACKLIST_CONTROL);
        }
        
        if (pSession->m_iClientSendLimitCtrlTypeFlag == BLACKLIST_CONTROL 
            && pSession->m_bSendControlOverRateWhiteList == false)//if 超频白名单，需要拦截，不能下发
        {
            uRet = ACCESS_ERROR_NONE;
        }
    }

    if (NULL != pRedisResp->m_RedisReply)
    {
        freeReplyObject(pRedisResp->m_RedisReply);
        pRedisResp->m_RedisReply = NULL;
    }

    return uRet ;

}

UInt32 CSessionThread::sessionTemplate(session_pt pSession)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    TSmsTemplateReq* pTempLateReq = new TSmsTemplateReq();
    pTempLateReq->m_strSmsid    = pSession->m_strSmsId;
    pTempLateReq->m_strPhone    = pSession->m_strPhone;
    pTempLateReq->m_strClientId = pSession->m_strClientId ;
    pTempLateReq->m_strSmsType = pSession->m_strSmsType ;
    pTempLateReq->m_strContent = pSession->m_strContent ;
    pTempLateReq->m_strSign = pSession->m_strSign ;
    pTempLateReq->m_strSignSimple = pSession->m_strSignSimple ;
    pTempLateReq->m_strContentSimple = pSession->m_strContentSimple ;
    pTempLateReq->m_bTestChannel   = !pSession->m_strTestChannelId.empty();
    pTempLateReq->m_bIncludeChinese = pSession->m_bIncludeChinese;

    LogDebug("[STATE_TEMPLATE REQ ][%s:%s:%s] [smsType:%s, Sign:%s, Content:%s] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_strSmsType.c_str(),
        pSession->m_strSignSimple.c_str(),
        pSession->m_strContentSimple.c_str());

    pTempLateReq->m_pSender = this;
    pTempLateReq->m_iSeq = pSession->m_uSessionId;
    pTempLateReq->m_iMsgType = MSGTYPE_SEESION_TO_TEMPLATE_REQ;

    if (!CommPostMsg(STATE_TEMPLATE, pTempLateReq))
    {
        SAFE_DELETE(pTempLateReq);
        return ACCESS_ERROR_COMM_POST;
    }

    return ACCESS_ERROR_NONE;

}


UInt32 CSessionThread::sessionTemplateReport(session_pt pSession, TMsg* pMsg)
{
    CHECK_PARAM_NULL_RETURN(pSession);
    CHECK_PARAM_NULL_RETURN(pMsg);

    TSmsTemplateResp* pTemplateRsp = (TSmsTemplateResp*)pMsg;

    UInt32 uiRet = ACCESS_ERROR_NONE ;

    switch (pTemplateRsp->nMatchResult)
    {
        case NoMatch:                        // 未匹配到智能模板
        case MatchConstant:                  // 匹配到智能模板，且是固定模板
        case MatchVariable_NoMatchRegex:     // 匹配到智能模板，且是变量模板，变量部分未匹配到正则
        case MatchVariable_MatchRegex:       // 匹配上智能模板，且匹配到变量模板，变量部分匹配到正则
            uiRet = ACCESS_ERROR_NONE;
            break;

        case MatchBlack:                     // 匹配到黑模板
            pSession->m_uState = SMS_STATUS_AUDIT_FAIL;
            pSession->m_uAuditState = SMS_AUDIT_STATUS_BLACK_TEMP;
            pSession->m_strExtraErrorDesc = Comm::int2str(pTemplateRsp->nTemplateId);
            uiRet = ACCESS_ERROR_TEMPLATE_MATCH_BLACK;
            break;

        case MatchKeyWord:                   // 匹配到关键字
            pSession->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            uiRet = ACCESS_ERROR_TEMPLATE_MATCH_KEYWORD;
            pSession->m_strExtraErrorDesc = pTemplateRsp->m_strKeyWord;
            break;

        default:
            uiRet = ACCESS_ERROR_TEMPLATE_PARAM;
            pSession->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            break;

    }

    pSession->m_eMatchSmartTemplateRes = pTemplateRsp->nMatchResult;

    if (uiRet != ACCESS_ERROR_NONE)
    {
        pSession->m_strErrorCode = pTemplateRsp->m_strErrorCode;
        pSession->m_strYZXErrCode = pTemplateRsp->m_strYZXErrCode;
    }

    LogDebug("[STATE_TEMPLATE REPORT][%s:%s:%s] [MattchResult:%d, ID:%d, yzxError:%s, errorCode:%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pTemplateRsp->nMatchResult,
        pTemplateRsp->nTemplateId,
        pTemplateRsp->m_strErrorCode.c_str(),
        pTemplateRsp->m_strYZXErrCode.c_str());

    return uiRet;

}

UInt32 CSessionThread::sessionAudit(session_pt pSession)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    if ((true == pSession->m_bReadyAudit)
        || (1 == pSession->m_uAuditFlag))
    {
        pSession->m_uNeedAudit = 0;
    }

    AuditReqMsg* pAuditState = new AuditReqMsg();
    CHECK_PARAM_NULL_RETURN(pAuditState);

    pAuditState->m_strClientId = pSession->m_strClientId ;
    pAuditState->m_strUserName = pSession->m_strUserName ;
    pAuditState->m_strSmsId    = pSession->m_strSmsId ;
    pAuditState->m_strPhone    = pSession->m_strPhone;
    pAuditState->m_strSign     = pSession->m_strSign;
    pAuditState->m_strSignSimple = pSession->m_strSignSimple ;
    pAuditState->m_strContent = pSession->m_strContent ;
    pAuditState->m_strContentSimple = pSession->m_strContentSimple ;
    pAuditState->m_strSmsType  = pSession->m_strSmsType ;
    pAuditState->m_uiPayType   = pSession->m_uPayType ;
    pAuditState->m_uiOperater  = pSession->m_uOperater ;
    pAuditState->m_strC2sDate  = pSession->m_strDate;
    pAuditState->m_uiSmsFrom    = pSession->m_uSmsFrom;
    pAuditState->m_uiShowSignType = pSession->m_uShowSignType ;
    pAuditState->m_strSignPort = pSession->m_strSignPort  ;
    pAuditState->m_strExtpendPort = pSession->m_strExtpendPort;
    pAuditState->m_strIds      = pSession->m_strIds ;
    pAuditState->m_uiSmsNum   = pSession->m_uSmsNum ;
    pAuditState->m_strCSid    = pSession->m_strCSid;
    pAuditState->m_strUid     = pSession->m_strUid;
    pAuditState->m_bIncludeChinese = pSession->m_bIncludeChinese;

    pAuditState->m_eMatchSmartTemplateRes = pSession->m_eMatchSmartTemplateRes;
    pAuditState->m_iClientSendLimitCtrlTypeFlag =  pSession->m_iClientSendLimitCtrlTypeFlag;

    LogDebug("[STATE_AUDIT REQ][%s:%s:%s] [smsType:%s, Sign:%s, Content:%s, sendcontrolFlag:%d] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_strSmsType.c_str(),
        pSession->m_strSignSimple.c_str(),
        pSession->m_strContentSimple.c_str(),
        pSession->m_iClientSendLimitCtrlTypeFlag);

    pAuditState->m_iSeq = pSession->m_uSessionId;
    pAuditState->m_iMsgType = MSGTYPE_AUDIT_REQ;
    pAuditState->m_pSender = this;

    if (!CommPostMsg(STATE_AUDIT, pAuditState))
    {
        SAFE_DELETE(pAuditState);
        return ACCESS_ERROR_COMM_POST;
    }

    return ACCESS_ERROR_NONE;
}

UInt32 CSessionThread::sessionAuditReport(session_pt pSession, TMsg* pMsg)
{
    CHECK_PARAM_NULL_RETURN(pSession);
    CHECK_PARAM_NULL_RETURN(pMsg);

    AuditRspMsg* pAuditRsp = (AuditRspMsg*)pMsg;

    UInt32 uRet = ACCESS_ERROR_NONE;

    pSession->m_strAuditContent = pAuditRsp->m_strAuditContent;
    pSession->m_strAuditId = pAuditRsp->m_strAuditId;
    pSession->m_strAuditDate = pAuditRsp->m_strAuditDate;
    pSession->m_uAuditState = pAuditRsp->m_uiAuditState;

    switch (pAuditRsp->m_uiAuditState)
    {
        case SMS_AUDIT_STATUS_CACHE:
        case SMS_AUDIT_STATUS_GROUPSENDLIMIT:
        case SMS_AUDIT_STATUS_WAIT:
            uRet = ACCESS_ERROR_AUDIT_DONE;
            break;

        case SMS_AUDIT_STATUS_AUTO_PASS:
            uRet = ACCESS_ERROR_NONE;
            break;

        case SMS_AUDIT_STATUS_AUTO_FAIL:
            uRet = ACCESS_ERROR_AUDIT_FAIL;
            pSession->m_uState = SMS_STATUS_AUDIT_FAIL;
            break;

        default:
            uRet = ACCESS_ERROR_NONE;
    }


    if (uRet != ACCESS_ERROR_NONE)
    {
        pSession->m_strErrorCode = pAuditRsp->m_strErrCode;
        pSession->m_strYZXErrCode = pAuditRsp->m_strErrCode;
    }

    LogDebug("[STATE_AUDIT REPORT][%s:%s:%s] [RET:%u , m_uiAuditState:%u] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        uRet,
        pAuditRsp->m_uiAuditState);

    return uRet;

}

UInt32 CSessionThread::sessionRouter(session_pt pSession)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    TSmsRouterReq* pRouterReq = new TSmsRouterReq();
    CHECK_PARAM_NULL_RETURN(pRouterReq);

    pRouterReq->m_strSmsId = pSession->m_strSmsId ;
    pRouterReq->m_strSid   = pSession->m_strSid ; // ?
    pRouterReq->m_strClientId = pSession->m_strClientId;
    pRouterReq->m_strPhone   = pSession->m_strPhone ;
    pRouterReq->m_uOperater  = pSession->m_uOperater ;
    pRouterReq->m_uSmsFrom  = pSession->m_uSmsFrom ;
    pRouterReq->m_strSmsType = pSession->m_strSmsType ;
    pRouterReq->m_strContent = pSession->m_strContent;
    pRouterReq->m_strSign    = pSession->m_strSign;
    pRouterReq->m_strTemplateId = pSession->m_strTemplateId;
    pRouterReq->m_strUcpaasPort = pSession->m_strUcpaasPort ;
    pRouterReq->m_strExtpendPort = pSession->m_strExtpendPort;
    pRouterReq->m_strSignPort   = pSession->m_strSignPort;
    pRouterReq->m_uSendType    = pSession->m_uSendType;
    pRouterReq->m_strFailedResendChannels = "";
    pRouterReq->m_strTestChannelId = pSession->m_strTestChannelId ;
    pRouterReq->m_strChannelTemplateId = pSession->m_strChannelTemplateId ;
    pRouterReq->m_bIncludeChinese = pSession->m_bIncludeChinese;
    pRouterReq->m_uPhoneOperator = pSession->m_uOperater;                // 号码所属运营商
    pRouterReq->m_uFreeChannelKeyword = pSession->m_uFreeChannelKeyword;
    pRouterReq->m_uSelectChannelNum = pSession->m_uSelectChannelNum;
    // eg: value:0:3 the phone is in sysblacklist ,value:2154 the phone is in channel 2154 blacklist
    pRouterReq->m_phoneBlackListInfo = pSession->m_vecPhoneBlackListInfo;
    pRouterReq->m_FailedResendChannelsSet = pSession->m_FailedResendChannelsSet;
    pRouterReq->m_channelOverrateResendChannelsSet = pSession->m_vecFailedResendChannelsSet;
    pRouterReq->m_uFromSmsReback = pSession->m_uFromSmsReback;
    pRouterReq->m_iFailedResendTimes = pSession->m_iFailedResendTimes;
    pRouterReq->m_uOverTime = pSession->m_uOverTime;
    pRouterReq->m_iOverCount = pSession->m_iOverCount;
    pRouterReq->m_bInOverRateWhiteList  = pSession->m_bInOverRateWhiteList;
    if (pSession->m_bSendControlOverRateWhiteList)
    {
        if (pSession->m_iClientSendLimitCtrlTypeFlag == BLACKLIST_CONTROL)
        {
            pSession->m_iClientSendLimitCtrlTypeFlag = NOT_CONTROL;
            LogDebug("[%s:%s]overrate white list,delete blacklist_control flag."
                , pSession->m_strClientId.data(),pSession->m_strPhone.data());
        }
        
    }
    else
    {
        LogDebug("before judge if random control,sendControlType[%d]", pSession->m_iClientSendLimitCtrlTypeFlag);
        if (pSession->m_iClientSendLimitCtrlTypeFlag == NOT_CONTROL)
            checkSendLimit(pSession, RANDOM_CONTROL);
    }
    pRouterReq->m_iClientSendLimitCtrlTypeFlag = pSession->m_iClientSendLimitCtrlTypeFlag;
    pRouterReq->m_iSeq = pSession->m_uSessionId;
    pRouterReq->m_iMsgType = MSGTYPE_SEESION_TO_ROUTER_REQ;
    pRouterReq->m_pSender = this;

    LogDebug("[STATE_ROUTER REQ][%s:%s:%s] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str());

    if (! CommPostMsg(STATE_ROUTE, pRouterReq))
    {
        SAFE_DELETE(pRouterReq);
        return ACCESS_ERROR_COMM_POST;
    }

    return ACCESS_ERROR_NONE;
}

UInt32 CSessionThread::sessionRouterReport(session_pt pSession, TMsg* pMsg)
{
    CHECK_PARAM_NULL_RETURN(pSession);
    CHECK_PARAM_NULL_RETURN(pMsg);

    TSmsRouterResp* pRouterResp = (TSmsRouterResp*)pMsg;

    UInt32 uRet = ACCESS_ERROR_NONE;

    switch (pRouterResp->nRouterResult)
    {
        case CHANNEL_GET_SUCCESS:
        {
            channelInfoMap::iterator itr = m_mapChannelInfo.find(pRouterResp->m_uChannleId);

            if (itr == m_mapChannelInfo.end())
            {
                LogElk("session cant't find Channel %d", pSession->m_uChannleId);
                uRet =  ACCESS_ERROR_ROUTER_NO_CHANNEL;
            }
            else
            {
                models::Channel& smsChannel = itr->second;
                pSession->m_uLongSms = smsChannel.longsms;
                pSession->m_strCodeType = smsChannel.coding;
                pSession->m_strChannelName = smsChannel.channelname;
                pSession->m_uChannelMQID = smsChannel.m_uMqId;
                pSession->m_uChannleId  = pRouterResp->m_uChannleId;
                pSession->m_strHttpChannelUrl = smsChannel.url;
                pSession->m_strHttpChannelData = smsChannel.postdata;
                pSession->m_uChannelSendMode = smsChannel.httpmode;
                pSession->m_strUcpaasPort  = pRouterResp->m_strUcpaasPort;
                pSession->m_strExtpendPort = pRouterResp->m_strExtpendPort;
                pSession->m_strSignPort    = pRouterResp->m_strSignPort;
                const string strTime = getCurrentDateTime("%Y-%m-%d");

                if (FOREIGN != pSession->m_uOperater)//guonei
                {
                    map<UInt64, list<channelPropertyLog_ptr_t> >::iterator itr = m_channelPropertyLogMap.find(pRouterResp->m_uChannleId);

                    if (itr != m_channelPropertyLogMap.end())
                    {
                        list<channelPropertyLog_ptr_t>::iterator itlist = itr->second.begin();

                        while (itlist != itr->second.end())
                        {
                            if ((**itlist).m_strEffectDate <= strTime)
                            {
                                pSession->m_fCostFee = (**itlist).m_dChannelCostPrice;
                                break;
                            }

                            itlist++;
                        }
                    }

                    if (pSession->m_uPayType == 1)
                    {
                        string key = pSession->m_strClientId + "_" + pSession->m_strSmsType;
                        map<string, list<userPriceLog_ptr_t> >::iterator itrSalefee = m_userPriceLogMap.find(key);

                        if (itrSalefee != m_userPriceLogMap.end())
                        {
                            list<userPriceLog_ptr_t>::iterator itlist = itrSalefee->second.begin();

                            while (itlist != itrSalefee->second.end())
                            {
                                if ((**itlist).m_strEffectDate <= strTime)
                                {
                                    pSession->m_uSaleFee = (**itlist).m_dUserPrice * 1000000;
                                    break;
                                }

                                itlist++;
                            }
                        }
                    }

                    if ((pSession->m_uPayType == 2 && pSession->m_iAgentPayType == 1))
                    {
                        map<string, list<userPropertyLog_ptr_t> >::iterator itrSalefee = m_userPropertyLogMap.find(pSession->m_strClientId);

                        if (itrSalefee != m_userPropertyLogMap.end())
                        {
                            list<userPropertyLog_ptr_t>::iterator itlist = itrSalefee->second.begin();

                            while (itlist != itrSalefee->second.end())
                            {
                                if ((**itlist).m_strEffectDate <= strTime)
                                {
                                    if (BusType::YIDONG == pSession->m_uOperater || BusType::XNYD == pSession->m_uOperater)
                                    {
                                        pSession->m_uSaleFee = (**itlist).m_dydPrice * 1000000;
                                    }
                                    else if (BusType::LIANTONG == pSession->m_uOperater || BusType::XNLT == pSession->m_uOperater)
                                    {
                                        pSession->m_uSaleFee = (**itlist).m_dltPrice * 1000000;
                                    }
                                    else if (BusType::DIANXIN == pSession->m_uOperater || BusType::XNDX == pSession->m_uOperater)
                                    {
                                        pSession->m_uSaleFee = (**itlist).m_ddxPrice * 1000000;
                                    }

                                    LogDebug("deduct master account,post paytype,operator:%d,%f,%s", pSession->m_uOperater, pSession->m_uSaleFee, (**itlist).m_strEffectDate.data());
                                    break;
                                }

                                itlist++;
                            }
                        }
                    }
                }
                else
                {
                    pSession->m_fCostFee = pRouterResp->m_fCostFee;

                    if ((pSession->m_uPayType == 2 && pSession->m_iAgentPayType == 1))
                    {
                        map<string, list<userPropertyLog_ptr_t> >::iterator itrSalefee = m_userPropertyLogMap.find(pSession->m_strClientId);

                        if (itrSalefee != m_userPropertyLogMap.end())
                        {
                            list<userPropertyLog_ptr_t>::iterator itlist = itrSalefee->second.begin();

                            while (itlist != itrSalefee->second.end())
                            {
                                if ((**itlist).m_strEffectDate <= strTime)
                                {
                                    pSession->m_uSaleFee = (**itlist).m_dfjDiscount * 0.1 * pRouterResp->m_dSaleFee;
                                    break;
                                }

                                itlist++;
                            }
                        }
                    }
                    else
                    {
                        pSession->m_uSaleFee = pRouterResp->m_dSaleFee;
                    }

                    pSession->m_uProductCost = pRouterResp->m_dSaleFee;
                }

                pSession->m_uExtendSize = smsChannel.m_uExtendSize;
                pSession->m_uChannelType = smsChannel.m_uChannelType;
                pSession->m_uSendNodeId = smsChannel.m_uSendNodeId;

                pSession->m_uChannelOperatorsType = smsChannel.operatorstyle;
                pSession->m_strChannelRemark = smsChannel.m_strRemark;
                pSession->m_uChannelIdentify = smsChannel.m_uChannelIdentify;
                pSession->m_uChannelExValue = smsChannel.m_uExValue;
                pSession->m_strHttpOauthUrl = smsChannel.m_strOauthUrl;
                pSession->m_strHttpOauthData = smsChannel.m_strOauthData;

                if (3 == smsChannel._showsigntype)
                {
                    pSession->m_uShowSignType = smsChannel._showsigntype;
                }
                else if ((0 != pSession->m_uShowSignType) && (0 == smsChannel._showsigntype))
                {
                    pSession->m_uShowSignType = pSession->m_uShowSignType;
                }
                else
                {
                    pSession->m_uShowSignType = smsChannel._showsigntype;
                }

                pSession->m_uState = SMS_STATUS_INITIAL;
                pSession->m_strErrorCode = "";
                pSession->m_strYZXErrCode = "";
                uRet = ACCESS_ERROR_NONE;
            }

            break;
        }

        case CHANNEL_GET_FAILURE_LOGINSTATUS:
        case CHANNEL_GET_FAILURE_OVERFLOW:
        {
            if (pSession->m_uFromSmsReback == 1)
            {
                if (pRouterResp->m_uIsSendToReback)
                {
                    pSession->m_uRepushPushType = MQ_PUSH_IOREBACK;
                    uRet = ACCESS_ERROR_ROUTER_OVERFLOW;
                }
                else
                {
                    pSession->m_uRepushPushType = MQ_PUSH_REPORT;
                    uRet = ACCESS_ERROR_ROUTER_FAIL;
                }

                pSession->m_strSubmit = REBACK_OVER_TIME;
                pSession->m_uState = SMS_STATUS_SUBMIT_FAIL;
            }
            else
            {
                pSession->m_uSelectChannelNum ++ ;

                if (pRouterResp->m_strErrorCode.empty())
                {
                    pSession->m_uRepushPushType = MQ_PUSH_OVERFLOW;
                    uRet = ACCESS_ERROR_ROUTER_OVERFLOW;
                }
                else
                {
                    uRet = ACCESS_ERROR_ROUTER_FAIL;
                }
            }

            break;
        }

        case CHANNEL_GET_FAILURE_NO_CHANNELGROUP:
        case CHANNEL_GET_FAILURE_NOT_RESEND_MSG:
        case CHANNEL_GET_FAILURE_OTHER:
            uRet = ACCESS_ERROR_ROUTER_FAIL;

            if (pSession->m_uFromSmsReback == 1)
            {
                pSession->m_uState = SMS_STATUS_SUBMIT_FAIL;
                pSession->m_strErrorCode = pRouterResp->m_strErrorCode;
                pSession->m_strYZXErrCode = pRouterResp->m_strYZXErrCode;
                pSession->m_uRepushPushType = MQ_PUSH_REPORT;
            }

            break;

        default:
            uRet = ACCESS_ERROR_ROUTER_PARAM;
    }

    if (uRet != ACCESS_ERROR_NONE && pSession->m_uFromSmsReback == 0)
    {
        pSession->m_uState = SMS_STATUS_GW_SEND_HOLD;
        pSession->m_strErrorCode = pRouterResp->m_strErrorCode;
        pSession->m_strYZXErrCode = pRouterResp->m_strYZXErrCode;
    }

    LogDebug("[STATE_ROUTER REPORT ][%s:%s:%s] [ Ret:%d, Result:%d, Channelid:%d, yzxError:%s, errorCode:%s, m_strSubmit:%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        uRet,
        pRouterResp->nRouterResult,
        pSession->m_uChannleId,
        pRouterResp->m_strYZXErrCode.c_str(),
        pRouterResp->m_strErrorCode.c_str(),
        pSession->m_strSubmit.c_str());

    return uRet;
}

UInt32 CSessionThread::sessionOverRate(session_pt pSession, OVERRATE_TYPE eOverRateType)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    if (pSession->m_iClientSendLimitCtrlTypeFlag > NOT_CONTROL)
    {
        LogDebug("[%s:%s:%s] No need for over rate, OverRateType[%s].",
            pSession->m_strClientId.data(),
            pSession->m_strSmsId.data(),
            pSession->m_strPhone.data(),
            (OVERRATE_CHECK == eOverRateType) ? "OVERRATE_CHECK" : "OVERRATE_SET");

        return ACCESS_ERROR_NEXT_STATE;
    }

    bool bPostOk = false;

    if (OVERRATE_CHECK == eOverRateType)
    {
        //超频检查请求
        OverRateCheckReqMsg* pReq = new OverRateCheckReqMsg();
        pReq->m_strClientId = pSession->m_strClientId;
        pReq->m_strContent = pSession->m_strContent;
        pReq->m_strPhone = pSession->m_strPhone;
        pReq->m_strSign = pSession->m_strSign;
        pReq->m_strSmsId = pSession->m_strSmsId;
        pReq->m_strSmsType = pSession->m_strSmsType;
        pReq->m_uiChannelId = pSession->m_uChannleId;
        pReq->m_strC2sDate = pSession->m_strDate;
        pReq->m_strTemplateType = pSession->m_strTemplateType;
        pReq->m_bIncludeChinese = pSession->m_bIncludeChinese;
        pReq->m_bInOverRateWhiteList = pSession->m_bInOverRateWhiteList;

        if (!pSession->m_vecFailedResendChannelsSet.empty() || pSession->m_uFromSmsReback == 1)
        {
            pReq->m_eOverRateCheckType = OverRateCheckReqMsg::ChannelOverRateCheck;
        }

        pReq->m_iMsgType = MSGTYPE_OVERRATE_CHECK_REQ;
        pReq->m_iSeq = pSession->m_uSessionId ;
        pReq->m_pSender = this;

        bPostOk = CommPostMsg(STATE_OVERRATE,  pReq);

        if (!bPostOk)
        {
            SAFE_DELETE(pReq);
        }

    }
    else if (OVERRATE_SET == eOverRateType)
    {
        //超频设置
        OverRateSetReqMsg* pOverRateSet = new OverRateSetReqMsg();
        pOverRateSet->m_uiState     = (pSession->m_uState == SMS_STATUS_SUBMIT_SUCCESS) ?  0 : 1 ;
        pOverRateSet->m_iMsgType    = MSGTYPE_OVERRATE_SET_RSQ;
        pOverRateSet->m_iSeq        = pSession->m_ulOverRateSessionId;
        pOverRateSet->m_pSender     = this;

        bPostOk = CommPostMsg(STATE_OVERRATE,  pOverRateSet);

        if (!bPostOk)
        {
            SAFE_DELETE(pOverRateSet);
        }
    }
    else
    {
        LogWarn("[OVERRATE_STATE] [%s:%s:%s] Process Wrong eOverRateType %d ",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            eOverRateType );

        return ACCESS_ERROR_SESSION_STAT_WRONG;
    }

    if (!bPostOk)
    {
        return ACCESS_ERROR_COMM_POST;
    }

    return ACCESS_ERROR_NONE;
}

UInt32 CSessionThread::sessionOverRateReport(session_pt pSession, TMsg* pMsg)
{
    CHECK_PARAM_NULL_RETURN(pSession);
    CHECK_PARAM_NULL_RETURN(pMsg);

    OverRateCheckRspMsg* pOverRateRsp = (OverRateCheckRspMsg*)pMsg;

    UInt32 uRet = ACCESS_ERROR_NONE;

    //超频结果处理
    switch (pOverRateRsp->m_eOverRateResult)
    {
        case OverRateNone:
            /*超频检测成功后，设置处理过超频逻辑*/
            pSession->m_bOverRateSessionState = true;
            pSession->m_strChannelOverRateKeyall = pOverRateRsp->m_strChannelOverRateKeyall;
            uRet = ACCESS_ERROR_NONE;
            break;

        case OverRateChannel:
            pSession->m_vecFailedResendChannelsSet.insert(pSession->m_uChannleId);
            uRet = ACCESS_ERROR_OVERRATE_CHANNEL;
            break;

        default:
            uRet = ACCESS_ERROR_OVERRATE_OTHER;

            if (pSession->m_uOverRateCharge == 1)
            {
                //设置超频计费标志
                pSession->m_bOverRatePlanFlag = true;
                uRet = ACCESS_ERROR_OVERRATE_CHARGE;
            }
            
            if (pSession->m_iClientSendLimitCtrlTypeFlag == OVERRATE_CONTROL)
            {
                 uRet = ACCESS_ERROR_OVERRATE_CHANNEL;//复用
            }
            else
            {
                checkSendLimit(pSession, OVERRATE_CONTROL);
                if (pSession->m_iClientSendLimitCtrlTypeFlag != OVERRATE_CONTROL)
                {
                    refreshSendLimit(pSession, OVERRATE_CONTROL);
                }
                else
                {
                     uRet = ACCESS_ERROR_OVERRATE_CHANNEL;//复用
                }
            }

            
    }

    pSession->m_ulOverRateSessionId = pOverRateRsp->m_ulOverRateSessionId;

    if (uRet != ACCESS_ERROR_NONE)
    {
        pSession->m_uState = SMS_STATUS_OVERRATE;
        pSession->m_strErrorCode = pOverRateRsp->m_strErrCode;
        pSession->m_strYZXErrCode = pOverRateRsp->m_strErrCode;
        pSession->m_uSaleFee = 0;
        pSession->m_fCostFee = 0;
        pSession->m_uProductCost = 0;
    }

    LogDebug("[STATE_OVERRATE REPORT][%s:%s:%s] [Ret:%d, OverRateResult:%d, Channelid:%d, errorCode:%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        uRet,
        pOverRateRsp->m_eOverRateResult,
        pSession->m_uChannleId,
        pOverRateRsp->m_strErrCode.c_str());

    return uRet;
}

UInt32 CSessionThread::sessionCharge(session_pt pSession, CHARGE_TYPE chargeType)
{
    CChargeThread* pChargeThread = (CChargeThread*)m_pChargeThread;

    if (CHARGE_POST == chargeType)
    {
        if (!pChargeThread->smsChargePost(pSession))
        {
            string strRespon = "orderid=&sale_price=&product_cost=&result=2";

            if (pSession->m_uState == SMS_STATUS_SEND_FAIL)
            {
                pChargeThread->smsChargeResult(pSession, 2, strRespon);
            }
            else
            {
                pChargeThread->smsChargeResult(pSession, 200, strRespon);
            }

            return ACCESS_ERROR_CHARGE;
        }
    }
    else
    {
        if (pSession->m_uPayType == CHARGE_POST || (pSession->m_uPayType == DEDUCT_MASTAR_ACCOUNT && pSession->m_iAgentPayType == 1))
        {
            LogDebug("[%s:%s:%s] m_uAgentId[%u],m_uPayType[%u],m_iAgentPayType[%d],m_strClientId[%s] not need to send charge",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentId,
                pSession->m_uPayType,
                pSession->m_iAgentPayType,
                pSession->m_strClientId.c_str());

            return ACCESS_ERROR_NEXT_STATE;
        }

        if (!pChargeThread->smsChargePre(pSession))
        {
            if (pSession->m_uState == SMS_STATUS_INITIAL)
            {
                pSession->m_uState = SMS_STATUS_SEND_FAIL;
            }

            return ACCESS_ERROR_CHARGE;
        }
    }

    return ACCESS_ERROR_NONE;

}

UInt32 CSessionThread::sessionChargeReport(session_pt pSession, TMsg* pMsg)
{
    UInt32 uRet = ACCESS_ERROR_NONE ;

    THttpResponseMsg* pHttpResponse = (THttpResponseMsg*)pMsg;

    CHECK_PARAM_NULL_RETURN(pHttpResponse);
    CHECK_PARAM_NULL_RETURN(pSession);

    const string& strContent = pHttpResponse->m_tResponse.GetContent();
    UInt32  uStatusCode = pHttpResponse->m_tResponse.GetStatusCode();

    uRet = static_cast<CChargeThread*>(m_pChargeThread)->smsChargeResult(pSession, uStatusCode, strContent);

    if (uRet != ACCESS_ERROR_NONE && pSession->m_uState == SMS_STATUS_INITIAL)
    {
        pSession->m_uState = (200 == uStatusCode ? SMS_STATUS_GW_SEND_HOLD : SMS_STATUS_SEND_FAIL);
    }

    LogDebug("[STATE_CHARGE REPORT][%s:%s:%s] [Ret:%u, uStatusCode:%d, strContent:%s] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        uRet,
        uStatusCode,
        strContent.c_str());

    return uRet;

}

bool CSessionThread::SendToReback(session_pt pInfo)
{
    CHK_NULL_RETURN_FALSE(pInfo);

    string strExchange = "", strRoutingKey = "";

    if (!pInfo->m_strChannelOverrate_access.empty())
    {
        pInfo->m_strSendUrl.append("&channelOverrateDate=");
        string ChannelDayOverRate = pInfo->m_strChannelOverrate_access;
        pInfo->m_strSendUrl.append(ChannelDayOverRate);

        pInfo->m_strSendUrl.append("&oriChannelid=");
        pInfo->m_strSendUrl.append(Comm::int2str(pInfo->m_uOriginChannleId));

        pInfo->m_strSendUrl.append("&isReback=");
        pInfo->m_strSendUrl.append("1");
    }

    if (!GetRebackMQInfo(pInfo, strExchange, strRoutingKey))
    {
        return false;
    }

    LogNotice("[%s:%s:%s] Transfer to Reback again, exchange[%s] routingkey[%s] Data[%s].",
        pInfo->m_strClientId.c_str(),
        pInfo->m_strSmsId.c_str(),
        pInfo->m_strPhone.c_str(),
        strExchange.c_str(),
        strRoutingKey.c_str(),
        pInfo->m_strSendUrl.c_str());

    TMQPublishReqMsg* pMQ2Reback = new TMQPublishReqMsg();
    pMQ2Reback->m_strData = pInfo->m_strSendUrl;
    pMQ2Reback->m_strExchange = strExchange;
    pMQ2Reback->m_strRoutingKey = strRoutingKey;
    pMQ2Reback->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMQIOProducerThread->PostMsg(pMQ2Reback);

    return true;
}

bool CSessionThread::GetRebackMQInfo(session_pt pInfo, string& strExchange, string& strRoutingKey)
{
    CHK_NULL_RETURN_FALSE(pInfo);

    int Operatorstype = pInfo->m_uOperater;
    int SmsType = atoi(pInfo->m_strSmsType.data());
    map<string, ComponentRefMq>::iterator iter;
    map<UInt64, MqConfig>::iterator itor;
    string strKey = "";
    char temp[128] = {0};
    string strMessageType = "";

    if (Operatorstype == YIDONG)
    {
        if (SMS_TYPE_MARKETING == SmsType
            || SMS_TYPE_USSD == SmsType
            || SMS_TYPE_FLUSH_SMS == SmsType)
        {
            strMessageType.assign("12");
        }
        else
        {
            strMessageType.assign("11");
        }
    }
    else if (DIANXIN == Operatorstype)
    {
        if (SMS_TYPE_MARKETING ==  SmsType
            || SMS_TYPE_USSD == SmsType
            || SMS_TYPE_FLUSH_SMS == SmsType)
        {
            strMessageType.assign("16");
        }
        else
        {
            strMessageType.assign("15");
        }
    }
    else if ((LIANTONG == Operatorstype)
        || (FOREIGN == Operatorstype))
    {
        if (SMS_TYPE_MARKETING == SmsType
            || SMS_TYPE_USSD == SmsType
            || SMS_TYPE_FLUSH_SMS == SmsType)
        {
            strMessageType.assign("14");
        }
        else
        {
            strMessageType.assign("13");
        }
    }
    else
    {
        LogWarn("[%s:%s:%s] Operatorstype[%d] SmsType[%d] is Invalid",
            pInfo->m_strClientId.c_str(),
            pInfo->m_strSmsId.c_str(),
            pInfo->m_strPhone.c_str(),
            Operatorstype, SmsType);

        goto SEND_FAILED;
    }

    snprintf(temp, 250, "%lu_%s_0", g_uComponentId, strMessageType.data());
    strKey.assign(temp);

    iter = m_mqConfigRef.find(strKey);

    if (iter == m_mqConfigRef.end())
    {
        LogWarn("[%s:%s:%s] Key[%s] is not found in m_componentRefMqInfo",
            pInfo->m_strClientId.c_str(),
            pInfo->m_strSmsId.c_str(),
            pInfo->m_strPhone.c_str(),
            strKey.data());

        goto SEND_FAILED;
    }

    itor = m_mqConfigInfo.find(iter->second.m_uMqId);

    if (itor == m_mqConfigInfo.end())
    {
        LogWarn("[%s:%s] Key[%lu] is not found in m_mqConfigInfo",
            pInfo->m_strClientId.c_str(),
            pInfo->m_strSmsId.c_str(),
            pInfo->m_strPhone.c_str(),
            iter->second.m_uMqId);

        goto SEND_FAILED;
    }

    strExchange = itor->second.m_strExchange;
    strRoutingKey = itor->second.m_strRoutingKey;

    return true;


SEND_FAILED:

    return false;
}

UInt32 CSessionThread::sessionDone(session_pt pSession)
{
    CHECK_PARAM_NULL_RETURN(pSession);

    do
    {
        //已经送审核，session模块结束会话，无需要处理
        if ((STATE_AUDIT == pSession->m_uSessionPreState)
            && ((SMS_AUDIT_STATUS_WAIT == pSession->m_uAuditState)
                || (SMS_AUDIT_STATUS_GROUPSENDLIMIT == pSession->m_uAuditState)
                || (SMS_AUDIT_STATUS_CACHE == pSession->m_uAuditState)))
        {
            break;
        }

        if (SMS_STATUS_INITIAL == pSession->m_uState)
        {
            // 发布MQ消息
            MQChannelpublishMeaasge(pSession);

            // 发送控制刷新缓存
            if (pSession->m_iClientSendLimitCtrlTypeFlag == NOT_CONTROL)
            {
                refreshSendLimit(pSession, RANDOM_CONTROL);
            }

            // 发送控制
            if (pSession->m_iClientSendLimitCtrlTypeFlag > NOT_CONTROL)
            {
                getChargeRule(pSession);

                if ((0 == pSession->m_ucChargeRule) || (1 == pSession->m_ucChargeRule))
                {
                    pSession->m_uState = SMS_STATUS_HUMAN_SUBMIT_SUCCESS;
                    pSession->m_strSubmit = "send to smsp_send ok";

                    DBUpdateRecord(pSession);
                }
                else
                {
                    pSession->m_uState = SMS_STATUS_HUMAN_CONFIRM_SUCCESS;
                    pSession->m_strSubmit = "send to smsp_send ok";

                    sendLimitSuccessReport(pSession);
                }
            }
            else
            {
                // 更新DB
                pSession->m_uState = SMS_STATUS_SUBMIT_SUCCESS;
                pSession->m_strSubmit = "send to smsp_send ok";

                DBUpdateRecord(pSession);
            }
        }
        else
        {
            if (MQ_PUSH_REPORT == pSession->m_uRepushPushType)
            {
                MQC2sioFailedSendReport(pSession);
            }
            else if (MQ_PUSH_IOREBACK == pSession->m_uRepushPushType)
            {
                SendToReback(pSession);
            }
            else
            {
                MQPushBackToC2sIO(pSession, pSession->m_uRepushPushType);
            }
        }

        // 走过超频逻辑，需要告知超频模块处理结果
        if (pSession->m_bOverRateSessionState && !pSession->m_bOverRatePlanFlag)
        {
            sessionOverRate(pSession, OVERRATE_SET);
        }
    } while (0);

    string strStateTrace = "";
    pSession->m_apiTrace->InfoString(strStateTrace);

    LogNotice("[STATE_DONE][%s:%s:%s]\n sid:%lu, preState:%s, smsState:%u, sessionSize:%d, traceInfo: %s ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_uSessionId,
        state2String(pSession->m_uSessionPreState).c_str(),
        pSession->m_uState,
        m_mapSMSSession.size() - 1,
        strStateTrace.c_str());

    m_uCurSessionSize --;

    m_mapSMSSession.erase(pSession->m_uSessionId);
    SAFE_DELETE(pSession);

    return ACCESS_ERROR_NONE;
}

UInt32 CSessionThread::MQPushBackToC2sIO(session_pt pSession, int iPushType)
{
    std::string strTip = "audit_pass";

    MqConfig* mqconf = MQGetC2SIOConfigDown(pSession);

    if (mqconf == NULL)
    {
        LogError("getC2SIOMQConfig failed. csid[%s]", pSession->m_strCSid.c_str());
        return ACCESS_ERROR_MQ_NOT_FIND;
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strExchange    = mqconf->m_strExchange;
    pMQ->m_strRoutingKey  = mqconf->m_strRoutingKey;
    pMQ->m_iMsgType       = MSGTYPE_MQ_PUBLISH_REQ;

    pMQ->m_strData.append("clientId=");
    pMQ->m_strData.append(pSession->m_strClientId);

    pMQ->m_strData.append("&userName=");
    pMQ->m_strData.append(Base64::Encode(pSession->m_strUserName));

    pMQ->m_strData.append("&smsId=");
    pMQ->m_strData.append(pSession->m_strSmsId);

    pMQ->m_strData.append("&phone=");
    pMQ->m_strData.append(pSession->m_strPhone);

    pMQ->m_strData.append("&sign=");
    pMQ->m_strData.append(Base64::Encode(pSession->m_strSign));

    pMQ->m_strData.append("&content=");
    pMQ->m_strData.append(Base64::Encode(pSession->m_strContent));

    pMQ->m_strData.append("&smsType=");
    pMQ->m_strData.append(pSession->m_strSmsType);

    pMQ->m_strData.append("&smsfrom=");
    pMQ->m_strData.append(Comm::int2str(pSession->m_uSmsFrom));

    pMQ->m_strData.append("&showsigntype=");
    pMQ->m_strData.append(Comm::int2str(pSession->m_uShowSignType));

    pMQ->m_strData.append("&userextpendport=");
    pMQ->m_strData.append(pSession->m_strExtpendPort);

    pMQ->m_strData.append("&signport=");
    pMQ->m_strData.append(pSession->m_strSignPort);

    pMQ->m_strData.append("&csid=");
    pMQ->m_strData.append(pSession->m_strCSid);

    pMQ->m_strData.append("&ids=");
    pMQ->m_strData.append(pSession->m_strIds);

    pMQ->m_strData.append("&csdate=");
    pMQ->m_strData.append(pSession->m_strCSdate);

    pMQ->m_strData.append("&audit_flag=");
    pMQ->m_strData.append("1");

    pMQ->m_strData.append("&send_limit_flag=");
    pMQ->m_strData.append(Comm::int2str(pSession->m_iClientSendLimitCtrlTypeFlag));

    if (MQ_PUSH_OVERFLOW == iPushType)
    {
        strTip = "overflower";
        pMQ->m_strData.append("&paytype=");
        pMQ->m_strData.append(Comm::int2str(pSession->m_uPayType));
        pMQ->m_strData.append("&clientcnt=");
        pMQ->m_strData.append(Comm::int2str(pSession->m_uSmsNum));
        pMQ->m_strData.append("&route_times=");
        pMQ->m_strData.append(Comm::int2str(pSession->m_uSelectChannelNum));
        pMQ->m_strData.append("&test_channelid=");
        pMQ->m_strData.append(pSession->m_strTestChannelId);
    }
    else if (MQ_PUSH_AUDITPASS  == iPushType)
    {
    }

    LogDebug("==%s==insert to rabbitMQ, strMQdate[%s], smstype[%s]",
        strTip.c_str(),
        pMQ->m_strData.data(),
        pSession->m_strSmsType.c_str());

    g_pMQIOProducerThread->PostMsg(pMQ);

    return ACCESS_ERROR_NONE;
}

UInt32 CSessionThread::MQC2sioFailedSendReport(session_pt pSession)
{
    string strMq = "";
    //send report to c2siomq, with update sql
    strMq.append("type=0");

    strMq.append("&clientid=");
    strMq.append(pSession->m_strClientId);

    strMq.append("&smsid=");
    strMq.append(pSession->m_strSmsId);

    strMq.append("&phone=");
    strMq.append(pSession->m_strPhone);

    strMq.append("&status=6");

    string strdesc;

    if (!pSession->m_strSubmit.empty())
    {
        strdesc = pSession->m_strSubmit;
    }
    else
    {
        if (!pSession->m_strYZXErrCode.empty())
        {
            strdesc = pSession->m_strYZXErrCode;
        }
        else
        {
            strdesc = pSession->m_strErrorCode;
        }
    }

    strMq.append("&desc=");
    strMq.append(Base64::Encode(strdesc));

    strMq.append("&reportdesc=");
    strMq.append(Base64::Encode(strdesc));

    strMq.append("&channelcnt=");
    strMq.append(Comm::int2str(pSession->m_uSmsNum));

    strMq.append("&reportTime=");
    strMq.append(Comm::int2str(time(NULL)));

    strMq.append("&smsfrom=");
    strMq.append(Comm::int2str(pSession->m_uSmsFrom));

    DBUpdateRecord(pSession, strMq.c_str());

    return ACCESS_ERROR_NONE;
}

void CSessionThread::sendLimitSuccessReport(session_pt pSession)
{
    static const string strDesc = Base64::Encode("DELIVRD");
    string strMq;

    strMq.append("type=0&clientid=");
    strMq.append(pSession->m_strClientId);

    strMq.append("&channelid=");
    strMq.append(to_string(pSession->m_uChannleId));

    strMq.append("&smsid=");
    strMq.append(pSession->m_strSmsId);

    strMq.append("&phone=");
    strMq.append(pSession->m_strPhone);

    strMq.append("&status=3&desc=");
    strMq.append(strDesc);

    strMq.append("&reportdesc=");
    strMq.append(strDesc);

    strMq.append("&channelcnt=");
    strMq.append(to_string(pSession->m_uSmsNum));

    strMq.append("&reportTime=");
    strMq.append(to_string(time(NULL)));

    strMq.append("&smsfrom=");
    strMq.append(to_string(pSession->m_uSmsFrom));

    DBUpdateRecord(pSession, strMq.c_str());
}

bool CSessionThread::MQChannelpublishMeaasge(session_pt pSession)
{
    if (NULL == pSession)
    {
        LogError("session is NULL.");
        return ACCESS_ERROR_PARAM_NULL ;
    }

    if (pSession->m_iClientSendLimitCtrlTypeFlag > NOT_CONTROL)
    {
        LogDebug("not send to channel.");
        return ACCESS_ERROR_NONE;
    }

    mqConfigMap::iterator itmq = m_mqConfigInfo.find(pSession->m_uChannelMQID);

    if (itmq == m_mqConfigInfo.end())
    {
        LogError("[%s:%s:%s] post to channelmq failed. mqid[%ld] is not find in m_mqConfigInfo",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            pSession->m_uChannelMQID);

        pSession->m_uState = SMS_STATUS_GW_SEND_HOLD;
        pSession->m_strSubmit = "channel mq not find";
        return ACCESS_ERROR_MQ_NOT_FIND;
    }

    if (itmq->second.m_strExchange.empty()
        || itmq->second.m_strRoutingKey.empty())
    {
        LogError("[%s:%s:%s] post to channelmq failed. mqid[%ld] m_strExchange[%s] m_strRoutingKey[%s] ",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            pSession->m_uChannelMQID,
            itmq->second.m_strExchange.c_str(),
            itmq->second.m_strRoutingKey.c_str());

        pSession->m_uState = SMS_STATUS_GW_SEND_HOLD;
        pSession->m_strSubmit = "exchange or routing is empty";
        return ACCESS_ERROR_MQ_ROUTER_EMPTY;
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(itmq->second.m_strExchange);
    pMQ->m_strRoutingKey.assign(itmq->second.m_strRoutingKey);

    if (pSession->m_uFromSmsReback)
    {
        setSendUrlFromReback2Send(pSession, pMQ->m_strData);
    }
    else
    {
        setSessionSendUrl(pSession, pMQ->m_strData);
    }

    pMQ->m_iPriority = ChannelPriorityHelper::GetClientChannelPriority(pSession, m_clientPriorities);
    LogDebug("priority: %d", pMQ->m_iPriority);

    LogDebug("access send one channel mq msg,exchange:%s,routingkey:%s,msg:%s.",
        pMQ->m_strExchange.c_str(),
        pMQ->m_strRoutingKey.c_str(),
        pMQ->m_strData.c_str());

    g_pMQSendIOProducerThread->PostMsg(pMQ);

    return ACCESS_ERROR_NONE ;
}

void CSessionThread::HandleTimeOut(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    if (pMsg->m_strSessionID == ACCESS_SESSION_TIMER_ID)
    {
        session_pt pSession = GetSessionBySeqID(pMsg->m_iSeq);

        if (pSession != NULL)
        {
            LogWarn("[%s:%s:%s] [%s] session id[%lu] timeout",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                state2String(pSession->m_uSessionState).c_str(),
                pSession->m_uSessionId);

            SAFE_DELETE(pSession->m_pSessionTimer);
            pSession->m_apiTrace->End(pSession->m_uSessionState);
            pSession->m_apiTrace->setError(pSession->m_uSessionState, ACCESS_ERROR_SESSION_TIMEOUT);
            /*设置错误码*/
            pSession->m_strErrorCode = INVALID_TIMEOUT;
            pSession->m_strYZXErrCode = INVALID_TIMEOUT;
            pSession->m_uState = SMS_STATUS_GW_SEND_HOLD;
            sessionStateProc(pSession,  STATE_DONE);
        }
    }
    else if (pMsg->m_strSessionID == ACCESS_STATISTIC_TIMER_ID)
    {
        LogFatal("[Session Statistic] sessionMapSize:%u, sessionCount:%u",
            m_mapSMSSession.size(),
            m_uCurSessionSize );

        SAFE_DELETE( m_pThreadTimer );
        m_pThreadTimer = SetTimer( 0, ACCESS_STATISTIC_TIMER_ID, ACCESS_STATISTIC_TIMEOUT );
    }
}

void CSessionThread::DBUpdateRecord(session_pt pSession, const char* pStrMq)
{
    CHK_NULL_RETURN(pSession);

    if (pSession->m_strSubmit.empty())
    {
        pSession->m_strSubmit.assign(pSession->m_strErrorCode);
    }

    string strDate = pSession->m_strDate.substr(0, 8);   //get date.
    string sendDate = Comm::getCurrentTime();     //send date

    if (pSession->m_strAuditDate.empty())
    {
        pSession->m_strAuditDate.assign(sendDate);
    }

    for (vector<string>::iterator it = pSession->m_vecLongMsgContainIDs.begin(); it != pSession->m_vecLongMsgContainIDs.end(); it++)
    {
        string strFindError = pSession->m_strErrorCode;

        if (pSession->m_uState == SMS_STATUS_SUBMIT_FAIL)
        {
            strFindError = pSession->m_strSubmit;
        }

        string strTempDesc(strFindError);
        strErrorMap::iterator itrError = m_mapSystemErrorCode.find(strFindError);

        if (itrError != m_mapSystemErrorCode.end())
        {
            strTempDesc.assign(strFindError);
            strTempDesc.append("*");
            strTempDesc.append(itrError->second);

            if (!pSession->m_strExtraErrorDesc.empty())
            {
                strTempDesc.append("*");
                strTempDesc.append(pSession->m_strExtraErrorDesc);
            }
        }

        pSession->m_strInnerErrorcode = strTempDesc;

        if (pSession->m_uState == SMS_STATUS_HUMAN_CONFIRM_SUCCESS
            || pSession->m_uState == SMS_STATUS_HUMAN_SUBMIT_SUCCESS)
        {
            pSession->m_strInnerErrorcode.clear();
            char sendlimitReason[10][64] = {"不控制","随机控制","黑名单控制","超频控制"};
            if (pSession->m_iClientSendLimitCtrlTypeFlag >=0 && pSession->m_iClientSendLimitCtrlTypeFlag <= 3)
                pSession->m_strInnerErrorcode.assign(sendlimitReason[pSession->m_iClientSendLimitCtrlTypeFlag]);
        }

        MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
        char sign[128] = {0};
        UInt32 position = pSession->m_strSign.length();

        if (pSession->m_strSign.length() > 100)
        {
            position = Comm::getSubStr(pSession->m_strSign, 100);
        }

        if (MysqlConn != NULL)
        {
            mysql_real_escape_string(MysqlConn, sign, pSession->m_strSign.substr(0, position).data(),
                pSession->m_strSign.substr(0, position).length());
        }

        char sql[9048]  = {0};
        int  n = 0;

        if (pSession->m_uState == SMS_STATUS_SUBMIT_FAIL)
        {
            n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s "
                         "SET state='%d',submit='%s',innerErrorcode='%s',area='%u',process_times='%d',channelid='0',"
                         "failed_resend_times='%d',failed_resend_channel='%s',access_id='%lu'",
                         pSession->m_uIdentify,
                         strDate.c_str(),
                         SMS_STATUS_SUBMIT_FAIL,
                         strTempDesc.data(),
                         pSession->m_strInnerErrorcode.data(),
                         pSession->m_uArea,
                         pSession->m_iOverCount,
                         pSession->m_iFailedResendTimes,
                         pSession->m_strFailedResendChannels.data(),
                         g_uComponentId);	//id
        }
        else if( pSession->m_uState == SMS_STATUS_GW_SEND_HOLD
                 || pSession->m_uState == SMS_STATUS_GW_RECEIVE_HOLD
                 || pSession->m_uState == SMS_STATUS_SEND_FAIL
                 || pSession->m_uState == SMS_STATUS_OVERRATE
                 || pSession->m_uState == SMS_STATUS_AUDIT_FAIL )
        {
            if (pSession->m_uFromSmsReback)
            {
                n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s "
                             "SET state='%d',errorcode='%s',innerErrorcode='%s',area='%u',submit='%s',"
                             "process_times='%d',channelid='0',failed_resend_times='%d',failed_resend_channel='%s',access_id='%lu'",
                             pSession->m_uIdentify,
                             strDate.c_str(),
                             SMS_STATUS_SUBMIT_FAIL,
                             strTempDesc.data(),
                             pSession->m_strInnerErrorcode.data(),
                             pSession->m_uArea,
                             strTempDesc.data(),
                             pSession->m_iOverCount,
                             pSession->m_iFailedResendTimes,
                             pSession->m_strFailedResendChannels.data(),
                             g_uComponentId);
            }
            else
            {
                n = snprintf(sql, sizeof(sql) - 1, "UPDATE t_sms_access_%d_%s SET state='%u',errorcode='%s',channelid='%d',"
                             "costfee='%f',salefee='%lf',productfee='%lf',sub_id='%s',product_type='%d',channeloperatorstype='%d',"
                             "channelremark='%s',submitdate='%s',channel_tempid='%s',sign='%s',innerErrorcode='%s',area='%u',access_id='%lu'",
                             pSession->m_uIdentify,
                             strDate.data(),
                             pSession->m_uState,
                             strTempDesc.data(),
                             0,
                             (float)0,
                             pSession->m_uSaleFee / 1000.0,
                             pSession->m_uProductCost / 1000.0,
                             pSession->m_strSubId.c_str(),
                             pSession->m_uProductType,
                             pSession->m_uChannelOperatorsType,
                             pSession->m_strChannelRemark.data(),
                             sendDate.c_str(),
                             pSession->m_strChannelTemplateId.data(),
                             sign,
                             pSession->m_strInnerErrorcode.data(),
                             pSession->m_uArea,
                             g_uComponentId);
            }
        }
        else
        {
            if (pSession->m_uFromSmsReback)
            {
                n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s "
                             "SET state='%d',submit='%s',channelid='%d',costfee='%f',"
                             "channeloperatorstype='%d',channelremark='%s',process_times='%d',submitdate='%s',"
                             "channel_tempid='%s',failed_resend_times='%d',failed_resend_channel='%s',area='%u',access_id='%lu'",
                             pSession->m_uIdentify,
                             strDate.data(),
                             pSession->m_uState,
                             pSession->m_strSubmit.data(),
                             pSession->m_uChannleId,
                             pSession->m_fCostFee * 1000,
                             pSession->m_uChannelOperatorsType,
                             pSession->m_strChannelRemark.data(),
                             pSession->m_iOverCount,
                             sendDate.c_str(),
                             pSession->m_strChannelTemplateId.data(),
                             pSession->m_iFailedResendTimes,
                             pSession->m_strFailedResendChannels.data(),
                             pSession->m_uArea,
                             g_uComponentId);	//id
            }
            else
            {
                if (pSession->m_uState == SMS_STATUS_HUMAN_CONFIRM_SUCCESS)
                {
                    n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s SET state='%u',submit='%s',channelid='%d',"
                                 "costfee='%f',salefee='%lf',productfee='%lf',sub_id='%s',product_type='%d',channeloperatorstype='%d',"
                                 "channelremark='%s',submitdate='%s',channel_tempid='%s',sign='%s',area='%u',access_id='%lu',"
                                 "report='DELIVRD',reportdate='%s',innerErrorcode='%s',channelcnt='%u'",
                                 pSession->m_uIdentify,
                                 strDate.data(),
                                 pSession->m_uState,
                                 pSession->m_strSubmit.data(),
                                 pSession->m_uChannleId,
                                 pSession->m_fCostFee * 1000,
                                 pSession->m_uSaleFee / 1000.0,
                                 pSession->m_uProductCost / 1000.0,
                                 pSession->m_strSubId.c_str(),
                                 pSession->m_uProductType,
                                 pSession->m_uChannelOperatorsType,
                                 pSession->m_strChannelRemark.data(),
                                 sendDate.c_str(),
                                 pSession->m_strChannelTemplateId.data(),
                                 sign,
                                 pSession->m_uArea,
                                 g_uComponentId,
                                 Comm::getCurrentTime_2().data(),
                                 pSession->m_strInnerErrorcode.data(),
                                 pSession->m_uSmsNum);
                }
                else if (pSession->m_uState == SMS_STATUS_HUMAN_SUBMIT_SUCCESS)
                {
                     n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s SET state='%u',submit='%s',channelid='%d',"
                                 "costfee='%f',salefee='%lf',productfee='%lf',sub_id='%s',product_type='%d',channeloperatorstype='%d',"
                                 "channelremark='%s',submitdate='%s',channel_tempid='%s',sign='%s',innerErrorcode='%s',area='%u',access_id='%lu'",
                                 pSession->m_uIdentify,
                                 strDate.data(),
                                 pSession->m_uState,
                                 pSession->m_strSubmit.data(),
                                 pSession->m_uChannleId,
                                 pSession->m_fCostFee * 1000,
                                 pSession->m_uSaleFee / 1000.0,
                                 pSession->m_uProductCost / 1000.0,
                                 pSession->m_strSubId.c_str(),
                                 pSession->m_uProductType,
                                 pSession->m_uChannelOperatorsType,
                                 pSession->m_strChannelRemark.data(),
                                 sendDate.c_str(),
                                 pSession->m_strChannelTemplateId.data(),
                                 sign,
                                 pSession->m_strInnerErrorcode.data(),
                                 pSession->m_uArea,
                                 g_uComponentId);
                }
                else
                {
                    n = snprintf(sql, sizeof(sql)-1, "UPDATE t_sms_access_%d_%s SET state='%u',submit='%s',channelid='%d',"
                                 "costfee='%f',salefee='%lf',productfee='%lf',sub_id='%s',product_type='%d',channeloperatorstype='%d',"
                                 "channelremark='%s',submitdate='%s',channel_tempid='%s',sign='%s',area='%u',access_id='%lu'",
                                 pSession->m_uIdentify,
                                 strDate.data(),
                                 pSession->m_uState,
                                 pSession->m_strSubmit.data(),
                                 pSession->m_uChannleId,
                                 pSession->m_fCostFee * 1000,
                                 pSession->m_uSaleFee / 1000.0,
                                 pSession->m_uProductCost / 1000.0,
                                 pSession->m_strSubId.c_str(),
                                 pSession->m_uProductType,
                                 pSession->m_uChannelOperatorsType,
                                 pSession->m_strChannelRemark.data(),
                                 sendDate.c_str(),
                                 pSession->m_strChannelTemplateId.data(),
                                 sign,
                                 pSession->m_uArea,
                                 g_uComponentId);
                }
            }
        }

        if ((pSession->m_uAuditState == SMS_AUDIT_STATUS_AUTO_FAIL) || (pSession->m_uAuditState == SMS_AUDIT_STATUS_AUTO_PASS))
        {
            n += snprintf(sql + n, sizeof(sql) - n, ",audit_id='%s',auditcontent='%s',audit_state='%u',audit_date='%s'",
                          pSession->m_strAuditId.c_str(),
                          pSession->m_strAuditContent.c_str(),
                          pSession->m_uAuditState,
                          pSession->m_strAuditDate.c_str());
        }

        n += snprintf( sql + n, sizeof(sql) - n, "  where id = '%s' and date ='%s';",
                       it->data(), pSession->m_strDate.data() );

        string strsql(sql);
        strsql.append("RabbitMQFlag=");
        strsql.append(it->data());

        if (NULL != pStrMq)
        {
            MqConfig *mqconf = MQGetC2SIOConfigUP( pSession->m_strCSid, pSession->m_uSmsFrom );
            if( mqconf == NULL )
            {
                LogError("smsid:%s,phone:%s, Get MQID failed. csid[%s]",
                         pSession->m_strSmsId.data(),
                         pSession->m_strPhone.data(),
                         pSession->m_strCSid.c_str());
                return;
            }

            TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
            pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
            pMQ->m_strExchange.assign(mqconf->m_strExchange);
            pMQ->m_strRoutingKey.assign(mqconf->m_strRoutingKey);
            pMQ->m_strData.append(pStrMq);
            pMQ->m_strData.append("&sql=");
            pMQ->m_strData.append(Base64::Encode(strsql));
            g_pMQIOProducerThread->PostMsg(pMQ);

            LogNotice("Exchange[%s],RoutingKey[%s],data[%s].",
                pMQ->m_strExchange.data(),
                pMQ->m_strRoutingKey.data(),
                strsql.data());
        }
        else
        {
            TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
            pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
            pMQ->m_strData.assign(strsql);
            g_pMQC2SDBProducerThread->PostMsg(pMQ);
        }

        string strTable = "t_sms_access_";
        strTable.append(Comm::int2str(pSession->m_uIdentify));
        strTable.append("_");
        strTable.append(strDate);

        MONITOR_INIT(MONITOR_ACCESS_SMS_SUBMIT);
        MONITOR_VAL_SET("clientid", pSession->m_strClientId);
        MONITOR_VAL_SET("username", pSession->m_strUserName);
        MONITOR_VAL_SET_INT("channelid", pSession->m_uChannleId);

        MONITOR_VAL_SET_INT("operatorstype", pSession->m_uOperater);
        MONITOR_VAL_SET_INT("smsfrom", pSession->m_uSmsFrom);
        MONITOR_VAL_SET_INT("paytype", pSession->m_uPayType);
        MONITOR_VAL_SET("sign", pSession->m_strSign);
        MONITOR_VAL_SET("smstype", pSession->m_strSmsType);
        MONITOR_VAL_SET("date", pSession->m_strDate);
        MONITOR_VAL_SET("uid", pSession->m_strUid);
        MONITOR_VAL_SET("dbtable",  strTable);
        MONITOR_VAL_SET("smsid", pSession->m_strSmsId);
        MONITOR_VAL_SET("phone", pSession->m_strPhone);
        MONITOR_VAL_SET("id", pSession->m_strIds);
        MONITOR_VAL_SET_INT("state", pSession->m_uState);
        MONITOR_VAL_SET("errrorcode", pSession->m_strErrorCode);
        MONITOR_VAL_SET("errordesc", strTempDesc);
        MONITOR_VAL_SET("submitcode", pSession->m_strSubmit);
        MONITOR_VAL_SET("submitdate", pSession->m_strSubmitDate);
        MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));

        Comm comm;
        MONITOR_VAL_SET_INT("costtime", time(NULL) - comm.strToTime((char*)pSession->m_strDate.data()));
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);

        MONITOR_PUBLISH(g_pMQMonitorProducerThread);
    }
}

/*动态调整会话超时时间 10s-->30S*/
UInt32 CSessionThread::GetSessionTimeoutMS()
{
    UInt32  divSessionNum  = m_mapSMSSession.size() / (ACCESS_SESSION_MAX_SIZE / 3);
    return  ACCESS_SESSION_TIMEOUT * (divSessionNum == 0 ? 1 : divSessionNum);
}
bool CSessionThread::CheckRebackSendTimes(session_pt         pInfo)
{
    do
    {
        if (pInfo->m_iOverCount >= m_iOverCount)
        {
            LogWarn("[%s:%s:%s] reback times[%d] arrived setting reback times[%d], Stop sending!",
                pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                pInfo->m_iOverCount, m_iOverCount);

            break;
        }

        long now = time(NULL);
        long iDiff = now - (long)pInfo->m_uOverTime;

        if ((iDiff > 0) && (iDiff >= (long)m_uOverTime))
        {
            LogWarn("[%s:%s:%s] time difference [%d - %d] = [%d] arrived setting time difference [%d], send fail!",
                pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(),
                now, pInfo->m_uOverTime, iDiff, m_uOverTime);

            break;
        }

        return true;
    } while (0);

    return false;
}

void CSessionThread::HandleMQRebackReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    web::HttpParams param;
    param.Parse(pMsg->m_strSessionID);

    LogNotice("==get one reback msg== msg:%s.", pMsg->m_strSessionID.data());

    session_pt pSession = SESSION_NEW();

    pSession->m_strClientId = param._map["clientId"];
    pSession->m_strUserName = Base64::Decode(param._map["userName"]);
    pSession->m_strSmsId = param._map["smsId"];
    pSession->m_strPhone = param._map["phone"];

    pSession->m_strSign = Base64::Decode(param._map["sign"]);
    m_pCChineseConvert->ChineseTraditional2Simple(pSession->m_strSign, pSession->m_strSignSimple);

    pSession->m_strContent = Base64::Decode(param._map["content"]);
    m_pCChineseConvert->ChineseTraditional2Simple(pSession->m_strContent, pSession->m_strContentSimple);

    LogDebug("[%s:%s:%s] Chinese Simple Sign[%s] Content[%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_strSignSimple.c_str(),
        pSession->m_strContentSimple.c_str());

    pSession->m_strSmsType = param._map["smsType"];
    pSession->m_uSmsFrom = atoi(param._map["smsfrom"].c_str());
    pSession->m_uPayType = atoi(param._map["paytype"].c_str());
    pSession->m_uShowSignType = atoi(param._map["showsigntype"].c_str());
    pSession->m_strExtpendPort = param._map["userextpendport"];
    pSession->m_strSignPort = param._map["signextendport"];
    pSession->m_uLongSms = atoi(param._map["longsms"].c_str());
    pSession->m_uOperater = atoi(param._map["operater"].c_str());
    pSession->m_iOverCount = atoi(param._map["process_times"].c_str());
    pSession->m_uOverTime = atol(param._map["errordate"].c_str());
    pSession->m_uSmsNum = atoi(param._map["clientcnt"].c_str());
    pSession->m_strCSid = param._map["csid"];
    pSession->m_strCSdate = param._map["csdate"];
    pSession->m_strDate = pSession->m_strCSdate;

    pSession->m_strTemplateId = param._map["templateid"];
    pSession->m_strTemplateParam = param._map["temp_params"];
    pSession->m_uOriginChannleId = atoi(param._map["oriChannelid"].c_str());
    pSession->m_strIsReback = param._map["isReback"];
    pSession->m_strChannelOverrate_access = param._map["channelOverrateDate"];

    map<string, string>::iterator iter = param._map.find("failed_resend_times");

    if (iter != param._map.end())
    {
        pSession->m_iFailedResendTimes = atoi(iter->second.c_str());
    }

    iter = param._map.find("failed_resend_channel");

    if (iter != param._map.end())
    {
        pSession->m_strFailedResendChannels = iter->second;
        Comm::split(iter->second, ",", pSession->m_FailedResendChannelsSet);
    }
    else
    {
        pSession->m_FailedResendChannelsSet.clear();
    }

    string strIDs = param._map["ids"];
    Comm::splitExVector(strIDs, ",", pSession->m_vecLongMsgContainIDs);

    pSession->m_strIds.assign(strIDs);

    pSession->m_uSessionId = m_pSnMng->getSn();
    pSession->m_uSendType = GetSendType(pSession->m_strSmsType);
    pSession->m_uArea = GetPhoneArea(pSession->m_strPhone, m_mapPhoneArea);
    m_mapSMSSession[ pSession->m_uSessionId ] = pSession;
    m_uCurSessionSize++;

    LogDebug("------m_strChannelOverrate_access[%s], m_uOriginChannleId[%u], m_iFailedResendTimes[%d], m_iOverCount[%d]",
        pSession->m_strChannelOverrate_access.data(),
        pSession->m_uOriginChannleId,
        pSession->m_iFailedResendTimes,
        pSession->m_iOverCount);

    if (!pSession->m_strChannelOverrate_access.empty()
        && pSession->m_uOriginChannleId != 0
        && pSession->m_iFailedResendTimes == 0
        && ((pSession->m_iOverCount == 1)
            || (pSession->m_iOverCount > 1 && pSession->m_strIsReback.empty())))
    {
        g_pOverRateThread->smsChannelDayOverRateDescRedis(pSession->m_strChannelOverrate_access, -1);
    }

    pSession->m_uFromSmsReback = 1;

    ////get some account info
    AccountMap::iterator itrAccount = m_mapAccount.find(pSession->m_strClientId);

    if (itrAccount == m_mapAccount.end())
    {
        LogError("[%s:%s:%s] is not find in m_AccountMap.",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str());

        pSession->m_uState = SMS_STATUS_SUBMIT_FAIL;
        pSession->m_strSubmit = CLIENTID_IS_INVALID;

        goto SEND_FAILED;
    }

    pSession->m_uIdentify = itrAccount->second.m_uIdentify;

    if (CheckRebackSendTimes(pSession) == false)
    {
        goto SEND_FAILED;
    }

    pSession->m_iOverCount++;

    GenerateMQSendUrl(pSession);
    checkOverRateWhiteList(pSession);
    sessionStateProc(pSession, STATE_BLACKLIST);

    return;


SEND_FAILED:
    pSession->m_uRepushPushType = MQ_PUSH_REPORT;
    pSession->m_strSubmit = REBACK_OVER_TIME;
    pSession->m_uState = SMS_STATUS_SUBMIT_FAIL;
    sessionStateProc(pSession, STATE_DONE);
}

void CSessionThread::GenerateMQSendUrl(session_pt pSmsInfo)
{
    //clientId
    pSmsInfo->m_strSendUrl.append("clientId=").append(pSmsInfo->m_strClientId);
    //userName
    pSmsInfo->m_strSendUrl.append("&userName=").append(Base64::Encode(pSmsInfo->m_strUserName));
    //smsId
    pSmsInfo->m_strSendUrl.append("&smsId=").append(pSmsInfo->m_strSmsId);
    //phone
    pSmsInfo->m_strSendUrl.append("&phone=").append(pSmsInfo->m_strPhone);
    //sign
    pSmsInfo->m_strSendUrl.append("&sign=").append(Base64::Encode(pSmsInfo->m_strSign));
    //content
    pSmsInfo->m_strSendUrl.append("&content=").append(Base64::Encode(pSmsInfo->m_strContent));
    //smsType
    pSmsInfo->m_strSendUrl.append("&smsType=").append(pSmsInfo->m_strSmsType);
    //smsfrom
    pSmsInfo->m_strSendUrl.append("&smsfrom=").append(Comm::int2str(pSmsInfo->m_uSmsFrom));
    //paytype
    pSmsInfo->m_strSendUrl.append("&paytype=").append(Comm::int2str(pSmsInfo->m_uPayType));

    ////operater
    pSmsInfo->m_strSendUrl.append("&operater=");
    pSmsInfo->m_strSendUrl.append(Comm::int2str(pSmsInfo->m_uOperater));
    ////ids
    pSmsInfo->m_strSendUrl.append("&ids=");
    pSmsInfo->m_strSendUrl.append(pSmsInfo->m_strIds);
    ////clientcnt
    pSmsInfo->m_strSendUrl.append("&clientcnt=");
    pSmsInfo->m_strSendUrl.append(Comm::int2str(pSmsInfo->m_uSmsNum));

    //process_times
    pSmsInfo->m_strSendUrl.append("&process_times=");
    pSmsInfo->m_strSendUrl.append(Comm::int2str(pSmsInfo->m_iOverCount));
    ////csid
    pSmsInfo->m_strSendUrl.append("&csid=");
    pSmsInfo->m_strSendUrl.append(pSmsInfo->m_strCSid);
    ////csdate
    pSmsInfo->m_strSendUrl.append("&csdate=");
    pSmsInfo->m_strSendUrl.append(pSmsInfo->m_strCSdate);
    ////errordate
    pSmsInfo->m_strSendUrl.append("&errordate=");
    pSmsInfo->m_strSendUrl.append(Comm::int2str(pSmsInfo->m_uOverTime));

    pSmsInfo->m_strSendUrl.append("&templateid=").append(pSmsInfo->m_strTemplateId);
    pSmsInfo->m_strSendUrl.append("&temp_params=").append(pSmsInfo->m_strTemplateParam);

    //userextpendport
    pSmsInfo->m_strSendUrl.append("&userextpendport=").append(pSmsInfo->m_strExtpendPort);
    //signextendport
    pSmsInfo->m_strSendUrl.append("&signextendport=").append(pSmsInfo->m_strSignPort);
    //showsigntype
    pSmsInfo->m_strSendUrl.append("&showsigntype=").append(Comm::int2str(pSmsInfo->m_uShowSignType));

    pSmsInfo->m_strSendUrl.append("&failed_resend_times=").append(Comm::int2str(pSmsInfo->m_iFailedResendTimes));
    pSmsInfo->m_strSendUrl.append("&failed_resend_channel=").append(pSmsInfo->m_strFailedResendChannels);
}

void CSessionThread::HandleMQReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    TMsg* pHttpSend = (TMsg*)(pMsg);
    web::HttpParams param;
    param.Parse(pHttpSend->m_strSessionID);

    session_pt pSession = SESSION_NEW();
    CHK_NULL_RETURN(pSession);

    pSession->m_strClientId = param._map["clientId"];
    pSession->m_strUserName = Base64::Decode(param._map["userName"]);
    pSession->m_strSmsId = param._map["smsId"];
    pSession->m_strPhone = param._map["phone"];
    pSession->m_strSmsType = param._map["smsType"];
    pSession->m_uSmsFrom = atoi(param._map["smsfrom"].c_str());
    pSession->m_uPayType = atoi(param._map["paytype"].c_str());
    pSession->m_uShowSignType = atoi(param._map["showsigntype"].c_str());
    pSession->m_strExtpendPort = param._map["userextpendport"];
    pSession->m_uSmsNum = atoi(param._map["clientcnt"].c_str());
    pSession->m_strCSid = param._map["csid"];
    pSession->m_strCSdate = param._map["csdate"];
    pSession->m_strDate = pSession->m_strCSdate;
    pSession->m_strSignPort = param._map["signport"];

    pSession->m_strSign = Base64::Decode(param._map["sign"]);
    m_pCChineseConvert->ChineseTraditional2Simple(pSession->m_strSign, pSession->m_strSignSimple);

    pSession->m_strContent = Base64::Decode(param._map["content"]);
    m_pCChineseConvert->ChineseTraditional2Simple(pSession->m_strContent, pSession->m_strContentSimple);

    LogDebug("[%s:%s:%s] Chinese Simple Sign[%s] Content[%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_strSignSimple.c_str(),
        pSession->m_strContentSimple.c_str());

    pSession->m_strTemplateId = param._map["templateid"];
    pSession->m_strTemplateParam = param._map["temp_params"];
    pSession->m_strTemplateType = param._map["temp_type"];
    pSession->m_uAuditFlag = atoi(param._map["audit_flag"].c_str());
    pSession->m_uSelectChannelNum = atoi(param._map["route_times"].c_str());
    pSession->m_strTestChannelId = param._map["test_channelid"];
    pSession->m_iClientSendLimitCtrlTypeFlag = atoi(param._map["send_limit_flag"].c_str());

    string strIDs = param._map["ids"];
    pSession->m_strIds = strIDs;
    Comm comm;
    comm.splitExVector(strIDs, ",", pSession->m_vecLongMsgContainIDs);

    //保存会话
    pSession->m_uSessionId = m_pSnMng->getSeq64();
    m_mapSMSSession[ pSession->m_uSessionId ] = pSession;
    pSession->m_pSessionTimer = SetTimer(pSession->m_uSessionId, ACCESS_SESSION_TIMER_ID, GetSessionTimeoutMS());
    m_uCurSessionSize++;

    LogDebug("[%s:%s:%s] SessionId[%lu], msg:%s == access process message ==",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_uSessionId,
        pHttpSend->m_strSessionID.c_str());

    sessionStateProc(pSession, STATE_INIT);
}

MqConfig* CSessionThread::MQGetC2SIOConfigDown(session_pt pSession)
{
    CHK_NULL_RETURN_NULL(pSession);

    SmsAccount* account = GetAccountInfo(pSession->m_strClientId, m_mapAccount);

    if (!account)
    {
        LogError("[%s:%s:%s] can not find account in account map!",
            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());
        return NULL;
    }

    string strMessageType = "";
    UInt32 uPhoneType = pSession->m_uOperater;
    UInt64 uSmsType = atoi(pSession->m_strSmsType.data());

    if (FOREIGN == uPhoneType)
    {
        if (SMS_TYPE_USSD == uSmsType || SMS_TYPE_FLUSH_SMS == uSmsType
            || TEMPLATE_TYPE_HANG_UP_SMS == atoi((pSession->m_strTemplateType).c_str()))
        {
            LogError("[%s:%s:%s] phone operator[%d] error for smstype[%d]",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                uPhoneType, uSmsType);
        }
    }

    if (account->m_uClientLeavel == 2
        || account->m_uClientLeavel == 3)  // big and middle client
    {
        if (YIDONG == uPhoneType)
        {
            if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
            {
                strMessageType.assign("02");
            }
            else
            {
                strMessageType.assign("01");
            }
        }
        else if (DIANXIN == uPhoneType)
        {
            if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
            {
                strMessageType.assign("06");
            }
            else
            {
                strMessageType.assign("05");
            }
        }
        else if ((LIANTONG == uPhoneType) || (FOREIGN == uPhoneType))
        {
            if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
            {
                strMessageType.assign("04");
            }
            else
            {
                strMessageType.assign("03");
            }
        }
        else
        {
            LogError("[%s:%s:%s] ==except== phoneType:%u is invalid.",
                pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
                uPhoneType);
            return NULL;
        }
    }
    else if (account->m_uClientLeavel == 1) // small client
    {
        if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
        {
            strMessageType.assign("08");
        }
        else
        {
            strMessageType.assign("07");
        }
    }
    else
    {
        LogError("[%s:%s:%s] client leavel[%d] is error",
            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
            account->m_uClientLeavel);
        return NULL;
    }

    string strKey = "";
    char temp[250] = {0};
    snprintf(temp, 250, "%s_%s_0", pSession->m_strCSid.c_str(), strMessageType.data());
    strKey.assign(temp);

    MqConfig* mqconfig = GetMQConfig(strKey, m_mqConfigRef, m_mqConfigInfo);

    if (!mqconfig)
    {
        LogError("[%s:%s:%s] mq Config Not Find , smstype[%s], mqid[--]",
            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
            pSession->m_strSmsType.c_str());
    }

    return mqconfig;

}

MqConfig* CSessionThread::MQGetC2SIOConfigUP(string strC2Sid, UInt32 uSmsfrom)
{
    if (strC2Sid.empty())
    {
        LogError("errorc2sid is empty");
        return NULL;
    }

    string strKey = "";
    strKey.assign(strC2Sid);

    if ((SMS_FROM_ACCESS_HTTP == uSmsfrom) || (SMS_FROM_ACCESS_HTTP_2 == uSmsfrom))
    {
        strKey.append(COMPONENT_TYPE_HTTP);
    }
    else
    {
        strKey.append(COMPONENT_TYPE_C2S);
    }

    ComponentConfig* pCompConf = getComponentConfig(strKey, m_componentConfigInfo);

    if (!pCompConf)
    {
        LogError("find c2sid_type[%s] from m_componentConfigInfoMap failed", strKey.c_str());
        return NULL;
    }

    return GetMQConfig(pCompConf->m_uMqId, m_mqConfigInfo);

}

smsSessionInfo* CSessionThread::GetSessionBySeqID(UInt64 uSeq)
{
    SMSSessionMAp::iterator itr = m_mapSMSSession.find(uSeq);

    if (itr !=  m_mapSMSSession.end())
    {
        return itr->second;
    }

    return NULL;
}

void CSessionThread::checkSendLimit(session_pt pSession, int iControlType)
{
    CHK_NULL_RETURN(pSession);

    LogDebug("FromSmsReback[%u],PhoneOperater[%u].",
        pSession->m_uFromSmsReback, pSession->m_uOperater);

    if (1 == pSession->m_uFromSmsReback) return;
    if (BusType::FOREIGN == pSession->m_uOperater) return;

    // 用户发送控制配置判断
    string strSendLimitCfgKey;
    strSendLimitCfgKey.append(pSession->m_strClientId);
    strSendLimitCfgKey.append("_");
    strSendLimitCfgKey.append(Comm::int2str(iControlType));
    strSendLimitCfgKey.append("_");
    strSendLimitCfgKey.append(pSession->m_strSmsType);

    AppDataMapCIter iter = m_mapClientSendLimitCfg.find(strSendLimitCfgKey);

    if (m_mapClientSendLimitCfg.end() == iter)
    {
        LogDebug("[%s:%s:%s] not find send limit cfg, key[%s].",
            strSendLimitCfgKey.data(),
            pSession->m_strSmsId.data(),
            pSession->m_strPhone.data(),
            strSendLimitCfgKey.data());

        return;
    }

    const TsmsClientSendLimit& limitCfg = *(TsmsClientSendLimit*)(iter->second);

    // 时间判断
    string strCurHourMins = Comm::getCurrHourMins();

    if ((strCurHourMins < limitCfg.m_strLimitTimeStart) || (strCurHourMins > limitCfg.m_strLimitTimeEnd))
    {
        LogDebug("[%s:%s:%s] CurHourMins[%s] is not within SendLimitCfgTimeRange[%s~%s].",
            strSendLimitCfgKey.data(),
            pSession->m_strSmsId.data(),
            pSession->m_strPhone.data(),
            strCurHourMins.data(),
            limitCfg.m_strLimitTimeStart.data(),
            limitCfg.m_strLimitTimeEnd.data());

        return;
    }

    // 刷新缓存
    string strCurDay = Comm::getCurrentDay();
    SendLimitCtrlMap::iterator iter1 = m_mapSendLimitCtrl.find(strSendLimitCfgKey);

    if (m_mapSendLimitCtrl.end() == iter1)
    {
        SendLimitCtrl limitCtrl;
        limitCtrl.m_strDate = strCurDay;
        limitCtrl.m_uiCount = 0;
        limitCtrl.m_uiThreshold = 100 / limitCfg.m_dLimitRate;
        limitCtrl.m_strUpdateTime = limitCfg.m_strUpdateTime;
        m_mapSendLimitCtrl[strSendLimitCfgKey] = limitCtrl;

        LogWarn("[%s:%s:%s] ==send limit== state[start],UpdateTime[%s],LimitRate[%lf],Threshold[%u].",
            strSendLimitCfgKey.data(),
            pSession->m_strSmsId.data(),
            pSession->m_strPhone.data(),
            limitCtrl.m_strUpdateTime.data(),
            limitCfg.m_dLimitRate,
            limitCtrl.m_uiThreshold);

        if ((limitCtrl.m_uiCount + 1) >= limitCtrl.m_uiThreshold)
        {
            // 标识当前短信需要进行发送控制
            pSession->m_iClientSendLimitCtrlTypeFlag = iControlType;
            
            LogWarn("[%s:%s:%s] ==send limit== state[flag],LimitRate[%lf],Count[%u],Threshold[%u].",
                strSendLimitCfgKey.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                limitCfg.m_dLimitRate,
                limitCtrl.m_uiCount + 1,
                limitCtrl.m_uiThreshold);
            limitCtrl.m_uiCount = 0;
        }
    }
    else
    {
        SendLimitCtrl& limitCtrl = iter1->second;

        LogDebug("[%s:%s:%s] Date[%s],UpdateTime[%s],Count[%u],Threshold[%u].",
            pSession->m_strClientId.data(),
            pSession->m_strSmsId.data(),
            pSession->m_strPhone.data(),
            limitCtrl.m_strDate.data(),
            limitCtrl.m_strUpdateTime.data(),
            limitCtrl.m_uiCount,
            limitCtrl.m_uiThreshold);

        if (limitCtrl.m_strDate != strCurDay)
        {
            limitCtrl.m_strDate = strCurDay;
            limitCtrl.m_uiCount = 0;
        }

        if ((limitCtrl.m_uiCount + 1) >= limitCtrl.m_uiThreshold)
        {
            // 标识当前短信需要进行发送控制
            pSession->m_iClientSendLimitCtrlTypeFlag = iControlType;
            LogWarn("[%s:%s:%s] ==send limit== state[flag],LimitRate[%lf],Count[%u],Threshold[%u].",
                strSendLimitCfgKey.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                limitCfg.m_dLimitRate,
                limitCtrl.m_uiCount + 1,
                limitCtrl.m_uiThreshold);
            limitCtrl.m_uiCount = 0;
        }
    }
}

void CSessionThread::refreshSendLimit(session_pt pSession, int iControlType)
{
    CHK_NULL_RETURN(pSession);

    if (1 == pSession->m_uFromSmsReback) return;
    if (BusType::FOREIGN == pSession->m_uOperater) return;

    string strSendLimitCfgKey;
    strSendLimitCfgKey.append(pSession->m_strClientId);
    strSendLimitCfgKey.append("_");
    strSendLimitCfgKey.append(Comm::int2str(iControlType));
    strSendLimitCfgKey.append("_");
    strSendLimitCfgKey.append(pSession->m_strSmsType);

    SendLimitCtrlMap::iterator iter = m_mapSendLimitCtrl.find(strSendLimitCfgKey);

    if (m_mapSendLimitCtrl.end() != iter)
    {
        SendLimitCtrl& ctrl = iter->second;

       /* if (pSession->m_iClientSendLimitCtrlTypeFlag == iControlType)
        {
            ctrl.m_uiCount = 0;

            LogWarn("[%s:%s:%s] ==send limit== state[reset],Date[%s],Count[0],Threshold[%u].",
                strSendLimitCfgKey.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                ctrl.m_strDate.data(),
                ctrl.m_uiThreshold);
        }
        else*/
       // {
            ctrl.m_uiCount = ctrl.m_uiCount + 1;

            LogNotice("[%s:%s:%s] ==send limit== state[increase],Date[%s],Count[%u],Threshold[%u].",
                strSendLimitCfgKey.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                ctrl.m_strDate.data(),
                ctrl.m_uiCount,
                ctrl.m_uiThreshold);
        //}
    }
}

void CSessionThread::clearSendLimit(AppDataMap& mapAppData)
{
    // 清缓存
    for (AppDataMapIter iter = mapAppData.begin(); iter != mapAppData.end(); ++iter)
    {
        const string& strKey = iter->first;
        const TsmsClientSendLimit& limitCfg = *(TsmsClientSendLimit*)(iter->second);

        SendLimitCtrlMap::iterator iter1 = m_mapSendLimitCtrl.find(strKey);

        if (m_mapSendLimitCtrl.end() != iter1)
        {
            const SendLimitCtrl& limitCtrl = iter1->second;

            if (limitCtrl.m_strUpdateTime != limitCfg.m_strUpdateTime)
            {
                LogWarn("==send limit== state[clear],ClientId[%s],LimitType[%u],SmsType[%u],UpdateTime[%s->%s].",
                    limitCfg.m_strClientId.data(),
                    limitCfg.m_uiLimitType,
                    limitCfg.m_uiSmsType,
                    limitCtrl.m_strUpdateTime.data(),
                    limitCfg.m_strUpdateTime.data());

                m_mapSendLimitCtrl.erase(iter1);
            }
        }
    }

    for (SendLimitCtrlMap::iterator iter = m_mapSendLimitCtrl.begin(); iter != m_mapSendLimitCtrl.end();)
    {
        const string& strKey = iter->first;
        const SendLimitCtrl& limitCtrl = iter->second;

        if (mapAppData.end() == mapAppData.find(strKey))
        {
            LogWarn("==send limit== state[clear],ClientId_LimitType_SmsType[%s],UpdateTime[%s].",
                strKey.data(),
                limitCtrl.m_strUpdateTime.data());

            m_mapSendLimitCtrl.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }

    // 更新
    clearAppData(m_mapClientSendLimitCfg);
    m_mapClientSendLimitCfg = mapAppData;
    LogNotice("update t_sms_client_send_limit:%u. size[%u].", t_sms_client_send_limit, m_mapClientSendLimitCfg.size());
}

void CSessionThread::getChargeRule(session_pt pSession)
{
    CHK_NULL_RETURN(pSession);

    string strKey = pSession->m_strClientId + ":charge_rule";
    AppDataVecMapIter iter = m_mapVecUserProperty.find(strKey);

    if (m_mapVecUserProperty.end() != iter)
    {
        const vector<AppData*>& v = iter->second;
        const string strTime = getCurrentDateTime("%Y-%m-%d");

        for (vector<AppData*>::const_iterator iter = v.begin(); iter != v.end(); ++iter)
        {
            const TSmsUserPropertyLog* pUserProperty = (TSmsUserPropertyLog*)(*iter);

            LogDebug(" --> ClientId[%s],ChargeRule[%s],EffectDate[%s].",
                pUserProperty->m_strClientId.data(),
                pUserProperty->m_strValue.data(),
                pUserProperty->m_strEffectDate.data());

            if (pUserProperty->m_strEffectDate <= strTime)
            {
                pSession->m_ucChargeRule = to_double(pUserProperty->m_strValue);
            }
            else
            {
                break;
            }
        }
    }
}

