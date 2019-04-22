#include "TpsAndHoldCount.h"
#include "main.h"
#include "UrlCode.h"
#include "base64.h"
#include "Uuid.h"
#include "ssl/md5.h"
#include "CUnityThread.h"
#include "Comm.h"
#include "BusTypes.h"


CUnityThread::CUnityThread(const char *name): CThread(name)
{
    // IOMQERR_MAXCHCHESIZE default: 3000000
    m_uIOProduceMaxChcheSize = 3000 * 1000;
}

CUnityThread::~CUnityThread()
{
}

bool CUnityThread::Init(UInt64 SwitchNumber)
{
    if (false == CThread::Init())
    {
        printf("CDispatchThread::init -> CThread::Init is failed.\n");
        return false;
    }

    m_honeDao.Init();

    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    m_TestAccount.clear();
    g_pRuleLoadThread->getSmsTestAccount(m_TestAccount);

    m_mapClientSignPort.clear();
    g_pRuleLoadThread->getSmsClientAndSignMap(m_mapClientSignPort);

    m_userGwMap.clear();
    g_pRuleLoadThread->getUserGwMap(m_userGwMap);

    m_mqConfigInfo.clear();
    g_pRuleLoadThread->getMqConfig(m_mqConfigInfo);

    m_componentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);
    updateMQWeightGroupInfo(m_componentRefMqInfo);

    m_mapSystemErrorCode.clear();
    g_pRuleLoadThread->getSystemErrorCode(m_mapSystemErrorCode);

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    m_mqId.clear();
    for (map<string, ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin(); itr != m_componentRefMqInfo.end(); ++itr)
    {
        if (g_uComponentId == itr->second.m_uComponentId)
        {
            m_mqId.insert(itr->second.m_uComponentId);
        }
    }
    m_uSwitchNumber = SwitchNumber;

    time_t now = time(NULL);
    struct tm today =  {0};
    localtime_r(&now, &today);
    int seconds;
    today.tm_hour = 0;
    today.tm_min = 0;
    today.tm_sec = 0;
    seconds = 24 * 60 * 60 - difftime(now, mktime(&today));
    m_UnlockTimer = SetTimer(ACCOUNT_UNLOCK_TIMER_ID, "", seconds * 1000);

    // 20s后再开始执行定时短信任务
    m_bTimersendTaskStartExecution = false;
    m_TimersendTaskStartExecuteTimer = SetTimer(TIMERSEND_TASKS_START_EXECUTE_TIMER_ID, "",
                                       TIMERSEND_TASKS_START_EXEUCTE_DELAY * 1000);

    // 清除定时短信任务的过期的进度缓存
    m_TimersendTaskProgressCleanTimer = SetTimer(TIMERSEND_TASKS_PROGRESS_ERASE_TIMER_ID, "",
                                        TIMERSEND_TASKS_PROGRESS_CACHE_CLEANER_INTERVAL * 1000);

    return true;
}

void CUnityThread::GetSysPara(const std::map<std::string, std::string> &mapSysPara)
{
    string strSysPara = "";
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "IOMQERR_MAXCHCHESIZE";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if ((0 > nTmp) || (1000 * 1000 * 1000 < nTmp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                     strSysPara.c_str(), strTmp.c_str(), nTmp);

            break;
        }

        m_uIOProduceMaxChcheSize = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uIOProduceMaxChcheSize);

    do
    {
        strSysPara = "TIMER_SEND_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string &strTmp = iter->second;
        std::vector<std::string> vCfg;
        Comm::splitExVectorSkipEmptyString(const_cast<std::string &>(strTmp), ";", vCfg);
        if (vCfg.size() < 6)
        {
            LogError("Invalid 'TIMER_SEND_CFG' value: [%s] -> reason:size<6", strTmp.c_str());
            break;
        }
        UInt32 uTmp = atoi(vCfg[3].c_str());

        if (uTmp > 50000)
        {
            LogError("Invalid system parameter(%s) value(%s, %d). reason:Out of range[0, 50000]",
                     strSysPara.c_str(), strTmp.c_str(), uTmp);

            break;
        }

        m_uTimersendTaskPrefetchPhonesCnt = uTmp;
    }
    while (0);

    LogNotice("timersend task prefetch(%s) value(%u).", strSysPara.c_str(), m_uTimersendTaskPrefetchPhonesCnt);
}

void CUnityThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg *pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            usleep(10);
        }
        else
        {
            HandleMsg(pMsg);
            SAFE_DELETE(pMsg);
        }
    }
}


void CUnityThread::HandleMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
    case MSGTYPE_HTTPS_TO_UNITY_REQ:
    {
        HandleHttpsReqMsg(pMsg);
        break;
    }
    case MSGTYPE_DB_QUERY_RESP:
    {
        HandleDBResp(pMsg);
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
    case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
    {
        HandleAccountUpdateReq(pMsg);
        LogNotice("RuleUpdate account update!");
        break;
    }
    case MSGTYPE_RULELOAD_SMSTESTACCOUNT_UPDATE_REQ:
    {
        TUpdateSmsTestAccontReq *pReq = (TUpdateSmsTestAccontReq *)pMsg;
        if (pReq)
        {
            m_TestAccount.clear();
            m_TestAccount = pReq->m_SmsTestAccount;
            LogNotice("Update t_sms_test_account!");
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_ERROR_DESC_EEQ:
    {
        TUpdateSystemErrorCodeReq *pReq = (TUpdateSystemErrorCodeReq *)pMsg;
        if (pReq)
        {
            m_mapSystemErrorCode.clear();
            m_mapSystemErrorCode = pReq->m_mapSystemErrorCode;
            LogNotice("update t_sms_system_error_desc size:%d.", m_mapSystemErrorCode.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ:
    {
        TUpdateClientIdAndSignReq *pReq = (TUpdateClientIdAndSignReq *)pMsg;
        if (pReq)
        {
            m_mapClientSignPort.clear();
            m_mapClientSignPort = pReq->m_ClientIdAndSignMap;
            LogNotice("RuleUpdate ClientAndSign update. size[%d]", pReq->m_ClientIdAndSignMap.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_OPERATERSEGMENT_UPDATE_REQ:
    {
        TUpdateOperaterSegmentReq *pReq = (TUpdateOperaterSegmentReq *)pMsg;
        if (pReq)
        {
            m_honeDao.m_OperaterSegmentMap.clear();
            m_honeDao.m_OperaterSegmentMap = pReq->m_OperaterSegmentMap;
            LogNotice("RuleUpdate t_sms_operater_segment update. size[%d]", pReq->m_OperaterSegmentMap.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
    {
        TUpdateSysParamRuleReq *pReq = (TUpdateSysParamRuleReq *)pMsg;
        if (pReq)
        {
            GetSysPara(pReq->m_SysParamMap);
            LogNotice("Update sys param!");
        }
        break;
    }
    case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
    {
        TUpdateMqConfigReq *pReq = (TUpdateMqConfigReq *)pMsg;
        if (pReq)
        {
            m_mqConfigInfo.clear();
            m_mqConfigInfo = pReq->m_mapMqConfig;
            LogNotice("RuleUpdate Mq config update. size[%d]", pReq->m_mapMqConfig.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
    {
        TUpdateComponentRefMqReq *pReq = (TUpdateComponentRefMqReq *)pMsg;
        if (pReq)
        {
            m_componentRefMqInfo.clear();
            m_componentRefMqInfo = pReq->m_componentRefMqInfo;
            updateMQWeightGroupInfo(m_componentRefMqInfo);
            LogNotice("RuleUpdate component ref Mq  update. size[%d]", pReq->m_componentRefMqInfo.size());
        }
        break;
    }
    case MSGTYPE_RULELOAD_TIMERSEND_TASKINFO_UPDATE_REQ:
    {
        if (!m_bTimersendTaskStartExecution)
        {
            LogWarn("Ignore MSGTYPE_RULELOAD_TIMERSEND_TASKINFO_UPDATE_REQ msg cause timersend task not started yet! It'll load new task later.");
            break;
        }
        TUpdateTimerSendTaskInfoReq *pReq = (TUpdateTimerSendTaskInfoReq *)pMsg;
        if (pReq)
        {
            updateTimersendTaskInfo(pReq->m_mapTimersendTaskInfo);
        }
        else
        {
            LogError("Received NULL pMsg of TASKINFO_UPDATE_REQ");
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
        LogWarn("msg type[%x] is invalid!", pMsg->m_iMsgType);
        break;
    }
    }
    return;
}

bool CUnityThread::getPortSign( string strClientId, string strSignPort, string &strSignOut )
{
    string strSignDefault = "" ;
    ClientIdAndSignMap::iterator itr = m_mapClientSignPort.begin();
    for( ; itr != m_mapClientSignPort.end(); itr++ )
    {
        if ( strClientId == itr->second.m_strClientId
                && ( strSignPort == itr->second.m_strSignPort
                     || itr->second.m_strSignPort == "*" ))
        {

            if ( strSignPort == itr->second.m_strSignPort )
            {
                strSignOut = itr->second.m_strSign;
                LogDebug("ClientId:%s, SignPort:%s Match Result[ Sign:%s ] ",
                         strClientId.data(), strSignPort.data(),  strSignOut.data());
                return true;
            }

            strSignDefault = itr->second.m_strSign ;
        }
    }

    /**
    ** 如果默认配置不为空 并且没哟匹配到对应的端口号
    ** 采用默认处理
    **/
    if ( itr == m_mapClientSignPort.end()
            && !strSignDefault.empty() )
    {
        LogDebug("ClientId:%s, SignPort:%s Match Default[ Sign:%s ] ",
                 strClientId.data(), strSignPort.data(), strSignDefault.data());
        strSignOut = strSignDefault;
        return true;
    }

    LogWarn("clientId[%s], strSignPort[%s] not find in t_sms_clientid_signport.",
            strClientId.data(), strSignPort.data());

    return false;

}

bool CUnityThread::getContentWithSign( SMSSmsInfo *pSmsInfo, string &strSign, UInt32 &uiChina )
{
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    LogDebug("clientid:%s, accountExtAttr:%u, extpendPort:%s.",
             pSmsInfo->m_strClientId.data(),
             pSmsInfo->m_uAccountExtAttr,
             pSmsInfo->m_strExtpendPort.data());

    if ((pSmsInfo->m_uAccountExtAttr & ACCOUNT_EXT_PORT_TO_SIGN )
            && (1 == pSmsInfo->m_uNeedExtend )
            && (getPortSign(pSmsInfo->m_strClientId, pSmsInfo->m_strExtpendPort, strSign)))
    {
        bool bChineseSign = Comm::IncludeChinese((char *)strSign.data());
        bool bChineseContent = Comm::IncludeChinese((char *)pSmsInfo->m_strContent.data());
        if ( bChineseSign || bChineseContent )
        {
            static string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
            static string strRight = http::UrlCode::UrlDecode("%e3%80%91");
            pSmsInfo->m_strContentWithSign = pSmsInfo->m_strContent + strLeft + strSign + strRight;
            uiChina = 1;
        }
        else
        {
            pSmsInfo->m_strContentWithSign = pSmsInfo->m_strContent + "[" + strSign + "]";
            uiChina = 0;
        }
        return true;
    }

    return false;

}


void CUnityThread::HandleHttpsReqMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    CHttpServerToUnityReqMsg *pHttps = (CHttpServerToUnityReqMsg *)pMsg;

    struct tm timenow = {0};
    time_t now = time(NULL);
    localtime_r(&now, &timenow);
    char submitTime[64] = { 0 };
    strftime(submitTime, sizeof(submitTime), "%Y%m%d%H%M%S", &timenow);

    string strContent = pHttps->m_strContent;
    Comm::trimSign(strContent);
    ShortSmsInfo shortInfo;
    shortInfo.m_uPkNumber = 1;
    shortInfo.m_uPkTotal = 1;
    shortInfo.m_strShortContent = strContent;

    SMSSmsInfo smsInfo;
    SMSSmsInfo *pSmsInfo = &smsInfo;

    Comm::split(pHttps->m_strPhone, "," , pSmsInfo->m_Phonelist);

    for(vector<string>::iterator itp = pSmsInfo->m_Phonelist.begin(); itp != pSmsInfo->m_Phonelist.end(); itp++)
    {
        //rm 86 +86 and some
        if(m_honeDao.m_pVerify->CheckPhoneFormat(*itp))
        {
            map<string, string>::iterator itphone = shortInfo.m_mapPhoneRefIDs.find(*itp);  //
            if(itphone == shortInfo.m_mapPhoneRefIDs.end())
            {
                shortInfo.m_mapPhoneRefIDs[itp->data()] = getUUID();        //generator uuid for every phone in every submits
            }
            else
            {
                pSmsInfo->m_RepeatPhonelist.push_back(*itp);        //save repeat phones
            }
        }
    }

    pSmsInfo->m_strClientId.assign(pHttps->m_strAccount);
    pSmsInfo->m_strContent.assign(strContent);
    pSmsInfo->m_strDate.assign(submitTime);
    pSmsInfo->m_strExtpendPort.assign(pHttps->m_strExtendPort);
    pSmsInfo->m_vecShortInfo.push_back(shortInfo);
    pSmsInfo->m_uSmsFrom = pHttps->m_uSmsFrom;
    pSmsInfo->m_uShortSmsCount = 1;
    pSmsInfo->m_uRecvCount = 1;
    pSmsInfo->m_uPkTotal = 1;
    pSmsInfo->m_uPkNumber = 1;
    pSmsInfo->m_strUid = pHttps->m_strUid;
    pSmsInfo->m_strSmsId = getUUID();
    pSmsInfo->m_strSmsType = pHttps->m_strSmsType;
    pSmsInfo->m_strSrcPhone.assign(pHttps->m_strExtendPort);
    pSmsInfo->m_uLongSms = pHttps->m_uLongSms;
    pSmsInfo->m_strTestChannelId.assign(pHttps->m_strChannelId);
    pSmsInfo->m_strTemplateId.assign(pHttps->m_strTemplateId);
    pSmsInfo->m_strTemplateParam.assign(pHttps->m_strTemplateParam);
    pSmsInfo->m_strTemplateType.assign(pHttps->m_strTemplateType);
    pSmsInfo->m_ucHttpProtocolType = pHttps->m_ucHttpProtocolType;

    generateSmsIdForEveryPhone(pSmsInfo);

    AccountMap::iterator iterAccount;

    if (!checkAccount(pHttps, pSmsInfo, iterAccount))
    {
        LogError("Call checkAccount failed.");
        return;
    }

    if (!checkAccountIp(pHttps, pSmsInfo, iterAccount))
    {
        LogError("Call checkAccountIp failed.");
        return;
    }

    if (!checkAccountPwd(pHttps, pSmsInfo, iterAccount))
    {
        LogError("Call checkAccountPwd failed.");
        return;
    }

    if (!checkProtocolType(pHttps, pSmsInfo, iterAccount))
    {
        LogError("Call checkProtocolType failed.");
        return;
    }

    pSmsInfo->m_strSid.assign(iterAccount->second.m_strSid);
    pSmsInfo->m_strUserName.assign(iterAccount->second.m_strname);
    pSmsInfo->m_uNeedExtend = iterAccount->second.m_uNeedExtend;
    pSmsInfo->m_uAgentId = iterAccount->second.m_uAgentId;
    pSmsInfo->m_uNeedSignExtend = iterAccount->second.m_uNeedSignExtend;
    pSmsInfo->m_uNeedAudit = iterAccount->second.m_uNeedaudit;
    pSmsInfo->m_strDeliveryUrl = iterAccount->second.m_strDeliveryUrl;
    pSmsInfo->m_uPayType = iterAccount->second.m_uPayType;
    pSmsInfo->m_uOverRateCharge = iterAccount->second.m_uOverRateCharge;
    pSmsInfo->m_uIdentify = iterAccount->second.m_uIdentify;
    pSmsInfo->m_uBelong_sale = iterAccount->second.m_uBelong_sale;
    pSmsInfo->m_uAccountExtAttr = iterAccount->second.m_uAccountExtAttr;

    // 判断是否为直接扣费
    if (2 == pSmsInfo->m_uPayType || 3 == pSmsInfo->m_uPayType)
    {
        pSmsInfo->m_strSmsType = iterAccount->second.m_strSmsType;
    }

    if (HttpProtocolType_TX_Json == pHttps->m_ucHttpProtocolType
            || HttpProtocolType_JD_Webservice == pHttps->m_ucHttpProtocolType)
    {
        // sp + extend
        pSmsInfo->m_strExtpendPort.insert(0, iterAccount->second.m_strSpNum);
        pSmsInfo->m_strSmsType = iterAccount->second.m_strSmsType;
    }

    if (!checkContent(pHttps, pSmsInfo))
    {
        LogError("Call checkContent failed.");
        return;
    }

    if (!checkSignExtend(pHttps, pSmsInfo))
    {
        LogError("Call checkSignExtend failed.");
        return;
    }

    if (!checkPhoneList(pHttps, pSmsInfo))
    {
        LogError("Call checkPhoneList failed.");
        return;
    }

    vector<string> vecErrorPhone;
    vector<string> vecCorrectPhone;
    vector<string> vecNomatchPhone;

    if (!checkPhoneFormat(pSmsInfo, vecErrorPhone, vecNomatchPhone))
    {
        LogError("[%s:%s] checkPhoneFormat is failed!",
                 pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str());

        CUnityToHttpServerRespMsg *pResp = new CUnityToHttpServerRespMsg();
        pResp->m_strSmsid = pSmsInfo->m_strSmsId;
        pResp->m_mapPhoneSmsId = pSmsInfo->m_mapPhoneSmsId;
        pResp->m_strDisplayNum = pSmsInfo->m_strExtpendPort;
        pResp->m_strError = PHONE_FORMAT_IS_ERROR;
        pResp->m_bTimerSend = pHttps->m_bTimerSend;
        pResp->m_uSessionId = pHttps->m_uSessionId;
        pResp->m_vecFarmatErrorPhone.insert(pResp->m_vecFarmatErrorPhone.end(), vecErrorPhone.begin(), vecErrorPhone.end());
        pResp->m_vecNomatchErrorPhone.insert(pResp->m_vecNomatchErrorPhone.end(), vecNomatchPhone.begin(), vecNomatchPhone.end());
        pResp->m_iMsgType = MSGTYPE_UNITY_TO_HTTPS_RESP;
        g_pHttpServiceThread->PostMsg(pResp);
        return ;
    }

    ////check ok

    // 定时短信插入定时任务表,及号码关联表
    if (pHttps->m_bTimerSend)
    {
        InsertTimerSendSmsDB(pSmsInfo, pHttps, vecErrorPhone, vecNomatchPhone);
    }
    else // 非定时短信直接插access流水表
    {
        InsertAccessDb(pSmsInfo);
        vecCorrectPhone = pSmsInfo->m_Phonelist;

        pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;

        ////phone format error
        pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
        pSmsInfo->m_Phonelist.clear();
        pSmsInfo->m_Phonelist = vecErrorPhone;
        InsertAccessDb(pSmsInfo);

        ////repeat phone
        pSmsInfo->m_strErrorCode = REPEAT_PHONE;
        pSmsInfo->m_Phonelist.clear();
        pSmsInfo->m_Phonelist = pSmsInfo->m_RepeatPhonelist;
        InsertAccessDb(pSmsInfo);

        pSmsInfo->m_strErrorCode = ERRORCODE_TEMPLATE_PHONE_NO_MATCH;
        pSmsInfo->m_Phonelist.clear();
        pSmsInfo->m_Phonelist = vecNomatchPhone;
        InsertAccessDb(pSmsInfo);

        pSmsInfo->m_strErrorCode = "";
        pSmsInfo->m_Phonelist.clear();
        pSmsInfo->m_Phonelist = vecCorrectPhone;
    }

    /////respone https
    CUnityToHttpServerRespMsg *pResp = new CUnityToHttpServerRespMsg();
    CHK_NULL_RETURN(pResp);
    pResp->m_strSmsid = pSmsInfo->m_strSmsId;
    pResp->m_mapPhoneSmsId = pSmsInfo->m_mapPhoneSmsId;
    pResp->m_strDisplayNum = pSmsInfo->m_strExtpendPort;
    pResp->m_strError = PHONE_FORMAT_IS_ERROR;
    pResp->m_uSessionId = pHttps->m_uSessionId;
    pResp->m_bTimerSend = pHttps->m_bTimerSend;
    pResp->m_vecCorrectPhone = pSmsInfo->m_Phonelist;
    pResp->m_vecFarmatErrorPhone.insert(pResp->m_vecFarmatErrorPhone.end(), vecErrorPhone.begin(), vecErrorPhone.end());
    pResp->m_vecRepeatPhone.insert(pResp->m_vecRepeatPhone.end(), pSmsInfo->m_RepeatPhonelist.begin(), pSmsInfo->m_RepeatPhonelist.end());
    pResp->m_vecNomatchErrorPhone.insert(pResp->m_vecNomatchErrorPhone.end(), vecNomatchPhone.begin(), vecNomatchPhone.end());
    pResp->m_iMsgType = MSGTYPE_UNITY_TO_HTTPS_RESP;
    g_pHttpServiceThread->PostMsg(pResp);

    if (pHttps->m_bTimerSend)
    {
        LogNotice("Timer send request-----");
        return;
    }

    ////build msg send to chttpsendThread
    proSetInfoToRedis(pSmsInfo);

    return;
}

void CUnityThread::proSetInfoToRedis(SMSSmsInfo *pSmsInfo)
{
    CHK_NULL_RETURN(pSmsInfo);

    UInt32 uShortInfoSize = pSmsInfo->m_vecShortInfo.size();
    UInt32 uPhoneListSize = pSmsInfo->m_Phonelist.size();

    for (UInt32 i = 0; i < uPhoneListSize; i++)
    {
        string strSmsId = getSmsId(pSmsInfo, pSmsInfo->m_Phonelist.at(i));

        string strKey = "accesssms:" + strSmsId + "_" + pSmsInfo->m_Phonelist.at(i);

        string strCmd = "";
        strCmd.append("HMSET ");
        strCmd.append(strKey);

        pSmsInfo->m_vecLongMsgContainIDs.clear();

        string strIDs = "", strPhone = pSmsInfo->m_Phonelist.at(i);
        for (UInt32 j = 0; j < uShortInfoSize; j++)
        {
            if(strIDs.empty())
            {
                string stridtmp = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[strPhone];
                strIDs = stridtmp;
                pSmsInfo->m_vecLongMsgContainIDs.push_back(stridtmp);
            }
            else
            {
                string stridtmp = pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[strPhone];
                strIDs.append(",");
                strIDs.append(stridtmp);
                pSmsInfo->m_vecLongMsgContainIDs.push_back(stridtmp);
            }
        }

        strCmd.append(" submitid ");
        strCmd.append("0");
        strCmd.append(" clientid ");
        strCmd.append(pSmsInfo->m_strClientId);

        strCmd.append(" uid ");
        string strUid = (pSmsInfo->m_strUid.empty() ? "NULL" : pSmsInfo->m_strUid);
        strCmd.append(strUid);

        strCmd.append(" id ");
        strCmd.append(strIDs);

        if(pSmsInfo->m_strDeliveryUrl.empty())
        {
            pSmsInfo->m_strDeliveryUrl = "NULL";
        }
        strCmd.append(" deliveryurl ");
        strCmd.append(pSmsInfo->m_strDeliveryUrl);

        strCmd.append(" date ");
        strCmd.append(pSmsInfo->m_strDate);

        LogDebug("[%s:%s:%s] redis cmd[%s], length[%d].",
                 pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
                 strCmd.c_str(), strCmd.length());

        TRedisReq *pRed = new TRedisReq();
        CHK_NULL_RETURN(pRed);
        pRed->m_RedisCmd = strCmd;
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_strKey = strKey;
        //g_pRedisThreadPool->PostMsg(pRed);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed);

        TRedisReq *pRedisExpire = new TRedisReq();
        CHK_NULL_RETURN(pRedisExpire);
        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisExpire->m_strKey = strKey;
        pRedisExpire->m_RedisCmd.assign("EXPIRE ");
        pRedisExpire->m_RedisCmd.append(strKey);
        pRedisExpire->m_RedisCmd.append("  259200");
        //g_pRedisThreadPool->PostMsg(pRedisExpire);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRedisExpire);

        string strMQdate = "";

        strMQdate.append("clientId=");
        strMQdate.append(pSmsInfo->m_strClientId);

        strMQdate.append("&userName=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strUserName));

        strMQdate.append("&smsId=");
        strMQdate.append(strSmsId);

        strMQdate.append("&phone=");
        strMQdate.append(strPhone);

        strMQdate.append("&sign=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strSign));

        strMQdate.append("&content=");
        strMQdate.append(Base64::Encode(pSmsInfo->m_strContent));

        strMQdate.append("&smsType=");
        strMQdate.append(pSmsInfo->m_strSmsType);

        strMQdate.append("&smsfrom=");
        strMQdate.append(Comm::int2str(pSmsInfo->m_uSmsFrom));

        strMQdate.append("&paytype=");
        strMQdate.append(Comm::int2str(pSmsInfo->m_uPayType));

        ////signport
        strMQdate.append("&signport=");
        strMQdate.append(pSmsInfo->m_strSignPort);

        strMQdate.append("&showsigntype=");
        strMQdate.append(Comm::int2str(pSmsInfo->m_uShowSignType));

        strMQdate.append("&userextpendport=");
        strMQdate.append(pSmsInfo->m_strExtpendPort);

        strMQdate.append("&longsms=");
        char sLongSms[10] = {0};
        snprintf(sLongSms, sizeof(sLongSms), "%d", pSmsInfo->m_uLongSms);
        strMQdate.append(sLongSms);

        string strIDs1 = "";
        vector<string> vecIDs = pSmsInfo->m_vecLongMsgContainIDs;
        for(vector<string>::iterator itids = vecIDs.begin(); itids != vecIDs.end(); itids++)
        {
            if(itids == vecIDs.begin())
            {
                strIDs1 = (*itids);
            }
            else
            {
                strIDs1.append(",");
                strIDs1.append(*itids);
            }
        }
        strMQdate.append("&ids=");
        strMQdate.append(strIDs1);

        strMQdate.append("&clientcnt=");
        char sRecvCount[16] = {0};
        snprintf(sRecvCount, sizeof(sRecvCount), "%d", pSmsInfo->m_uRecvCount);
        strMQdate.append(sRecvCount);

        strMQdate.append("&csid=");
        char strComponentId[32] = {0};
        snprintf(strComponentId, sizeof(strComponentId), "%ld", g_uComponentId);
        strMQdate.append(strComponentId);

        strMQdate.append("&csdate=");
        strMQdate.append(pSmsInfo->m_strDate);

        //模板短信相关
        strMQdate.append("&templateid=").append(pSmsInfo->m_strTemplateId);
        strMQdate.append("&temp_params=").append(pSmsInfo->m_strTemplateParam);
        strMQdate.append("&temp_type=").append(pSmsInfo->m_strTemplateType);
        //for channel test
        strMQdate.append("&test_channelid=");
        strMQdate.append(pSmsInfo->m_strTestChannelId);
        map<string, SmsAccount>::iterator iter = m_AccountMap.find(pSmsInfo->m_strClientId);
        if(iter == m_AccountMap.end())
        {
            LogError("[%s:%s:%s] can not find account in account map! ", pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str());
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }

        string strMessageType = "";
        UInt64 uPhoneType = m_honeDao.getPhoneType(strPhone);
        UInt64 uSmsType = atoi(pSmsInfo->m_strSmsType.c_str());

        if(2 == iter->second.m_uClientLeavel || 3 == iter->second.m_uClientLeavel)
        {
            switch (uPhoneType)
            {
            case YIDONG:
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("02");
                }
                else
                {
                    strMessageType.assign("01");
                }
                break;
            case DIANXIN:
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("06");
                }
                else
                {
                    strMessageType.assign("05");
                }
                break;
            case LIANTONG:
            case FOREIGN:
                if (uSmsType == SMS_TYPE_MARKETING || uSmsType == SMS_TYPE_USSD || uSmsType == SMS_TYPE_FLUSH_SMS)
                {
                    strMessageType.assign("04");
                }
                else
                {
                    strMessageType.assign("03");
                }
                break;
            default:
                LogError("[%s:%s:%s] phoneType:%u is invalid.",
                         pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
                         uPhoneType);
                signalErrSmsPushRtAndDb2(pSmsInfo, i);
                continue;
            }
        }
        else if( 1 == iter->second.m_uClientLeavel)
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
            LogError("client leavel[%d] is error ,client id is %s", iter->second.m_uClientLeavel, pSmsInfo->m_strClientId.data());
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }
#if 1
        mq_weightgroups_map_t::iterator itReq = m_mapMQWeightGroups.find(strMessageType);
        if (itReq == m_mapMQWeightGroups.end())
        {
            LogError("[%s:%s:%s] MessageType:%s is not find in m_mapMQWeightGroups.",
                     pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
                     strMessageType.data());
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }

        MQWeightDefaultCalculator calc;
        itReq->second.weightSort(calc);

        UInt64 uMqId = (*itReq->second.begin())->getData();
        // Get first mqId
        map<UInt64, MqConfig>::iterator itrMq = m_mqConfigInfo.find(uMqId);

        if (itrMq == m_mqConfigInfo.end())
        {
            LogError("[%s:%s:%s] mqid:%lu is not find in m_mqConfigInfo.",
                     pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
                     uMqId);
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }
#else

        string strComKey = "";
        char temp[250] = {0};
        snprintf(temp, 250, "%lu_%s_0", g_uComponentId, strMessageType.data());
        strComKey.assign(temp);

        map<string, ComponentRefMq>::iterator itReq =  m_componentRefMqInfo.find(strComKey);
        if (itReq == m_componentRefMqInfo.end())
        {
            LogError("==except== strKey:%s is not find in m_componentRefMqInfo.", strComKey.data());
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }

        map<UInt64, MqConfig>::iterator itrMq = m_mqConfigInfo.find(itReq->second.m_uMqId);
        if (itrMq == m_mqConfigInfo.end())
        {
            LogError("==except== mqid:%lu is not find in m_mqConfigInfo.", itReq->second.m_uMqId);
            signalErrSmsPushRtAndDb(pSmsInfo, i);
            continue;
        }
#endif

        string strExchange = itrMq->second.m_strExchange;
        string strRoutingKey = itrMq->second.m_strRoutingKey;

        LogNotice("[%s:%s:%s] insert to rabbitMQ, strMQdate[%s]", 
            pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
            strMQdate.data());

        //modify by fangjinxiong 20161023, check Mq link
        UInt32 Queuesize = g_pMQC2sIOProducerThread->GetMsgSize();
        if(Queuesize > m_uIOProduceMaxChcheSize && (g_pMQC2sIOProducerThread->m_linkState == false))
        {
            //save old phone list
            std::vector<std::string> list = pSmsInfo->m_Phonelist;

            //update db
            pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
            pSmsInfo->m_strErrorCode = MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE;
            pSmsInfo->m_strYZXErrCode = MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE;
            pSmsInfo->m_Phonelist.clear();
            pSmsInfo->m_Phonelist.push_back(strPhone);
            UpdateRecord(pSmsInfo);

            //report http
            TReportReceiveToSMSReq *pReq = new TReportReceiveToSMSReq();
            CHK_NULL_RETURN(pReq);
            pReq->m_iStatus = SMS_STATUS_GW_RECEIVE_HOLD;
            pReq->m_strSmsId = strSmsId;
            pReq->m_lReportTime = time(NULL);
            pReq->m_strDesc = MQ_LINK_ERROR_AND_QUEUESIZE_TOO_LARGE;
            pReq->m_iType = 1;	//1: report   2: upstream
            pReq->m_strPhone = strPhone;
            pReq->m_uUpdateFlag = 1;
            pReq->m_uSmsfrom = pSmsInfo->m_uSmsFrom;
            pReq->m_iMsgType = MSGTYPE_REPORTRECEIVE_TO_SMS_REQ;

            g_pReportPushThread->PostMsg(pReq);

            LogWarn("[%s:%s:%s] MQlink error, and g_pMQIOProducerThread.queuesize[%d] > m_uIOProduceMaxChcheSize[%d]",
                    pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(),
                    Queuesize, m_uIOProduceMaxChcheSize);
            //refill phonelist
            pSmsInfo->m_Phonelist = list;
        }
        else
        {
            TMQPublishReqMsg *pMQ = new TMQPublishReqMsg();
            pMQ->m_strData = strMQdate;
            pMQ->m_strExchange = strExchange;
            pMQ->m_strRoutingKey = strRoutingKey;
            pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
            g_pMQC2sIOProducerThread->PostMsg(pMQ);
        }
    }

    return;
}

bool CUnityThread::signalErrSmsPushRtAndDb(SMSSmsInfo *pSmsInfo, UInt32 i)
{
    std::vector<std::string> list = pSmsInfo->m_Phonelist;
    string strCurPhone = pSmsInfo->m_Phonelist.at(i);

    //update db
    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    pSmsInfo->m_strErrorCode = C2S_MQ_CONFIG_ERROR;
    pSmsInfo->m_strYZXErrCode = C2S_MQ_CONFIG_ERROR;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist.push_back(strCurPhone);
    UpdateRecord(pSmsInfo);

    //report http
    TReportReceiveToSMSReq *pReq = new TReportReceiveToSMSReq();
    pReq->m_iStatus = SMS_STATUS_GW_RECEIVE_HOLD;
    pReq->m_strSmsId = getSmsId(pSmsInfo, strCurPhone);
    pReq->m_lReportTime = time(NULL);
    pReq->m_strDesc = C2S_MQ_CONFIG_ERROR;
    pReq->m_iType = 1;	//1: report   2: upstream
    pReq->m_strPhone = strCurPhone;
    pReq->m_uUpdateFlag = 1;
    pReq->m_uSmsfrom = pSmsInfo->m_uSmsFrom;
    pReq->m_iMsgType = MSGTYPE_REPORTRECEIVE_TO_SMS_REQ;

    g_pReportPushThread->PostMsg(pReq);


    LogWarn("[%s:%s:%s] sms push to mq is error",
            pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str(), strCurPhone.c_str());

    //refill phonelist
    pSmsInfo->m_Phonelist = list;
    return true;

}

bool CUnityThread::signalErrSmsPushRtAndDb2(SMSSmsInfo *pSmsInfo, UInt32 i)
{
    std::vector<std::string> list = pSmsInfo->m_Phonelist;
    string strCurPhone = pSmsInfo->m_Phonelist.at(i);

    //update db
    pSmsInfo->m_uState = SMS_STATUS_GW_RECEIVE_HOLD;
    pSmsInfo->m_strErrorCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_strYZXErrCode = PHONE_FORMAT_IS_ERROR;
    pSmsInfo->m_Phonelist.clear();
    pSmsInfo->m_Phonelist.push_back(strCurPhone);
    UpdateRecord(pSmsInfo);

    //report http
    TReportReceiveToSMSReq *pReq = new TReportReceiveToSMSReq();
    pReq->m_iStatus = SMS_STATUS_GW_RECEIVE_HOLD;
    pReq->m_strSmsId = getSmsId(pSmsInfo, strCurPhone);
    pReq->m_lReportTime = time(NULL);
    pReq->m_strDesc = PHONE_FORMAT_IS_ERROR;
    pReq->m_iType = 1;	//1: report   2: upstream
    pReq->m_strPhone = strCurPhone;
    pReq->m_uUpdateFlag = 1;
    pReq->m_uSmsfrom = pSmsInfo->m_uSmsFrom;
    pReq->m_iMsgType = MSGTYPE_REPORTRECEIVE_TO_SMS_REQ;

    g_pReportPushThread->PostMsg(pReq);


    LogWarn("[%s:%s:%s] sms push to mq is error",
            pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str(), strCurPhone.c_str());

    //refill phonelist
    pSmsInfo->m_Phonelist = list;
    return true;

}

void CUnityThread::HandleTimeOut(TMsg *pMsg)
{
    switch ((UInt32)pMsg->m_iSeq)
    {
    case ACCOUNT_UNLOCK_TIMER_ID:
    {
        if(m_UnlockTimer)
        {
        	SAFE_DELETE(m_UnlockTimer);
        }
        m_UnlockTimer = SetTimer(ACCOUNT_UNLOCK_TIMER_ID, "", 24 * 60 * 60 * 1000);

        time_t nowTime = time(NULL);
        struct tm timenow = {0};
        localtime_r(&nowTime, &timenow);
        for (std::map<string, SmsAccount>::iterator iter = m_AccountMap.begin(); iter != m_AccountMap.end(); ++iter)
        {
            if(7 == iter->second.m_uStatus)
            {
                iter->second.m_uStatus = 0;
                char tempTime[64] = { 0 };
                /////if (timenow != 0)
                strftime(tempTime, sizeof(tempTime), "%Y%m%d%H%M%S", &timenow);

                string unlockTime;
                unlockTime.assign(tempTime);

                char sql[1024]  = {0};
                snprintf(sql, 1024, "update t_sms_account set status = 1 where clientid = '%s'", iter->second.m_strAccount.data());

                TDBQueryReq *updateAccountTable = new TDBQueryReq();
                updateAccountTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                updateAccountTable->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(updateAccountTable);

                snprintf(sql, 1024, "update t_sms_account_login_status set status = 1, unlocktime = '%s', unlockby = 'access',updatetime = '%s' where clientid = '%s' and status = 0",
                         unlockTime.data(), unlockTime.data(), iter->second.m_strAccount.data());
                TDBQueryReq *updateLoginStatusTable = new TDBQueryReq();
                updateLoginStatusTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                updateLoginStatusTable->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(updateLoginStatusTable);

            }
        }
        break;
    }
    case TIMERSEND_TASK_INSTALL_TIMER_ID:
    {
        const std::string &strTaskUuid = pMsg->m_strSessionID;
        executeTimersendTask(strTaskUuid);
        break;
    }
    // 定时短信任务从现在开始检测
    case TIMERSEND_TASKS_START_EXECUTE_TIMER_ID:
    {
        LogNotice("TimerSend Task Loop check begin!");

        std::map<std::string, timersend_taskinfo_ptr_t> mapTimersendTaskInfo;
        g_pRuleLoadThread->getTimersendTaskInfo(mapTimersendTaskInfo);
        updateTimersendTaskInfo(mapTimersendTaskInfo);

        m_bTimersendTaskStartExecution = true;

        // 删除定时器，启动以后不再需要了
        SAFE_DELETE(m_TimersendTaskStartExecuteTimer);
        break;
    }
    // 清理进度缓存
    case TIMERSEND_TASKS_PROGRESS_ERASE_TIMER_ID:
    {
        std::map<std::string, timersend_taskinfo_ptr_t> mapTimersendTaskInfo;

        time_t now = time(NULL);
        std::map<string, StProgress>::iterator proIter =  m_timerSndPhonesInsrtProgressCache.begin();
        for (; proIter != m_timerSndPhonesInsrtProgressCache.end();)
        {
            StProgress &progress = proIter->second;
            time_t elapsed = difftime(now, progress.createTime);
            if (elapsed > TIMERSEND_TASKS_PROGRESS_CACHE_TIMEOUT)
            {
                LogNotice("clean task[%s]'s progress-cache, elapsed[%lf]", proIter->first.c_str(), elapsed);
                m_timerSndPhonesInsrtProgressCache.erase(proIter++);
            }
            else
            {
                proIter++;
            }
        }
        SAFE_DELETE(m_TimersendTaskProgressCleanTimer);

        m_TimersendTaskProgressCleanTimer = SetTimer(TIMERSEND_TASKS_PROGRESS_ERASE_TIMER_ID, "",
                                            TIMERSEND_TASKS_PROGRESS_CACHE_CLEANER_INTERVAL * 1000);
        break;
    }
    default:
        break;
    }
}

void CUnityThread::HandleAccountUpdateReq(TMsg *pMsg)
{
    TUpdateSmsAccontReq *pAccountUpdateReq = (TUpdateSmsAccontReq *)pMsg;

    AccountMap::iterator oldIter = m_AccountMap.begin();
    for( ; oldIter != m_AccountMap.end(); oldIter++)
    {
        AccountMap::iterator newIter = pAccountUpdateReq->m_SmsAccountMap.find(oldIter->first);
        if(newIter != pAccountUpdateReq->m_SmsAccountMap.end())
        {
            newIter->second.m_uLoginErrorCount = (1 != newIter->second.m_uStatus ? 0 : oldIter->second.m_uLoginErrorCount);

            newIter->second.m_uLinkCount = oldIter->second.m_uLinkCount;
            newIter->second.m_uLockTime = oldIter->second.m_uLockTime;
        }
    }
    m_AccountMap.clear();
    m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
}

void CUnityThread::AccountLock(SmsAccount &accountInfo, const char *accountLockReason)
{
    TDBQueryReq *pUpdateAccountTable = new TDBQueryReq();
    CHK_NULL_RETURN(pUpdateAccountTable);

    char sql[1024] = {0};
    snprintf(sql, 1024, "update t_sms_account set status = 7 where clientid = '%s'", accountInfo.m_strAccount.c_str());
    pUpdateAccountTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pUpdateAccountTable->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pUpdateAccountTable);


    time_t now = time(NULL);
    accountInfo.m_uLockTime = now;
    LogWarn("lock account[%s], locktime[%ld],login errorNum[%d] more than 5 times.",
            accountInfo.m_strAccount.data(), accountInfo.m_uLockTime, accountInfo.m_uLoginErrorCount);

    accountInfo.m_uStatus = 7;
    accountInfo.m_uLoginErrorCount = 0;

    struct tm timenow = {0};
    localtime_r(&now, &timenow);
    char tempTime[64] = {0};
    strftime(tempTime, sizeof(tempTime), "%Y%m%d%H%M%S", &timenow);

    string locktime = "";
    locktime.assign(tempTime);

    snprintf(sql, 1024, "insert t_sms_account_login_status(clientid, remarks, locktime, status, updatetime) values('%s', '%s', '%s', '0', '%s')",
             accountInfo.m_strAccount.data(), accountLockReason, locktime.data(), locktime.data());
    TDBQueryReq *pUpdateLoginStatusTable = new TDBQueryReq();
    CHK_NULL_RETURN(pUpdateLoginStatusTable);
    pUpdateLoginStatusTable->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
    pUpdateLoginStatusTable->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pUpdateLoginStatusTable);
}

bool CUnityThread::getSignPort(const string &strClientId, string &strSign, string &strOut)
{
    string strKey = strClientId + "&" + strSign;
    ClientIdAndSignMap::iterator itr = m_mapClientSignPort.find(strKey);
    if (itr == m_mapClientSignPort.end())
    {
        LogWarn("clientId[%s], sign[%s] is not find table in t_sms_clientid_signport.", strClientId.c_str(), strSign.c_str());
        return false;
    }

    strOut = itr->second.m_strSignPort;
    return true;
}

int CUnityThread::ExtractSignAndContent(const string &strContent, string &strOut, string &strSign, UInt32 &uChina, UInt32 &uShowSignType)
{
    string strLeft = "[";
    string strRight = "]";
    uChina = 0;

    if (true == Comm::IncludeChinese((char *)strContent.data()))
    {
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";
        strLeft = http::UrlCode::UrlDecode(strLeft);
        strRight = http::UrlCode::UrlDecode(strRight);
        uChina = 1;
    }

    std::string::size_type lPos = strContent.find(strLeft);
    if (lPos == std::string::npos)
    {
        LogWarn("this is not find leftSign[%s]", strLeft.data());
        strOut = strContent;
        return HTTPS_RESPONSE_CODE_26; /// -6
    }
    else if (0 == lPos)   ////sign is before
    {
        std::string::size_type rPos = strContent.find(strRight);
        if((rPos == std::string::npos))
        {
            //can not find sign
            LogWarn("can not find sign. content[%s]", strContent.data());
            strOut = strContent;
            return HTTPS_RESPONSE_CODE_26;	//-6

        }
        else if((rPos + strRight.length()) == strContent.length())
        {
            //cant find content
            LogWarn("can not find content. content[%s]", strContent.data());
            strOut = strContent;
            return HTTPS_RESPONSE_CODE_24;	// -6
        }
        ////extract sign and content
        uShowSignType = 1;
        strOut = strContent.substr(rPos + strRight.length());
        strSign = strContent.substr(strLeft.length(), rPos - strLeft.length());
    }
    else //// sign is after
    {
        lPos = strContent.rfind(strLeft);
        std::string::size_type rPos = strContent.rfind(strRight);
        if ((lPos == std::string::npos) || (rPos == std::string::npos))
        {
            LogWarn("cannot find sign.content[%s]", strContent.data());
            strOut = strContent;
            return HTTPS_RESPONSE_CODE_26; //// -6
        }
        else if ((rPos + strRight.length()) != strContent.length())
        {
            //can not find sign
            LogWarn("can not find sign. content[%s]", strContent.data());
            strOut = strContent;
            return HTTPS_RESPONSE_CODE_26;	// -6
        }

        uShowSignType = 2;
        strOut = strContent.substr(0, lPos);
        strSign = strContent.substr(lPos + strLeft.length(), rPos - lPos - strLeft.length());
    }

    //// check sign length
    utils::UTFString utfHelper;
    int length = utfHelper.getUtf8Length(strSign);

    if(length <= 1)
    {
        LogWarn("WARN, sign length=0. length[%d]", length);
        return HTTPS_RESPONSE_CODE_27;  ///-8
    }
    else if(length > 100)
    {
        LogWarn("WARN, sign length>100. length[%d]", length);
        return HTTPS_RESPONSE_CODE_28;  ///-8
    }
    else
    {
        //OK
        return 0;
    }
}

bool CUnityThread::checkPhoneFormat(SMSSmsInfo *pSmsInfo,
                                    vector<string> &formatErrorVec,
                                    vector<string> &nomatchVec)
{

    CHK_NULL_RETURN_FALSE(pSmsInfo);

    UInt64 uPhoneType = 0;
    UInt64 uSmsType = atoi(pSmsInfo->m_strSmsType.c_str());

    for (vector<string>::iterator itrPhone = pSmsInfo->m_Phonelist.begin();
            itrPhone != pSmsInfo->m_Phonelist.end();)
    {
        string &strTempPhone = *itrPhone;

        if(false == m_honeDao.m_pVerify->CheckPhoneFormat(strTempPhone))
        {
            LogError("[%s: ] phone[%s] format is error!",
                     pSmsInfo->m_strSmsId.data(),
                     strTempPhone.data());

            if(false == m_honeDao.m_pVerify->isVisiblePhoneFormat(strTempPhone))
            {
                formatErrorVec.push_back(INVISIBLE_NUMBER_STRING);
            }
            else
            {
                formatErrorVec.push_back(strTempPhone);
            }

            itrPhone = pSmsInfo->m_Phonelist.erase(itrPhone);
        }
        else
        {
            uPhoneType = m_honeDao.getPhoneType(strTempPhone);
            if (FOREIGN == uPhoneType &&
                    (SMS_TYPE_USSD == uSmsType || SMS_TYPE_FLUSH_SMS == uSmsType
                     || TEMPLATE_TYPE_HANG_UP_SMS == atoi((pSmsInfo->m_strTemplateType).c_str())))
            {
                LogError("[%s:%s:%s] phone operator[%d] error for smstype[%d]",
                         pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str(), strTempPhone.c_str(),
                         uPhoneType, uSmsType);

                nomatchVec.push_back(strTempPhone);
                itrPhone = pSmsInfo->m_Phonelist.erase(itrPhone);
            }
            else
            {
                if(pSmsInfo->m_strPhone.empty())
                {
                    pSmsInfo->m_strPhone.assign(*itrPhone);
                }
                else
                {
                    pSmsInfo->m_strPhone.append(",");
                    pSmsInfo->m_strPhone.append(*itrPhone);
                }
                itrPhone++;
            }
        }
    }

    ////check repeat phone
    map<string, int> phoneMapInfo;
    for (vector<string>::iterator itrPhone = pSmsInfo->m_Phonelist.begin();
            itrPhone != pSmsInfo->m_Phonelist.end();)
    {
        map<string, int>::iterator itrMap = phoneMapInfo.find(*itrPhone);
        if (itrMap == phoneMapInfo.end())
        {
            phoneMapInfo[*itrPhone] = 1;
            itrPhone++;
        }
        else
        {
            itrPhone = pSmsInfo->m_Phonelist.erase(itrPhone);
        }
    }

    if (pSmsInfo->m_strPhone.empty())
    {
        LogError("[%s:%s] no send phone!", pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str());
        return false;
    }

    return true;
}

void CUnityThread::InsertAccessDb(SMSSmsInfo *pSmsInfo)
{
    CHK_NULL_RETURN(pSmsInfo);

    UInt32 j = 0;
    if (1 != pSmsInfo->m_uPkTotal)
    {
        j = pSmsInfo->m_uRecvCount - 1;
    }

    string strDate = pSmsInfo->m_strDate.substr(0, 8);	//get date.

    string strTempDesc = "";
    map<string, string>::iterator itrDesc = m_mapSystemErrorCode.find(pSmsInfo->m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(pSmsInfo->m_strErrorCode);
    }

    UInt32 uOperaterType = 0;
    UInt32 uPhoneCount = pSmsInfo->m_Phonelist.size();
    MYSQL *pMysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    CHK_NULL_RETURN(pMysqlConn);

    for(UInt32 i = 0; i < uPhoneCount; i++)
    {
        string strPhone = pSmsInfo->m_Phonelist.at(i);
        if (m_honeDao.m_pVerify->CheckPhoneFormat(strPhone))
        {
            uOperaterType = m_honeDao.getPhoneType(strPhone);
        }

        string tmpId = pSmsInfo->m_strErrorCode.empty() ? pSmsInfo->m_vecShortInfo.at(j).m_mapPhoneRefIDs[strPhone]:getUUID(); // here only ok phones, repeatphones need generat another uuid

        std::string msg = pSmsInfo->m_vecShortInfo.at(j).m_strShortContent;

        char strTempParam[2048] = { 0 };
        char content[3072] = { 0 };
        char strUid[64] = { 0 };
        char sign[128] = { 0 };

        UInt32 position = pSmsInfo->m_strSign.length();
        if(pSmsInfo->m_strSign.length() > 100)
        {
            position = Comm::getSubStr(pSmsInfo->m_strSign, 100);
        }

        if(pMysqlConn)
        {
            mysql_real_escape_string(pMysqlConn, content, msg.data(), msg.length());

            //将uid字段转义
            mysql_real_escape_string(pMysqlConn, strUid, pSmsInfo->m_strUid.substr(0, 60).data(), pSmsInfo->m_strUid.substr(0, 60).length());

            //将sign字段转义
            mysql_real_escape_string(pMysqlConn, sign, pSmsInfo->m_strSign.substr(0, position).data(), pSmsInfo->m_strSign.substr(0, position).length());

            if (!pSmsInfo->m_strTemplateParam.empty())
            {
                mysql_real_escape_string(pMysqlConn, strTempParam, pSmsInfo->m_strTemplateParam.data(),
                                         pSmsInfo->m_strTemplateParam.length());
            }
        }

        string strSmsId = getSmsId(pSmsInfo, strPhone);
        UInt32 uiSmsFrom = (SMS_FROM_ACCESS_HTTP_2 == pSmsInfo->m_uSmsFrom) ? SMS_FROM_ACCESS_HTTP : pSmsInfo->m_uSmsFrom;

        char sql[10240]  = {0};
        char cBelongSale[32] = {0};
        snprintf(cBelongSale, 32, "%ld", pSmsInfo->m_uBelong_sale);

        string strBelongSale = cBelongSale;

        snprintf(sql, 10240, "insert into t_sms_access_%d_%s(id,content,srcphone,phone,smscnt,smsindex,sign,submitid,smsid,clientid,"
                 "operatorstype,smsfrom,state,errorcode,date,innerErrorcode,channelid,smstype,charge_num,paytype,agent_id,username ,isoverratecharge,"
                 "uid,showsigntype,product_type,c2s_id,process_times,longsms,channelcnt,template_id,temp_params,sid,belong_sale,sub_id,timer_send_taskid)"
                 "	values('%s', '%s', '%s', '%s', '%d', '%d', '%s', '%ld', '%s', '%s', '%d',"
                 "'%d', '%d', '%s', '%s','%s','%d','%d','%d','%d','%ld','%s','%d','%s','%d','%d','%lu','%d','%d','%d', %s, '%s', '%s', %s ,'%s', '%s');",
                 pSmsInfo->m_uIdentify,
                 strDate.data(),
                 tmpId.data(),
                 content,
                 pSmsInfo->m_strSrcPhone.substr(0, 20).data(),
                 pSmsInfo->m_Phonelist[i].substr(0, 20).data(),
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkTotal,
                 pSmsInfo->m_vecShortInfo.at(j).m_uPkNumber,
                 sign,
                 pSmsInfo->m_vecShortInfo.at(j).m_uSubmitId,
                 strSmsId.data(),
                 pSmsInfo->m_strClientId.data(),
                 uOperaterType,
                 uiSmsFrom,
                 pSmsInfo->m_uState,
                 strTempDesc.data(),
                 pSmsInfo->m_strDate.data(),
                 strTempDesc.data(),
                 0,
                 atoi(pSmsInfo->m_strSmsType.data()),
                 pSmsInfo->m_uSmsNum,
                 pSmsInfo->m_uPayType,
                 pSmsInfo->m_uAgentId,
                 pSmsInfo->m_strUserName.data(),
                 pSmsInfo->m_uOverRateCharge,
                 strUid,
                 pSmsInfo->m_uShowSignType,
                 pSmsInfo->m_uProductType,
                 g_uComponentId,
                 pSmsInfo->m_uProcessTimes,
                 pSmsInfo->m_uLongSms,
                 1,
                 (pSmsInfo->m_strTemplateId.empty()) ? "NULL" : pSmsInfo->m_strTemplateId.data(),
                 strTempParam,
                 " ",
                 (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
                 "0",
                 pSmsInfo->m_strTaskUuid.c_str());

        LogDebug("[%s:%s%s] access insert DB sql[%s].", pSmsInfo->m_strClientId.c_str(), strSmsId.c_str(), strPhone.c_str(), sql);

        TMQPublishReqMsg *pMQ = new TMQPublishReqMsg();
        CHK_NULL_RETURN(pMQ);

        pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
        pMQ->m_strExchange.assign(g_strMQDBExchange);
        pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
        pMQ->m_strData.assign(sql);
        pMQ->m_strData.append("RabbitMQFlag=");
        pMQ->m_strData.append(tmpId.c_str());
        g_pMQDBProducerThread->PostMsg(pMQ);

        {
            MONITOR_INIT(MONITOR_RECEIVE_MT_SMS );
            MONITOR_VAL_SET("clientid", pSmsInfo->m_strClientId);
            MONITOR_VAL_SET("username", pSmsInfo->m_strUserName);
            MONITOR_VAL_SET_INT("operatorstype", uOperaterType);
            MONITOR_VAL_SET_INT("smsfrom", pSmsInfo->m_uSmsFrom);
            MONITOR_VAL_SET_INT("paytype", pSmsInfo->m_uPayType);
            MONITOR_VAL_SET("sign", sign );
            MONITOR_VAL_SET("smstype", pSmsInfo->m_strSmsType);
            MONITOR_VAL_SET("date", pSmsInfo->m_strDate);
            MONITOR_VAL_SET("uid",  strUid);
            MONITOR_VAL_SET("smsid", pSmsInfo->m_strSmsId);
            MONITOR_VAL_SET("phone", pSmsInfo->m_Phonelist[i].substr(0, 20).data());
            MONITOR_VAL_SET_INT("id", pSmsInfo->m_uIdentify);
            MONITOR_VAL_SET_INT("state", pSmsInfo->m_uState);
            MONITOR_VAL_SET("errrorcode", pSmsInfo->m_strErrorCode);
            MONITOR_VAL_SET("errordesc", strTempDesc);
            MONITOR_VAL_SET_INT("component_id", g_uComponentId);

            if (!g_pMQMonitorPublishThread)
            {
                LogError("g_pMQMonitorPublishThread NULL ");
                return;
            }
            MONITOR_PUBLISH(g_pMQMonitorPublishThread );
        }

    }
    return;
}

/*
	IOMQ link err, and g_pIOMQPublishThread.queuesize > m_uIOProduceMaxChcheSize.
	then will update db and send report.
*/
void CUnityThread::UpdateRecord(SMSSmsInfo *pInfo)
{
    CHK_NULL_RETURN(pInfo);

    pInfo->m_strSubmit.assign(pInfo->m_strErrorCode);

    string strDate = pInfo->m_strDate.substr(0, 8);	//get date.

    string strTempDesc = "";
    map<string, string>::iterator itrDesc = m_mapSystemErrorCode.find(pInfo->m_strErrorCode);
    if (itrDesc != m_mapSystemErrorCode.end())
    {
        strTempDesc.assign(pInfo->m_strErrorCode);
        strTempDesc.append("*");
        strTempDesc.append(itrDesc->second);
    }
    else
    {
        strTempDesc.assign(pInfo->m_strErrorCode);
    }

    for(vector<string>::iterator it = pInfo->m_vecLongMsgContainIDs.begin(); it != pInfo->m_vecLongMsgContainIDs.end(); it++)
    {
        char sql[1024]  = {0};
        snprintf(sql, 1024, "UPDATE t_sms_access_%d_%s SET state='%d', errorcode='%s',innerErrorcode='%s' where id = '%s' and date ='%s';",
                 pInfo->m_uIdentify,
                 strDate.data(),
                 pInfo->m_uState,
                 strTempDesc.data(),
                 strTempDesc.data(),
                 it->data(),
                 pInfo->m_strDate.data()
                );	//id

        LogDebug("[%s:%s:%s] update sql[%s].", pInfo->m_strClientId.c_str(), pInfo->m_strSmsId.c_str(), pInfo->m_strPhone.c_str(), sql);

        TMQPublishReqMsg *pMQ = new TMQPublishReqMsg();
        CHK_NULL_RETURN(pMQ);

        pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
        pMQ->m_strExchange.assign(g_strMQDBExchange);
        pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
        pMQ->m_strData.assign(sql);
        pMQ->m_strData.append("RabbitMQFlag=");
        pMQ->m_strData.append(it->data());
        g_pMQDBProducerThread->PostMsg(pMQ);
        /* END:   Modified by fxh, 2016/10/14   PN:RabbitMQ */
    }

    return;
}

void CUnityThread::InsertTimerSendSmsDB(SMSSmsInfo *pSmsInfo, CHttpServerToUnityReqMsg *pHttps, const vector<string> &vecErrorPhone, const vector<string> &vecNomatchPhone)
{
    CHK_NULL_RETURN(pSmsInfo);
    CHK_NULL_RETURN(pHttps);

    MYSQL *pMysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    CHK_NULL_RETURN(pMysqlConn);

    char szUid[64] = { 0 };
    char sign[1024] = { 0 };
    char content[3072] = { 0 };

    if(pMysqlConn != NULL)
    {
        mysql_real_escape_string(pMysqlConn, content, pHttps->m_strContent.data(), pHttps->m_strContent.length());

        //将uid字段转义
        mysql_real_escape_string(pMysqlConn, szUid, pSmsInfo->m_strUid.substr(0, 60).data(), pSmsInfo->m_strUid.substr(0, 60).length());

        //将sign字段转义
        mysql_real_escape_string(pMysqlConn, sign, pSmsInfo->m_strSign.data(), pSmsInfo->m_strSign.size());
    }

    std::string strTaskUuid = getUUID();
    // 插入定时短信任务表
    char sql[10240]  = {0};

    snprintf(sql, sizeof(sql),  "insert into t_sms_timer_send_task("
             "task_id, clientid, agent_id, smstype, submittype, "
             "total_phones_num, smsid, smsfrom, component_id, extend, "
             "srcphone, uid, sign, content, charge_num, showsigntype, "
             "is_china, submit_time, set_send_time, status) "
             "values("
             "'%s', '%s', %lu, %d, %u, "
             " %u, '%s', %u, %ld, '%s', "
             "'%s', '%s', '%s', '%s', %u, %u,"
             "%s, '%s', '%s', %u"
             ")",
             strTaskUuid.c_str(),
             pSmsInfo->m_strClientId.c_str(),
             pSmsInfo->m_uAgentId,
             atoi(pSmsInfo->m_strSmsType.c_str()),
             pHttps->m_uTimerSendSubmittype,
             (UInt32)pSmsInfo->m_Phonelist.size(),   // 存储成功的号码数
             pSmsInfo->m_strSmsId.c_str(),
             pSmsInfo->m_uSmsFrom,
             g_uComponentId,
             pSmsInfo->m_strExtpendPort.c_str(),
             pSmsInfo->m_strSrcPhone.c_str(),
             szUid,
             sign,
             pSmsInfo->m_strContent.c_str(),
             pSmsInfo->m_uSmsNum,
             pSmsInfo->m_uShowSignType,
             pSmsInfo->m_strIsChina.c_str(),
             pSmsInfo->m_strDate.c_str(),
             pHttps->m_strTimerSendTime.c_str(),
             TimerSendTaskState_PreHanding
            );
    LogDebug("[%s:%s:%s] insert timer_send_task[task_id:'%s', smsid:'%s'] DB sql[%s].",
             pSmsInfo->m_strClientId.c_str(), pSmsInfo->m_strSmsId.c_str(), pSmsInfo->m_strSrcPhone.c_str(),
             strTaskUuid.c_str(), pSmsInfo->m_strSmsId.c_str(), sql);

    TDBQueryReq *pTaskInsertDBReq = new TDBQueryReq;
    CHK_NULL_RETURN(pTaskInsertDBReq);

    pTaskInsertDBReq->m_SQL = sql;
    pTaskInsertDBReq->m_strSessionID = "TimerSendUpdate_";
    pTaskInsertDBReq->m_strSessionID.append(strTaskUuid);
    pTaskInsertDBReq->m_strSessionID.append("_INIT");
    pTaskInsertDBReq->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    pTaskInsertDBReq->m_pSender = this;
    g_pDisPathDBThreadPool->PostMsg(pTaskInsertDBReq);

    StProgress stPhonesInsertProgress;
    stPhonesInsertProgress.uTotal = pSmsInfo->m_Phonelist.size();
    stPhonesInsertProgress.createTime = time(NULL);
    m_timerSndPhonesInsrtProgressCache[strTaskUuid] = stPhonesInsertProgress;

    memset(sql, 0, sizeof(sql));
    // 插入时使用当前的周一时间
    LogDebug("time:%s", pSmsInfo->m_strDate.c_str());
    std::string strTblIdx = Comm::getLastMondayByDate(pSmsInfo->m_strDate, "%Y%m%d%H%M%S");

    for (int i = 0; i < 4; ++i)
    {
        const std::vector<std::string> *pPhoneList = NULL;
        UInt8 uPhoneState = TimerSendPhoneState_AcceptFail;

        std::string strSessionPrefix = "TimerSendUpdate_" + strTaskUuid + "_WP"; // Wrong-Phone
        switch(i)
        {
        case 0:
            pPhoneList = &pSmsInfo->m_Phonelist;
            uPhoneState = TimerSendPhoneState_AcceptSucc;
            strSessionPrefix = "TimerSendUpdate_" + strTaskUuid + "_CP"; // Correct-Phone
            break;
        case 1:
            pPhoneList = &pSmsInfo->m_RepeatPhonelist;
            break;
        case 2:
            pPhoneList = &vecErrorPhone;
            break;
        case 3:
            pPhoneList = &vecNomatchPhone;
            break;
        }

        for (size_t s = 0; s < pPhoneList->size(); ++s)
        {
            const std::string &phone = (*pPhoneList)[s];
            snprintf(sql, sizeof(sql), "INSERT INTO t_sms_timer_send_phones_%s("
                     "task_id, clientid, phone, status, submit_time"
                     ")VALUES("
                     "'%s', '%s', '%s', %u, '%s'"
                     ")",
                     strTblIdx.c_str(),
                     strTaskUuid.c_str(),
                     pSmsInfo->m_strClientId.c_str(),
                     phone.c_str(),
                     uPhoneState,
                     pSmsInfo->m_strDate.c_str()
                    );
            LogDebug("insert timer_send_task phones[task_id:'%s', smsid:'%s', phone:'%s'] DB sql[%s].",
                     strTaskUuid.c_str(),
                     pSmsInfo->m_strSmsId.c_str(),
                     phone.c_str(),
                     sql);

            TDBQueryReq *pPhoneDBReq = new TDBQueryReq;
            CHK_NULL_RETURN(pPhoneDBReq);

            pPhoneDBReq->m_SQL = sql;
            pPhoneDBReq->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
            pPhoneDBReq->m_pSender = this;
            pPhoneDBReq->m_strSessionID = strSessionPrefix + "_" + phone; // 1_taskuuid_phone: 插入手机号
            g_pDisPathDBThreadPool->PostMsg(pPhoneDBReq);
        }
    }

    return;
}

void CUnityThread::HandleDBResp(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);
    //if failed, should retry another product
    TDBQueryResp *pResp = (TDBQueryResp *)(pMsg);

    //check session type
    //LogDebug("DBResp sessionID: [%s]", pResp->m_strSessionID.c_str());

    if (pResp->m_strSessionID.find("TimerSendQuery") != std::string::npos)
    {
        HandleTimerSendTaskQueryDBResp(pResp);
        return;
    }
    if (pResp->m_strSessionID.find("TimerSendUpdate") != std::string::npos)
    {
        HandleTimerSendTaskUpdateDBResp(pResp);
        return;
    }

    LogWarn("Invalid sessionID:[%s]", pResp->m_strSessionID.c_str());
    return;
}

// 处理定时任务DB更新的返回
void CUnityThread::HandleTimerSendTaskUpdateDBResp(TMsg *pMsg)
{
    TDBQueryResp *pResp = (TDBQueryResp *)(pMsg);
    CHK_NULL_RETURN(pResp);

    const std::string &strSessionID = pResp->m_strSessionID;
    std::vector<std::string> vDBRespSession;
    Comm::splitExVectorSkipEmptyString(const_cast<std::string &>(strSessionID), "_", vDBRespSession);
    if (vDBRespSession.size() < 3)
    {
        LogWarn("Unknown DBResp SessionID:[%s]", strSessionID.c_str());
        return;
    }

    const std::string &strTaskUuid = vDBRespSession[1];
    const std::string &strIdentifer = vDBRespSession[2];

    bool bDBRetSucc = (pResp->m_iResult == 0 && pResp->m_iAffectRow >= 1);
    std::string strResultDesc =  bDBRetSucc ? "SUCC" : "FAILED";

    if (strIdentifer == "INIT")   // 插入定时任务, 任务初始化
    {
        LogNotice("TimerSend Task '%s' Insert [%s]!",
                  strTaskUuid.c_str(), strResultDesc.c_str());
    }
    else if (strIdentifer == "WAIT")   // 更新状态为'等待发送'
    {
        LogNotice("timersend-task[%s]'s state updated to 'WAIT' [%s]",
                  strTaskUuid.c_str(), strResultDesc.c_str());
    }
    else if ( strIdentifer == "SENDING")
    {
        LogNotice("timersend-task[%s]'s state updated to 'SENDING' [%s]",
                  strTaskUuid.c_str(), strResultDesc.c_str());

        if (!bDBRetSucc)
        {
            LogError("timersend-task[%s] updated to 'SENDING' failed. Cancel&clean it", strTaskUuid.c_str());
            CancelAndEreaseTask(strTaskUuid);
            return;
        }

        // 开始取号码处理
        FetchNextBatchTimesendPhonesFromDB(strTaskUuid);
    }
    else if ( strIdentifer == "SENT"
              || strIdentifer == "CANCEL" )
    {
        // 清除任务的工作由updateTimersendTaskInfo()处理
        LogNotice("timersend-task[%s]'s state updated to '%s' [%s].",
                  strTaskUuid.c_str(), strIdentifer.c_str(), strResultDesc.c_str());
    }
    else if (strIdentifer == "PID")  // Phone-ID,
    {
        // Format: TimerSendUpdate_981d322d-6fd6-43d5-b506-2fe457f8b902_PID_10
        if (vDBRespSession.size() < 4)
        {
            LogWarn("Invalid db reply session[%s]. '*_PID' needs '_' splited<4", strSessionID.c_str());
            return;
        }
        if (!bDBRetSucc)
        {
            LogError("timersend-task[%s]'s phone[%s]-state updated to 'handled' [FAILED]. !",
                     strTaskUuid.c_str(), vDBRespSession[3].c_str());
        }
        LogDebug("timersend-task[%s]'s phone[%s]-state updated to 'handled' [SUCC].",
                 strTaskUuid.c_str(), vDBRespSession[3].c_str());

        // 找到缓存中的任务
        std::map<std::string, timersend_taskinfo_ptr_t>::iterator taskCacheIter = m_mapTimersendTaskInfo.find(strTaskUuid);
        if (taskCacheIter == m_mapTimersendTaskInfo.end())
        {
            LogError("Not found timersend task[%s] in TaskInfoCache", strTaskUuid.c_str());
            return;
        }

        StTimerSendTaskInfo &stTaskInfo = (*taskCacheIter->second);
        ++stTaskInfo.uHandledPhonesNumInBatch;
        LogDebug("timersend-task [%s] handling-process-in-batch (%u/%u)",
                 strTaskUuid.c_str(), stTaskInfo.uHandledPhonesNumInBatch, stTaskInfo.uTotalPhonesNumInBatch);
        // 本批次的号码还未处理完
        if (stTaskInfo.uHandledPhonesNumInBatch < stTaskInfo.uTotalPhonesNumInBatch)
        {
            return;
        }

        // 本批次处理完了
        // 处理完后累加本批次取的总数
        stTaskInfo.uHandledPhonesNum += stTaskInfo.uTotalPhonesNumInBatch;
        LogNotice("This batch[%u] of timersend task [%s] finished. Total-Progress [%u/%u]",
                  stTaskInfo.uTotalPhonesNumInBatch,
                  strTaskUuid.c_str(),
                  stTaskInfo.uHandledPhonesNum,
                  stTaskInfo.uTotalPhonesNum);

        // 全部号码都处理完了，更新任务状态
        if (stTaskInfo.uTotalPhonesNum <= stTaskInfo.uHandledPhonesNum )
        {
            LogNotice("timersend-task [%s] handle done! Updating DB state to 'Sent'", strTaskUuid.c_str());

            UInt32 uNewState = TimerSendTaskState_Sent;
            stTaskInfo.uStatus = uNewState;
            // 更新状态
            // 1: 更新任务为已发送
            UpdateTimerSendTaskDBStateAndExtra(strTaskUuid, "SENT", uNewState);
        }
        else
        {
            // 否则未处理完，取下一批
            FetchNextBatchTimesendPhonesFromDB(strTaskUuid);
        }
    }
    else if (strIdentifer == "WP"      // 插入接收失败的号码的DB结果
             || strIdentifer == "CP")   // 插入接收成功的号码的DB结果
    {
        if (vDBRespSession.size() < 4)
        {
            LogWarn("Invalid timer-send-task phone insert DB reply sessionID:[%s]", strSessionID.c_str());
            return;
        }
        const std::string &strPhone = vDBRespSession[3];

        // 是未接收的手机号码插入成功，不用再处理，not care
        if (strIdentifer == "WP")
        {
            return;
        }

        // 插入号码进度缓存。 在所有接收成功的号码插入成功后,更新任务状态为'WaitSend'
        std::map<string, StProgress>::iterator proIter =  m_timerSndPhonesInsrtProgressCache. find(strTaskUuid);
        if (proIter == m_timerSndPhonesInsrtProgressCache.end())
        {
            LogWarn("Not found timer-send-phones-insert-progress-cache of task[%s] when DBResp", strTaskUuid.c_str());
            return;
        }

        if (!bDBRetSucc)
        {
            LogError("TimerSend Task [%s] insert phone '%s' Failed! Ignore this task, let it 'Handling'", strTaskUuid.c_str(), strPhone.c_str());
            m_timerSndPhonesInsrtProgressCache.erase(proIter);
            return;
        }

        if (++proIter->second.uCurIndx >= proIter->second.uTotal)
        {
            LogNotice("all phones of timer-send-task[%s] inserted! Sending update task's state to 'WaitingSend' to DB", strTaskUuid.c_str());

            // 更新任务表状态为发送中
            UpdateTimerSendTaskDBStateAndExtra(strTaskUuid, "WAIT", TimerSendTaskState_WaitingSend);
            // 信息收集齐了，清除相应缓存
            m_timerSndPhonesInsrtProgressCache.erase(proIter);
        }
    }
    else
    {
        LogWarn("Invalid Timertask reply DB reply sessionID:[%s] identifier:[%s]", strSessionID.c_str(), strIdentifer.c_str());
        return;
    }
}

// 对定时任务执行的DB操作响应
void CUnityThread::HandleTimerSendTaskQueryDBResp(TMsg *pMsg)
{
    TDBQueryResp *pResp = (TDBQueryResp *)(pMsg);
    CHK_NULL_RETURN(pResp);

    const std::string &strSessionID = pResp->m_strSessionID;
    RecordSet *pRs = NULL;

    do
    {
        if(0 != pResp->m_iResult)
        {
            LogError("session[%s] sql query error!", strSessionID.c_str());
            break;
        }

        // 从SessionID中取taskUuid
        std::vector<std::string> vDBRespSession;
        Comm::splitExVectorSkipEmptyString(const_cast<std::string &>(strSessionID), "_", vDBRespSession);
        if (vDBRespSession.size() < 3)
        {
            LogWarn("Invalid SessionID of DBResp:[%s]. '_' splited <3", strSessionID.c_str());
            break;
        }

        const std::string &strSessTaskUuid = vDBRespSession[1];
        const std::string &strIdentifer = vDBRespSession[2];

        pRs = pResp->m_recordset;

        if (strIdentifer == "NP")  // New-Phone
        {
            // FORMAT:[TimerSendQuery_981d322d-6fd6-43d5-b506-2fe457f8b902_NP]
            HandleNewFetchedPhones(strSessTaskUuid, pRs);
            break;
        }
        else
        {
            LogWarn("Invalid db reply session[%s], identifier[%s]", strSessionID.c_str(), strIdentifer.c_str());
            break;
        }

    }
    while(0);

    SAFE_DELETE(pRs);
}

void CUnityThread::HandleNewFetchedPhones(const std::string &strSessTaskUuid, RecordSet *pRs)
{
    CHK_NULL_RETURN(pRs);

    int iTotalFetched = pRs->GetRecordCount();
    LogDebug("timersend task fetch phones num[%d]", iTotalFetched);

    // 找到缓存中的任务，并预加载其它信息
    std::map<std::string, timersend_taskinfo_ptr_t>::iterator taskCacheIter = m_mapTimersendTaskInfo.find(strSessTaskUuid);
    if (taskCacheIter == m_mapTimersendTaskInfo.end())
    {
        LogError("Not found timersend task[%s] in TaskInfoCache", strSessTaskUuid.c_str());
        return;
    }

    StTimerSendTaskInfo &stTaskInfo = (*taskCacheIter->second);

    if (iTotalFetched <= 0)
    {
        LogError("timersend-task[%s] fetched 0 phones, process[%d/%d]. Something wrong. Check it", strSessTaskUuid.c_str(), stTaskInfo.uHandledPhonesNum, stTaskInfo.uTotalPhonesNum);
        if (stTaskInfo.uTotalPhonesNum <= stTaskInfo.uHandledPhonesNum)
        {
            LogError("timersend-task[%s] fetched 0 phones, but all sent, update state to 'SENT'", strSessTaskUuid.c_str());
            UInt32 uNewState = TimerSendTaskState_Sent;
            stTaskInfo.uStatus = uNewState;
            UpdateTimerSendTaskDBStateAndExtra(strSessTaskUuid.c_str(), "SENT", uNewState);
        }

        return;
    }

    // 取了新一批号码，已处理数置0，总数置成记录总数
    stTaskInfo.uTotalPhonesNumInBatch = iTotalFetched;
    stTaskInfo.uHandledPhonesNumInBatch = 0;

    if (!stTaskInfo.bSmsInfoLoaded
            && CheckAndLoadSmsInfoByTaskInfo(stTaskInfo))
    {
        return;
    }

    for (int i = 0; i < iTotalFetched; ++i)
    {
        std::string strID = (*pRs)[i]["id"];
        std::string strTaskUuid = (*pRs)[i]["task_id"];
        std::string strPhone = (*pRs)[i]["phone"];

        // 检查SessionID里的taskID和数据库中查出来的taskID是否一致
        if (strSessTaskUuid != strTaskUuid)
        {
            LogError("timersend phones query DB reply's session-taskUuid is [%s], "
                     "but phone's taskUuid from DB is[%s], NOT MATCHED!",
                     strSessTaskUuid.c_str(), strTaskUuid.c_str());
            continue;
        }

        std::vector<std::string> vPhoneList;
        vPhoneList.push_back(strPhone);

        LogNotice("Handling timersend-task[%s]'s phones-ID(%s)", strSessTaskUuid.c_str(), strID.c_str());

        // 把这批手机号打包处理发送到MQ
        PacketSmsAndSend2MQ(stTaskInfo, vPhoneList);

        LogNotice("phone-id[%s:%s] sent, sending update state to 'Handled' to DB",
                  strSessTaskUuid.c_str(), strID.c_str());
        UpdateTimerSendTaskPhonesDBStateAndExtra(strID, strSessTaskUuid, TimerSendPhoneState_Handled);
    }

}

bool CUnityThread::GetTimersendTaskPhonesTblIdx(const std::string &strTaskUuid, std::string &strTblIdx)
{
    std::map<std::string, timersend_taskinfo_ptr_t>::iterator taskCacheIter = m_mapTimersendTaskInfo.find(strTaskUuid);
    if (taskCacheIter == m_mapTimersendTaskInfo.end())
    {
        LogError("Not found timersend task[%s] in TaskInfoCache", strTaskUuid.c_str());
        return false;
    }

    strTblIdx = (*taskCacheIter->second).strPhonesTblIdx;

    return true;
}

void CUnityThread::UpdateTimerSendTaskPhonesDBStateAndExtra(const std::string &strID,
        const std::string &strTaskUuid,
        int iNewState,
        const std::string &strExtraSql)
{
    std::string strTblIdx = "";
    if (!GetTimersendTaskPhonesTblIdx(strTaskUuid, strTblIdx))
    {
        LogError("Update timer send task phones state failed!");
        return;
    }

    // 更新号码状态为已处理
    std::ostringstream oss;
    oss << "update t_sms_timer_send_phones_" << strTblIdx << " "
        << "set status = " << iNewState << " ";
    if (!strExtraSql.empty())
    {
        oss << "," << strExtraSql << " ";
    }
    oss << "where id = " << strID << " ";

    std::string strSql = oss.str();

    LogDebug("sql:[%s]", strSql.c_str());

    std::string strSessionID = "TimerSendUpdate_";
    strSessionID.append(strTaskUuid);
    strSessionID.append("_");
    strSessionID.append("PID");       // Phone-ID
    strSessionID.append("_");
    strSessionID.append(strID);

    TDBQueryReq *pUpdatePhoneStateMsg = new TDBQueryReq();
    CHK_NULL_RETURN(pUpdatePhoneStateMsg);

    pUpdatePhoneStateMsg->m_strSessionID = strSessionID;
    pUpdatePhoneStateMsg->m_pSender = this;
    pUpdatePhoneStateMsg->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    pUpdatePhoneStateMsg->m_SQL.assign(strSql);
    g_pDisPathDBThreadPool->PostMsg(pUpdatePhoneStateMsg);
}


void CUnityThread::FetchNextBatchTimesendPhonesFromDB(const std::string &strTaskUuid)
{

    LogNotice("Fetch next [%u] batch of phones from timersend task[%s]",
              m_uTimersendTaskPrefetchPhonesCnt, strTaskUuid.c_str());

    // 找到缓存中的任务，并预加载其它信息
    string strTblIdx = "";
    if (!GetTimersendTaskPhonesTblIdx(strTaskUuid, strTblIdx))
    {
        LogError("Fetch next batch of timersend phones failed, cause get phones db tbl index failed.");
        return;
    }
    char sql[512] = {0};
    snprintf(sql, sizeof(sql), "select * from t_sms_timer_send_phones_%s where task_id='%s' AND status=%d limit %d;",
            strTblIdx.c_str(),
            strTaskUuid.c_str(),
            TimerSendPhoneState_AcceptSucc,
            m_uTimersendTaskPrefetchPhonesCnt);

    TDBQueryReq *pMsg = new TDBQueryReq();
    CHK_NULL_RETURN(pMsg);

    pMsg->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
    pMsg->m_pSender = this;
    pMsg->m_strSessionID = "TimerSendQuery_" + strTaskUuid + "_NP"; // New-Phone
    pMsg->m_SQL.assign(sql);
    LogDebug("sql=%s", pMsg->m_SQL.data());
    g_pDisPathDBThreadPool->PostMsg(pMsg);
}

bool CUnityThread::PacketSmsAndSend2MQ(StTimerSendTaskInfo &stTaskInfo, const std::vector<std::string> &vPhoneList)
{
    SMSSmsInfo smsInfo;
    SMSSmsInfo *pSmsInfo = &smsInfo;

    pSmsInfo->m_Phonelist = vPhoneList;
    pSmsInfo->m_strClientId = stTaskInfo.strClientId;
    pSmsInfo->m_strDate = Comm::getCurrentTime_2();
    pSmsInfo->m_strExtpendPort = stTaskInfo.strExtendPort;
    pSmsInfo->m_uSmsFrom = stTaskInfo.uSmsFrom;
    pSmsInfo->m_uShortSmsCount = 1;
    pSmsInfo->m_uRecvCount = 1;
    pSmsInfo->m_uPkTotal = 1;
    pSmsInfo->m_uPkNumber = 1;
    pSmsInfo->m_strUid = stTaskInfo.strUid;
    pSmsInfo->m_strSmsId = stTaskInfo.strSmsId;
    pSmsInfo->m_strDeliveryUrl = stTaskInfo.strDeliveryUrl;
    pSmsInfo->m_strSmsType = Comm::int2str(stTaskInfo.uSmsType);
    pSmsInfo->m_strSrcPhone = stTaskInfo.strSrcPhone;
    pSmsInfo->m_uLongSms = stTaskInfo.uChargeNum > 1 ? 1 : 0;
    pSmsInfo->m_strSign = stTaskInfo.strSign;
    pSmsInfo->m_uShowSignType = stTaskInfo.uShowSignType;
    pSmsInfo->m_strTaskUuid = stTaskInfo.strTaskUuid;
    pSmsInfo->m_uSmsNum = stTaskInfo.uChargeNum;
    pSmsInfo->m_strUserName = stTaskInfo.strUserName;
    pSmsInfo->m_uIdentify = stTaskInfo.uIdentify;
    pSmsInfo->m_strTestChannelId = "";
    pSmsInfo->m_strTemplateId = "";
    pSmsInfo->m_strTemplateParam = "";
    pSmsInfo->m_strTemplateType = "-1";
    pSmsInfo->m_uBelong_sale = stTaskInfo.uBelong_sale;
    pSmsInfo->m_uOverRateCharge = stTaskInfo.uOverRateCharge;
    pSmsInfo->m_uPayType = stTaskInfo.uPayType;
    pSmsInfo->m_uNeedAudit = stTaskInfo.uNeedAudit;
    pSmsInfo->m_uNeedSignExtend = stTaskInfo.uNeedSignExtend;
    pSmsInfo->m_strSid = stTaskInfo.strSid;
    pSmsInfo->m_uNeedExtend = stTaskInfo.uNeedExtend;
    pSmsInfo->m_uAgentId = stTaskInfo.uAgentId;

    pSmsInfo->m_strSignPort.assign(stTaskInfo.strSignPort);

    pSmsInfo->m_strContent = stTaskInfo.strContent;

    string strLeft = "[";
    string strRight = "]";

    if (stTaskInfo.bIsChinese)
    {
        strLeft = http::UrlCode::UrlDecode("%e3%80%90");
        strRight = http::UrlCode::UrlDecode("%e3%80%91");
    }

    ShortSmsInfo shortInfo;
    shortInfo.m_uPkNumber = 1;
    shortInfo.m_uPkTotal = 1;

    // shortContent是包含签名的内容，重新组装好
    if (BusType::SHOWSIGNTYPE_PRO == stTaskInfo.uShowSignType)
    {
        shortInfo.m_strShortContent = strLeft + stTaskInfo.strSign + strRight + stTaskInfo.strContent;
    }
    else
    {
        shortInfo.m_strShortContent = stTaskInfo.strContent + strLeft + stTaskInfo.strSign + strRight;
    }

    UInt32 uPhoneCount = vPhoneList.size();
    for (UInt32 i = 0; i < uPhoneCount; ++i)
    {
        shortInfo.m_mapPhoneRefIDs[vPhoneList[i]] = getUUID();
    }
    pSmsInfo->m_vecShortInfo.push_back(shortInfo);

    // Insert access flow
    InsertAccessDb(pSmsInfo);

    proSetInfoToRedis(pSmsInfo);

    return true;
}

bool CUnityThread::CheckAndLoadSmsInfoByTaskInfo(StTimerSendTaskInfo &stTaskInfo)
{
    int iRet = 0;
    std::string strExtraSql = "remark='";
    do
    {
        map<string, SmsAccount>::iterator iter = m_AccountMap.find(stTaskInfo.strClientId);
        if(iter == m_AccountMap.end())
        {
            iRet = -1;
            LogError("[%s:%s:%s] can not find account in account map! ",
                     stTaskInfo.strClientId.c_str(), stTaskInfo.strSmsId.c_str(), stTaskInfo.strSrcPhone.c_str());
            strExtraSql.append("Account Not found!");
            break;
        }

        const SmsAccount &acnt = iter->second;
        if (BusType::CLIENT_STATUS_REGISTERED != acnt.m_uStatus)
        {
            LogError("[%s:%s:%s] Client status not 'register' [%d] when executing timersend task[%s]",
                     stTaskInfo.strClientId.c_str(), stTaskInfo.strSmsId.c_str(), stTaskInfo.strSrcPhone.c_str(),
                     acnt.m_uStatus, stTaskInfo.strTaskUuid.c_str());
            iRet = -2;
            switch(acnt.m_uStatus)
            {
            case BusType::CLIENT_STATUS_FREEZED:
                strExtraSql.append("Account Freezed");
                break;
            case BusType::CLIENT_STATUS_LOGOFF:
                strExtraSql.append("Account LogOff");
                break;
            case BusType::CLIENT_STATUS_LOCKED:
                strExtraSql.append("Account Locked");
                break;
            }
        }

        if (iRet)
        {
            break;
        }

        stTaskInfo.strUserName = acnt.m_strname;
        stTaskInfo.strDeliveryUrl = acnt.m_strDeliveryUrl;
        stTaskInfo.uPayType = acnt.m_uPayType;
        stTaskInfo.uIdentify = acnt.m_uIdentify;
        stTaskInfo.strSid = acnt.m_strSid;
        stTaskInfo.uNeedExtend = acnt.m_uNeedExtend;
        stTaskInfo.uAgentId = acnt.m_uAgentId;
        stTaskInfo.uNeedSignExtend = acnt.m_uNeedSignExtend;
        stTaskInfo.uNeedAudit = acnt.m_uNeedaudit;
        stTaskInfo.uOverRateCharge = acnt.m_uOverRateCharge;
        stTaskInfo.uBelong_sale = acnt.m_uBelong_sale;

        if (!stTaskInfo.strSign.empty()
                && 0 != acnt.m_uNeedSignExtend
                && !getSignPort(stTaskInfo.strClientId, stTaskInfo.strSign, stTaskInfo.strSignPort))
        {
            LogError("[%s:%s:%s] timersend task[%s] of client call getSignPort failed.",
                     stTaskInfo.strClientId.c_str(), stTaskInfo.strSmsId.c_str(), stTaskInfo.strSrcPhone.c_str(),
                     stTaskInfo.strTaskUuid.c_str());

            strExtraSql.append("getSignPort Failed");
            break;
        }
    }
    while(0);

    if (iRet != 0)
    {
        strExtraSql.append("'");
        UInt32 uNewState = TimerSendTaskState_Cancled;
        stTaskInfo.uStatus = uNewState;
        UpdateTimerSendTaskDBStateAndExtra(stTaskInfo.strTaskUuid, "CANCEL", uNewState, strExtraSql);
        return false;
    }
    stTaskInfo.bSmsInfoLoaded = true;

    return true;
}

bool CUnityThread::updateMQWeightGroupInfo(const map<string, ComponentRefMq> &componentRefMqInfo)
{
    m_mapMQWeightGroups.clear();

    for (map<string, ComponentRefMq>::iterator itr = m_componentRefMqInfo.begin(); itr != m_componentRefMqInfo.end(); ++itr)
    {
        const ComponentRefMq &componentRefMq = itr->second;
        // Only do weight choice when it's the producer of mq, and it's not DB message
        if (componentRefMq.m_uMode == PRODUCT_MODE
                && componentRefMq.m_strMessageType != MESSAGE_TYPE_DB
                && componentRefMq.m_uComponentId == g_uComponentId)
        {
            if (componentRefMq.m_uWeight == 0)
            {
                LogWarn("MQ ref weight of component[%ld]&message_type[%s]&mq[%u] is 0. Ignore it",
                        g_uComponentId, componentRefMq.m_strMessageType.c_str(), componentRefMq.m_uMqId);
                continue;
            }

            LogNotice("Add MQ ref weight of component[%ld]&message_type[%s] => mq[%u]:[%u]",
                      g_uComponentId, componentRefMq.m_strMessageType.c_str(),
                      componentRefMq.m_uMqId, componentRefMq.m_uWeight);

            mq_weightgroups_map_t::iterator wgIter = m_mapMQWeightGroups.find(componentRefMq.m_strMessageType);
            if (wgIter == m_mapMQWeightGroups.end())
            {
                MQWeightGroup mqWeightGroup(componentRefMq.m_strMessageType);
                mqWeightGroup.addMQWeightInfo(componentRefMq.m_uMqId, componentRefMq.m_uWeight);
                m_mapMQWeightGroups.insert(std::make_pair(componentRefMq.m_strMessageType, mqWeightGroup));
            }
            else
            {
                wgIter->second.addMQWeightInfo(componentRefMq.m_uMqId, componentRefMq.m_uWeight);
            }
        }
    }

    return true;
}

bool CUnityThread::updateTimersendTaskInfo(const std::map<std::string, timersend_taskinfo_ptr_t> &mapUpdateTimersendTaskInfo)
{
    std::map<std::string, timersend_taskinfo_ptr_t>::const_iterator cUpdateIter;
    std::map<std::string, timersend_taskinfo_ptr_t>::iterator cacheIter;

    /* 先处理取消的任务 */
    for(cacheIter = m_mapTimersendTaskInfo.begin();
            cacheIter != m_mapTimersendTaskInfo.end();
            /* No ++ here */)
    {
        cUpdateIter = mapUpdateTimersendTaskInfo.find(cacheIter->first);
        if (cUpdateIter == mapUpdateTimersendTaskInfo.end())
        {
            UInt32 uStatus = (*(cacheIter->second)).uStatus;
            if (uStatus == TimerSendTaskState_Cancled)
            {
                LogNotice("timersend task[%s] is canceled by component. Now cancel the running-task.", cacheIter->first.c_str());
            }
            else if (uStatus == TimerSendTaskState_Sent)
            {
                LogNotice("timersend task[%s] is sent. Now clear the task.", cacheIter->first.c_str());
            }
            else if (uStatus == TimerSendTaskState_WaitingSend)
            {
                LogNotice("timersend task[%s] is waitingsend. Maybe canceled by user, "
                          "or timerange of load shrinked. Clear it",
                          cacheIter->first.c_str());
            }
            else if (uStatus == TimerSendTaskState_Sending)
            {
                LogWarn("cache-timersend-task[%s] is Sending, but CRuleLoadThread not loads it. "
                        "Maybe it executing too long, and timerange-config like [-10min, 30min], "
                        "making it not-loaded. It's OK.",
                        cacheIter->first.c_str());
                cacheIter++;
                continue;
            }
            else
            {
                LogWarn("timersend task[%s]'s status[%u] not sent/cancel/waiting. Something wrong.",
                        cacheIter->first.c_str(), uStatus);
            }
            if (!(cacheIter->second->taskTimerPtr))
            {
                cacheIter->second->taskTimerPtr.reset();
            }

            m_mapTimersendTaskInfo.erase(cacheIter++);
        }
        else
        {
            cacheIter++;
        }
    }

    /* 处理新加的任务 */
    for (cUpdateIter = mapUpdateTimersendTaskInfo.begin();
            cUpdateIter != mapUpdateTimersendTaskInfo.end();
            ++cUpdateIter)
    {
        cacheIter = m_mapTimersendTaskInfo.find(cUpdateIter->first);
        // Not exist in cache, it must be a new task, add it and set timer
        if (cacheIter == m_mapTimersendTaskInfo.end())
        {
            handleNewTimersendTask(cUpdateIter->first, cUpdateIter->second);
        }
    }

    return true;
}

void CUnityThread::handleNewTimersendTask(const std::string &strTaskUuid, timersend_taskinfo_ptr_t pNewTaskPtr)
{
    CHK_NULL_RETURN(pNewTaskPtr);

    LogNotice("Add new timersend task:[%s]", strTaskUuid.c_str());
    m_mapTimersendTaskInfo.insert(std::make_pair(strTaskUuid, pNewTaskPtr));

    StTimerSendTaskInfo &taskInfo = (*pNewTaskPtr);

    time_t now_time = time(NULL);
    time_t send_time = Comm::asctime2seconds(taskInfo.strSetSendTime.c_str(), "%Y-%m-%d %H:%M:%S");
    // time elapsed from send_time ~~~~> now_time
    double interval = difftime(send_time, now_time);

    // 已经到了执行时间,直接执行
    if (interval <= 0)
    {
        LogNotice("new timersend task [%s] is over time with interval[%lf], being executed right now!",
                  strTaskUuid.c_str(),
                  interval);
        executeTimersendTask(strTaskUuid);
        return;
    }

    LogNotice("timersend task [%s] is installed, sending interval '%lfs'", strTaskUuid.c_str(), interval);
    taskInfo.taskTimerPtr.reset(SetTimer(TIMERSEND_TASK_INSTALL_TIMER_ID, strTaskUuid, interval * 1000));
}

void CUnityThread::UpdateTimerSendTaskDBStateAndExtra(const std::string &strTaskUuid,
        const std::string &strStateIdentifier,
        int iNewState,
        const std::string &strExtraSql)
{
    LogNotice("Update timersend task [%s] state to [%d], and extrasql[%s]", strTaskUuid.c_str(), iNewState, strExtraSql.c_str());
    std::ostringstream oss;
    oss << "update t_sms_timer_send_task set status = " << iNewState << " ";
    if (!strExtraSql.empty())
    {
        oss << "," << strExtraSql << " ";
    }
    oss << "where task_id = '" << strTaskUuid << "'";

    std::string strSql = oss.str();

    TDBQueryReq *pMsg = new TDBQueryReq();
    CHK_NULL_RETURN(pMsg);

    pMsg->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    pMsg->m_SQL.assign(strSql);
    pMsg->m_strSessionID = "TimerSendUpdate_" + strTaskUuid + "_" + strStateIdentifier;
    pMsg->m_pSender = this;
    g_pDisPathDBThreadPool->PostMsg(pMsg);
}

void CUnityThread::executeTimersendTask(const std::string &strTaskUuid)
{
    LogNotice("Task[%s] is executing....", strTaskUuid.c_str());

    std::map<std::string, timersend_taskinfo_ptr_t>::iterator taskInfoIter = m_mapTimersendTaskInfo.find(strTaskUuid);
    if (taskInfoIter == m_mapTimersendTaskInfo.end())
    {
        LogError("timer-task[%s] not found in taskInfoCache when exeucting it", strTaskUuid.c_str());
        return;
    }

    // S1: 预加载发送相关的其它数据
    StTimerSendTaskInfo &stTaskInfo = (*taskInfoIter->second);
    if (!stTaskInfo.bSmsInfoLoaded
            && false == CheckAndLoadSmsInfoByTaskInfo(stTaskInfo))
    {
        return;
    }

    // S2: 更新任务表状态为'发送中'
    if (stTaskInfo.uStatus != TimerSendTaskState_Sending)
    {
        std::string strExtraSql = "do_send_time='" + Comm::getCurrentTime() + "'";
        UInt32 uNewState = TimerSendTaskState_Sending;
        stTaskInfo.uStatus = uNewState;
        UpdateTimerSendTaskDBStateAndExtra(strTaskUuid, "SENDING", uNewState, strExtraSql);
    }
    else
    {
        LogNotice("timersend-task[%s]'s state is 'SENDING', no need to update", strTaskUuid.c_str());
        // 开始取号码处理
        FetchNextBatchTimesendPhonesFromDB(strTaskUuid);
    }
}

void CUnityThread::CancelAndEreaseTask(const std::string &strTaskUuid)
{
    std::map<std::string, timersend_taskinfo_ptr_t>::iterator cacheIter = m_mapTimersendTaskInfo.find(strTaskUuid);
    if (cacheIter == m_mapTimersendTaskInfo.end())
    {
        LogWarn("timersend task[%s] not exists when trying to cancel&erase it", strTaskUuid.c_str());
        return;
    }

    if (!(cacheIter->second->taskTimerPtr))
    {
        cacheIter->second->taskTimerPtr.reset();
    }
    m_mapTimersendTaskInfo.erase(cacheIter);
    LogNotice("timersend task[%s] canceled and erased", strTaskUuid.c_str());
}

void CUnityThread::generateSmsIdForEveryPhone(SMSSmsInfo *const pSmsInfo)
{
    CHK_NULL_RETURN(pSmsInfo);

    // for tencent cloud
    if (HttpProtocolType_TX_Json == pSmsInfo->m_ucHttpProtocolType)
    {
        pSmsInfo->m_mapPhoneSmsId.clear();

        for (string::size_type i = 0; i < pSmsInfo->m_Phonelist.size(); ++i)
        {
            pSmsInfo->m_mapPhoneSmsId[pSmsInfo->m_Phonelist[i]] = getUUID();
        }
    }
}

const string &CUnityThread::getSmsId(const SMSSmsInfo *const pSmsInfo, const string &strPhone)
{
    if (NULL == pSmsInfo)
    {
        LogError("pSmsInfo is NULL.");
        return INVALID_STR;
    }

    if (HttpProtocolType_TX_Json == pSmsInfo->m_ucHttpProtocolType)
    {
        // for tencent cloud
        map<string, string>::const_iterator iter = pSmsInfo->m_mapPhoneSmsId.find(strPhone);
        if (pSmsInfo->m_mapPhoneSmsId.end() != iter)
        {
            return iter->second;
        }
    }

    return pSmsInfo->m_strSmsId;
}

bool CUnityThread::checkAccount(const CHttpServerToUnityReqMsg *const pHttps,
                                const SMSSmsInfo *const pSmsInfo,
                                AccountMap::iterator &iterAccount)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    if(pHttps->m_strAccount.empty())
    {
        LogError("[%s:%s:%s] account is empty.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str());

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_1, CHINESE_1);
        return false;
    }

    bool AccountCheckRes = true;
    if(false == pHttps->m_strChannelId.empty())
    {
        set<string>::iterator iterTestAccount = m_TestAccount.find(pHttps->m_strAccount);
        if(iterTestAccount == m_TestAccount.end())
        {
            LogError("[%s:%s:%s] ChannelId[%s] is not find in Test accountMap.",
                     pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                     pHttps->m_strChannelId.c_str());

            AccountCheckRes = false;
        }
    }

    iterAccount = m_AccountMap.find(pHttps->m_strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogError("[%s:%s:%s] account is not find in accountMap.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str());

        AccountCheckRes = false;
    }

    if (false == AccountCheckRes)
    {
        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_1, CHINESE_3);
        return false;
    }

    if (1 != iterAccount->second.m_uStatus)
    {
        LogError("[%s:%s:%s] status[%d] is not ok.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 iterAccount->second.m_uStatus);

        // 1: Account registration completed
        // 5: Account is frozen
        // 6: Account is canceled
        // 7: Account is locked

        int iErrorCode;
        string strError;

        if(iterAccount->second.m_uStatus == 5)
        {
            iErrorCode = HTTPS_RESPONSE_CODE_22;
            strError = CHINESE_4;
        }
        else if(iterAccount->second.m_uStatus == 6)
        {
            iErrorCode = HTTPS_RESPONSE_CODE_3;
            strError = CHINESE_3;
        }
        else if(iterAccount->second.m_uStatus == 7)
        {
            iErrorCode = HTTPS_RESPONSE_CODE_4;
            strError = ACCOUNT_LOCKED;
        }
        else
        {
            LogError("[%s:%s:%s] Invalid status[%d] config.",
                     pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                     iterAccount->second.m_uStatus);

            iErrorCode = HTTPS_RESPONSE_CODE_4;
            strError = ACCOUNT_LOCKED;
        }

        rsponseToClient(pHttps, pSmsInfo, iErrorCode, strError);
        return false;
    }

    return true;
}

bool CUnityThread::checkAccountIp(const CHttpServerToUnityReqMsg *const pHttps,
                                  const SMSSmsInfo *const pSmsInfo,
                                  const AccountMap::iterator &iterAccount)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    if (false == iterAccount->second.m_bAllIpAllow)
    {
        bool IsIPInWhiteList = false;

        for(list<string>::iterator it = iterAccount->second.m_IPWhiteList.begin();
                it != iterAccount->second.m_IPWhiteList.end(); it++)
        {
            if(string::npos != pHttps->m_strIp.find((*it), 0))
            {
                IsIPInWhiteList = true;
                break;
            }
        }

        if(false == IsIPInWhiteList)
        {
            LogError("[%s:%s:%s] IP[%s] is not in whitelist.",
                     pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                     pHttps->m_strIp.c_str());

            rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_5, CHINESE_5);
            return false;
        }
    }

    LogDebug("[%s:%s:%s] allowed all IP[%s].",
             pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
             pHttps->m_strIp.c_str());

    return true;
}

bool CUnityThread::checkAccountPwd(const CHttpServerToUnityReqMsg *const pHttps,
                                   const SMSSmsInfo *const pSmsInfo,
                                   const AccountMap::iterator &iterAccount)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    if ((pHttps->m_strPwdMd5.empty())
            || (pHttps->m_strPwdMd5.length() != 32))
    {
        LogError("[%s:%s:%s] Pwdmd5[%s] is empty or length is not 32.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 pHttps->m_strPwdMd5.c_str());

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_1, CHINESE_1);
        return false;
    }

    string strMd5Db;
    unsigned char md5[16] = { 0 };
    MD5((const unsigned char *) iterAccount->second.m_strPWD.data(),
        iterAccount->second.m_strPWD.length(), md5);

    std::string HEX_CHARS = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        strMd5Db.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
        strMd5Db.append(1, HEX_CHARS.at(md5[i] & 0x0F));
    }

    string strLowerPwdMd5 = pHttps->m_strPwdMd5;
    for (std::string::iterator itor = strLowerPwdMd5.begin();
            itor != strLowerPwdMd5.end(); ++itor)
    {
        *itor = tolower(*itor);
    }

    if (0 != memcmp(strMd5Db.data(), strLowerPwdMd5.data(), 32))
    {
        LogError("[%s:%s:%s] pwd Md5[%s] is not match with Md5Db[%s].",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 pHttps->m_strPwdMd5.c_str(), strMd5Db.data());

        iterAccount->second.m_uLoginErrorCount++;
        if(iterAccount->second.m_uLoginErrorCount >= 5)
        {
            string reason = http::UrlCode::UrlDecode("%e9%89%b4%e6%9d%83%e5%a4%b1%e8%b4%a5%ef%bc%88%e8%b4%a6%e5%8f%b7%e6%88%96%e5%af%86%e7%a0%81%e9%94%99%e8%af%af%ef%bc%89");
            AccountLock(iterAccount->second, reason.c_str());
        }

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_1, CLIENTID_IS_INVALID);
        return false;
    }

    return true;
}

bool CUnityThread::checkProtocolType(const CHttpServerToUnityReqMsg *const pHttps,
                                     SMSSmsInfo *const pSmsInfo,
                                     const AccountMap::iterator &iterAccount)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    // check smsfrom
    if (SMS_FROM_ACCESS_HTTP != iterAccount->second.m_uSmsFrom)
    {
        LogError("[%s:%s:%s] config smsfrom[%d] and login smsfrom[%d] do not match.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 iterAccount->second.m_uSmsFrom, SMS_FROM_ACCESS_HTTP);

        iterAccount->second.m_uLoginErrorCount++;
        if(iterAccount->second.m_uLoginErrorCount >= 5)
        {
            string reason = http::UrlCode::UrlDecode("%e5%8d%8f%e8%ae%ae%e7%b1%bb%e5%9e%8b%e4%b8%8d%e5%8c%b9%e9%85%8d");
            AccountLock(iterAccount->second, reason.c_str());
        }

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_10, SMSFROM_IS_NOT_MATCH);
        return false;
    }

    // check http_protocol_type
    if (pHttps->m_ucHttpProtocolType != HttpProtocolType_ML)
    {
        if (pHttps->m_ucHttpProtocolType != iterAccount->second.m_ucHttpProtocolType)
        {
            LogError("[%s:%s:%s] The HttpProtocolType(%u) of the account is not match http request(%u).",
                     pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                     iterAccount->second.m_ucHttpProtocolType, pHttps->m_ucHttpProtocolType);

            rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_10, SMSFROM_IS_NOT_MATCH);
            return false;
        }
    }

    if (HttpProtocolType_TX_Json == pHttps->m_ucHttpProtocolType)
    {
        if (HttpProtocolType_TX_Json != iterAccount->second.m_ucHttpProtocolType)
        {
            LogError("[%s:%s:%s] The HttpProtocolType(%u) of is not match http request(%u).",
                     pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                     iterAccount->second.m_ucHttpProtocolType, pHttps->m_ucHttpProtocolType);

            rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_10, SMSFROM_IS_NOT_MATCH);
            return false;
        }

        pSmsInfo->m_strSmsType = iterAccount->second.m_strSmsType;
    }

    return true;
}

bool CUnityThread::checkContent(const CHttpServerToUnityReqMsg *const pHttps,
                                SMSSmsInfo *const pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    if (pHttps->m_strContent.empty())
    {
        LogError("[%s:%s:%s] content[%s] is empty.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 pHttps->m_strContent.c_str());

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_24, CONTENT_EMPTY);
        return false;
    }

    utils::UTFString utfHelper;
    if (utfHelper.getUtf8Length(pHttps->m_strContent) > 610)
    {
        LogError("[%s:%s:%s] content[%s] length[%d] is over 610.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 pHttps->m_strContent.data(), pHttps->m_strContent.length());

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_8, CHINESE_2);
        return false;
    }

    char strDst[64] = {0};
    if (!utfHelper.IsTextUTF8(pSmsInfo->m_strContent.c_str(), pSmsInfo->m_strContent.length(), strDst))
    {
        LogError("[%s:%s:%s] content[%x,%x,%x,%x,%x,%x] is over utf8.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 strDst[0], strDst[1], strDst[2], strDst[3], strDst[4], strDst[5]);

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_31, ERRORCODE_CONTENT_ENCODE_ERROR);
        return false;
    }

    // check if short message or long message
    int iLength = utfHelper.getUtf8Length(pSmsInfo->m_strContent);
    pSmsInfo->m_uSmsNum = ( iLength <= 70 ? 1 : ((iLength + 66) / 67));

    return true;
}

bool CUnityThread::checkSignExtend(const CHttpServerToUnityReqMsg *const pHttps,
                                   SMSSmsInfo *const pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    string strSign = "";
    string strContent = "";
    string strExtpendPort = "";
    string strSignPort = "";
    UInt32 uShowSignType = 0;
    UInt32 uChina = 0;    // 0 english,  1 china

    int ret = -1;
    if (HttpProtocolType_JD_Webservice != pSmsInfo->m_ucHttpProtocolType)
    {
        ret = ExtractSignAndContent(pSmsInfo->m_strContent, strContent, strSign, uChina, uShowSignType);
    }

    if (0 != ret)
    {
        if ( !getContentWithSign( pSmsInfo, strSign, uChina ))
        {
            LogError("[%s:%s:%s] Call ExtractSignAndContent failed.",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str());

            rsponseToClient(pHttps, pSmsInfo, ret, CHINESE_2);
            return false;
        }
    }

    if (0 == pSmsInfo->m_uNeedExtend)
    {
        strExtpendPort = "";
    }
    else
    {
        strExtpendPort = pSmsInfo->m_strExtpendPort;
    }

    if (false == strSign.empty())
    {
        //// not support sign port
        if (0 == pSmsInfo->m_uNeedSignExtend)
        {
            ;
        }
        else
        {
            if (false == getSignPort(pSmsInfo->m_strClientId, strSign, strSignPort))
            {
                LogError("[%s:%s:%s] Call getSignPort failed.",
                    pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str());

                rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_9, SIGN_NOT_REGISTER);
                return false;
            }
        }
    }
    else
    {
        LogError("[%s:%s:%s] content[%s] is not contain sign.",
            pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(), 
            pSmsInfo->m_strContent.data());

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_26, CHINESE_1);
        return false;
    }

    pSmsInfo->m_strIsChina.assign(Comm::int2str(uChina));
    pSmsInfo->m_strSign.assign(strSign);
    pSmsInfo->m_strExtpendPort.assign(strExtpendPort);
    pSmsInfo->m_uShowSignType = uShowSignType;
    pSmsInfo->m_strSignPort.assign(strSignPort);

    if (!strContent.empty())
    {
        pSmsInfo->m_strContent.assign(strContent);
    }

    return true;
}

bool CUnityThread::checkPhoneList(const CHttpServerToUnityReqMsg *const pHttps,
                                  const SMSSmsInfo *const pSmsInfo)
{
    CHK_NULL_RETURN_FALSE(pHttps);
    CHK_NULL_RETURN_FALSE(pSmsInfo);

    std::size_t maxPhoneListSize = 1000;

    if (HttpProtocolType_TX_Json == pHttps->m_ucHttpProtocolType)
    {
        maxPhoneListSize = 200;
    }

    if (pHttps->m_bTimerSend)
    {
        maxPhoneListSize = 10 * ONE_WAN;
    }

    size_t llSize = pSmsInfo->m_Phonelist.size();
    if (llSize > maxPhoneListSize)
    {
        LogError("[%s:%s:%s] The number of phones [%lu] exceeds [%u].",
                 pHttps->m_strAccount.c_str(), pSmsInfo->m_strSmsId.c_str(), pHttps->m_strPhone.c_str(),
                 llSize, maxPhoneListSize);

        rsponseToClient(pHttps, pSmsInfo, HTTPS_RESPONSE_CODE_25, PHONE_COUNT_TOO_MANY);
        return false;
    }

    return true;
}

void CUnityThread::rsponseToClient(const CHttpServerToUnityReqMsg *const pHttps,
                                   const SMSSmsInfo *const pSmsInfo,
                                   const int iErrorCode,
                                   const string &strError)
{
    CHK_NULL_RETURN(pHttps);
    CHK_NULL_RETURN(pSmsInfo);
    CHK_NULL_RETURN(g_pHttpServiceThread);

    CUnityToHttpServerRespMsg *pResp = new CUnityToHttpServerRespMsg();
    CHK_NULL_RETURN(pResp);

    pResp->m_strSmsid = pSmsInfo->m_strSmsId;
    pResp->m_strError = strError;
    pResp->m_iErrorCode = iErrorCode;
    pResp->m_uSessionId = pHttps->m_uSessionId;
    pResp->m_vecParamErrorPhone = pSmsInfo->m_Phonelist;
    pResp->m_mapPhoneSmsId = pSmsInfo->m_mapPhoneSmsId;
    pResp->m_iMsgType = MSGTYPE_UNITY_TO_HTTPS_RESP;
    pResp->m_bTimerSend = pHttps->m_bTimerSend;
    g_pHttpServiceThread->PostMsg(pResp);
}

