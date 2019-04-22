#include "CDeliveryReportThread.h"
#include "main.h"
#include "Comm.h"
#include "alarm.h"
#include "base64.h"
#include "HttpParams.h"
CDeliveryReportThread::CDeliveryReportThread(const char *name) : CThread(name)
{
    // CHANNEL_REPORT_FAIL  default: 100;10
    m_uReportFailedAlarmValue = 100;
    m_uReportFailedSendTimeInterval = 10 * 60;

    m_bBlankPhoneSwitch = 0;
    m_strBlankPhoneErr = BLANK_PHONE_ERROR;
    m_strBlankPhoneErr.append("|");
}

CDeliveryReportThread::~CDeliveryReportThread()
{

}

bool CDeliveryReportThread::Init()
{
    if (false == CThread::Init())
    {
        printf("CThread::Init is failed.");
        return false;
    }

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        printf("m_pInternalService is NULL.");
        return false;
    }
    m_pInternalService->Init();

    m_pLinkedBlockPool = new LinkedBlockPool();

    g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
    g_pRuleLoadThread->getComponentConfig(m_mapComponentConfig);
    m_accountMap.clear();
    g_pRuleLoadThread->getAccountMap(m_accountMap);
    m_ChannelMap.clear();
    g_pRuleLoadThread->getChannlelMap( m_ChannelMap );
    m_componentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);
    map<string,string> mapSysParam;
    g_pRuleLoadThread->getSysParamMap(mapSysParam);
    GetSysPara(mapSysParam);
    return true;
}
void CDeliveryReportThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "CHANNEL_REPORT_FAIL";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        Comm comm;
        std::vector<string> paramslist;
        comm.splitExVector(iter->second, ";", paramslist);
        if (paramslist.size() != 3)
        {
             LogError("Invalid System parameter(%s,%s) final value(%u, %u, %u seconds).",
                strSysPara.c_str(),
                iter->second.c_str(),
                m_uReportFailedAlarmSwitch,
                m_uReportFailedAlarmValue,
                m_uReportFailedSendTimeInterval);
            return;
        }

        m_uReportFailedAlarmSwitch = atoi(paramslist[0].data());
        m_uReportFailedAlarmValue = atoi(paramslist[1].data());
        int iIntervalMinute = atoi(paramslist[2].data());
        m_uReportFailedSendTimeInterval = iIntervalMinute * 60;

        if ((0 > m_uReportFailedAlarmValue) || (700 < m_uReportFailedAlarmValue)

|| (0 > iIntervalMinute) || (24 * 60 * 60 < iIntervalMinute))
        {
            LogError("Invalid system parameter(%s) value(%s, %d, %d).",
                strSysPara.c_str(),
                iter->second.c_str(),
                m_uReportFailedAlarmValue,
                iIntervalMinute);
            m_uReportFailedAlarmValue = 200;//default
            m_uReportFailedSendTimeInterval = 60;//default
            break;
        }

        if (m_uReportFailedAlarmSwitch != 0 && m_uReportFailedAlarmSwitch != 1)
        {
             LogError("Invalid system parameter(%s) value(%s, %d), final the switch is 0",
                strSysPara.c_str(),
                iter->second.c_str(),
                m_uReportFailedAlarmSwitch);
             m_uReportFailedAlarmSwitch = 0;
            break;
        }
    }
    while (0);

    LogNotice("System parameter(%s) final value(%u, %u, %u seconds). ",
        strSysPara.c_str(),
        m_uReportFailedAlarmSwitch,
        m_uReportFailedAlarmValue,
        m_uReportFailedSendTimeInterval);


    do
    {
        strSysPara = "FAILED_RESEND_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        std::string strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }

        int index = -1;
        if(-1 == (index = strTmp.find("|")))
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
            break;
        }
        m_bResendSysSwitch = (1 == atoi(strTmp.substr(0,index).c_str()));
        string strSysCfg = strTmp.substr(index+1);
        vector<std::string> vecCfg;
        Comm::split(strSysCfg, ";", vecCfg);
        //m_FailedResendSysCfgMap.clear();
        for(UInt32 i = 0; i < vecCfg.size(); i++)
        {

            vector<string> vecData;
            Comm::split(vecCfg[i], ",", vecData);

            if(3 != vecData.size())
                continue;

            FailedResendSysCfg cfginfo;
            Int32 smstype = atoi(vecData[0].data());
            Int32 MaxTimes = atoi(vecData[1].data());
            Int32 MaxDuration = atoi(vecData[2].data());
            if(smstype < 0 ||  MaxTimes < 0 || MaxTimes > 50 || MaxDuration < 1 || MaxDuration > 86400)
            {
                LogWarn("FAILED_RESEND_CFG Invalid value: smstype[%d],MaxTimes[%d],MaxDuration[%d]",smstype,MaxTimes,MaxDuration);
                continue;
            }
            cfginfo.m_uResendMaxTimes = MaxTimes;
            cfginfo.m_uResendMaxDuration = MaxDuration;
            m_FailedResendSysCfgMap[(UInt32)smstype] = cfginfo;
            LogNotice("smsytpe[%d],ResendMaxTimes[%d],ResendMaxDuration[%d]",smstype,MaxTimes,MaxDuration);
        }
    }
    while (0);
    LogNotice("System parameter(%s) value m_bResendSysSwitch[%d],m_FailedResendSysCfgMap size[%d]",
        strSysPara.c_str(), m_bResendSysSwitch ? 1:0 ,m_FailedResendSysCfgMap.size());

    do
    {
        strSysPara = "BLANK_PHONE_CFG";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        std::string strTmp = iter->second;
        if (strTmp.empty())
        {
            LogError("system parameter(%s) is empty, pls reconfig.", strSysPara.c_str());
            break;
        }

        int index = -1;
        if(-1 == (index = strTmp.find(";")))
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
            break;
        }

        vector<std::string> vecCfg;
        Comm::split(strTmp, ";", vecCfg);
        if (vecCfg.size() < 3)
        {
            LogError("system parameter(%s) cfg(%s)format error",strSysPara.c_str(),strTmp.c_str());
        }
        else
        {
            m_bBlankPhoneSwitch = (1 == atoi(vecCfg[1].c_str()));
            if (vecCfg[2].length() >= 8)
                m_strBlankPhoneErr = vecCfg[2];
        }

    }
    while (0);
    LogNotice("System parameter(%s) value m_bBlankPhoneSwitch[%d],m_strBlankPhoneErr[%s]",
        strSysPara.c_str(), m_bBlankPhoneSwitch ? 1:0 ,m_strBlankPhoneErr.c_str());

}

void CDeliveryReportThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }
        else if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void CDeliveryReportThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_SEND_TO_DELIVERYREPORT_REQ:
        {
            HandleSendToDeliveryReqMsg(pMsg);
            break;
        }
        case MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ:
        {
            HandleDirectToDeliveryReqMsg(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisResp(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_MQ_CONFIG_UPDATE_REQ:
        {
            TUpdateMqConfigReq* pMqCon = (TUpdateMqConfigReq*)pMsg;
            m_mapMqConfig.clear();

            m_mapMqConfig = pMqCon->m_mapMqConfig;

            LogNotice("update t_sms_mq_configure size:%d.",m_mapMqConfig.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENT_UPDATE_REQ:
        {
            TUpdateComponentConfigReq* pMqCom = (TUpdateComponentConfigReq*)pMsg;
            m_mapComponentConfig.clear();

            m_mapComponentConfig = pMqCom->m_mapComponentConfigInfo;

            LogNotice("update t_sms_component_configure size:%d.",m_mapComponentConfig.size());
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            if (NULL == pSysParamReq)
            {
                break;
            }

            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        /* 用户账号变更*/
        case MSGTYPE_RULELOAD_USERGW_UPDATE_REQ:
        {
            TUpdateAccountReq* pAccount = (TUpdateAccountReq*)pMsg;
            m_accountMap.clear();
            m_accountMap = pAccount->m_accountMap;
            LogNotice("update t_sms_account size:%d.",m_accountMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            TUpdateChannelReq *pChannelMsg = (TUpdateChannelReq *)pMsg;
            m_ChannelMap.clear();
            m_ChannelMap = pChannelMsg->m_ChannelMap;
            LogNotice("update t_sms_channel size:%d.",m_accountMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_COMPONENTREFMQ_UPDATE_REQ:
        {
            TUpdateComponentRefMqConfigReq* pMqCom = (TUpdateComponentRefMqConfigReq*)pMsg;
            m_componentRefMqInfo.clear();
            m_componentRefMqInfo = pMqCom->m_componentRefMqInfo;
            LogNotice("update t_sms_component_ref_mq size:%d.",m_componentRefMqInfo.size());
            break;
        }
        case MSGTYPE_DB_NOTQUERY_RESP:
        {
            handleDBResponseMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOutMsg(pMsg->m_iSeq);
            break;
        }
        default:
        {
            LogWarn("msgType[%d] is invalid.",pMsg->m_iMsgType);
            break;
        }

    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeliveryReportThread::HandleRedisRespGetKey(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }

    DeliveryReportSessionMap::iterator itr = m_mapSession.find(iSeq);
    if(itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", iSeq)
        return;
    }

    string strKeys = "";
    strKeys = pRedisReply->element[0]->str;
    LogDebug("***getKey[%s],smsId[%s],channelSmsId[%s].",
        strKeys.data(),itr->second->m_strSmsId.data(),itr->second->m_strChannelSmsId.data());

    std::string::size_type pos = strKeys.find_last_of("_");
    if (pos != std::string::npos)
    {
        itr->second->m_strPhone = strKeys.substr(pos+1);
    }

    TRedisReq* pRedis = new TRedisReq();
    string strRedis = "";
    strRedis.append("HGETALL  ");
    strRedis.append(strKeys);

    LogDebug("redis cmd[%s], length[%d].",strRedis.data(),strRedis.length());
    pRedis->m_RedisCmd = strRedis;
    pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedis->m_iSeq = iSeq;
    pRedis->m_pSender = this;
    pRedis->m_strSessionID = "getReportInfo";
    pRedis->m_strKey = strKeys;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
    return;
}

void CDeliveryReportThread::HandleRedisRespGetReportInfo(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    bool bResend = false;//false : not resend ,ture: resend not return report to client
    DeliveryReportSessionMap::iterator itr = m_mapSession.find(iSeq);
    if(itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", iSeq)
        return;
    }

    string strTmp = "";
    string strSendtime = "";
    string strSmsId = "";
    string strClientId = "";
    UInt32 uSmsFrom = 0;
    string strSendUuid = "";
    UInt32 uLongSms = 0;
    string strPhone = "";
    UInt64 uC2sId = 0;
    UInt32 uChannelCnt = 0;
    UInt64 uSubmitDate = 0;
    string strKey = "";
    ////sendTime
    paserReply("sendTime", pRedisReply, strSendtime);
    strSendtime = http::UrlCode::UrlDecode(strSendtime);
    ////smsid
    paserReply("smsid", pRedisReply, strSmsId);
    ////clientid
    paserReply("clientid", pRedisReply, strClientId);
    ////smsfrom
    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());
    strTmp = "";
    ////senduid
    paserReply("senduid", pRedisReply, strSendUuid);
    ////longsms
    paserReply("longsms", pRedisReply, strTmp);
    uLongSms = atoi(strTmp.data());
    strTmp = "";
    ////phone
    paserReply("phone", pRedisReply, strPhone);
    ////c2sid
    paserReply("c2sid", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());
    strTmp = "";
    ////channelcnt
    paserReply("channelcnt", pRedisReply, strTmp);
    uChannelCnt = atoi(strTmp.data());
    strTmp = "";
    ////submitdate
    paserReply("submitdate", pRedisReply, strTmp);
    uSubmitDate = atol(strTmp.data());

    itr->second->m_strSmsId.assign(strSmsId);
    itr->second->m_strClientId.assign(strClientId);
    itr->second->m_uSmsFrom = uSmsFrom;
    itr->second->m_strPhone.assign(strPhone);
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uChannelCnt = uChannelCnt;
    itr->second->m_strType = "1";
    itr->second->m_uLongSms = (uLongSms > 1) ? 1 : 0;

    if(Int32(SMS_STATUS_CONFIRM_SUCCESS) != itr->second->m_iStatus)
    {
        bResend = CheckFailedResend(pRedisReply,itr->second);
    }
    ////update database

    UpdateRecord(strSendUuid,strSendtime,uSubmitDate,itr->second);

    bool result = false;
    if(itr->second->m_iStatus == Int32(SMS_STATUS_CONFIRM_SUCCESS))
    {
        result = true;
    }

    if (m_uReportFailedAlarmSwitch)
    {
        char szreachtime[64] = {0};
        UInt64 time_now = time(NULL);
        struct tm Reportptm = {0};
        localtime_r((time_t*)&time_now,&Reportptm);
        strftime(szreachtime, sizeof(szreachtime), "%Y%m%d%H%M%S", &Reportptm);
        string to_day ;
        to_day.assign(szreachtime,8);
        string SendDay = strSendtime.substr(0,8);

        if(SendDay.compare(to_day) == 0 && time_now - uSubmitDate <= m_uReportFailedSendTimeInterval) ///if this message is sended at today,&& report - send <= interval should count the faild time
        {
            countReportFailed(itr->second->m_uChannelId, itr->second->m_industrytype, result,to_day, time_now);
        }
        else
        {
            LogDebug("to_day = %s, channelid = %u, time_now = %lu, uSubmitDate = %lu", to_day.data()
                , itr->second->m_uChannelId, time_now, uSubmitDate);
        }

    }

    ////Del redis
    TRedisReq* pDel = new TRedisReq();
    string strDel = "";
    char cChannelId[25] = {0};
    snprintf(cChannelId,25,"%u",itr->second->m_uChannelId);
    strDel.append("sendmsgid:");
    strDel.append(cChannelId);
    strDel.append("_");
    strDel.append(itr->second->m_strChannelSmsId);

    pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDel->m_RedisCmd.assign("DEL  ");
    pDel->m_RedisCmd.append(strDel);
    pDel->m_strKey = strDel;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);

    if (uLongSms > 1)  ////access long msg
    {
        TRedisReq* pRedis = new TRedisReq();
        pRedis->m_strKey = "sendsms:"+strSmsId+"_"+strPhone;
        strKey = pRedis->m_strKey;
        pRedis->m_RedisCmd = "GET "+ pRedis->m_strKey;
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_iSeq = iSeq;
        pRedis->m_pSender = this;
        pRedis->m_strSessionID = "getStatus";
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

        TRedisReq* pDel = new TRedisReq();
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = strKey;
        pDel->m_RedisCmd = "DEL "+pDel->m_strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);
    }
    else
    {
        string innerError = itr->second->m_strInnerErrorcode + "|";
        if (itr->second->m_iStatus == Int32(SMS_STATUS_CONFIRM_FAIL) && m_bBlankPhoneSwitch
            && (m_strBlankPhoneErr.find(innerError) != string::npos)
            && !itr->second->m_strPhone.empty())
        {
            TRedisReq* pRedis = new TRedisReq();
            string strKey = "black_list:" + itr->second->m_strPhone;
            pRedis->m_strSessionID = "check blankphone";
            pRedis->m_iSeq = iSeq;
            pRedis->m_RedisCmd = "GET " + strKey;
            pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRedis->m_pSender = this;
            pRedis->m_strKey = strKey;
            pRedis->m_string = itr->second->m_strPhone + "&" + itr->second->m_strInnerErrorcode;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

        }

        if (false == bResend)
        {
            HandleRedisComplete(iSeq);
        }
        else
        {
             if (NULL != itr->second->m_pWheelTime)
            {
                delete itr->second->m_pWheelTime;
                itr->second->m_pWheelTime = NULL;
            }
            delete itr->second;
            m_mapSession.erase(itr);
        }
    }

    return;
}

void CDeliveryReportThread::HandleRedisRespGetStatus(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }

    DeliveryReportSessionMap::iterator itr = m_mapSession.find(iSeq);
    if(itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", iSeq)
        return;
    }
    string innerError = itr->second->m_strInnerErrorcode + "|";
    if (itr->second->m_iStatus == Int32(SMS_STATUS_CONFIRM_FAIL) && m_bBlankPhoneSwitch
        && (m_strBlankPhoneErr.find(innerError) != string::npos)
        && !itr->second->m_strPhone.empty())
    {
        TRedisReq* pRedis = new TRedisReq();
        string strKey = "black_list:" + itr->second->m_strPhone;
        pRedis->m_strSessionID = "check blankphone";
        pRedis->m_iSeq = iSeq;
        pRedis->m_RedisCmd = "GET " + strKey;
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_pSender = this;
        pRedis->m_strKey = strKey;
        pRedis->m_string = itr->second->m_strPhone + "&" + itr->second->m_strInnerErrorcode;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
    }
    HandleRedisComplete(iSeq);
    return;
}

void CDeliveryReportThread::HandleRedisComplete(UInt64 iSeq)
{
    DeliveryReportSessionMap::iterator itr = m_mapSession.find(iSeq);
    if(itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", iSeq)
        return;
    }

    map<UInt64,ComponentConfig>::iterator itrCom = m_mapComponentConfig.find(itr->second->m_uC2sId);
    if (itrCom == m_mapComponentConfig.end())
    {
        LogError("==direct report==[%s:%s]c2sid:%lu is not find in m_mapComponentConfig.",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itr->second->m_uC2sId);

        if (NULL != itr->second->m_pWheelTime)
        {
            delete itr->second->m_pWheelTime;
            itr->second->m_pWheelTime = NULL;
        }
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    map<UInt64,MqConfig>::iterator itrMq =  m_mapMqConfig.find(itrCom->second.m_uMqId);
    if (itrMq == m_mapMqConfig.end())
    {
        LogError("==direct report==[%s:%s],mqid:%lu is not find in m_mapMqConfig.",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itrCom->second.m_uMqId);

        if (NULL != itr->second->m_pWheelTime)
        {
            delete itr->second->m_pWheelTime;
            itr->second->m_pWheelTime = NULL;
        }
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    string strExchange = itrMq->second.m_strExchange;
    string strRoutingKey = itrMq->second.m_strRoutingKey;

    string strData = "";

    ////type
    strData.append("type=");
    strData.append("1");
    ////clientid
    strData.append("&clientid=");
    strData.append(itr->second->m_strClientId);
    ////channelid
    strData.append("&channelid=");
    strData.append(Comm::int2str(itr->second->m_uChannelId));
    ////smsid
    strData.append("&smsid=");
    strData.append(itr->second->m_strSmsId);
    ////phone
    strData.append("&phone=");
    strData.append(itr->second->m_strPhone);
    ////status
    strData.append("&status=");
    strData.append(Comm::int2str(itr->second->m_iStatus));
    ////desc
    strData.append("&desc=");
    strData.append(Base64::Encode(itr->second->m_strDesc));
    ////reportdesc
    strData.append("&reportdesc=");
    strData.append(Base64::Encode(itr->second->m_strReportDesc));
    ////InnerErrorcode
    strData.append("&innerErrorcode=");
    strData.append(Base64::Encode(itr->second->m_strInnerErrorcode));
    ////channelcnt
    strData.append("&channelcnt=");
    strData.append(Comm::int2str(itr->second->m_uChannelCnt));
    ////reportTime
    strData.append("&reportTime=");
    strData.append(Comm::int2str(itr->second->m_uReportTime));
    ////smsfrom
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(itr->second->m_uSmsFrom));

    LogDebug("==push report==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;
    pMQ->m_strRoutingKey = strRoutingKey;
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);


    if (NULL != itr->second->m_pWheelTime)
    {
        delete itr->second->m_pWheelTime;
        itr->second->m_pWheelTime = NULL;
    }
    delete itr->second;
    m_mapSession.erase(itr);

    return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeliveryReportThread::HandleSendToDeliveryReqMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    TSendToDeliveryReportReqMsg* pSMsg = (TSendToDeliveryReportReqMsg*)pMsg;


    UInt64 iTmep = m_SnMng.getSn();
    DeliveryReportSession* pSession = new DeliveryReportSession();

    pSession->m_strClientId = pSMsg->m_strClientId;
    pSession->m_uChannelId = pSMsg->m_uChannelId;
    pSession->m_strSmsId = pSMsg->m_strSmsId;
    pSession->m_strPhone = pSMsg->m_strPhone;
    pSession->m_iStatus = pSMsg->m_iStatus;
    pSession->m_strDesc = pSMsg->m_strDesc;
    pSession->m_strReportDesc = pSMsg->m_strReportDesc;
    pSession->m_uChannelCnt = pSMsg->m_uChannelCnt;
    pSession->m_uReportTime = pSMsg->m_uReportTime;
    pSession->m_uSmsFrom = pSMsg->m_uSmsFrom;
    pSession->m_uC2sId = pSMsg->m_uC2sId;
    pSession->m_strType = pSMsg->m_strType;

    pSession->m_pWheelTime = SetTimer(iTmep, "", 100*1000);
    m_mapSession[iTmep] = pSession;

    if ((pSMsg->m_uLongSms > 1))
    {
        TRedisReq* pRedis = new TRedisReq();
        string strKey = "";
        pRedis->m_strKey= "sendsms:"+pSession->m_strSmsId+"_"+pSession->m_strPhone;
        strKey = pRedis->m_strKey;
        pRedis->m_RedisCmd = "GET "+ pRedis->m_strKey;
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_iSeq = iTmep;
        pRedis->m_pSender = this;
        pRedis->m_strSessionID = "getStatus";
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
        TRedisReq* pDel = new TRedisReq();
        pDel->m_strKey = strKey;        //for hash redis thread
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_RedisCmd.assign("DEL "+pDel->m_strKey);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);
    }
    else
    {
        HandleRedisComplete(iTmep);
    }
    return;
}

void CDeliveryReportThread::HandleDirectToDeliveryReqMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    TDirectToDeliveryReportReqMsg* pDMsg = (TDirectToDeliveryReportReqMsg*)pMsg;
    //状态报告缓存开关打开,则把状态报告信息存入send对应的状态报告缓存mq队列
    if(pDMsg->m_bSaveReport && g_pReportMQProducerThread)
    {
        TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
        string strMQdate = "";

        strMQdate.append("channelid=");
        strMQdate.append(Comm::int2str(pDMsg->m_uChannelId));

        strMQdate.append("&channelsmsid=");
        strMQdate.append(pDMsg->m_strChannelSmsId);

        strMQdate.append("&phone=");
        strMQdate.append(pDMsg->m_strPhone);

        strMQdate.append("&status=");
        strMQdate.append(Comm::int2str(pDMsg->m_iStatus));

        strMQdate.append("&desc=");
        strMQdate.append(Base64::Encode(pDMsg->m_strDesc));

        strMQdate.append("&errorcode=");
        strMQdate.append(Base64::Encode(pDMsg->m_strErrorCode));

        strMQdate.append("&channelreport=");
        strMQdate.append(Base64::Encode(pDMsg->m_strReportStat));

        strMQdate.append("&innerErrorcode=");
        strMQdate.append(Base64::Encode(pDMsg->m_strInnerErrorcode));

        strMQdate.append("&reporttime=");
        strMQdate.append(Comm::int2str(pDMsg->m_lReportTime));

        strMQdate.append("&identify=");
        strMQdate.append(Comm::int2str(pDMsg->m_uChannelIdentify));

        strMQdate.append("&industrytype=");
        strMQdate.append(Comm::int2str(pDMsg->m_industrytype));

        LogDebug("==push report for cache==exchange:%s,routingkey:%s,data:%s.",g_strMQReportExchange.data(),g_strMQReportRoutingKey.data(),strMQdate.data());

        pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
        pMQ->m_strExchange.assign(g_strMQReportExchange);
        pMQ->m_strRoutingKey.assign(g_strMQReportRoutingKey);
        pMQ->m_strData.assign(strMQdate);
        g_pReportMQProducerThread->PostMsg(pMQ);
        return;
    }
    DeliveryReportSession* pSession = new DeliveryReportSession();
    pSession->m_strChannelSmsId = pDMsg->m_strChannelSmsId;
    pSession->m_iStatus = pDMsg->m_iStatus;
    pSession->m_uReportTime = pDMsg->m_lReportTime;
    pSession->m_strDesc = pDMsg->m_strDesc;
    pSession->m_strReportStat = pDMsg->m_strReportStat;
    pSession->m_strReportDesc = pDMsg->m_strErrorCode;
    pSession->m_strInnerErrorcode = pDMsg->m_strInnerErrorcode;
    pSession->m_uChannelId = pDMsg->m_uChannelId;
    pSession->m_strPhone = pDMsg->m_strPhone;
    pSession->m_uChannelIdentify = pDMsg->m_uChannelIdentify;
    pSession->m_industrytype = pDMsg->m_industrytype;
    UInt64 uSeq = m_SnMng.getSn();

    GetReportInfoRedis(pSession, uSeq);
    m_mapSession[uSeq] = pSession;
}


void CDeliveryReportThread::handleDBResponseMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    TDBQueryResp* pResult = (TDBQueryResp*)pMsg;
    LogDebug("affectRow(%d), result(%d).", pResult->m_iAffectRow, pResult->m_iResult);

    if ("insertBlankPhone" == pResult->m_strSessionID && pResult->m_iResult == 0  && pResult->m_iAffectRow >= 1)
    {
        TRedisReq* pRedis = new TRedisReq();
        string strBlackListKey;
        char strAppendValue[32] = " blank&";
        strBlackListKey = "black_list:" + pResult->m_strKey;
        pRedis->m_RedisCmd = "APPEND " + strBlackListKey + strAppendValue;

        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_strKey = strBlackListKey;
        LogNotice("[%s] insert redis blacklist.[%s].", pResult->m_strKey.data(), pRedis->m_RedisCmd.data());
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
    }


}

void CDeliveryReportThread::InsertToBlankPhone(std::string& strResp)
{
    LogDebug("enter InsertToBlankPhone strResp[%s]", strResp.data())
    time_t now = time(NULL);
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {
        struct tm tmp = {0};
        localtime_r((time_t*)&now,&tmp);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &tmp);
    }
    string strPhone;
    string errorcode;
    std::size_t pos = strResp.find_first_of("&");
    if (pos != std::string::npos)
    {
        strPhone = strResp.substr(0, pos);
        errorcode = strResp.substr(pos+1);
    }
    if (!strPhone.empty())
    {
        char chSql[1024] = { 0 };
        snprintf(chSql,1024,"insert into t_sms_blank_phone(blank_phone,update_time,remarks)values('%s','%s','%s');",
            strPhone.data(), szreachtime, errorcode.data());
        LogNotice("chSql, insert t_sms_blank_phone.[%s].", chSql);

        TDBQueryReq* pMsg = new TDBQueryReq();
        pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        pMsg->m_SQL.assign(chSql);
        pMsg->m_pSender = this;
        pMsg->m_strSessionID = "insertBlankPhone";
        pMsg->m_strKey = strPhone;
        g_pDisPathDBThreadPool->PostMsg(pMsg);
    }
    else
    {
        LogError("can not find phone in strResp[%s]", strResp.data())
    }


}

void CDeliveryReportThread::updateBlankPhone(std::string& strResp)
{
    time_t now = time(NULL);
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {
        struct tm tmp = {0};
        localtime_r((time_t*)&now,&tmp);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &tmp);
    }
    string strPhone;
    std::size_t pos = strResp.find_first_of("&");
    if (pos != std::string::npos)
    {
        strPhone = strResp.substr(0, pos);
    }

    if (!strPhone.empty())
    {
        char chSql[1024] = { 0 };
        snprintf(chSql,1024,"update t_sms_blank_phone set update_time='%s' where blank_phone='%s';",
        szreachtime, strPhone.data());
        LogNotice("chSql, update t_sms_blank_phone.[%s].", chSql);

        TDBQueryReq* pMsg = new TDBQueryReq();
        pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        pMsg->m_SQL.assign(chSql);
        g_pDisPathDBThreadPool->PostMsg(pMsg);
    }
    else
    {
        LogError("can not find phone is redisres[%s]", strResp.data());
    }

}

void CDeliveryReportThread::GetReportInfoRedis(DeliveryReportSession* rtSession, UInt64 seq)
{
    TRedisReq* pRedis = new TRedisReq();
    char pChannelId[25] = {0};
    snprintf(pChannelId,25,"%u", rtSession->m_uChannelId);
    pRedis->m_strKey = "sendmsgid:" + string(pChannelId) + "_" + rtSession->m_strChannelSmsId;
    pRedis->m_RedisCmd = "HGETALL "+pRedis->m_strKey;
    pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedis->m_iSeq = seq;
    pRedis->m_pSender = this;
    pRedis->m_uReqTime = time(NULL);
    pRedis->m_strSessionID = "getReportInfo";
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
    rtSession->m_pWheelTime = SetTimer(seq, "",100*1000);
}

void CDeliveryReportThread::HandleRedisResp(TMsg* pMsg)
{
    TRedisResp* pRMsg = (TRedisResp*)pMsg;
    if (NULL == pRMsg)
    {
        LogError("******pRMsg is NULL.******")
        return;
    }

    if ((NULL == pRMsg->m_RedisReply)
        || (pRMsg->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pRMsg->m_RedisReply->type == REDIS_REPLY_NIL)
        || ((pRMsg->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pRMsg->m_RedisReply->elements == 0)))
    {
        if(NULL != pRMsg->m_RedisReply)
        {
            LogWarn("redis replay failed, replayType[%d],elements[%d],pMsg->m_strSessionID[%s]"
                , pRMsg->m_RedisReply->type, pRMsg->m_RedisReply->elements, pRMsg->m_strSessionID.data());
        }

        if ("check blankphone" == pRMsg->m_strSessionID)
        {
            InsertToBlankPhone(pRMsg->m_string);
        }
        else
        {
            DeliveryReportSessionMap::iterator itr = m_mapSession.find(pMsg->m_iSeq);
            if(itr == m_mapSession.end())
            {
                LogWarn("******iSeq[%lu] is not find in m_mapSession.", pMsg->m_iSeq);
                freeReply(pRMsg->m_RedisReply);
                return;
            }

            if (NULL != itr->second->m_pWheelTime)
            {
                delete itr->second->m_pWheelTime;
                itr->second->m_pWheelTime = NULL;
            }

            LogWarn("==internal redis error== channelsmsid[%s:%s],iSeq[%lu] redisSessionId[%s],query redis is failed.",
                itr->second->m_strChannelSmsId.data(),itr->second->m_strPhone.data(),pMsg->m_iSeq,pMsg->m_strSessionID.data());

            delete itr->second;
            m_mapSession.erase(itr);
        }


        freeReply(pRMsg->m_RedisReply);
        return;
    }

    if ("getReportInfo" == pMsg->m_strSessionID)
    {
        HandleRedisRespGetReportInfo(pRMsg->m_RedisReply,pRMsg->m_iSeq);
    }
    else if ("getKey" == pMsg->m_strSessionID)
    {
        HandleRedisRespGetKey(pRMsg->m_RedisReply,pRMsg->m_iSeq);
    }
    else if ("getStatus" == pMsg->m_strSessionID)
    {
        HandleRedisRespGetStatus(pRMsg->m_RedisReply,pRMsg->m_iSeq);
    }
    else if ("check blankphone" == pMsg->m_strSessionID)
    {

        if (REDIS_REPLY_STRING == pRMsg->m_RedisReply->type)
        {
            string strOut = pRMsg->m_RedisReply->str;
            if (!strOut.empty())
            {
                if (strOut.find("blank&") == string::npos)
                {
                    InsertToBlankPhone(pRMsg->m_string);
                }
                else
                {
                    updateBlankPhone(pRMsg->m_string);
                }
            }
        }

    }
    else
    {
        LogError("==internal error==this Redis type[%s] is invalid.",pMsg->m_strSessionID.data());
        return;
    }

    freeReply(pRMsg->m_RedisReply);
    return;
}

void CDeliveryReportThread::HandleTimeOutMsg(UInt64 iSeq)
{
    DeliveryReportSessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogWarn("iSeq[%lu] is not find in m_mapSession.", iSeq);
        return;
    }

    LogWarn("==time out warn==[%s:%s],iSeq[%lu].",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),iSeq);

    if (NULL != itr->second->m_pWheelTime)
    {
        delete itr->second->m_pWheelTime;
        itr->second->m_pWheelTime = NULL;
    }
    delete itr->second;
    m_mapSession.erase(itr);
}

void CDeliveryReportThread::UpdateRecord(string& strSmsUuid,string& strSendTime,UInt64 uSubmitDate,DeliveryReportSession* psession)
{
    char sql[2048]  = {0};
    char szreachtime[64] = {0};

    struct tm Reportptm = {0};
    localtime_r((time_t*)&psession->m_uReportTime,&Reportptm);

    string strRecordDate = strSendTime.substr(0,8);

    if(psession->m_uReportTime != 0 )
    {
        strftime(szreachtime, sizeof(szreachtime), "%Y%m%d%H%M%S", &Reportptm);
    }

    int duration = psession->m_uReportTime -uSubmitDate;
    if(duration < 0 || uSubmitDate <= 0)
    {
        duration = 0;
    }
    if(psession->m_uFailedResendTimes == 0)
    {
        if (psession->m_strInnerErrorcode.empty())
        {
            snprintf(sql,2048,
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d' where smsuuid='%s' and date='%s';",
            psession->m_uChannelIdentify,
            strRecordDate.data(),
            psession->m_iStatus,
            szreachtime,
            psession->m_strDesc.data(),
            szreachtime,
            duration,
            strSmsUuid.data(),
            strSendTime.data());
        }
        else
        {
            snprintf(sql,2048,
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d',innerErrorcode='%s'"
            " where smsuuid='%s' and date='%s';",
            psession->m_uChannelIdentify,
            strRecordDate.data(),
            psession->m_iStatus,
            szreachtime,
            psession->m_strDesc.data(),
            szreachtime,
            duration,
            psession->m_strInnerErrorcode.c_str(),
            strSmsUuid.data(),
            strSendTime.data());
        }

    }
    else
    {
        if (psession->m_strInnerErrorcode.empty())
        {
            snprintf(sql,2048,
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d', "
            "failed_resend_flag='%u',failed_resend_times='%u' where smsuuid='%s' and date='%s';",
            psession->m_uChannelIdentify,
            strRecordDate.data(),
            psession->m_iStatus,
            szreachtime,
            psession->m_strDesc.data(),
            szreachtime,
            duration,
            psession->m_uFailedResendFlag,
            psession->m_uFailedResendTimes,
            strSmsUuid.data(),
            strSendTime.data());
        }
        else
        {
            snprintf(sql,2048,
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d', "
            "failed_resend_flag='%u',failed_resend_times='%u',innerErrorcode='%s' where smsuuid='%s' and date='%s';",
            psession->m_uChannelIdentify,
            strRecordDate.data(),
            psession->m_iStatus,
            szreachtime,
            psession->m_strDesc.data(),
            szreachtime,
            duration,
            psession->m_uFailedResendFlag,
            psession->m_uFailedResendTimes,
            psession->m_strInnerErrorcode.c_str(),
            strSmsUuid.data(),
            strSendTime.data());
        }

    }
    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange);
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append(strSmsUuid);
    g_pRecordMQProducerThread->PostMsg(pMQ);

    MONITOR_INIT( MONITOR_CHANNEL_SMS_REPORT );
    MONITOR_VAL_SET("clientid", psession->m_strClientId );
    MONITOR_VAL_SET_INT("channelid", psession->m_uChannelId );
    MONITOR_VAL_SET_INT("smsfrom", psession->m_uSmsFrom );
    MONITOR_VAL_SET("smsid", psession->m_strSmsId );
    MONITOR_VAL_SET("phone", psession->m_strPhone );
    MONITOR_VAL_SET("smsuuid", strSmsUuid );
    MONITOR_VAL_SET_INT("state", psession->m_iStatus );
    MONITOR_VAL_SET("reportcode", psession->m_strDesc );
    MONITOR_VAL_SET("reportdesc", psession->m_strReportDesc );
    MONITOR_VAL_SET("reportdate", szreachtime );
    MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));
    MONITOR_VAL_SET_INT("costtime", duration );
    MONITOR_VAL_SET_INT("component_id", g_uComponentId );
    string strTable = "t_sms_record_";
    strTable.append(Comm::int2str(psession->m_uChannelIdentify));
    strTable.append("_" + strRecordDate );
    MONITOR_VAL_SET("dbtable", strTable );
    MONITOR_PUBLISH( g_pMQMonitorPublishThread );

}

UInt32 CDeliveryReportThread::GetSessionMapSize()
{
    return m_mapSession.size();
}

void CDeliveryReportThread::countReportFailed(UInt32 uChannelId, UInt32 industrytype, bool bResult,string toDay, UInt64 uNowTime)
{
    map<UInt32,ReportFailedTime>::iterator iter = m_mapReportFailedAlarm.find(uChannelId);
    if (iter == m_mapReportFailedAlarm.end())       ///this channel do not have any record yet
    {
        ReportFailedTime ReportInfo;
        ReportInfo.m_strToDay = toDay;
        if(true == bResult)
        {
            ReportInfo.m_uTime = 0;
            ReportInfo.m_strReportTimeList.clear();
        }
        else
        {
            ReportInfo.m_uTime = 1;
            ReportInfo.m_strReportTimeList.assign(Comm::int2str( uNowTime ) + "&");
        }

        m_mapReportFailedAlarm.insert(make_pair(uChannelId,ReportInfo));
        LogDebug("====report failed count==== channel %d  continue faild %d mapsize is %d",uChannelId,ReportInfo.m_uTime,m_mapReportFailedAlarm.size());
        return;
    }
    else                                            ////this channel has record
    {
        if(true == bResult)                         ///this report is true,,clear the data
        {
            iter->second.m_strToDay = toDay;
            iter->second.m_uTime = 0;
            iter->second.m_strReportTimeList.clear();
            LogDebug("====report failed count==== channel %d  continue faild %d mapsize is %d",uChannelId,iter->second.m_uTime,m_mapReportFailedAlarm.size());
            return;
        }
        ///this report is fail
        if(iter->second.m_strToDay == toDay)        ///this message is sended at today, record it
        {
            UInt32 uNum = 0;
            uNum = iter->second.m_uTime;
            uNum++;
            iter->second.m_strReportTimeList.append(Comm::int2str( uNowTime ) + "&");
            LogDebug("channelid = %u, uNowTime = %uL, reportListTime = %s", uChannelId, uNowTime, iter->second.m_strReportTimeList.data());
            if (uNum >= m_uReportFailedAlarmValue)  ///if report fail time is larger than the value
            {
                string &reportListTemp = iter->second.m_strReportTimeList;
                long startTime = 0;
                std::string::size_type pos = 0;
                while(uNum > m_uReportFailedAlarmValue)
                {
                    uNum--;
                    pos = reportListTemp.find("&");
                    reportListTemp.erase(0, pos + 1);
                    LogDebug("delete after reportListTime = %s", iter->second.m_strReportTimeList.data());
                }
                iter->second.m_uTime = uNum;
                pos = reportListTemp.find("&");
                startTime = atol(reportListTemp.substr(0, pos).c_str());
                LogDebug("channel = %d, startTime = %uL, uNowTime = %uL, reportListTime = %s", uChannelId, startTime, uNowTime, iter->second.m_strReportTimeList.data());
                if (uNowTime - startTime <= m_uReportFailedSendTimeInterval)
                {
                    char temp[15] = {0};
                    snprintf(temp,15,"%u",uChannelId);
                    LogWarn("channelid[%d] continue report failed num[%d] is over systemValue[%d] in [%d] minutes"
                        ,uChannelId,uNum,m_uReportFailedAlarmValue, m_uReportFailedSendTimeInterval/60);

                    std::string strChinese = "";
                    strChinese = DAlarm::getAlarmStr2(uChannelId,industrytype,uNum,m_uReportFailedAlarmValue,m_uReportFailedSendTimeInterval/60);
                    Alarm(CHNN_STATUS_REPORT_FAILED_ALARM,temp,strChinese.data());
                }

            }
            else
            {
                iter->second.m_uTime = uNum;        ////report fail time is smaller than the value,just add one
            }

        }
        else                                        ///this message was sended at another day,do not record it
        {
            iter->second.m_strToDay = toDay;
            iter->second.m_uTime = 0;
            iter->second.m_strReportTimeList.clear();
            LogDebug("====report failed count==== channel %d  continue faild %d mapsize is %d",uChannelId,iter->second.m_uTime,m_mapReportFailedAlarm.size());
        }
    }
    return;
}

/*
11：异常移动行业，
12：异常移动营销，
13：异常联通行业，
14：异常联通营销，
15：异常电信行业，
16：异常电信营销
map<string,ComponentRefMq> m_componentRefMqInfo;
*/
bool CDeliveryReportThread::FailedReSendToReBack( DeliveryReportSession* pReportInfo )
{
    string strExchange;
    string strRoutingKey;
    string strMessageType = "";
    string strKey = "";
    char temp[250] = {0};
    map<string,ComponentRefMq>::iterator itReq ;
    map<UInt64,MqConfig>::iterator itrMq;

    if ( YIDONG == pReportInfo->m_uOperater )
    {
        if ( SMS_TYPE_MARKETING == pReportInfo->m_uSmsType
            || SMS_TYPE_USSD == pReportInfo->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pReportInfo->m_uSmsType )
        {
            strMessageType.assign("12");
        }
        else
        {
            strMessageType.assign("11");
        }
    }
    else if (DIANXIN == pReportInfo->m_uOperater )
    {
        if (SMS_TYPE_MARKETING ==  pReportInfo->m_uSmsType
            || SMS_TYPE_USSD == pReportInfo->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pReportInfo->m_uSmsType )
        {
            strMessageType.assign("16");
        }
        else
        {
            strMessageType.assign("15");
        }
    }
    else if ((LIANTONG == pReportInfo->m_uOperater)
         || (FOREIGN == pReportInfo->m_uOperater))
    {
        if (SMS_TYPE_MARKETING == pReportInfo->m_uSmsType
            || SMS_TYPE_USSD == pReportInfo->m_uSmsType
            || SMS_TYPE_FLUSH_SMS == pReportInfo->m_uSmsType)
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
        LogError("[%s:%s] phoneType:%u is invalid.",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(),pReportInfo->m_uSmsType );
        return false;
    }

    snprintf(temp,250,"%lu_%s_0", g_uComponentId, strMessageType.data());
    strKey.assign(temp);

    itReq =  m_componentRefMqInfo.find(strKey);
    if (itReq == m_componentRefMqInfo.end())
    {
        LogError("[%s:%s]strKey:%s is not find in m_componentRefMqInfo.",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(),strKey.data());
        return false;
    }

    itrMq = m_mapMqConfig.find(itReq->second.m_uMqId);
    if ( itrMq == m_mapMqConfig.end() )
    {

        LogError("[%s:%s]mqid:%lu is not find in m_mqConfigInfo.",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(), itReq->second.m_uMqId );
        return false;
    }

    strExchange = itrMq->second.m_strExchange;
    strRoutingKey = itrMq->second.m_strRoutingKey;
    LogDebug("== push except== exchange:%s,routingkey:%s,data:%s.",
                strExchange.data(), strRoutingKey.data(), pReportInfo->m_strResendMsg.data());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = pReportInfo->m_strResendMsg;
    pMQ->m_strExchange = strExchange;
    pMQ->m_strRoutingKey = strRoutingKey;
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);

    return true ;
}

bool CDeliveryReportThread::CheckFailedResend(redisReply* pRedisReply,DeliveryReportSession* pReportInfo)
{
    string strSmsType;
    UInt32 uResendTimes = 0;
    string strResendChannels = "";
    string strResendMsg = "";
    string strCsTime = "";
    string strUuids = "";
    //check sys switch
    if(false == m_bResendSysSwitch)
    {
        LogDebug("[%s:%s],sys resend switch is not open!",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    /* long sms not support failed_resend */
    if(1 == pReportInfo->m_uLongSms)
    {
        LogDebug("[%s:%s],long sms !",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    //account switch
    std::map<std::string,SmsAccount>::iterator iterAccount = m_accountMap.find( pReportInfo->m_strClientId);
    if( (iterAccount != m_accountMap.end()) && (1 == iterAccount->second.m_uFailedResendFlag) )
    {
        //continue
    }
    else
    {
        LogDebug("[%s:%s],account[%s] switch is not open!",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str(),
            pReportInfo->m_strClientId.c_str());
        return false;
    }


    channelMap_t::iterator iterChannel = m_ChannelMap.find((int)pReportInfo->m_uChannelId);
    if(iterChannel != m_ChannelMap.end() &&
        !iterChannel->second.m_strResendErrCode.empty() &&
        (string::npos != iterChannel->second.m_strResendErrCode.find(pReportInfo->m_strReportStat)))
    {
        //continue
    }
    else
    {
        LogDebug("[%s:%s] Channel[%u] Report[%s] is not in cfg",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(),pReportInfo->m_uChannelId,pReportInfo->m_strReportStat.c_str());
        return false;
    }

    paserReply("smstype", pRedisReply, strSmsType);
    if(strSmsType.empty())
    {
        LogNotice("[%s:%s] get resend redis info smstype is null",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    UInt32 uSmsType = atoi(strSmsType.c_str());

    std::map<UInt32,FailedResendSysCfg>::iterator iterSysCfg = m_FailedResendSysCfgMap.find(uSmsType);
    if((iterSysCfg != m_FailedResendSysCfgMap.end()) &&
        (0 < iterSysCfg->second.m_uResendMaxTimes) &&
        (0 < iterSysCfg->second.m_uResendMaxDuration))
    {
        //continue
    }
    else
    {
        LogNotice("[%s:%s] smstype[%u] not support resend",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str(),
            uSmsType);
        return false;
    }
    //check failed resend times;
    string strTemp;
    paserReply("failed_resend_times", pRedisReply, strTemp);
    uResendTimes = atoi(strTemp.c_str());
    if(uResendTimes >= iterSysCfg->second.m_uResendMaxTimes)
    {
        LogWarn("[%s:%s] resend times[%u] over cfg[%u]",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(),uResendTimes,iterSysCfg->second.m_uResendMaxTimes);
        return false;
    }
    paserReply("failed_resend_channel", pRedisReply, strResendChannels);
    if(!strResendChannels.empty())
    {
        strResendChannels.append(",");
    }
    strResendChannels.append(Comm::int2str(pReportInfo->m_uChannelId));

    paserReply("failed_resend_msg", pRedisReply, strResendMsg);
    if(strResendMsg.empty())
    {
        LogWarn("[%s:%s] redis failed_resend_msg is null",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    web::HttpParams param;
    param.Parse(strResendMsg);
    //check sendtime
    strCsTime = param._map["csdate"];
    time_t nowTime = time(NULL);
    time_t c2sTime = Comm::getTimeStampFormDate(strCsTime,"%Y%m%d%H%M%S");
    if(-1 == c2sTime)
    {
        LogError("[%s:%s] get c2stime error",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    if((nowTime - c2sTime) > iterSysCfg->second.m_uResendMaxDuration)
    {
        LogWarn("[%s:%s] resend timeout",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str());
        return false;
    }
    pReportInfo->m_uOperater = atoi(param._map["operater"].c_str());

    pReportInfo->m_uFailedResendFlag = 1;
    pReportInfo->m_uFailedResendTimes = uResendTimes + 1;

    string strMqData = "";
    strMqData.assign("clientId=");
    strMqData.append(pReportInfo->m_strClientId);
    strMqData.append("&smsId=").append(pReportInfo->m_strSmsId);
    strMqData.append("&phone=").append(pReportInfo->m_strPhone);
    strMqData.append("&smsfrom=").append(Comm::int2str(pReportInfo->m_uSmsFrom));
    strMqData.append("&longsms=").append(Comm::int2str(pReportInfo->m_uLongSms));
    strMqData.append("&").append(strResendMsg);
    strMqData.append("&failed_resend_times=").append(Comm::int2str(pReportInfo->m_uFailedResendTimes));
    strMqData.append("&failed_resend_channel=").append(strResendChannels);

    pReportInfo->m_strResendMsg.assign(strMqData);
    if(false == FailedReSendToReBack(pReportInfo))
    {
        pReportInfo->m_uFailedResendTimes = 0;
        return false;
    }
    return true;
}







