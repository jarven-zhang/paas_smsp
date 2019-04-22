#include "main.h"
#include "json/value.h"
#include "json/writer.h"
#include "base64.h"
#include "UrlCode.h"
#include "CHttpsPushThread.h"
#include "cityhash/cityHashShare.h"
#include "Fmt.h"

CReportPushThread::CReportPushThread(const char *name) : CThread(name)
{
    m_uiHttpsReportRetry = 3;
}

CReportPushThread::~CReportPushThread()
{
}

bool CReportPushThread::Init()
{
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }
    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }
    m_pInternalService->Init();

    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);
    g_pRuleLoadThread->getComponentConfig(m_componentCfg);
    g_pRuleLoadThread->getMqConfig(m_mapMqCfg);

    map<string, string> mapSysPara;
    g_pRuleLoadThread->getSysParamMap(mapSysPara);
    initSysPara(mapSysPara);

    m_pLinkedBlockPool = new LinkedBlockPool();

    //errorCode link errorDesc
    m_mapReportErrCode_Desc["YX:1000"] = "系统超频错误";
    m_mapReportErrCode_Desc["YX:5009"] = "订单余额不足";
    m_mapReportErrCode_Desc["YX:7000"] = "审核不通过";
    m_mapReportErrCode_Desc["YX:9001"] = "号码格式错误";
    m_mapReportErrCode_Desc["YX:9002"] = "账号不存在";
    m_mapReportErrCode_Desc["YX:9004"] = "无可用通道组";
    m_mapReportErrCode_Desc["YX:9006"] = "无可用通道";
    m_mapReportErrCode_Desc["YX:9999"] = "其他错误";

    return true;
}

void CReportPushThread::initSysPara(const map<string, string> &mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "HTTPS_REPORT_RETRY";
        iter = mapSysPara.find(strSysPara);

        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;
        int uiTmp = to_uint<UInt32>(strTmp);
        if ((uiTmp < 1) || (uiTmp > 10))
        {
            LogError("Invalid system parameter(%s) value(%s, %u).",
                     strSysPara.c_str(), strTmp.c_str(), uiTmp);

            break;
        }

        m_uiHttpsReportRetry = uiTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uiHttpsReportRetry);
}

void CReportPushThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER
    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg *pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }
        else if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            SAFE_DELETE(pMsg);
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void CReportPushThread::HandleMsg(TMsg *pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
    case MSGTYPE_REPORTRECEIVE_TO_SMS_REQ:      //TODO . USERTABLE,
    {
        HandleReportReciveTOReportPushReq(pMsg);
        break;
    }
    case MSGTYPE_ACCESS_MO_MSG:
    {
        HandleMoMsg(pMsg);
        break;
    }
    case MSGTYPE_REPORTRECEIVE_TO_REPUSH_REQ:
    {
        handleReportRePushReq(pMsg);
        break;
    }
    case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
    {
        HandleAccountUpdateReq(pMsg);
        break;
    }
    case MSGTYPE_HTTPRESPONSE:
    {
        HandleHttpResponseMsg(pMsg);
        break;
    }
    case MSGTYPE_HOSTIP_RESP:
    {
        THostIpResp *pHost = (THostIpResp *)pMsg;
        HandleHostIpResp(pHost->m_iResult, pHost->m_uIp, pHost->m_iSeq);
        break;
    }
    case MSGTYPE_REDIS_RESP:
    {
        HandleRedisResp(pMsg);
        break;
    }

    case MSGTYPE_HTTPS_ASYNC_RESPONSE:
    {
        HandleHttpsResponseMsg(pMsg);
        break;
    }

    case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
    {
        TUpdateComponentConfigReq *pReq = (TUpdateComponentConfigReq *)pMsg;
        m_componentCfg = pReq->m_componentConfig;
        LogNotice("update t_sms_component_configure.");
        break;
    }

    case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
    {
        TUpdateMqConfigReq *pReq = (TUpdateMqConfigReq *)pMsg;

        LogNotice("update t_sms_mq_configure. size[%u->%u].",
                  m_mapMqCfg.size(), pReq->m_mapMqConfig.size());

        m_mapMqCfg = pReq->m_mapMqConfig;
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
    {
        TUpdateSysParamRuleReq *msg = (TUpdateSysParamRuleReq *)pMsg;
        initSysPara(msg->m_SysParamMap);
        break;
    }
    case MSGTYPE_TIMEOUT:
    {
        LogNotice("this is timeout iSeq[%ld]", pMsg->m_iSeq);
        HandleTimeOutMsg(pMsg);
        break;
    }
    default:
    {
        LogWarn("msgType[%d] is invalid.", pMsg->m_iMsgType);
        break;
    }
    }
}

void CReportPushThread::HandleAccountUpdateReq(TMsg *pMsg)
{
    LogDebug("Account update.");
    TUpdateSmsAccontReq *pAccountUpdateReq = (TUpdateSmsAccontReq *)pMsg;

    AccountMap::iterator itOldAccount = m_AccountMap.begin();
    for( ; itOldAccount != m_AccountMap.end(); itOldAccount++)
    {
        AccountMap::iterator itNewAccount = pAccountUpdateReq->m_SmsAccountMap.find(itOldAccount->second.m_strAccount);
        if(itNewAccount != pAccountUpdateReq->m_SmsAccountMap.end())
        {
            itNewAccount->second.m_uLoginErrorCount = itOldAccount->second.m_uLoginErrorCount;
            itNewAccount->second.m_uLinkCount = itOldAccount->second.m_uLinkCount;
            itNewAccount->second.m_uLockTime = itOldAccount->second.m_uLockTime;
        }
    }
    m_AccountMap.clear();
    m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
}

void CReportPushThread::HandleReportReciveTOReportPushReq(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TReportReceiveToSMSReq *pReq = (TReportReceiveToSMSReq *)pMsg;

    ReportPushSession *pSession = new ReportPushSession(this);
    CHK_NULL_RETURN(pSession);
    m_mapSession[pSession->m_ulSeq] = pSession;
    pSession->m_strSmsId = pReq->m_strSmsId;
    pSession->m_strDesc = pReq->m_strDesc;
    pSession->m_strReportDesc = pReq->m_strReportDesc;
    pSession->m_strPhone = pReq->m_strPhone;
    pSession->m_strContent = pReq->m_strContent;
    pSession->m_strUpstreamTime = pReq->m_strUpstreamTime;
    pSession->m_strClientId = pReq->m_strClientId;
    pSession->m_strSrcId = pReq->m_strSrcId;
    pSession->m_iLinkId = pReq->m_iLinkId;
    pSession->m_iType = 1;
    pSession->m_iStatus = pReq->m_iStatus;
    pSession->m_strInnerErrcode = pReq->m_strInnerErrcode;
    pSession->m_lReportTime = pReq->m_lReportTime;
    pSession->m_uUpdateFlag = pReq->m_uUpdateFlag;
    pSession->m_uIdentify = pReq->m_uIdentify;
    pSession->m_uChannelCount = pReq->m_uChannelCount;
    pSession->m_uSmsfrom = pReq->m_uSmsfrom;

    //accesssms:smsid_phone submitid *** clientid ***
    string strKey;
    strKey.append("accesssms:");
    strKey.append(pSession->m_strSmsId);
    strKey.append("_");
    strKey.append(pSession->m_strPhone);

    TRedisReq *pRedisReq = new TRedisReq();
    pRedisReq->m_RedisCmd = "HGETALL " + strKey;
    pRedisReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedisReq->m_iSeq = pSession->m_ulSeq;
    pRedisReq->m_pSender = g_pReportPushThread;
    pRedisReq->m_strSessionID = "getReportInfo";
    pRedisReq->m_strKey = strKey;

    LogDebug("[%s:%s:%s] redis cmd[%s].",
             pSession->m_strClientId.data(),
             pSession->m_strSmsId.data(),
             pSession->m_strPhone.data(),
             pRedisReq->m_RedisCmd.data());

    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisReq);

    MONITOR_INIT(MONITOR_ACCESS_SMS_REPORT);
    MONITOR_VAL_SET("clientid", pReq->m_strClientId);
    //MONITOR_VAL_SET("username", pSession->m_strU);
    MONITOR_VAL_SET("channelid", pReq->m_strChannelId);
    //MONITOR_VAL_SET("channelname", pReq->m_strCha);
    MONITOR_VAL_SET("smsid", pReq->m_strSmsId);
    MONITOR_VAL_SET("phone", pReq->m_strPhone);
    //MONITOR_VAL_SET("id", );
    MONITOR_VAL_SET_INT("state",  pReq->m_iStatus);
    //MONITOR_VAL_SET("reportcode", pReq->m_strDesc);
    MONITOR_VAL_SET("reportdesc", pReq->m_strReportDesc);
    MONITOR_VAL_SET("reportdate", Comm::getCurrentTime_z(pReq->m_lReportTime));
    MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));
    MONITOR_VAL_SET_INT("costtime", time(NULL) - pReq->m_lReportTime );
    MONITOR_VAL_SET_INT("component_id", g_uComponentId);
    MONITOR_PUBLISH(g_pMQMonitorPublishThread);
}

void CReportPushThread::HandleMoMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TReportReceiverMoMsg *pReq = (TReportReceiverMoMsg *)pMsg;

    ReportPushSession *pSession = new ReportPushSession(this);
    CHK_NULL_RETURN(pSession);
    m_mapSession[pSession->m_ulSeq] = pSession;
    pSession->m_strClientId = pReq->m_strClientId;
    pSession->m_strSign = pReq->m_strSign;
    pSession->m_strSmsId = pReq->m_strMoId;
    pSession->m_strSignPort = pReq->m_strSignPort;
    pSession->m_strUserPort = pReq->m_strUserPort;
    pSession->m_strSourceExtend = pReq->m_strUserPort;
    pSession->m_strPhone = pReq->m_strPhone;
    pSession->m_strContent = pReq->m_strContent;
    pSession->m_strTime = pReq->m_strTime;
    pSession->m_strMourl = pReq->m_strMoUrl;
    pSession->m_iType = 2; //upstream
    pSession->m_uSmsfrom = pReq->m_uSmsfrom;

    //get accountInfo from accountMap
    AccountMap::iterator itAccount = m_AccountMap.find(pReq->m_strClientId);
    if(itAccount == m_AccountMap.end())
    {
        LogWarn("up stream push to user failed!can not find CliendIDInfo from m_AccountMap. cliendID[%s]",
                pReq->m_strClientId.data());

        m_mapSession.erase(pSession->m_ulSeq);
        SAFE_DELETE(pSession);
        return;
    }

    LogDebug("[%s:%s] account get suc. clientID[%s]",
             pSession->m_strSmsId.data(),
             pSession->m_strPhone.data(),
             pReq->m_strClientId.data());

    string strTemp = itAccount->second.m_strSpNum + pReq->m_strUserPort;
    pSession->m_strUserPort = strTemp;
    pSession->m_uNeedMo = itAccount->second.m_uNeedMo;

    //no need to check redis , so set redis flag true
    //pSession->m_bRedisRetrunFlag = true;

    //check wheather need to push MO to user
    if (((itAccount->second.m_uNeedMo == 1) && (!itAccount->second.m_strMoUrl.empty()))
            || itAccount->second.m_uNeedMo == 3)
    {
        string strIp = "";
        if (true == m_SmspUtils.CheckIPFromUrl(itAccount->second.m_strMoUrl, strIp))
        {
            //pSession->m_bHostIPstate = true;
            HandleHostIpResp(0, inet_addr(strIp.data()), pSession->m_ulSeq);
        }
        else if(itAccount->second.m_uNeedMo == 3)           //ADD by fangjinxiong 20170412 . user get mo
        {
            HandleHostIpResp(0, 0, pSession->m_ulSeq);
        }
        else    //查hostIP
        {
            THostIpReq *pHost = new THostIpReq();
            pHost->m_DomainUrl = itAccount->second.m_strMoUrl;
            pHost->m_iSeq = pSession->m_ulSeq;
            pHost->m_iMsgType = MSGTYPE_HOSTIP_REQ;
            pHost->m_pSender = this;
            g_pHostIpThread->PostMsg(pHost);
        }
    }
    else    //不回调
    {
        LogWarn("[%s:%s] push upstream to user failed. m_uNeedMo[%d], m_strMoUrl[%s]",
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                itAccount->second.m_uNeedMo,
                itAccount->second.m_strMoUrl.data());

        m_mapSession.erase(pSession->m_ulSeq);
        SAFE_DELETE(pSession);
    }
}

void CReportPushThread::HandleHostIpResp(int iResult, UInt32 uIp, UInt64 iSeq)
{
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapCallbackInfo.", iSeq);
        return;
    }

    if (0 != iResult)
    {
        LogError("notifyUrl[%s] get host ip is failed.", itr->second->m_strDeliveryUrl.data());
        SAFE_DELETE(itr->second); //no need
        m_mapSession.erase(itr);
        return;
    }

    LogDebug("***Post***iSeq[%lu],cliendID[%s],smsId[%s],phone[%s].",
             iSeq,
             itr->second->m_strClientId.data(),
             itr->second->m_strSmsId.data(),
             itr->second->m_strPhone.data());

    itr->second->m_uIp = uIp;
    //itr->second->m_bHostIPstate = true;       //标志位值为true

    if (SMS_FROM_ACCESS_HTTP == itr->second->m_uSmsfrom)
    {
        CheckState(itr);
    }
    else
    {
        CheckState_2(itr);
    }
}

void CReportPushThread::HandleRedisResp(TMsg *pMsg)
{
    TRedisResp *pResp = (TRedisResp *)pMsg;

    if ((NULL == pResp->m_RedisReply)
            || (pResp->m_RedisReply->type == REDIS_REPLY_ERROR)
            || (pResp->m_RedisReply->type == REDIS_REPLY_NIL)
            || ((pResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pResp->m_RedisReply->elements == 0)))
    {
        LogWarn("******iSeq[%lu] redisSessionId[%s],query redis is failed.******",
                pMsg->m_iSeq,
                pMsg->m_strSessionID.data());

        if (NULL != pResp->m_RedisReply)
        {
            freeReply(pResp->m_RedisReply);
        }

        SessionMap::iterator itr = m_mapSession.find(pResp->m_iSeq);
        if (itr == m_mapSession.end())
        {
            LogError("iSeq[%lu] is not find m_mapSession.", pResp->m_iSeq);
            return;
        }

        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    if ("getReportInfo" == pMsg->m_strSessionID)
    {
        ////accesssms:smsid_phone submitid *** clientid *** uid
        SessionMap::iterator itr = m_mapSession.find(pResp->m_iSeq);
        if (itr == m_mapSession.end())
        {
            LogError("iSeq[%lu] is not find m_mapSession.", pResp->m_iSeq);
            return;
        }

        //fill session.m_strUid / session.m_strMourl
        string strUid = "";
        paserReply("uid", pResp->m_RedisReply, strUid);
        if (0 == strUid.compare("NULL"))
        {
            strUid.assign("");
        }
        itr->second->m_strUid.assign(strUid);
        paserReply("deliveryurl", pResp->m_RedisReply, itr->second->m_strDeliveryUrl);

        //ids
        string strIDs;
        paserReply("id", pResp->m_RedisReply, strIDs);      //httpServer only has 1 id, strIDs contains 1 id

        //ids
        string strDate;
        paserReply("date", pResp->m_RedisReply, strDate);

        //clientid
        string strClientID;
        paserReply("clientid", pResp->m_RedisReply, strClientID);
        freeReply(pResp->m_RedisReply);


        //del redis             accesssms:smsid_phone           m_strSmsId
        string cmdkey = "accesssms:";
        cmdkey = cmdkey + itr->second->m_strSmsId + "_" + itr->second->m_strPhone;
        TRedisReq *pRedisDel = new TRedisReq();
        if (NULL == pRedisDel)
        {
            LogError("pRedisDel is NULL.");
            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        pRedisDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisDel->m_RedisCmd.assign("DEL  ");
        pRedisDel->m_RedisCmd.append(cmdkey);
        pRedisDel->m_strKey = cmdkey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisDel);


        //通过clientID找到账户信息，
        AccountMap::iterator itAccount = m_AccountMap.find(strClientID);
        if(itAccount == m_AccountMap.end())
        {
            LogWarn("push report to user failed!can not find CliendIDInfo from m_AccountMap.  cliendID[%s]",
                    strClientID.data());
            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        LogDebug("account get suc. clientID[%s]", strClientID.data());
        //填充 session的m_strDeliveryUrl
        itr->second->m_strDeliveryUrl = itAccount->second.m_strDeliveryUrl;
        itr->second->m_uNeedReport = itAccount->second.m_uNeedReport;
        itr->second->m_uNeedMo = itAccount->second.m_uNeedMo;
        itr->second->m_strClientId = strClientID;

        //update db
        if (0 == itr->second->m_uUpdateFlag)
        {
            UpdateRecord(strIDs, itAccount->second.m_uIdentify, strDate, itr->second);
        }

        //是否需要回调给用户
        if(itAccount->second.m_uNeedReport == 3)    //push report to user
        {
            if (SMS_FROM_ACCESS_HTTP == itr->second->m_uSmsfrom)
            {
                CheckState(itr);
            }
            else
            {
                CheckState_2(itr);
            }
        }
        else if((itAccount->second.m_uNeedReport >= 1) && (!itAccount->second.m_strDeliveryUrl.empty()))    //需要回调，且回调地址不为空。
        {
            string strIp = "";
            if (true == m_SmspUtils.CheckIPFromUrl(itAccount->second.m_strDeliveryUrl, strIp))
            {
                HandleHostIpResp(0, inet_addr(strIp.data()), pResp->m_iSeq);    //发送
                //itr->second->m_bHostIPstate = true;
            }
            else    //查hostIP
            {
                THostIpReq *pHost = new THostIpReq();
                pHost->m_DomainUrl = itAccount->second.m_strDeliveryUrl;
                pHost->m_iSeq = pResp->m_iSeq;
                pHost->m_iMsgType = MSGTYPE_HOSTIP_REQ;
                pHost->m_pSender = this;
                g_pHostIpThread->PostMsg(pHost);
            }
        }
        else    //不回调
        {
            LogWarn("push report to user failed. m_uNeedReport[%d], m_strDeliveryUrl[%s]", itAccount->second.m_uNeedReport, itAccount->second.m_strDeliveryUrl.data());
            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }
    }
    else if ("lpushReport" == pMsg->m_strSessionID)
    {
        map<UInt64, RedisListReqSession *>::iterator itrl = m_redisListReqMap.find(pResp->m_iSeq);
        if (itrl == m_redisListReqMap.end())
        {
            LogError("iSeq[%lu] is not find m_redisListReqMap.", pResp->m_iSeq);
            return;
        }

        UInt32 listSize = pResp->m_RedisReply->integer;
        LogDebug("lpush redis response, size[%d], key[%s]", listSize, itrl->second->m_strKey.data());

        if(listSize == 1)
        {
            /////list has just builded
            //get remain time
            time_t now = time(NULL);
            struct tm today = {0};
            localtime_r(&now, &today);
            UInt64 seconds;
            today.tm_hour = 0;
            today.tm_min = 0;
            today.tm_sec = 0;
            seconds = 24 * 60 * 60 - difftime(now, mktime(&today));
            seconds += 2 * 24 * 60 * 60; //redislist dead time

            TRedisReq *pDel = new TRedisReq();
            pDel->m_RedisCmd = "EXPIRE ";
            pDel->m_RedisCmd.append(itrl->second->m_strKey);
            pDel->m_RedisCmd.append(" ");
            pDel->m_RedisCmd.append(Comm::int2str(seconds));
            pDel->m_strKey = itrl->second->m_strKey;
            LogInfo("set redisList EXPIRE time. cmd[%s]", pDel->m_RedisCmd.data());
            pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDel);
        }

        if (NULL != itrl->second->m_pTimer)
        {
        	SAFE_DELETE(itrl->second->m_pTimer);
        }

        SAFE_DELETE(itrl->second);
        m_redisListReqMap.erase(itrl);

    }
    else
    {
        LogError("this Redis type[%s] is invalid.", pMsg->m_strSessionID.data());
        return;
    }
}

void CReportPushThread::TransformDescForTX(string &strDesc)
{
    bool ChangeCode = true;

    if(3 != strDesc.length())
    {
        ChangeCode = false;
    }

    for (string::size_type i = 0; i < strDesc.length(); ++i)
    {
        if(isdigit(strDesc[i]) != true)
        {
            ChangeCode = false;
        }
    }

    if (ChangeCode)
    {
        int tempDesc = atoi(strDesc.data());

        static const char *const sArrayDesc[] =
        {
            "DELIVRD",
            "EXPIRED",
            "EXPIRED",
            "UNDELIV",
            "UNDELIV",
            "UNDELIV",
            "UNDELIV",
            "EXPIRED",
            "UNDELIV",
            "UNDELIV",
            "UNDELIV",
        };

        if ((0 <= tempDesc) && (10 >= tempDesc))
        {
            strDesc.assign(sArrayDesc[tempDesc]);
        }
        else if (999 == tempDesc)
        {
            strDesc.assign("UNKNOWN");
        }
    }
}

void CReportPushThread::CheckState(SessionMap::iterator itr)
{
    AccountMap::iterator iterAccount = m_AccountMap.find(itr->second->m_strClientId);
    if(iterAccount == m_AccountMap.end())
    {
        LogError("Internal error. Can not find clientid(%s).",
                 itr->second->m_strClientId.c_str());
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    UInt32 uIp = 0;
    string strData = "";
    string strUrl = "";
    //
    string strDesc = "";
    string strReport_status = "";

    struct tm timenow = {0};
    time_t now = time(NULL);
    char sztime[64] = {0};
    localtime_r(&now, &timenow);
    ////if (timenow != 0)
    {
        strftime(sztime, sizeof(sztime), "%Y-%m-%d %H:%M:%S", &timenow);
    }

    //组包
    if (1 == itr->second->m_iType || 0 == itr->second->m_iType)  ///report
    {
        string strDesc = "";
        if(itr->second->m_uNeedReport == 1)	//simple report
        {
            if(itr->second->m_iStatus == 3)     //status is ok
            {
                strDesc = "DELIVRD";
                strReport_status = "SUCCESS";
            }
            else
            {
                strDesc = "UNDELIVRD";
                strReport_status = "FAIL";
            }

        }
        else if(itr->second->m_uNeedReport == 2 || itr->second->m_uNeedReport == 3) //source report
        {
            if(itr->second->m_iStatus == 3)     //status is ok
            {
                strDesc = itr->second->m_strReportDesc;
                strReport_status = "SUCCESS";
            }
            else
            {
                if(std::string::npos != itr->second->m_strReportDesc.find("余额不足"))
                {
                    strDesc = "UNKNOW";
                }
                else
                {
                    strDesc = itr->second->m_strReportDesc;
                }

                strReport_status = "FAIL";
            }
        }
        else
        {
            LogError("[%s:%s] should not run here, m_uNeedReport[%d].", itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), itr->second->m_uNeedReport);
        }

        Json::Value tmpValue;
        if (HttpProtocolType_JD_Webservice == iterAccount->second.m_ucHttpProtocolType)
        {
            struct tm timenow = {0};
            time_t lt;
            char szTime[100] = {0};
            UInt64 llMill = Comm::getCurrentTimeMill();
            lt = llMill / 1000;
            localtime_r(&lt, &timenow);
            strftime(szTime, sizeof(szTime), "%a %b %d %T CST %Y", &timenow);
            string strTime = szTime;

            Json::Value report;
            std::string fmtstr = "SQLT"
                                 + strDesc
                                 + itr->second->m_strPhone
                                 + itr->second->m_strSmsId
                                 + strTime;

            report["desc"] = strDesc;
            report["reportId"] = Json::Value(itr->second->m_strSmsId);
            report["mobileNum"] = Json::Value(itr->second->m_strPhone);
            report["arrived"] = Json::Value(Comm::int2str(llMill));
            char sztoken[64] = {0};
            snprintf(sztoken, sizeof(sztoken), "%ld", cityHashShard::hash64(fmtstr.c_str()));
            report["token"] = Json::Value(sztoken);

            LogDebug("report:fmtstr:%s--%s", fmtstr.c_str(), sztoken);

            tmpValue["signature"] = Json::Value("sqlt.com");
            //tmpValue["signature"] = Json::Value(itr->second->m_strClientId);
            //tmpValue["signature"] = Json::Value(itr->second->m_strSign);
            tmpValue["reports"][Json::UInt(0)] = report;
        }
        else
        {
            TransformDescForTX(strDesc);

            tmpValue["sid"] = Json::Value(itr->second->m_strSmsId);
            tmpValue["uid"] = Json::Value(itr->second->m_strUid);
            tmpValue["mobile"] = Json::Value(itr->second->m_strPhone);
            tmpValue["report_status"] = Json::Value(strReport_status);
            tmpValue["desc"] = Json::Value(strDesc);

            if (HttpProtocolType_TX_Json == iterAccount->second.m_ucHttpProtocolType)
            {
                tmpValue["recv_time"] = Json::Value(sztime);
            }
            else
            {
                tmpValue["user_receive_time"] = Json::Value(sztime);
            }
        }

        Json::FastWriter fast_writer;
        strData = fast_writer.write(tmpValue);

        //add by fangjinxiong 20160919.
        if(itr->second->m_uNeedReport == 3) //should  uer get report
        {
            //send data to redis list
            string strCmd = "";
            string strkey = "";
            strCmd.append("lpush ");

            strkey.append("active_report_cache:");
            strkey.append(itr->second->m_strClientId);

            strCmd.append(strkey);
            strCmd.append(" ");
            strCmd.append(Base64::Encode(strData));

            //create a session to manage redisList req/response
            UInt64 uSeqPushReport = m_SnMng.getSn();
            RedisListReqSession *psession = new RedisListReqSession();
            psession->m_strKey = strkey;
            psession->m_pTimer = SetTimer(uSeqPushReport, "lpushReport", 100 * 1000);
            m_redisListReqMap[uSeqPushReport] = psession;

            LogNotice("==http== push report to redis clientId:%s,phone:%s,strKey:%s ,strData:%s.", itr->second->m_strClientId.data(),
                      itr->second->m_strPhone.data(), strkey.data(), strData.data());
            //
            TRedisReq *pRed = new TRedisReq();
            pRed->m_RedisCmd = strCmd;
            pRed->m_strKey = strkey;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = uSeqPushReport;
            pRed->m_pSender = g_pReportPushThread;
            pRed->m_strSessionID = "lpushReport";
            SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed);

            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        strUrl = itr->second->m_strDeliveryUrl;
        //should post report to user
        if (HttpProtocolType_JD_Webservice != iterAccount->second.m_ucHttpProtocolType)
        {
            strData = "[" + strData + "]";
        }

        uIp = itr->second->m_uIp;
        LogNotice("[%s:%s:%s] push report url:%s,HttpProtocolType:%d,body:%s.",
                  itr->second->m_strClientId.data(), itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), 
                  strUrl.data(), iterAccount->second.m_ucHttpProtocolType, strData.data());

    }
    else if (2 == itr->second->m_iType) ///upstream
    {
        Json::Value tmpValue;

        if (HttpProtocolType_JD_Webservice == iterAccount->second.m_ucHttpProtocolType)
        {
            string strTim = Comm::int2str(Comm::getCurrentTimeMill());
            Json::Value mo;

            std::string fmtstr = "SQLT"
                                 + itr->second->m_strContent
                                 + itr->second->m_strClientId
                                 + itr->second->m_strPhone
                                 + strTim;

            mo["content"] = Json::Value(itr->second->m_strContent);
            mo["refer"] = Json::Value(itr->second->m_strClientId);
            mo["mobileNum"] = Json::Value(itr->second->m_strPhone);
            mo["time"] = Json::Value(strTim);
            char sztoken[64] = {0};
            snprintf(sztoken, sizeof(sztoken), "%ld", cityHashShard::hash64(fmtstr.c_str()));
            mo["token"] = Json::Value(sztoken);

            LogDebug("upstream:fmtstr:%s--%s", fmtstr.c_str(), sztoken);

            tmpValue["signature"] = Json::Value("sqlt.com");
            //tmpValue["signature"] = Json::Value(itr->second->m_strClientId);
            //tmpValue["signature"] = Json::Value(itr->second->m_strSign);
            tmpValue["sms"][Json::UInt(0)] = mo;

        }
        else
        {
            tmpValue["content"] = Json::Value(itr->second->m_strContent);
            tmpValue["mobile"] = Json::Value(itr->second->m_strPhone);
            tmpValue["sign"] = Json::Value(itr->second->m_strSign);
            tmpValue["reply_time"] = Json::Value(sztime);

            if (HttpProtocolType_TX_Json == iterAccount->second.m_ucHttpProtocolType)
            {
                tmpValue["account"]         = Json::Value(itr->second->m_strClientId);
                tmpValue["dsisplay_number"] = Json::Value(itr->second->m_strUserPort);
                tmpValue["extend"]          = Json::Value(itr->second->m_strSourceExtend);
            }
            else
            {
                tmpValue["moid"]    = Json::Value(itr->second->m_strSmsId);
                tmpValue["extend"]  = Json::Value(itr->second->m_strUserPort);
            }
        }

        Json::FastWriter fast_writer;
        strData = fast_writer.write(tmpValue);

        //add by fangjinxiong 20170412
        if(itr->second->m_uNeedMo == 3) //should  uer get mo
        {
            //send data to redis list
            string strCmd = "";
            string strkey = "";
            strCmd.append("lpush ");

            strkey.append("active_mo_cache:");
            strkey.append(itr->second->m_strClientId);

            strCmd.append(strkey);
            strCmd.append(" ");
            strCmd.append(Base64::Encode(strData));

            //create a session to manage redisList req/response
            UInt64 uSeqPushReport = m_SnMng.getSn();
            RedisListReqSession *psession = new RedisListReqSession();
            psession->m_strKey = strkey;
            psession->m_pTimer = SetTimer(uSeqPushReport, "lpushReport", 100 * 1000);
            m_redisListReqMap[uSeqPushReport] = psession;

            LogNotice("==http== push mo to redis clientId:%s,strKey:%s,strData:%s", itr->second->m_strClientId.data(), strkey.data(), strData.data());
            //
            TRedisReq *pRed = new TRedisReq();
            pRed->m_RedisCmd = strCmd;
            pRed->m_strKey = strkey;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = uSeqPushReport;
            pRed->m_pSender = g_pReportPushThread;
            pRed->m_strSessionID = "lpushReport";
            SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed);
            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        strUrl = itr->second->m_strMourl;
        uIp = itr->second->m_uIp;
        LogNotice("==http== push mo clientId:%s,phone:%s,url:%s,body:%s.",
                  itr->second->m_strClientId.data(), itr->second->m_strPhone.data(), strUrl.data(), strData.data());

    }
    else
    {
        LogError("[%s:%s] not report or upstream m_iType[%d] is invalid.", itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), itr->second->m_iType);
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    ReportPushSession *pSession = itr->second;
    pSession->m_strData = strData;
    pSession->m_strUrl = strUrl;
    pSession->m_uIp = uIp;

    if(0 == strUrl.compare(0, 5, "https"))
    {
        http::HttpsSender httpsSender;
        std::vector<std::string> mpHeader;
        mpHeader.push_back("Content-Type: application/json;charset=utf-8");

        THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
        pHttpsReq->strBody = strData;
        pHttpsReq->strUrl  = strUrl;
        pHttpsReq->uIP     = uIp;
        pHttpsReq->vecHeader = mpHeader;
        pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_POST;
        pHttpsReq->m_iSeq     = itr->first;
        pHttpsReq->m_pSender  = this;

        CHttpsSendThread::sendHttpsMsg ( pHttpsReq );
        itr->second->m_pTimer = SetTimer(itr->first, "", 5000);
        return;
    }

    //check url and data, should no be null
    ////LogNotice("[%s:%s] Log3:strUrl[%s],postData[%s],uIp[%d].",itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(),strUrl.data(), strData.data(), uIp);
    if(strUrl.empty() || strData.empty())
    {
        LogError("[%s:%s] post report/upstream to user failed. Url[%s], postData[%s]."
                 , itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), strUrl.data(), strData.data());
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    std::vector<std::string> mvHeader;

    if (HttpProtocolType_JD_Webservice == iterAccount->second.m_ucHttpProtocolType)
    {
        string strbt = "";

        if (2 == itr->second->m_iType) // MO
        {
            strbt = "batchSms=";
        }
        else if (1 == itr->second->m_iType || 0 == itr->second->m_iType)  //report
        {
            strbt = "reports=";
        }
        mvHeader.push_back("Content-Type: application/x-www-form-urlencoded");
        strData = http::UrlCode::UrlEncode(strData);
        strData = strbt + strData;
    }

    //send report to user
    if(0 == strUrl.compare(0, 4, "http"))
    {
        THttpsRequestMsg *pHttpReq = new THttpsRequestMsg();
        pHttpReq->strBody = strData;
        pHttpReq->strUrl  = strUrl;
        pHttpReq->uIP     = uIp;
        pHttpReq->vecHeader	= mvHeader;
        pHttpReq->m_iMsgType = MSGTYPE_HTTP_ASYNC_POST;
        pHttpReq->m_iSeq     = itr->first;
        pHttpReq->m_pSender  = this;
        CHttpsSendThread::sendHttpMsg(pHttpReq);

    }

}

void CReportPushThread::CheckState_2(SessionMap::iterator itr)
{
    UInt32 uIp = 0;
    string strData;
    string strUrl;
    //
    string strDesc = "";
    string strReport_status = "";

    struct tm timenow = {0};
    time_t now = time(NULL);
    char sztime[64] = {0};
    localtime_r(&now, &timenow);
    ////if (timenow != 0)
    {
        strftime(sztime, sizeof(sztime), "%Y-%m-%d %H:%M:%S", &timenow);
    }

    //组包
    if (1 == itr->second->m_iType)  ///report
    {
        string strDesc;
        if(itr->second->m_uNeedReport == 1) //simple report
        {
            if(itr->second->m_iStatus == 3)     //status is ok
            {
                strDesc = "DELIVRD";
                strReport_status = "SUCCESS";
            }
            else
            {
                strDesc = "UNDELIVRD";
                strReport_status = "FAIL";
            }

        }
        else if(itr->second->m_uNeedReport == 2 || itr->second->m_uNeedReport == 3) //source report
        {
            if(itr->second->m_iStatus == 3)     //status is ok
            {
                strDesc = itr->second->m_strReportDesc;
                strReport_status = "SUCCESS";
            }
            else
            {
                if(std::string::npos != itr->second->m_strReportDesc.find("余额不足"))
                {
                    strDesc = "UNKNOW";
                }
                else
                {
                    strDesc = itr->second->m_strReportDesc;
                }

                strReport_status = "FAIL";
            }
        }
        else
        {
            LogError("[%s:%s] should not run here, m_uNeedReport[%d].", itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), itr->second->m_uNeedReport);
        }

        //desc jia miaoshu
        if(strDesc.find("YX:") != string::npos && strDesc.length() >= 7)
        {
            string::size_type strBegin = strDesc.find("YX:");
            string strCode = strDesc.substr(strBegin, 7);
            string strCode4 = strDesc.substr(strBegin + 3, 7);

            string strYXDesc = m_mapReportErrCode_Desc[strCode];
            if(!strYXDesc.empty())
            {
                strDesc.append(" ");
                strDesc.append(strYXDesc);      //YX:1000****
            }
            else
            {
                strDesc.assign("YX:9999 其他错误");
                //TODO:FJX
                strDesc.append(strCode4);
            }

        }


        Json::Value tmpValue;
        tmpValue["type"] = Json::Value("1");
        tmpValue["sid"] = Json::Value(itr->second->m_strSmsId);
        tmpValue["mobile"] = Json::Value(itr->second->m_strPhone);
        tmpValue["status"] = Json::Value(strReport_status); //???
        tmpValue["desc"] = Json::Value(http::UrlCode::UrlEncode(strDesc));
        tmpValue["extend"] = Json::Value("");
        tmpValue["batchid"] = Json::Value(itr->second->m_strUid);
        tmpValue["time"] = Json::Value(Comm::int2str(Comm::getCurrentTimeMill()));
        Json::FastWriter fast_writer;
        strData = fast_writer.write(tmpValue);

        //add by fangjinxiong 20160919.
        if(itr->second->m_uNeedReport == 3) //should  uer get report
        {
            //send data to redis list
            string strCmd = "";
            string strkey = "";
            strCmd.append("lpush ");

            strkey.append("active_report_cache:");
            strkey.append(itr->second->m_strClientId);

            strCmd.append(strkey);
            strCmd.append(" ");
            strCmd.append(Base64::Encode(strData));

            //create a session to manage redisList req/response
            UInt64 uSeqPushReport = m_SnMng.getSn();
            RedisListReqSession *psession = new RedisListReqSession();
            psession->m_strKey = strkey;
            psession->m_pTimer = SetTimer(uSeqPushReport, "lpushReport", 100 * 1000);
            m_redisListReqMap[uSeqPushReport] = psession;

            LogNotice("==http== push report to redis clientId:%s,phone:%s,strKey:%s,strData:%s.", itr->second->m_strClientId.data(), itr->second->m_strPhone.data(), strkey.data(), strData.data());
            //
            TRedisReq *pRed = new TRedisReq();
            pRed->m_RedisCmd = strCmd;
            pRed->m_strKey = strkey;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = uSeqPushReport;
            pRed->m_pSender = g_pReportPushThread;
            pRed->m_strSessionID = "lpushReport";
            SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed);

            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        //should post report to user
        strData = "[" + strData + "]";
        strUrl = itr->second->m_strDeliveryUrl;
        uIp = itr->second->m_uIp;

        LogNotice("==http== push report clientId:%s,smsid:%s,phone:%s,url:%s,body:%s.",
                  itr->second->m_strClientId.data(), itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), strUrl.data(), strData.data());

    }
    else if (2 == itr->second->m_iType) ///upstream
    {

        Json::Value tmpValue;
        tmpValue["type"] = Json::Value("2");
        tmpValue["sid"] = Json::Value(itr->second->m_strSmsId);
        tmpValue["mobile"] = Json::Value(itr->second->m_strPhone);
        tmpValue["status"] = Json::Value("");
        tmpValue["desc"] = Json::Value(http::UrlCode::UrlEncode(itr->second->m_strContent));
        tmpValue["extend"] = Json::Value(itr->second->m_strUserPort);
        tmpValue["batchid"] = Json::Value("");
        tmpValue["time"] = Json::Value(Comm::int2str(Comm::getCurrentTimeMill()));;
        Json::FastWriter fast_writer;
        strData = fast_writer.write(tmpValue);

        if(itr->second->m_uNeedMo == 3) //should  uer get mo
        {
            //send data to redis list
            string strCmd = "";
            string strkey = "";
            strCmd.append("lpush ");

            strkey.append("active_mo_cache:");
            strkey.append(itr->second->m_strClientId);

            strCmd.append(strkey);
            strCmd.append(" ");
            strCmd.append(Base64::Encode(strData));

            //create a session to manage redisList req/response
            UInt64 uSeqPushReport = m_SnMng.getSn();
            RedisListReqSession *psession = new RedisListReqSession();
            psession->m_strKey = strkey;
            psession->m_pTimer = SetTimer(uSeqPushReport, "lpushReport", 100 * 1000);
            m_redisListReqMap[uSeqPushReport] = psession;

            LogNotice("==http== push mo to redis clientId:%s,strKey:%s,strData:%s", itr->second->m_strClientId.data(), strkey.data(), strData.data());
            TRedisReq *pRed = new TRedisReq();
            pRed->m_RedisCmd = strCmd;
            pRed->m_strKey = strkey;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = uSeqPushReport;
            pRed->m_pSender = g_pReportPushThread;
            pRed->m_strSessionID = "lpushReport";
            SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed);

            SAFE_DELETE(itr->second);
            m_mapSession.erase(itr);
            return;
        }

        strUrl = itr->second->m_strMourl;
        uIp = itr->second->m_uIp;
        LogNotice("==http== push mo clientId:%s,phone:%s,url:%s,body:%s.",
                  itr->second->m_strClientId.data(), itr->second->m_strPhone.data(), strUrl.data(), strData.data());

    }
    else
    {
        LogError("[%s:%s] not report or upstream m_iType[%d] is invalid.", itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), itr->second->m_iType);
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    ReportPushSession *pSession = itr->second;
    pSession->m_strData = strData;
    pSession->m_strUrl = strUrl;
    pSession->m_uIp = uIp;

    if(0 == strUrl.compare(0, 5, "https"))
    {
        http::HttpsSender httpsSender;
        std::vector<std::string> mpHeader;
        mpHeader.push_back("Content-Type: application/json;charset=utf-8");

        THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
        pHttpsReq->strBody = strData;
        pHttpsReq->strUrl  = strUrl;
        pHttpsReq->uIP     = uIp;
        pHttpsReq->vecHeader = mpHeader;
        pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_POST;
        pHttpsReq->m_iSeq     = itr->first;
        pHttpsReq->m_pSender  = this;

        CHttpsSendThread::sendHttpsMsg ( pHttpsReq );
        itr->second->m_pTimer = SetTimer(itr->first, "", 5000);

        return;
    }

    //check url and data, should no be null
    ////LogNotice("[%s:%s] Log3:strUrl[%s],postData[%s],uIp[%d].",itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(),strUrl.data(), strData.data(), uIp);
    if(strUrl.empty() || strData.empty())
    {
        LogError("[%s:%s] post report/upstream to user failed. Url[%s], postData[%s]."
                 , itr->second->m_strSmsId.data(), itr->second->m_strPhone.data(), strUrl.data(), strData.data());
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        return;
    }

    //send report to user
    if(0 == strUrl.compare(0, 4, "http"))
    {
        THttpsRequestMsg *pHttpReq = new THttpsRequestMsg();
        pHttpReq->strBody = strData;
        pHttpReq->strUrl  = strUrl;
        pHttpReq->uIP     = uIp;
        pHttpReq->m_iMsgType = MSGTYPE_HTTP_ASYNC_POST;
        pHttpReq->m_iSeq     = itr->first;
        pHttpReq->m_pSender  = this;
        CHttpsSendThread::sendHttpMsg(pHttpReq);

    }
}

void CReportPushThread::HandleHttpResponseMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);
    THttpResponseMsg *pRsp = (THttpResponseMsg *)pMsg;

    SessionMap::iterator iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
    ReportPushSession *pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    LogNotice("[%s:%s:%s] http rsp, status[%u], status code[%d], rsp_content[%s], m_iSeq[%lu]..",
              pSession->m_strClientId.data(),
              pSession->m_strSmsId.data(),
              pSession->m_strPhone.data(),
              pRsp->m_uStatus,
              pRsp->m_tResponse.GetStatusCode(),
              pRsp->m_tResponse.GetContent().data(), pMsg->m_iSeq);

    if (200 != pRsp->m_tResponse.GetStatusCode())
    {
        checkRePush(pSession);
    }

    SAFE_DELETE(iter->second);
    m_mapSession.erase(iter);
}

void CReportPushThread::HandleHttpsResponseMsg( TMsg *pMsg )
{
    CHK_NULL_RETURN(pMsg);
    THttpsResponseMsg *pRsp = (THttpsResponseMsg *)pMsg;

    SessionMap::iterator iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
    ReportPushSession *pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    LogNotice("[%s:%s:%s] https rsp, status[%u], rsp_content[%s].",
              pSession->m_strClientId.data(),
              pSession->m_strSmsId.data(),
              pSession->m_strPhone.data(),
              pRsp->m_uStatus,
              pRsp->m_strResponse.data());

    if (0 != pRsp->m_uStatus)
    {
        checkRePush(pSession);
    }

    SAFE_DELETE(iter->second);
    m_mapSession.erase(iter);
}

void CReportPushThread::HandleTimeOutMsg(TMsg *pMsg)
{
    if ("lpushReport" == pMsg->m_strSessionID)
    {
        map<UInt64, RedisListReqSession *>::iterator itrl = m_redisListReqMap.find(pMsg->m_iSeq);
        if (itrl == m_redisListReqMap.end())
        {
            LogError("iSeq[%lu] is not find m_redisListReqMap.", pMsg->m_iSeq);
            return;
        }

        LogWarn("lpushReport cmd:%s, is response is time out.", itrl->second->m_strKey.data());

        if (NULL != itrl->second->m_pTimer)
        {
        	SAFE_DELETE(itrl->second->m_pTimer);
        }

        SAFE_DELETE(itrl->second);
        m_redisListReqMap.erase(itrl);
    }
    else
    {
        SessionMap::iterator iter;
        CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
        ReportPushSession *pSession = iter->second;
        CHK_NULL_RETURN(pSession);

        LogWarn("[%s:%s:%s] Session timeout, data[%s], url[%s].",
                pSession->m_strClientId.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                pSession->m_strData.data(),
                pSession->m_strUrl.data());

        checkRePush(pSession);
        SAFE_DELETE(iter->second);
        m_mapSession.erase(iter);
    }
}

void CReportPushThread::UpdateRecord(string strIDs, UInt32 uIdentify, string strDate, ReportPushSession *rpRecv)
{
    char sql[1024]  = {0};
    char szReportTime[64] = {0};

    string &strErrorCode = rpRecv->m_strDesc;
    Int32 iStatus = rpRecv->m_iStatus;
    UInt32 uChannelCount = rpRecv->m_uChannelCount;

    struct tm timenow = {0};
    time_t now = time(NULL);
    localtime_r(&now, &timenow);

    /////if (timenow != 0)
    {
        strftime(szReportTime, sizeof(szReportTime), "%Y%m%d%H%M%S", &timenow);
    }

    snprintf(sql, 1024, "UPDATE t_sms_access_%d_%s SET state = '%d', report = '%s', reportdate = '%s',channelcnt = '%d'"
             ",innerErrorcode='%s' where id = '%s' and date='%s';",
             uIdentify, strDate.substr(0, 8).data(), iStatus, strErrorCode.data(), szReportTime, uChannelCount
             , rpRecv->m_strInnerErrcode.c_str(), strIDs.data(), strDate.data());

    TMQPublishReqMsg *pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange);
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append(strIDs.data());
    g_pMQDBProducerThread->PostMsg(pMQ);

    return;
}


UInt32 CReportPushThread::GetSessionMapSize()
{
    return m_mapSession.size();
}

void CReportPushThread::checkRePush(ReportPushSession *pSession)
{
    CHK_NULL_RETURN(pSession);

    pSession->m_uiRetryCount++;

    if (pSession->m_uiRetryCount > m_uiHttpsReportRetry)
    {
        LogWarn("[%s:%s:%s] RetryCount[%u] > HttpsReportRetry[%u], discard report, data[%s], url[%s].",
                pSession->m_strClientId.data(),
                pSession->m_strSmsId.data(),
                pSession->m_strPhone.data(),
                pSession->m_uiRetryCount,
                m_uiHttpsReportRetry,
                pSession->m_strData.data(),
                pSession->m_strUrl.data());

        return;
    }

    SmsAccountMapIter iterAccount;
    CHK_MAP_FIND_STR_RET(m_AccountMap, iterAccount, pSession->m_strClientId);
    const SmsAccount &smsAccount = iterAccount->second;

    if (!(smsAccount.m_uAccountExtAttr & ACCOUNT_EXT_HTTPS_REPORT_RETRY))
    {
        LogDebug("[%s:%s:%s] not support repush https report.",
                 pSession->m_strClientId.data(),
                 pSession->m_strSmsId.data(),
                 pSession->m_strPhone.data());
        return;
    }

    MqConfigMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapMqCfg, iter, m_componentCfg.m_uMqId);
    const MqConfig &mqConfig = iter->second;

    if (mqConfig.m_strExchange.empty() || mqConfig.m_strRoutingKey.empty())
    {
        LogError("Invalid Exchange[] or RoutingKey[%s].",
                 mqConfig.m_strExchange.data(),
                 mqConfig.m_strRoutingKey.data());
        return;
    }

    string strData;
    strData.append("type=4&clientid=");
    strData.append(pSession->m_strClientId);

    strData.append("&data=");
    strData.append(Base64::Encode(pSession->m_strData));

    strData.append("&url=");
    strData.append(pSession->m_strUrl);

    strData.append("&ip=");
    strData.append(to_string(pSession->m_uIp));

    strData.append("&retry_count=");
    strData.append(to_string(pSession->m_uiRetryCount));

    TMQPublishReqMsg *pReq = new TMQPublishReqMsg();
    CHK_NULL_RETURN(pReq);
    pReq->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pReq->m_strExchange = mqConfig.m_strExchange;
    pReq->m_strRoutingKey = mqConfig.m_strRoutingKey;
    pReq->m_strData = strData;
    g_pMQC2sIOProducerThread->PostMsg(pReq);
}

void CReportPushThread::handleReportRePushReq(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);
    ReportRePushReqMsg *pReq = (ReportRePushReqMsg *)pMsg;

    ReportPushSession *pSession = new ReportPushSession(this);
    CHK_NULL_RETURN(pSession);
    m_mapSession[pSession->m_ulSeq] = pSession;
    pSession->m_strClientId = pReq->m_strClientId;
    pSession->m_strData = pReq->m_strData;
    pSession->m_strUrl = pReq->m_strUrl;
    pSession->m_uIp = pReq->m_uiIp;
    pSession->m_uiRetryCount = pReq->m_uiRetryCount;

    if ("https" == pSession->m_strUrl.substr(0, 5))
    {
        std::vector<std::string> mpHeader;
        mpHeader.push_back("Content-Type: application/json;charset=utf-8");

        THttpsRequestMsg *pReq = new THttpsRequestMsg();
        CHK_NULL_RETURN(pReq);
        pReq->vecHeader = mpHeader;
        pReq->strBody = pSession->m_strData;
        pReq->strUrl = pSession->m_strUrl;
        pReq->uIP = pSession->m_uIp;
        pReq->m_iSeq = pSession->m_ulSeq;
        pReq->m_pSender = this;
        pReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_POST;

        CHttpsSendThread::sendHttpsMsg (pReq);
        pSession->m_pTimer = SetTimer(pSession->m_ulSeq, "", 5000);
    }
    else
    {
        THttpsRequestMsg *pHttpReq = new THttpsRequestMsg();
        pHttpReq->strBody = pSession->m_strData;
        pHttpReq->strUrl  = pSession->m_strUrl;
        pHttpReq->uIP     = pSession->m_uIp;
        pHttpReq->m_iMsgType = MSGTYPE_HTTP_ASYNC_POST;
        pHttpReq->m_iSeq     = pSession->m_ulSeq;
        pHttpReq->m_pSender  = this;
        CHttpsSendThread::sendHttpMsg(pHttpReq);

    }
}


