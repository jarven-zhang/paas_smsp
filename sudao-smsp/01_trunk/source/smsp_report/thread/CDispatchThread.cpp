#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include "hiredis.h"
#include "ErrorCode.h"
#include "main.h"
#include "Uuid.h"
#include "UrlCode.h"
#include "Comm.h"
#include "alarm.h"
#include "base64.h"
#include "HttpParams.h"
CDispatchThread::CDispatchThread(const char *name):CThread(name)
{
    // CHANNEL_REPORT_FAIL default: 100;10
    m_uReportFailedAlarmValue = 100;
    m_uReportFailedSendTimeInterval = 10 * 60;

    m_bBlankPhoneSwitch = 0;
    m_strBlankPhoneErr = BLANK_PHONE_ERROR;
    m_strBlankPhoneErr.append("|");
}

CDispatchThread::~CDispatchThread()
{
    
}

bool CDispatchThread::Init()
{   
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }
    m_pVerify = new VerifySMS();
    g_pRuleLoadThread->getClientIdAndAppend(m_mapSignExtGw);
    g_pRuleLoadThread->getChannelBlackListMap(m_ChannelBlackListMap);

    ////5.0
    g_pRuleLoadThread->getChannelErrorCode(m_mapChannelErrorCode);
    g_pRuleLoadThread->getComponentConfig(m_mapComponentConfig);
    g_pRuleLoadThread->getMqConfig(m_mapMqConfig);
    g_pRuleLoadThread->getSmsAccountMap(m_SmsAccountMap);
    g_pRuleLoadThread->getChannelExtendPortTableMap(m_mapChannelExt);

    m_ChannelMap.clear();
    g_pRuleLoadThread->getChannlelMap( m_ChannelMap );
    m_componentRefMqInfo.clear();
    g_pRuleLoadThread->getComponentRefMq(m_componentRefMqInfo);
    
    map<string,string> mapSysParam;
    g_pRuleLoadThread->getSysParamMap(mapSysParam);
    GetSysPara(mapSysParam);

    return true;
}

void CDispatchThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
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

    LogNotice("System parameter(%s) final value(%u, %u, %u seconds).",
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

void CDispatchThread::HandleMsg(TMsg* pMsg)
{   
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    
    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);
    
    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_REPORT_TO_DISPATCH_REQ:
        {   
            HandleReportToDispatchReqMsg(pMsg);
            break;
        }
        case MSGTYPE_UPSTREAM_TO_DISPATCH_REQ:
        {
            HandleUpStreamToDisPatchReqMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SIGNEXTNOGW_UPDATE_REQ:
        {
            TUpdateSignextnoGwReq* pSign = (TUpdateSignextnoGwReq*)pMsg;
            LogDebug("RuleUpdate signextno_gw update,map size[%d].",pSign->m_SignextnoGwMap.size());
            m_mapSignExtGw.clear();
            m_mapSignExtGw = pSign->m_SignextnoGwMap;
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
        case MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ:
        {
            TUpdateChannelErrorCodeReq* pDirect = (TUpdateChannelErrorCodeReq*)pMsg;

            m_mapChannelErrorCode.clear();
            m_mapChannelErrorCode = pDirect->m_mapChannelErrorCode;
            LogNotice("update t_sms_channel_error_desc size:%d.",m_mapChannelErrorCode.size());
            break;
        }
         case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            TUpdateSmsAccontReq* pAccount = (TUpdateSmsAccontReq*)pMsg;

            m_SmsAccountMap.clear();
            m_SmsAccountMap = pAccount->m_SmsAccountMap;
            LogNotice("update t_sms_account size:%d.",m_SmsAccountMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_EXTENDPORT_UPDATE_REQ:
        {
            TUpdateChannelExtendPortReq* pExt = (TUpdateChannelExtendPortReq*)pMsg;
            LogNotice("RuleUpdate channel_extendport update,map size[%d].",pExt->m_ChannelExtendPortTable.size());
            m_mapChannelExt.clear();
            m_mapChannelExt = pExt->m_ChannelExtendPortTable;
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
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisResp(pMsg);
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
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            TUpdateChannelReq* pChannelMsg = (TUpdateChannelReq*)pMsg;
            if(pChannelMsg)
            {
                m_ChannelMap.clear();
                m_ChannelMap = pChannelMsg->m_ChannelMap;
                LogNotice("update t_sms_channel, size[%u]", m_ChannelMap.size());
            }
            break;
        }
        case MSGTYPE_DB_NOTQUERY_RESP:
        {
            handleDBResponseMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
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


void CDispatchThread::getErrorByChannelId(UInt32 uChannelId,string strIn,string& strDesc,string& strReportDesc)
{
    string strKey2 = "";
    strKey2.append(Comm::int2str(uChannelId));
    strKey2.append("_2_");
    strKey2.append(strIn);

    map<string,channelErrorDesc>::iterator itrDesc2 = m_mapChannelErrorCode.find(strKey2);
    if (itrDesc2 != m_mapChannelErrorCode.end())
    {   
        if (true == itrDesc2->second.m_strSysCode.empty()) ////syscode kong
        {
            strDesc.assign(itrDesc2->second.m_strErrorCode);
            strDesc.append("*");
            strDesc.append(itrDesc2->second.m_strErrorDesc);
            
            strReportDesc.assign(strIn);
        }
        else
        {
            strDesc.assign(itrDesc2->second.m_strErrorCode);
            strDesc.append("*");
            strDesc.append(itrDesc2->second.m_strErrorDesc);

            strReportDesc.assign(itrDesc2->second.m_strSysCode);
        }
    }
    else
    {
        string strUrl = "*%e6%9c%aa%e7%9f%a5";
        strUrl = http::UrlCode::UrlDecode(strUrl);

        strDesc.assign(strIn);
        strDesc.append(strUrl);
        strReportDesc.assign(strIn);
    }
    
}


void CDispatchThread::getErrorCode(string &strChannelLibName,UInt32 uChannelId,string strIn,string& strDesc,string& strReportDesc)
{

    if (!strChannelLibName.empty())
    {
        string strKey1 = "";
        strKey1.assign(strChannelLibName);
        strKey1.append("_2_");
        strKey1.append(strIn);

        map<string,channelErrorDesc>::iterator itrDesc1 = m_mapChannelErrorCode.find(strKey1);
        if (itrDesc1 == m_mapChannelErrorCode.end())
        {
            getErrorByChannelId(uChannelId,strIn,strDesc,strReportDesc);
        }
        else
        {
            if (true == itrDesc1->second.m_strSysCode.empty())
            {
                strDesc.assign(strIn);
                strDesc.append("*");
                strDesc.append(itrDesc1->second.m_strErrorDesc);
                    
                strReportDesc.assign(strIn);
            }
            else
            {
                strDesc.assign(strIn);
                strDesc.append("*");
                strDesc.append(itrDesc1->second.m_strErrorDesc);

                strReportDesc.assign(itrDesc1->second.m_strSysCode);
            }
        }
    }
    else
    {
        getErrorByChannelId(uChannelId,strIn,strDesc,strReportDesc);
    }

    return;
}

void CDispatchThread::smsGetInnerError(string strDircet, UInt32 uChannelId, string strType, string strIn, string& strInnerError)
{
   string strKey1 = "";
   strKey1.assign(strDircet);
   strKey1.append("_");
   strKey1.append(strType);
   strKey1.append("_");
   strKey1.append(strIn);

   map<string,channelErrorDesc>::iterator itrDesc1 = m_mapChannelErrorCode.find(strKey1);

   if (itrDesc1 == m_mapChannelErrorCode.end())
   {
       string strKey2 = "";
       strKey2.assign(Comm::int2str(uChannelId));
       strKey2.append("_");
       strKey2.append(strType);
       strKey2.append("_");
       strKey2.append(strIn);

       map<string,channelErrorDesc>::iterator itrDesc2 = m_mapChannelErrorCode.find(strKey2);
       if (itrDesc2 == m_mapChannelErrorCode.end())
       {
           string strUrl = "*%e6%9c%aa%e7%9f%a5";
           strUrl = http::UrlCode::UrlDecode(strUrl);
           
           strInnerError.assign(strIn);
           strInnerError.append(strUrl);
       }
       else
       {
           if (itrDesc2->second.m_strInnnerCode.empty())
           {
                strInnerError.assign(strIn);
                strInnerError.append("*");
                strInnerError.append(itrDesc2->second.m_strErrorDesc);
           }
           else
           {
                strInnerError.assign(itrDesc2->second.m_strInnnerCode);
           }
          
       }
   }
   else
   {
       if (itrDesc1->second.m_strInnnerCode.empty())
       {
            strInnerError.assign(strIn);
            strInnerError.append("*");
            strInnerError.append(itrDesc1->second.m_strErrorDesc);
       }
       else
       {
            strInnerError.assign(itrDesc1->second.m_strInnnerCode);
       }
   }

   return;
}

void CDispatchThread::GetReportInfoRedis(SessionStruct* rtSession, UInt64 seq)
{
    char cChannelId[25] = {0};
    snprintf(cChannelId,25,"%u",rtSession->m_uChannelId);
    
    string strCmd = "";
    string strkey = "";


    if( rtSession->m_uClusterType == 1){
        strkey.append("cluster_sms:");
    }else{  
        strkey.append("sendmsgid:");
    }
    
    strkey.append(cChannelId);
    strkey.append("_");
    strkey.append(rtSession->m_strChannelSmsId);

    if( rtSession->m_uClusterType == 1 ){
        strkey.append("_");
        strkey.append(rtSession->m_strPhone);
    }

    strCmd.append("HGETALL ");
    strCmd.append(strkey);
   
    TRedisReq* pRed = new TRedisReq();
    pRed->m_RedisCmd = strCmd;
    pRed->m_strKey = strkey;
    pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRed->m_iSeq = seq;
    pRed->m_pSender = this;
    pRed->m_strSessionID = "getReportInfo";
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);

}

void CDispatchThread::HandleReportToDispatchReqMsg(TMsg* pMsg)
{
    TReportToDispatchReqMsg* pReport = (TReportToDispatchReqMsg*)pMsg;
        //remove 86
    if (m_pVerify->CheckPhoneFormat(pReport->m_strPhone) == false)
    {
        LogError("channel return phone[%s] format error", pReport->m_strPhone.data())
    }
    //状态报告缓存开关打开,则把状态报告信息存入report对应的状态报告缓存mq队列
    if(pReport->m_bSaveReport && g_pReportMQProducerThread)
    {
        TMQPublishReqMsg* pMQ = new TMQPublishReqMsg(); 
        string strMQdate = "";
        
        strMQdate.append("channelid=");
        strMQdate.append(Comm::int2str(pReport->m_uChannelId));

        strMQdate.append("&channelsmsid=");
        strMQdate.append(pReport->m_strChannelSmsId);

        strMQdate.append("&phone=");
        strMQdate.append(pReport->m_strPhone);

        strMQdate.append("&status=");
        strMQdate.append(Comm::int2str(pReport->m_iStatus));

        strMQdate.append("&reportdesc=");
        strMQdate.append(Base64::Encode(pReport->m_strDesc));

        strMQdate.append("&reporttime=");
        strMQdate.append(Comm::int2str(pReport->m_lReportTime));

        strMQdate.append("&identify=");
        strMQdate.append(Comm::int2str(pReport->m_uChannelIdentify));

        strMQdate.append("&cluster_type=");
        strMQdate.append(Comm::int2str(pReport->m_uClusterType));
        
        LogDebug("==push report for cache==exchange:%s,routingkey:%s,data:%s.",g_strMQReportExchange.data(),g_strMQReportRoutingKey.data(),strMQdate.data());  
    
        pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
        pMQ->m_strExchange.assign(g_strMQReportExchange);
        pMQ->m_strRoutingKey.assign(g_strMQReportRoutingKey);
        pMQ->m_strData.assign(strMQdate);
        g_pReportMQProducerThread->PostMsg(pMQ);
        return;
    }
    SessionStruct* pSessionStruct = new SessionStruct();

    if (SMS_STATUS_CONFIRM_SUCCESS != UInt32(pReport->m_iStatus))
    {
        LogDebug("channellibname: %s", pReport->m_strChannelLibName.c_str());
        pSessionStruct->m_strReportStat = pReport->m_strDesc;//resave channel report desc for failed resend
        getErrorCode(pReport->m_strChannelLibName,pReport->m_uChannelId,pReport->m_strDesc,pSessionStruct->m_strDesc,pSessionStruct->m_strReportDesc);
        smsGetInnerError(pReport->m_strChannelLibName,pReport->m_uChannelId,string("2"),pSessionStruct->m_strReportStat,pReport->m_strInnerErrorcode);
        pSessionStruct->m_strInnerErrorcode = pReport->m_strInnerErrorcode;
    }
    else
    {
        pSessionStruct->m_strDesc = pReport->m_strDesc;
        pSessionStruct->m_strReportDesc = pReport->m_strDesc;
        pReport->m_strInnerErrorcode = pReport->m_strDesc;
        pSessionStruct->m_strInnerErrorcode = pReport->m_strInnerErrorcode;
    }
        
    pSessionStruct->m_strType= "1";
    pSessionStruct->m_iStatus = pReport->m_iStatus;
    pSessionStruct->m_lReportTime = pReport->m_lReportTime;
    pSessionStruct->m_strChannelSmsId = pReport->m_strChannelSmsId;
    pSessionStruct->m_uChannelId = pReport->m_uChannelId;
    pSessionStruct->m_strPhone = pReport->m_strPhone;
    pSessionStruct->m_uChannelIdentify = pReport->m_uChannelIdentify;
    pSessionStruct->m_uClusterType     = pReport->m_uClusterType;
    pSessionStruct->m_uClusterGetType  = pReport->m_uClusterType;
    
    UInt64 iSeq = m_SnMng.getSn();
    m_mapSession[iSeq] = pSessionStruct;

    GetReportInfoRedis(pSessionStruct, iSeq);
    return;
}

void CDispatchThread::QuerySignMoPort(SessionStruct* pSession, UInt64 iSeq)
{
    TRedisReq* pRds = new TRedisReq();
    string strCmd = "";
    strCmd.append("HGETALL  ");
    char tmpKey[250] = {0};
    snprintf(tmpKey,250,"sign_mo_port:%d_%s:%s",pSession->m_uChannelId,pSession->m_strAppendId.data(), pSession->m_strPhone.data());
    strCmd.append(tmpKey);

    LogDebug("[ :%s] temKey[%s].",pSession->m_strPhone.data(),strCmd.data());
    pRds->m_RedisCmd = strCmd;
    pRds->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRds->m_iSeq = iSeq;
    pRds->m_pSender = this;
    pRds->m_strSessionID = "get_sign_mo_port";
    pRds->m_strKey = tmpKey;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRds);
}

void CDispatchThread::HandleUpStreamToDisPatchReqMsg(TMsg* pMsg)
{
    TUpStreamToDispatchReqMsg* pUpStream = (TUpStreamToDispatchReqMsg*)pMsg;
    SessionStruct* pSessionStruct = new SessionStruct();
    pSessionStruct->m_strType = "2";
    //remove 86
    if (m_pVerify->CheckPhoneFormat(pUpStream->m_strPhone) == false)
    {
        LogError("channel return phone[%s] format error", pUpStream->m_strPhone.data())
    }
    pSessionStruct->m_strPhone = pUpStream->m_strPhone;
    pSessionStruct->m_strAppendId = pUpStream->m_strAppendId;
    pSessionStruct->m_strUpTime = pUpStream->m_strUpTime;
    pSessionStruct->m_uChannelId = pUpStream->m_iChannelId;
    pSessionStruct->m_lUpTime = pUpStream->m_lUpTime;
    pSessionStruct->m_strContent = pUpStream->m_strContent;
    pSessionStruct->m_uChannelType = pUpStream->m_uChannelType;
    
    UInt64 iSeq = m_SnMng.getSn();
    m_mapSession[iSeq] = pSessionStruct;

    /* 通过上行前缀来获取扩展号码*/ 
    if( !pUpStream->m_strMoPrefix.empty())
    {
        UInt32 uPrefixLen = pUpStream->m_strMoPrefix.length();
        if( uPrefixLen <= pSessionStruct->m_strAppendId.length())
        {
            pSessionStruct->m_strAppendId = pSessionStruct->m_strAppendId.substr( uPrefixLen );
            LogNotice("[%u: %s] ExtendPort moPrefix[%s,%d] Src[%s], Dst[%s]",
                     pSessionStruct->m_uChannelId,pSessionStruct->m_strPhone.data(),
                     pUpStream->m_strMoPrefix.data(), uPrefixLen,
                     pSessionStruct->m_strAppendId.data(), 
                     pUpStream->m_strAppendId.data());
        }
    }

    ////// 通道类型，0：自签平台用户端口，1：固签无自扩展，2：固签有自扩展，3：自签通道用户端口
    if ((1 == pSessionStruct->m_uChannelType) || (2 == pSessionStruct->m_uChannelType))
    {
        string strExtendPort = pSessionStruct->m_strAppendId;
        LogDebug("[ :%s] ExtendPort[%s].", pSessionStruct->m_strPhone.data(), strExtendPort.data());

        if (1 == pSessionStruct->m_uChannelType)//////not support auto sign and not support extend port
        {
            char cChannelId[64] = {0};
            snprintf(cChannelId,64,"%u",pSessionStruct->m_uChannelId);
            string strKey = "";
            strKey.append(cChannelId);
            strKey.append("&");
            strKey.append(strExtendPort);
             pSessionStruct->m_strGwExtend = strExtendPort;
            SignExtnoGwMap::iterator iter = m_mapSignExtGw.find(strKey);
            if (iter == m_mapSignExtGw.end())
            {
                LogNotice("[ :%s] key[%s] is not find t_signextno_gw in memory,not support auto extend port", pSessionStruct->m_strPhone.data(), strKey.data());
                QuerySignMoPort(pSessionStruct, iSeq);
                return;
            }
            else
            {
                bool isStar = false;
                vector<models::SignExtnoGw>::iterator itVec = iter->second.begin();
                for (; itVec != iter->second.end(); itVec++)
                {
                    if (itVec->m_strClient == "*")
                    {
                        isStar = true;
                        break;
                    }
                    
                }
                if (iter->second.size() > 1 || isStar)
                {
                    LogNotice("[ :%s] key[%s] too many client t_signextno_gw,not support auto extend port",
                    pSessionStruct->m_strPhone.data(),strKey.data());
                    QuerySignMoPort(pSessionStruct, iSeq);
                    return;
                }
                else
                {
                     pSessionStruct->m_strSign = iter->second[0].sign;
                    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
                    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(iter->second[0].m_strClient))
                    {
                        pSessionStruct->m_strClientId = iter->second[0].m_strClient;
                    }
                }
            }

        }
        else if (2 == pSessionStruct->m_uChannelType)//////not support auto sign but support extend port
        {
            ////bool bFlag = false;
            SignExtnoGwMap::iterator iter = m_mapSignExtGw.begin();
            SignExtnoGwMap::iterator itrLong = m_mapSignExtGw.end();
            
            for (; iter != m_mapSignExtGw.end(); ++iter)
            {
                UInt32 uChannelId = atoi(iter->second[0].channelid.data());
                if (pSessionStruct->m_uChannelId == uChannelId)
                {
                    if (0 == strExtendPort.compare(0,iter->second[0].appendid.length(),iter->second[0].appendid))
                    {
                        // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
                        if (BusType::CLIENT_STATUS_LOGOFF == GetAcctStatus(iter->second[0].m_strClient))
                        {
                            iter->second[0].m_strClient = "";
                        }

                        if (itrLong == m_mapSignExtGw.end())
                        {
                            itrLong = iter;
                        }
                        else
                        {
                            if (itrLong->second[0].appendid.length() >= iter->second[0].appendid.length())
                            {
                                ;
                            }
                            else
                            {
                                itrLong = iter;
                            }
                        }
                    }
                }
            }

            if (itrLong == m_mapSignExtGw.end())
            {
                LogWarn("[ :%s] channelid[%d],extendPort[%s] is not find must long match in t_signextno_gw.",
                    pSessionStruct->m_strPhone.data(),pSessionStruct->m_uChannelId,strExtendPort.data());
                InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);
                delete pSessionStruct;
                m_mapSession.erase(iSeq); 
                return;
            }
            else
            {
                bool isStar = false;
                vector<models::SignExtnoGw>::iterator itVec = itrLong->second.begin();
                for (; itVec != itrLong->second.end(); itVec++)
                {
                    if (itVec->m_strClient == "*")
                    {
                        isStar = true;
                        break;
                    }
                    
                }
                if (itrLong->second.size() > 1 || isStar)
                {
                    LogNotice("[ :%s] key[%s] too many client t_signextno_gw,not support auto extend port",
                    pSessionStruct->m_strPhone.data(),itrLong->first.data());
                    pSessionStruct->m_strGwExtend = itrLong->second[0].appendid;
                    QuerySignMoPort(pSessionStruct, iSeq);
                    return;
                }
                else
                {
                   //// must long match
                    pSessionStruct->m_strSign = itrLong->second[0].sign;
                    pSessionStruct->m_strUserPort = strExtendPort.substr(itrLong->second[0].appendid.length());
                    pSessionStruct->m_strClientId = itrLong->second[0].m_strClient;             }
            }
         
        }
        else
        {
            LogWarn("[ :%s] channelType[%d] is invalid.", pSessionStruct->m_strPhone.data(), pSessionStruct->m_uChannelType);
            InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);
            delete pSessionStruct;
            m_mapSession.erase(iSeq); 
        }
        UInt32 realSmsFrom = 0;
        std::map<string, SmsAccount>::iterator itrAccount = m_SmsAccountMap.find(pSessionStruct->m_strClientId);
        if(itrAccount == m_SmsAccountMap.end())
        {
            LogError("push mo to random http,cant not find smsfrom by account");
            return;
        }
        realSmsFrom = itrAccount->second.m_uSmsFrom ;
        

        TRedisReq* pRedReq = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
        snprintf(tmpKey,250,"mocsid:%s",pSessionStruct->m_strClientId.data());
        strCmd.append(tmpKey);
        pRedReq->m_RedisCmd = strCmd;
        pRedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedReq->m_iSeq = iSeq;
        pRedReq->m_pSender = this;
        pRedReq->m_strSessionID = "get_SMSP_CB";
        pRedReq->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedReq);
        LogDebug("[ :%s] temKey[%s],iSeq[%lu].", pSessionStruct->m_strPhone.data(), strCmd.data(),iSeq);
    }
    else
    {
        TRedisReq* pRed = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
//        snprintf(tmpKey,250,"moport:%d_%s",pUpStream->m_iChannelId,pUpStream->m_strAppendId.data());
        snprintf(tmpKey,250,"moport:%d_%s",pUpStream->m_iChannelId,pSessionStruct->m_strAppendId.data());
        strCmd.append(tmpKey);
        pRed->m_RedisCmd = strCmd;
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_iSeq = iSeq;
        pRed->m_pSender = this;
        pRed->m_strSessionID = "getMO";
        pRed->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);
        LogDebug("[ :%s] temKey[%s],iSeq[%lu].", pSessionStruct->m_strPhone.data(), strCmd.data(),iSeq);
    }

    return;
}

void CDispatchThread::HandleGWMORedisResp(redisReply* pRedisReply,UInt64 uSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", uSeq);
        return;
    }

    string strClientId = "";
    string strSign = "";
    string strSignPort = "";
    string strUserPort = "";
    UInt64 uC2sId = 0;
    UInt32 uSmsFrom = 0;
    string strTmp = "";

    paserReply("clientid", pRedisReply, strClientId);
    paserReply("sign", pRedisReply, strSign);
    paserReply("signport", pRedisReply, strSignPort);
    paserReply("userport", pRedisReply, strUserPort);
    
    paserReply("mourl", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    strTmp = "";
    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());
    
    std::map<string, SmsAccount>::iterator itrAccount = m_SmsAccountMap.find(strClientId);
    if(itrAccount == m_SmsAccountMap.end())
    {
        LogError("not have the clientid[%s]", strClientId.data());
        return;
    }
    
    UInt32 realSmsFrom = itrAccount->second.m_uSmsFrom;
    LogDebug("smsfrom=%u,redisSmsfrom=%u,strSign=%s,strUserPort=%s,uC2sId=%u"
        , realSmsFrom, uSmsFrom,strSign.data(),strUserPort.data(),uC2sId);
    
    if (0 == strSign.compare("null"))
    {
        strSign = "";
    }
    if (0 == strSignPort.compare("null"))
    {
        strSignPort = "";
    }    
    if (0 == strUserPort.compare("null"))
    {
        strUserPort = "";
    }

    itr->second->m_strClientId = strClientId;
    itr->second->m_strSign = strSign;
    itr->second->m_strSignPort = strSignPort;
    itr->second->m_strUserPort = strUserPort;
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsfrom = realSmsFrom;
    map<UInt64,ComponentConfig>::iterator itCom = m_mapComponentConfig.find(itr->second->m_uC2sId);
    if ((realSmsFrom == 6 || realSmsFrom == 7) && (uSmsFrom == realSmsFrom) &&
        (itCom != m_mapComponentConfig.end())&&(itCom->second.m_uComponentSwitch == 1))//https
    {

        InsertMoRecord(uSeq, CHANAUTOORCLIENTFOUNDORERROR);
        HandleRedisComplete(uSeq);
    }
    else
    {
        TRedisReq* pRed = new TRedisReq();
        string strCmd = "";
        strCmd.append("HGETALL  ");
        char tmpKey[250] = {0};
        snprintf(tmpKey,250,"mocsid:%s",strClientId.data());
        strCmd.append(tmpKey);

        LogDebug("[ :%s] temKey[%s].",itr->second->m_strPhone.data(),strCmd.data());
        pRed->m_RedisCmd = strCmd;
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_iSeq = uSeq;
        pRed->m_pSender = this;
        pRed->m_strSessionID = "get_SMSP_CB";
        pRed->m_strKey = tmpKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);    
    }
    
}

void CDispatchThread::handleDBResponseMsg(TMsg* pMsg)
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

void CDispatchThread::InsertToBlankPhone(std::string& strResp)
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

void CDispatchThread::updateBlankPhone(std::string& strResp)
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

void CDispatchThread::HandleRedisResp(TMsg* pMsg)
{
    TRedisResp* pResp = (TRedisResp*)pMsg;
    if ((NULL == pResp->m_RedisReply) 
        || (pResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pResp->m_RedisReply->elements == 0)))
    {
        LogWarn("iSeq[%lu] redisSessionId[%s],query redis is failed",pMsg->m_iSeq,pMsg->m_strSessionID.data());

          
        if ("getReportInfo" == pMsg->m_strSessionID )
        {
            HandleRedisGetReportInfoResp( pResp );          
            freeReply(pResp->m_RedisReply);
            return;
        }


        if (NULL != pResp->m_RedisReply)
        {
            LogWarn("m_RedisReply type[%d], m_RedisCmd[%s], m_uReqTime[%lu]", pResp->m_RedisReply->type, pResp->m_RedisCmd.data(), pResp->m_uReqTime);
            freeReply(pResp->m_RedisReply);
        }
        if ("check blankphone" == pMsg->m_strSessionID)
        {
            InsertToBlankPhone(pResp->m_string);
            return;
        }

        SessionMap::iterator itr = m_mapSession.find(pResp->m_iSeq);
        if (itr == m_mapSession.end())
        {
            LogError("iSeq[%lu] is not find m_mapSession.", pResp->m_iSeq);
            return;
        }
        

        if (("getMO" == pMsg->m_strSessionID) )  ///for auto channel get clientId again in account map
        {
            LogWarn("this is not find in redis mo.");
            if (false == getUserBySp(pResp->m_iSeq))
            {
                LogWarn("[ :%s] iSeq[%lu] is not find in t_user_gw",itr->second->m_strPhone.data(),pResp->m_iSeq);
                InsertMoRecord(pResp->m_iSeq, CHANAUTOORCLIENTFOUNDORERROR);
                delete itr->second;
                m_mapSession.erase(pResp->m_iSeq); 
                return;
            }
            TRedisReq* pRed = new TRedisReq();
            string strCmd = "";
            strCmd.append("HGETALL  ");
            char tmpKey[250] = {0};
            snprintf(tmpKey,250,"mocsid:%s",itr->second->m_strClientId.data());
            strCmd.append(tmpKey);

            LogDebug("[ :%s] temKey[%s].",itr->second->m_strPhone.data(),strCmd.data());
            pRed->m_RedisCmd = strCmd;
            pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRed->m_iSeq = pResp->m_iSeq;
            pRed->m_pSender = this;
            pRed->m_strSessionID = "get_SMSP_CB";
            pRed->m_strKey = tmpKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRed);
            return;
        }
        if(("get_SMSP_CB" == pMsg->m_strSessionID))
        {
            LogWarn("this is not find c2s_id. push mo to random c2s");
            PushMoToRandC2s(pMsg->m_iSeq);          
            InsertMoRecord(pMsg->m_iSeq, CHANAUTOORCLIENTFOUNDORERROR);

        }       
        if ("get_sign_mo_port" == pMsg->m_strSessionID)
        {
            InsertMoRecord(pResp->m_iSeq, CHANGWCLIENTNOTFOUND);
        }
            
        SAFE_DELETE(itr->second);
        m_mapSession.erase(itr);
        
        return;
    }

    
    if ("getReportInfo" == pMsg->m_strSessionID)
    {
        HandleRedisGetReportInfoResp( pResp );
    }
    else if ("getKey" == pMsg->m_strSessionID)
    {
        HandleRedisGetKeyResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("getStatus" == pMsg->m_strSessionID)
    {
        HandleRedisGetStatusResp( pResp->m_RedisReply,pResp->m_iSeq );
    }
    else if ("getMO" == pMsg->m_strSessionID)
    {
        HandleRedisGetMoResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("get_SMSP_CB" == pMsg->m_strSessionID)
    {
        HandleRedisGetSmspCbResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("get_sign_mo_port" == pMsg->m_strSessionID)
    {
        HandleGWMORedisResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else if ("check blankphone" == pMsg->m_strSessionID)
    {
        if (REDIS_REPLY_STRING == pResp->m_RedisReply->type)
        {
            string strOut = pResp->m_RedisReply->str;
            if (!strOut.empty())
            {
                if (strOut.find("blank&") == string::npos)
                {
                    InsertToBlankPhone(pResp->m_string);
                }
                else
                {
                    updateBlankPhone(pResp->m_string);
                }
            }
            
        }
        
    }
    else
    {
        LogError("this Redis type[%s] is invalid.",pMsg->m_strSessionID.data());
        return;
    }

    freeReply(pResp->m_RedisReply);
    return;

}

///////////////////////////////////////////////////////////////////////////////
bool CDispatchThread::getUserBySp(UInt64 uSeq)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {
        LogWarn("iSeq[%lu] is not find in m_mapSession.", uSeq);
        return false;
    }


    string strExtendPort = itr->second->m_strAppendId;

    if (0 == itr->second->m_uChannelType)
    {
        AccountMap::iterator itGw = m_SmsAccountMap.begin();
        AccountMap::iterator itGwLong = m_SmsAccountMap.end();
        for (; itGw != m_SmsAccountMap.end(); ++itGw)
        {
            if (0 == strExtendPort.compare(0,itGw->second.m_strExtNo.length(),itGw->second.m_strExtNo))
            {
                if (itGwLong == m_SmsAccountMap.end())
                {
                    itGwLong = itGw;
                }
                else
                {
                    if (itGwLong->second.m_strExtNo.length() < itGw->second.m_strExtNo.length())
                    {
                        itGwLong = itGw;
                    }
                }
            }
        }

        if (itGwLong == m_SmsAccountMap.end())
        {
            LogWarn("[ :%s] extendport[%s] is not find match in userGw table.", itr->second->m_strPhone.data(),strExtendPort.data());
            return false;
        }

        UInt32 uSignPortLen = itGwLong->second.m_uSignPortLen;
        itr->second->m_strClientId = itGwLong->second.m_strAccount;
        itr->second->m_strSignPort = strExtendPort.substr(itGwLong->second.m_strExtNo.length(),uSignPortLen);

        if (strExtendPort.length() >= itGwLong->second.m_strExtNo.length()+uSignPortLen)
        {
            itr->second->m_strUserPort = strExtendPort.substr(itGwLong->second.m_strExtNo.length()+uSignPortLen);
        }
        else
        {
            LogWarn("[ :%s] 2ExtendPort[%s],length[%d].",itr->second->m_strPhone.data(),strExtendPort.data(),strExtendPort.length());
            return false;
        }

        return true;

    }
    else if (3 == itr->second->m_uChannelType)
    {
        std::multimap<UInt32,ChannelExtendPort>::iterator itPort = m_mapChannelExt.begin();
        std::multimap<UInt32,ChannelExtendPort>::iterator itPortLong = m_mapChannelExt.end();

        for (; itPort != m_mapChannelExt.end(); itPort++)
        {
            if (itPort->first == itr->second->m_uChannelId)
            {
                if (0 == strExtendPort.compare(0,itPort->second.m_strExtendPort.length(),itPort->second.m_strExtendPort))
                {
                    if (itPortLong == m_mapChannelExt.end())
                    {
                        itPortLong = itPort;
                    }
                    else
                    {
                        if (itPortLong->second.m_strExtendPort.length() < itPort->second.m_strExtendPort.length())
                        {
                            itPortLong = itPort;
                        }
                    }
                }
            }
        }

        if (itPortLong == m_mapChannelExt.end())
        {
            LogWarn("[ :%s] channelid[%d] extendport[%s] is not find match in channel_extendport table.",
                itr->second->m_strPhone.data(),itr->second->m_uChannelId,strExtendPort.data());
            return false;
        }

        itr->second->m_strClientId = itPortLong->second.m_strClientId;
        ////UserGWMap::iterator itGwExt = m_mapUserGw.find(itPortLong->second.m_strClientId);
        ////if (itGwExt == m_mapUserGw.end())
        ////{
            ////LogError("clientid[%s] is not find in userGw.",itPortLong->second.m_strClientId.data());
            ////return false;
        ////}

        ////UInt32 uSignPortLen = itGwExt->second.m_uSignPortLen;

        ////itr->second->m_strSignPort = strExtendPort.substr(itPortLong->second.m_strExtendPort.length(),uSignPortLen);

        if (strExtendPort.length() >= itPortLong->second.m_strExtendPort.length())
        {
            itr->second->m_strUserPort = strExtendPort.substr(itPortLong->second.m_strExtendPort.length());
        }
        else
        {
            LogWarn("[ :%s] 2ExtendPort[%s],length[%d].",itr->second->m_strPhone.data(),strExtendPort.data(),strExtendPort.length());
            return false;
        }

        return true;
        
    }
    else
    {
        LogError("channelType[%d] is invalid.",itr->second->m_uChannelType);
        return false;
    }
}

void CDispatchThread::HandleRedisGetSmspCbResp(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    ////HMSET mocsid:clientid c2sid* smsfrom*
    string strTmp = "";
    UInt32 uSmsFrom = 0;
    UInt64 uC2sId = 0;

    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());

    strTmp = "";
    paserReply("c2sid", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsfrom = uSmsFrom;

    InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);

    HandleRedisComplete(iSeq);
    
    return;
}

void CDispatchThread::HandleRedisGetMoResp(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    ////redis HMSET moport:channelid_showphone clientid* sign* signport* userport* mourl* smsfrom*
    string strClientId = "";
    string strSign = "";
    string strSignPort = "";
    string strUserPort = "";
    UInt64 uC2sId = 0;
    UInt32 uSmsFrom = 0;
    string strTmp = "";

    paserReply("clientid", pRedisReply, strClientId);
    paserReply("sign", pRedisReply, strSign);
    paserReply("signport", pRedisReply, strSignPort);
    paserReply("userport", pRedisReply, strUserPort);
    
    paserReply("mourl", pRedisReply, strTmp);
    uC2sId = atol(strTmp.data());

    strTmp = "";
    paserReply("smsfrom", pRedisReply, strTmp);
    uSmsFrom = atoi(strTmp.data());

    if (0 == strSign.compare("null"))
    {
        strSign = "";
    }
    if (0 == strSignPort.compare("null"))
    {
        strSignPort = "";
    }    
    if (0 == strUserPort.compare("null"))
    {
        strUserPort = "";
    }

    itr->second->m_strClientId = strClientId;
    itr->second->m_strSign = strSign;
    itr->second->m_strSignPort = strSignPort;
    itr->second->m_strUserPort = strUserPort;
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uSmsfrom = uSmsFrom;

    InsertMoRecord(iSeq, CHANAUTOORCLIENTFOUNDORERROR);

    HandleRedisComplete(iSeq);

}

void CDispatchThread::HandleRedisGetKeyResp(redisReply* pRedisReply,UInt64 iSeq)
{
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    string strKeys = "";
    strKeys = pRedisReply->element[0]->str;

    std::string::size_type pos = strKeys.find_last_of("_");
    if (pos != std::string::npos)
    {
        itr->second->m_strPhone = strKeys.substr(pos+1);
    }

    LogDebug("***getKey[%s],iSeq[%lu],phone[%s].",strKeys.data(),iSeq,itr->second->m_strPhone.data());
    
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


void CDispatchThread::HandleRedisGetReportInfoResp( TRedisResp *pRedisRsp )
{
    redisReply* pRedisReply = pRedisRsp->m_RedisReply;
    UInt64      iSeq = pRedisRsp->m_iSeq;
    if ( NULL == pRedisReply )
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    /** 如果没有查到，需要再查询一次，
          用于解决中途修改了组包属性
     **/
    if( pRedisReply->type == REDIS_REPLY_NIL
        || (pRedisReply->type == REDIS_REPLY_ARRAY 
         && pRedisReply->elements == 0 ))
    {
        SessionStruct* pSession = itr->second;
        string strkey = "";

        if( pSession->m_uClusterType != pSession->m_uClusterGetType )
        {
            return ;
        }

        if( pSession->m_uClusterType != 1 )
        {
            strkey.append("cluster_sms:");          
            pSession->m_uClusterGetType = 1;
        }
        else
        {   
            strkey.append("sendmsgid:");            
            pSession->m_uClusterGetType = 0;
        }

        strkey.append(Comm::int2str( pSession->m_uChannelId));
        strkey.append("_");
        strkey.append(pSession->m_strChannelSmsId );

        if( pSession->m_uClusterType != 1 ){
            strkey.append("_");
            strkey.append(pSession->m_strPhone);
        }
       
        TRedisReq* pRed = new TRedisReq();      
        pRed->m_strKey = strkey;
        pRed->m_RedisCmd = "HGETALL ";
        pRed->m_RedisCmd.append(strkey );
        pRed->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRed->m_iSeq = iSeq;
        pRed->m_pSender = this;
        pRed->m_strSessionID = "getReportInfo";
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pRed );
        LogWarn("[ Cluster Redis] Pre Exec Redis Cmd[%s] Fail, Now Try Cmd[%s]", 
                pRedisRsp->m_RedisCmd.data(), pRed->m_RedisCmd.data());
        return ;
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
    bool   bResend = false;// false : not resend ,ture: resend not return report to client
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
    itr->second->m_uSmsfrom = uSmsFrom;
    itr->second->m_strPhone.assign(strPhone);
    itr->second->m_uC2sId = uC2sId;
    itr->second->m_uChannelCnt = uChannelCnt;
    itr->second->m_uLongSms = (uLongSms > 1) ? 1 : 0;
    
    //check failed resend   
    if(SMS_STATUS_CONFIRM_SUCCESS != UInt32(itr->second->m_iStatus))
    {
        bResend = CheckFailedResend(pRedisReply,itr->second);
    }
    ////update database
    UpdateRecord(strSendUuid,strSendtime,uSubmitDate,itr->second);

    bool result = false;
    if(UInt32(itr->second->m_iStatus) == SMS_STATUS_CONFIRM_SUCCESS)
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
        
        if(SendDay.compare(to_day) == 0 && time_now - uSubmitDate <= m_uReportFailedSendTimeInterval) ///if this message is sended at today and nowtime-sendtime<=interval,should count the faild time
        {
            countReportFailed(itr->second->m_uChannelId, result, to_day, time_now);
        }
        else
        {
            LogDebug("to_day = %s, channelid = %d, time_now = %uL, uSubmitDate = %uL", to_day.data(), itr->second->m_uChannelId, time_now, uSubmitDate);
        }
        
    }

    ////Del redis
    TRedisReq* pDel = new TRedisReq();
    string strDel = "";
    char cChannelId[25] = {0};

    if( itr->second->m_uClusterType == 1 ){
        strDel.append("cluster_sms:");
    }else{      
        strDel.append("sendmsgid:");
    }
    
    snprintf(cChannelId,25,"%u",itr->second->m_uChannelId);
    strDel.append(cChannelId);
    strDel.append("_");
    strDel.append(itr->second->m_strChannelSmsId);

    if( itr->second->m_uClusterType == 1){
        strDel.append("_");
        strDel.append(strPhone);
    }

    pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDel->m_RedisCmd.assign("DEL  ");
    pDel->m_RedisCmd.append(strDel);
    pDel->m_strKey = strDel;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);
    
    if ( uLongSms > 1 )  ////access long msg
    {
        TRedisReq* pRedis = new TRedisReq();
        string strKey = "";
        strKey.append("sendsms:");
        strKey.append(strSmsId);
        strKey.append("_");
        strKey.append(strPhone);
        pRedis->m_RedisCmd = "GET " + strKey;
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_iSeq = iSeq;
        pRedis->m_pSender = this;
        pRedis->m_strSessionID = "getStatus";
        pRedis->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
        
        TRedisReq* pDel = new TRedisReq();
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_RedisCmd.assign("DEL "+strKey);
        pDel->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);
    }
    else
    {
        string innerError = itr->second->m_strInnerErrorcode + "|";
        if (UInt32(itr->second->m_iStatus) == SMS_STATUS_CONFIRM_FAIL && m_bBlankPhoneSwitch 
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

        if(false == bResend)
        {
            HandleRedisComplete(iSeq);
        }
        else
        {
            delete itr->second;
            m_mapSession.erase(itr);
        }
    }        
   
    return;
}

void CDispatchThread::HandleRedisComplete(UInt64 iSeq)
{
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }
    
    map<UInt64,ComponentConfig>::iterator itrCom = m_mapComponentConfig.find(itr->second->m_uC2sId);
    if (itrCom == m_mapComponentConfig.end())
    {
        LogError("direct report smsid:%s,phone:%s,c2sid:%lu is not find in m_mapComponentConfig.",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itr->second->m_uC2sId);
        
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    map<UInt64,MqConfig>::iterator itrMq =  m_mapMqConfig.find(itrCom->second.m_uMqId);
    if (itrMq == m_mapMqConfig.end())
    {
        LogError("direct report msid:%s,phone:%s,mqid:%lu is not find in m_mapMqConfig.",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
        
        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    string strExchange = itrMq->second.m_strExchange;
    string strRoutingKey = itrMq->second.m_strRoutingKey;

    string strData = "";
    string strClientId = "";
    
    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
    {
        strClientId = itr->second->m_strClientId;
    }

    if ("1" == itr->second->m_strType) ////report
    {
        ////type
        strData.append("type=");
        strData.append("1");
        ////clientid
        strData.append("&clientid=");
        strData.append(strClientId);
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
        strData.append(Comm::int2str(itr->second->m_lReportTime));  
        ////smsfrom 
        strData.append("&smsfrom=");
        strData.append(Comm::int2str(itr->second->m_uSmsfrom)); 
        
        LogDebug("==push report==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());
    }
    else ////mo
    {
        ////type
        strData.append("type=");
        strData.append("2");
        ////clientid
        strData.append("&clientid=");
        strData.append(strClientId);
        ////channelid
        strData.append("&channelid=");
        strData.append(Comm::int2str(itr->second->m_uChannelId));
        ////phone
        strData.append("&phone=");
        strData.append(itr->second->m_strPhone);
        ////content
        strData.append("&content=");
        strData.append(Base64::Encode(itr->second->m_strContent));
        ////moid
        strData.append("&moid=");
        strData.append(itr->second->m_strMoId);
        ////time
        UInt64 uTime = time(NULL);
        strData.append("&time=");
        strData.append(Comm::int2str(uTime));
        ////sign
        strData.append("&sign=");
        strData.append(Base64::Encode(itr->second->m_strSign));
        ////signport
        strData.append("&signport=");
        strData.append(itr->second->m_strSignPort);
        ////userport
        strData.append("&userport=");
        strData.append(itr->second->m_strUserPort);
        ////smsfrom
        strData.append("&smsfrom=");
        strData.append(Comm::int2str(itr->second->m_uSmsfrom));
        ////showphone
        strData.append("&showphone=");
        strData.append(itr->second->m_strAppendId);

        LogDebug("==push mo==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;       
    pMQ->m_strRoutingKey = strRoutingKey;   
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pRtAndMoProducerThread->PostMsg(pMQ);

    delete itr->second;
    m_mapSession.erase(itr);

    return;
}

void CDispatchThread::HandleRedisGetStatusResp(redisReply* pRedisReply,UInt64 iSeq)
{   
    if (NULL == pRedisReply)
    {
        LogError("pRedisReply is NULL.");
        return;
    }
    
    SessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }
    string innerError = itr->second->m_strInnerErrorcode + "|";
    if (UInt32(itr->second->m_iStatus) == SMS_STATUS_CONFIRM_FAIL && m_bBlankPhoneSwitch 
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

///////////////////////////////////////////////////////////////////////////////

void CDispatchThread::UpdateRecord(string& strSmsUuid,string& strSendTime,UInt64 uSubmitDate,SessionStruct* psession)
{
    char sql[2048]  = {0};  
    char szreachtime[64] = {0};

    struct tm Reportptm = {0};
    localtime_r((time_t*)&psession->m_lReportTime,&Reportptm);

    string strRecordDate = strSendTime.substr(0,8);

    if(psession->m_lReportTime != 0 )
    {
        strftime(szreachtime, sizeof(szreachtime), "%Y%m%d%H%M%S", &Reportptm);
    }
    int duration = psession->m_lReportTime -uSubmitDate;
    if(duration < 0 || uSubmitDate <= 0)
    {
        duration = 0;
    }

    if(0 == psession->m_uFailedResendTimes)
    {
        if (psession->m_strInnerErrorcode.empty())
        {
            snprintf(sql,2048,
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d'"
            " where smsuuid='%s' and date='%s';",
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
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d'"
            ",failed_resend_flag='%u',failed_resend_times='%u' where smsuuid='%s' and date='%s';",
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
            "UPDATE t_sms_record_%d_%s SET state='%d',reportdate='%s',report='%s',recvreportdate='%s',duration='%d'"
            ",failed_resend_flag='%u',failed_resend_times='%u',innerErrorcode='%s' where smsuuid='%s' and date='%s';",
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
    g_pMQDbProducerThread->PostMsg(pMQ);

    MONITOR_INIT( MONITOR_CHANNEL_SMS_REPORT );
    MONITOR_VAL_SET("clientid", psession->m_strClientId);
    MONITOR_VAL_SET_INT("channelid", psession->m_uChannelId);
    MONITOR_VAL_SET_INT("smsfrom", psession->m_uSmsfrom);
    MONITOR_VAL_SET("smsid", psession->m_strSmsId);
    MONITOR_VAL_SET("phone", psession->m_strPhone);
    MONITOR_VAL_SET("smsuuid", strSmsUuid);
    MONITOR_VAL_SET_INT("state", psession->m_iStatus);
    MONITOR_VAL_SET("reportcode", psession->m_strDesc);
    MONITOR_VAL_SET("reportdesc", psession->m_strReportDesc);
    MONITOR_VAL_SET("reportdate", szreachtime);
    MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));
    MONITOR_VAL_SET_INT("costtime", duration );
    MONITOR_VAL_SET_INT("component_id", g_uComponentId );
    string strTable = "t_sms_record_";
    strTable.append(Comm::int2str(psession->m_uChannelIdentify));
    strTable.append("_" + strRecordDate );
    MONITOR_VAL_SET("dbtable", strTable );
    MONITOR_PUBLISH( g_pMQMonitorPublishThread );

}

void CDispatchThread::PushMoToRandC2s(UInt64 uSeq)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {   
        LogError("uSeq[%ld] is not find in mapSession.",uSeq);
        return;
    }
    
    UInt32 realSmsFrom = 0;
    std::map<string, SmsAccount>::iterator itrAccount = m_SmsAccountMap.find(itr->second->m_strClientId);
    if(itrAccount == m_SmsAccountMap.end())
    {
        LogError("push mo to random http,cant not find smsfrom by account");
        return;
    }
    realSmsFrom = itrAccount->second.m_uSmsFrom ;

    map<UInt64,MqConfig>::iterator itrMq;
    map<UInt64,ComponentConfig>::iterator itrCom = m_mapComponentConfig.begin();
    if(2 == realSmsFrom || 3 == realSmsFrom ||4 == realSmsFrom ||5 == realSmsFrom) //cmpp,smgp,sgip,smpp
    {
        for(;itrCom != m_mapComponentConfig.end();itrCom ++)
        {
            if(itrCom->second.m_strComponentType.compare("00") == 0 && itrCom->second.m_uComponentSwitch == 1)  //find c2s
            {
                LogDebug("*********weilu_test: c2s id is %ld mq id is %ld",itrCom ->first,itrCom->second.m_uMqId);
                 itrMq  =   m_mapMqConfig.find(itrCom->second.m_uMqId);
                if (itrMq == m_mapMqConfig.end())
                {
                    LogWarn("push mo ==smsid:%s,phone:%s,mqid:%lu is not find in m_mapMqConfig.",
                        itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
                    return;
                }
                break;
            }
        }
    }
    else if(6 == realSmsFrom || 7 == realSmsFrom) //http ,http2
    {
        for(;itrCom != m_mapComponentConfig.end();itrCom ++)
        {
            if(itrCom->second.m_strComponentType.compare("08") == 0 && itrCom->second.m_uComponentSwitch == 1)  //find http 
            {
                LogDebug("*********weilu_test: http id is %ld mq id is %ld",itrCom ->first,itrCom->second.m_uMqId);
                 itrMq  =   m_mapMqConfig.find(itrCom->second.m_uMqId);
                if (itrMq == m_mapMqConfig.end())
                {
                    LogWarn("push mo ==smsid:%s,phone:%s,mqid:%lu is not find in m_mapMqConfig.",
                        itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
                    return;
                }
                break;
            }
        }
    }
    else
    {
        LogWarn("account %s smsfrom  is %d ",itrAccount->second.m_strAccount.data(),realSmsFrom);
        return;
    }
    
    if(itrCom == m_mapComponentConfig.end())
    {
        LogError("push mo smsid:%s,phone:%s,mqid:%lu is not find in m_mapMqConfig.",
            itr->second->m_strSmsId.data(),itr->second->m_strPhone.data(),itrCom->second.m_uMqId);
        return;

    }
    
    string strExchange = itrMq->second.m_strExchange;
    string strRoutingKey = itrMq->second.m_strRoutingKey;

    string strData = "";
    string strClientId = "";
    
    // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
    if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
    {
        strClientId = itr->second->m_strClientId;
    }
    
    strData.append("type=");
    strData.append("2");
    ////clientid
    strData.append("&clientid=");
    strData.append(strClientId);
    ////channelid
    strData.append("&channelid=");
    strData.append(Comm::int2str(itr->second->m_uChannelId));
    ////phone
    strData.append("&phone=");
    strData.append(itr->second->m_strPhone);
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(itr->second->m_strContent));
    ////moid
    if(itr->second->m_strMoId.empty())
    {
        string strUUID = getUUID();
        itr->second->m_strMoId = strUUID;
    }
    strData.append("&moid=");
    strData.append(itr->second->m_strMoId);
    ////time
    UInt64 uTime = time(NULL);
    strData.append("&time=");
    strData.append(Comm::int2str(uTime));
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(itr->second->m_strSign));
    ////signport
    strData.append("&signport=");
    strData.append(itr->second->m_strSignPort);
    ////userport
    strData.append("&userport=");
    strData.append(itr->second->m_strUserPort);
    ////smsfrom
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(realSmsFrom)); 
    ////showphone
    strData.append("&showphone=");
    strData.append(itr->second->m_strAppendId);

    LogDebug("==push mo==exchange:%s,routingkey:%s,data:%s.",strExchange.data(),strRoutingKey.data(),strData.data());  
    
    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = strExchange;       
    pMQ->m_strRoutingKey = strRoutingKey;   
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pRtAndMoProducerThread->PostMsg(pMQ);

    return;     
}

void CDispatchThread::InsertMoRecord(UInt64 uSeq, int type)
{
    SessionMap::iterator itr = m_mapSession.find(uSeq);
    if (itr == m_mapSession.end())
    {   
        LogError("uSeq[%ld] is not find in mapSession.",uSeq);
        return;
    }
    
    char sql[8192]  = {0};
    time_t now = time(NULL);
    struct tm timeinfo = {0};
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {   
        localtime_r((time_t*)&now,&timeinfo);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &timeinfo);
    }

    string strUUID = getUUID();

    itr->second->m_strMoId = strUUID;
    
    char contentSQL[4096] = { 0 };
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    if(MysqlConn != NULL)
    {
        mysql_real_escape_string(MysqlConn, contentSQL, itr->second->m_strContent.data(), itr->second->m_strContent.length());
    }
    
    char table[32];
    memset(table, 0, sizeof(table));
    if (type == CHANAUTOORCLIENTFOUNDORERROR)
    {
        string strClientId = "";
        // 当账户为注销账户时，不对session账户赋值 add by shijh 2018/06/20
        if (BusType::CLIENT_STATUS_LOGOFF != GetAcctStatus(itr->second->m_strClientId))
        {
            strClientId = itr->second->m_strClientId;
        }
        snprintf(table, sizeof(table) - 1, "t_sms_record_molog_%04d%02d",timeinfo.tm_year + 1900, timeinfo.tm_mon + 1);
        snprintf(sql,8192,"insert into %s(moid,channelid,receivedate,phone,tophone,content,channelsmsid,"
            "clientid,mofee,chargetotal,paytime) values('%s','%d','%s','%s','%s','%s','%s','%s','%d','%d','%s');",
            table,
            strUUID.data(),
            itr->second->m_uChannelId,
            szreachtime,
            itr->second->m_strPhone.substr(0, 20).data(),
            itr->second->m_strAppendId.substr(0, 20).data(),
            contentSQL,
            "1",
            strClientId.data(),
            0,
            1,
            szreachtime);
    }
    else if (type == CHANGWCLIENTNOTFOUND)
    {
        snprintf(table, sizeof(table) - 1, "t_sms_record_sign_molog");
        snprintf(sql,8192,"insert into %s(mo_id,channel_id,receive_time,phone,to_phone,content,sign_port,state,"
        "update_time) values('%s','%d','%s','%s','%s','%s','%s','%d','%s');",
        table,
        itr->second->m_strMoId.data(),
        itr->second->m_uChannelId,
        szreachtime,
        itr->second->m_strPhone.substr(0, 20).data(),
        itr->second->m_strAppendId.substr(0, 20).data(),
        contentSQL,
        ( true == itr->second->m_strGwExtend.empty()) ? "NULL" : itr->second->m_strGwExtend.data(),
        //m_ChannelMap[itr->second->m_uChannelId].m_strMoPrefix.data(),
        0,
        szreachtime);
    }
    else
    {
        LogError("channel type error[%d]", type);
    }

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(g_strMQDBExchange);
    pMQ->m_strRoutingKey.assign(g_strMQDBRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append(strUUID);
    g_pMQDbProducerThread->PostMsg(pMQ);

    return;
}

UInt32 CDispatchThread::GetSessionMapSize()
{
    return m_mapSession.size();
}

UInt32 CDispatchThread::GetChannelIndustrytype(int ChannelId)
{
    ChannelMap_t::iterator iter = m_ChannelMap.find(ChannelId);

    if(iter != m_ChannelMap.end())
    {
        return iter->second.industrytype;
    }
    else
    {
        return 99;
    }
}

void CDispatchThread::countReportFailed(UInt32 uChannelId, bool bResult,string toDay, UInt64 uNowTime)
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
            //  LogDebug("channel = %d, startTime = %uL, uNowTime = %uL, reportListTime = %s", uChannelId, startTime, uNowTime, iter->second.m_strReportTimeList.data());
                if (uNowTime - startTime <= m_uReportFailedSendTimeInterval)
                {
                    char temp[15] = {0};
                    snprintf(temp,15,"%u",uChannelId);
                    LogWarn("channelid[%d] continue report failed num[%d] is over systemValue[%d] in [%d] minutes"
                        ,uChannelId,uNum,m_uReportFailedAlarmValue, m_uReportFailedSendTimeInterval/60);

                    std::string strChinese = "";
                    strChinese = DAlarm::getAlarmStr2(uChannelId, GetChannelIndustrytype(uChannelId), uNum,m_uReportFailedAlarmValue,m_uReportFailedSendTimeInterval/60);
                    Alarm(CHNN_STATUS_REPORT_FAILED_ALARM,temp,strChinese.data());
                }
                
                LogDebug("====report failed count==== channel %d  continue faild %d mapsize is %d",uChannelId,iter->second.m_uTime,m_mapReportFailedAlarm.size());

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


void CDispatchThread::HandleTimeOutMsg(TMsg* pMsg)
{
    LogWarn("iSeq[%lu,sessionId[%s]timeout]",pMsg->m_iSeq, pMsg->m_strSessionID.data());    
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
bool CDispatchThread::FailedReSendToReBack( SessionStruct* pReportInfo )
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
        LogError("[%s:%s]phoneType:%u is invalid.",pReportInfo->m_strSmsId.c_str(),
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

bool CDispatchThread::CheckFailedResend(redisReply* pRedisReply,SessionStruct* pReportInfo)
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
    std::map<std::string,SmsAccount>::iterator iterAccount = m_SmsAccountMap.find( pReportInfo->m_strClientId);
    if( (iterAccount != m_SmsAccountMap.end()) && (1 == iterAccount->second.m_uFailedResendFlag) )
    {
        //continue
    }
    else
    {
        LogDebug("[%s:%s],account[%s] switch is not open!",pReportInfo->m_strSmsId.c_str(),pReportInfo->m_strPhone.c_str(),
            pReportInfo->m_strClientId.c_str());
        return false;
    }

    
    ChannelMap_t::iterator iterChannel = m_ChannelMap.find((int)pReportInfo->m_uChannelId);
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
        LogWarn("[%s:%s] resend timeout strCsTime[%s]",pReportInfo->m_strSmsId.c_str(),
            pReportInfo->m_strPhone.c_str(),strCsTime.c_str());
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
    strMqData.append("&smsfrom=").append(Comm::int2str(pReportInfo->m_uSmsfrom));
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

// 获取账户状态 1：注册完成，5：冻结，6：注销，7：锁定
int CDispatchThread::GetAcctStatus(const string& strClientId)
{
    int nRet = -1;
    std::map<string, SmsAccount>::iterator itAcct = m_SmsAccountMap.find(strClientId);
    if (itAcct != m_SmsAccountMap.end())
    {
        nRet = itAcct->second.m_uStatus;
    }   
    return nRet;
}

