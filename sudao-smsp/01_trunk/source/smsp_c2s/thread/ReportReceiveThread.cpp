#include "ReportReceiveThread.h"
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"
#include "Comm.h"
#include "Channellib.h"
#include "MqManagerThread.h"
#include "commonmessage.h"

ReportReceiveThread::ReportReceiveThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
    m_strUnsubscribe = ";TD;T;N;";
}

ReportReceiveThread::~ReportReceiveThread()
{
    //m_pServerSocket->Close();
}

bool ReportReceiveThread::Init()
{
    m_pSnManager = new SnManager();
    if(NULL == m_pSnManager)
    {
        LogError("new SnManager() is failed.");
        return false;
    }

    if(false == CThread::Init())
    {
        return false;
    }

    m_pLinkedBlockPool = new LinkedBlockPool();

    g_pRuleLoadThread->getSmsClientAndSignPortMap(m_mapClientSignPort);
    g_pRuleLoadThread->getSmsAccountMap(m_mapAccount);

    std::map<std::string, std::string> mapSysPara;
    g_pRuleLoadThread->getSysParamMap(mapSysPara);
    GetSysPara(mapSysPara);

    g_pMqManagerThread->regMqConsumeCallback(this, CONSUME_FROM_MQ_C2S_IO);


    return true;
}

void ReportReceiveThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            usleep(g_uSecSleep);
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}

void ReportReceiveThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "UNSUBSCRIBE_ORDER";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        std::string strTmp = iter->second;
        vector<string>vec;
        Comm::split(strTmp, ";", vec);
        if (vec.size() >= 3)
        {
            Comm::Toupper(strTmp);
            m_strUnsubscribe = ";" + strTmp + ";";
        }
        else
        {
            LogError("system parameter(%s) set less.", strSysPara.c_str());
            break;
        }

    }
    while (0);

    LogNotice("System parameter(%s) value(%s).",
        strSysPara.c_str(),
        m_strUnsubscribe.data());

}

void ReportReceiveThread::InsertPhoneToBlackList(std::string &strPhone, std::string &strClient
    , std::string &oldStr, int smstype, int isSet,int isSetClass, UInt64 uClass)
{
    if(strClient.empty())
    {
        LogWarn("phone[%s] strClient[%s] is error",strPhone.data(), strClient.data());
        return;
    }
    time_t now = time(NULL);
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {
        struct tm tmp = {0};
        localtime_r((time_t*)&now,&tmp);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &tmp);
    }

    char chSql[2048] = { 0 };
    char chSqllaxin[2048] = { 0 };
    if (isSetClass == 1)
    {
        snprintf(chSql,sizeof(chSql),"update t_sms_blacklist_%d set category_id=%lu,update_date=%s,remarks='add by smsp' where phone='%s';",
            Comm::getHash(strPhone),
            uClass,
            szreachtime,
            strPhone.data());

    }
    else if (isSetClass == 0)//插入
    {
        snprintf(chSql,sizeof(chSql),"insert into t_sms_blacklist_%d(phone, category_id, remarks, update_date)values('%s','%lu','%s','%s');",
            Comm::getHash(strPhone),
            strPhone.data(),
            (UInt64)models::LAXINBLACKLIST,
            "add by smsp",
            szreachtime); 
    }

    if (isSet == 1)
    {
        snprintf(chSqllaxin,sizeof(chSqllaxin),"update t_sms_blacklist_unsubscribe set smstypes=%d,update_date=%s,state=1,remarks='add by smsp' where clientid='%s' and phone='%s';",
            smstype,
            szreachtime,
            strClient.data(),
            strPhone.data());
    }
    else  if (isSet == 0)
    {
        snprintf(chSqllaxin,sizeof(chSqllaxin),"insert into t_sms_blacklist_unsubscribe(clientid, phone, smstypes, state, remarks, update_date)values('%s','%s','%d','%d','%s','%s');",
            strClient.data(),
            strPhone.data(),
            smstype,
            1,
            "add by smsp",
            szreachtime); 
       
    }

    LogNotice("chSql, insert blacklist.[%s][%s]", chSql, chSqllaxin);
    if (strlen(chSql) > 0)
    {
        TDBQueryReq* pMsg = new TDBQueryReq();
        pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        pMsg->m_SQL.assign(chSql);
        g_pDisPathDBThreadPool->PostMsg(pMsg);
    }
    if (strlen(chSqllaxin) > 0)
    {
        TDBQueryReq* pMsg = new TDBQueryReq();
        pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        pMsg->m_SQL.assign(chSqllaxin);
        g_pDisPathDBThreadPool->PostMsg(pMsg);
    }
    TRedisReq* pRedis = new TRedisReq();
    string strBlackListKey;
    char strAppendValue[32] = {0};
    snprintf(strAppendValue,sizeof(strAppendValue),"%s:%d&c:%lu&", strClient.data(), smstype, uClass);
    strBlackListKey = "black_list:" + strPhone + " ";
    if (0 == isSet && 0 == isSetClass)
    {
         pRedis->m_RedisCmd = "APPEND " + strBlackListKey + strAppendValue;
    }
    else
    {
        pRedis->m_RedisCmd = "set " + strBlackListKey  + oldStr + strAppendValue;
    }
    pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
    pRedis->m_strKey = strBlackListKey;
    LogNotice("[%s] insert redis blacklist.[%s]", strPhone.data(), pRedis->m_RedisCmd.data());
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
    return;
}


void ReportReceiveThread::HandleBlacklistRedisResp(redisReply* pRedisReply,UInt64 uSeq)
{
    BlacklistMap::iterator itr = m_mapBlacklist.find(uSeq);
    if (itr == m_mapBlacklist.end())
    {
        LogElk("uSeq[%ld] is not find in m_mapBlacklist.",uSeq);
        return;
    }
    string strClientId = itr->second->m_strClientId;
    string strPhone = itr->second->m_strPhone;
    SAFE_DELETE(itr->second);
    m_mapBlacklist.erase(itr);
    
    set<string>::iterator itSet = m_setBlackList.find(strClientId + "_" + strPhone);
    if (itSet != m_setBlackList.end())
    {
        m_setBlackList.erase(itSet);
    }
    
    int smstype = 0;
    int isSet = 0;
    int isSetClass = 0;
    bool isFound = false;
    string oldStr;
    UInt64 uClass = 0;

    map<string, SmsAccount>::iterator itrAccount = m_mapAccount.find(strClientId);
    if (itrAccount == m_mapAccount.end())
    {
        LogElk("cannot find this account[%s],phone[%s]", strClientId.data(), strPhone.data());
        return;
    }
    if (!itrAccount->second.m_strSmsType.empty())
    {
        smstype = models::Blacklist::GetSmsTypeFlag(itrAccount->second.m_strSmsType);
    }
    else
    {
        LogElk("cannot find this account[%s],phone[%s],smstype is null", strClientId.data(), strPhone.data());
        return;
    }
    if (NULL != pRedisReply && REDIS_REPLY_STRING == pRedisReply->type )
    {
        string strOut = pRedisReply->str;
        if(!strOut.empty())//strOut:  0:63&5048&6004&a00001:4&
        {
            LogDebug("phone[%s]Get redis blacklist[%s]", strPhone.data(), pRedisReply->str);
            set<string> strSet;
            Comm::split(strOut, "&", strSet);

            string strSysInfo = "";
            string key = strClientId+":";
            int redisSmstype = 0;
            int redisBigSmstype = 0;
            int classCount = 0;
            set<string>::iterator itrSet = strSet.begin();
            for(; itrSet != strSet.end(); itrSet++)
            {
                if(string::npos != (*itrSet).find(key))
                {
                    redisSmstype = atoi((*itrSet).substr(strClientId.length() + 1).data());
                    smstype |= redisSmstype;
                    if (redisBigSmstype < redisSmstype)
                        redisBigSmstype = redisSmstype;
                    isFound = true;

                }
                else
                {
                    string strClass("c:");
                    if(string::npos != (*itrSet).find(strClass))
                    {
                        uClass |= atoi((*itrSet).substr(strClass.length()).data());
                        classCount++;
                    }
                    else
                    {
                        oldStr.append(*itrSet);
                        oldStr.append("&");
                    }
                }
            }

            if (isFound)
            {
                if (smstype > redisBigSmstype)
                {
                    isSet = 1;//更新及设置redis
                }

                if (redisBigSmstype == smstype)
                {
                    //不需要更新及设置redis
                    isSet = 2;
                    LogDebug("account[%s],phone[%s],smstype not need to set redis and table", strClientId.data(), strPhone.data());
             
                }
            }

            if (classCount)
            {
                if ((classCount == 1) && (uClass & models::LAXINBLACKLIST) )
                {
                    //不需要更新及设置redis
                    LogDebug("account[%s],phone[%s],class not need to set redis and table", strClientId.data(), strPhone.data());
                    if (isSet == 2)
                        return;
                    isSetClass = 2;
                }
                else
                    isSetClass = 1;
            }
        }

    }

    uClass = uClass | models::LAXINBLACKLIST;
    InsertPhoneToBlackList(strPhone,strClientId,oldStr,smstype,isSet,isSetClass,uClass);

}

void ReportReceiveThread::HandleRedisResp(TMsg* pMsg)
{
    TRedisResp* pResp = (TRedisResp*)pMsg;
    if ((NULL == pResp->m_RedisReply)
        || (pResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pResp->m_RedisReply->elements == 0)))
    {
        if ("check blacklist" == pMsg->m_strSessionID)
        {
            LogNotice("****iSeq[%lu] not find in blacklist redis.*****",pMsg->m_iSeq);
            HandleBlacklistRedisResp(pResp->m_RedisReply,pResp->m_iSeq);
            freeReply(pResp->m_RedisReply);
            return;
        }
        LogWarn("iSeq[%lu] redisSessionId[%s],query redis is failed",pMsg->m_iSeq,pMsg->m_strSessionID.data());

    }

    if ("check blacklist" == pMsg->m_strSessionID)
    {
        HandleBlacklistRedisResp(pResp->m_RedisReply,pResp->m_iSeq);
    }
    else
    {
        LogError("this Redis type[%s] is invalid.",pMsg->m_strSessionID.data());
    }

    freeReply(pResp->m_RedisReply);
    return;
}

void ReportReceiveThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_MQ_GETMQMSG_REQ:
        {
            HandleReportReciveReqMsg(pMsg);
            break;
        }
        case MSGTYPE_SOCKET_WRITEOVER:
        {
            HandleReportReciveReturnOverMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CLIENTID_AND_SIGN_UPDATE_REQ:
        {
            LogDebug("ClientAndSignPort is update.");
            TUpdateClientIdAndSignReq* pReq= (TUpdateClientIdAndSignReq*)pMsg;
            m_mapClientSignPort.clear();
            m_mapClientSignPort = pReq->m_ClientIdAndSignMap;
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            LogDebug("Account update.");
            TUpdateSmsAccontReq* pAccountUpdateReq= (TUpdateSmsAccontReq*)pMsg;
            m_mapAccount.clear();
            m_mapAccount = pAccountUpdateReq->m_SmsAccountMap;
            break;
        }
        case MSGTYPE_RULELOAD_SYSTEM_PARAM_UPDATE_REQ:
        {
            TUpdateSysParamRuleReq* pSysParamReq = (TUpdateSysParamRuleReq*)pMsg;
            GetSysPara(pSysParamReq->m_SysParamMap);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisResp(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleReportReciveReturnOverMsg(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}

void ReportReceiveThread::HandleReportReciveReqMsg(TMsg* pMsg)
{
    MqConsumeReqMsg *pReportReciveRequest = (MqConsumeReqMsg*)pMsg;
    LogDebug("ReportRecive, data[%s]", pReportReciveRequest->m_strData.data());

    string query = pReportReciveRequest->m_strData;

    map<string,string> mapValue;
    Comm comm;
    comm.splitExMap(query,string("&"),mapValue);
    string strType = mapValue["type"];

    if(strType == "1" || strType == "0")
    {
        LogDebug("here is report!");
        UInt32 uSmsFrom = 0;
        TReportReceiveToSMSReq* req = new TReportReceiveToSMSReq();
        if(strType == "0")
        {
            req->m_uUpdateFlag = 1;
            publishMqC2sDbMsg("", smsp::Channellib::decodeBase64(mapValue["sql"]));
        }
        req->m_strInnerErrcode = smsp::Channellib::decodeBase64(mapValue["innerErrorcode"]);
        req->m_strClientId = mapValue["clientid"];
        req->m_strChannelId = mapValue["channelid"];
        req->m_strSmsId = mapValue["smsid"];
        req->m_strPhone= mapValue["phone"];
        req->m_iStatus = atoi(mapValue["status"].data());
        req->m_strDesc = smsp::Channellib::decodeBase64(mapValue["desc"]);
        req->m_strReportDesc = smsp::Channellib::decodeBase64(mapValue["reportdesc"]);
        string strChannelCnt;
        strChannelCnt = mapValue["channelcnt"];
        req->m_uChannelCount = atoi(strChannelCnt.data());
        req->m_lReportTime = atoi(mapValue["reportTime"].data());
        uSmsFrom = atoi(mapValue["smsfrom"].data());
        req->m_iMsgType = MSGTYPE_REPORTRECEIVE_TO_SMS_REQ;

        LogDebug("[%s:%s] status[%d], smsid[%s], desc[%s], m_strPhone[%s],smsfrom[%u]",
                 req->m_strSmsId.data(),req->m_strPhone.data(), req->m_iStatus, req->m_strSmsId.data(),
                 req->m_strDesc.data(),req->m_strPhone.data(),uSmsFrom);

        MONITOR_INIT( MONITOR_ACCESS_SMS_REPORT );
        MONITOR_VAL_SET("clientid", req->m_strClientId);
        MONITOR_VAL_SET("channelid", req->m_strChannelId);
        MONITOR_VAL_SET("smsid", req->m_strSmsId);
        MONITOR_VAL_SET("phone", req->m_strPhone);
        MONITOR_VAL_SET_INT("state",  req->m_iStatus);
        MONITOR_VAL_SET_INT("smsFrom", uSmsFrom );
        MONITOR_VAL_SET("reportcode", req->m_strDesc);
        MONITOR_VAL_SET("reportdesc", req->m_strReportDesc);
        MONITOR_VAL_SET("reportdate", Comm::getCurrentTime_z(req->m_lReportTime));
        MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));
        MONITOR_VAL_SET_INT("costtime", time(NULL) - req->m_lReportTime );
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);
        MONITOR_PUBLISH( g_pMQMonitorPublishThread );

        if (uSmsFrom == SMS_FROM_ACCESS_CMPP)
        {
            g_pCMPPServiceThread->PostMsg(req);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_CMPP3)
        {
            g_pCMPP3ServiceThread->PostMsg(req);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_SMGP)
        {
            g_pSMGPServiceThread->PostMsg(req);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_SMPP)
        {
            g_pSMPPServiceThread->PostMsg(req);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_HTTP)
        {
            //g_pReportPushThread->PostMsg(req);
            LogError("this is http report!");
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_SGIP)
        {
            g_pSgipRtAndMoThread->PostMsg(req);
        }
        else
        {
            LogError("[%s:%s] smsfrom[%u] is invalid type.",req->m_strSmsId.data(),req->m_strPhone.data(),uSmsFrom);
            delete req;
        }
    }
    else if (strType == "2")
    {
        LogDebug("here is Mo!");
        UInt32 uSmsFrom = 0;
        TReportReceiverMoMsg* pMo = new TReportReceiverMoMsg();
        uSmsFrom = atoi(mapValue["smsfrom"].data());

        pMo->m_strClientId = mapValue["clientid"];
        pMo->m_strContent = smsp::Channellib::decodeBase64(mapValue["content"]);
        pMo->m_strMoId = mapValue["moid"];
        pMo->m_strPhone = mapValue["phone"];
        pMo->m_strSign = smsp::Channellib::decodeBase64(mapValue["sign"]);
        pMo->m_strSignPort = mapValue["signport"];
        pMo->m_strTime = mapValue["time"];
        pMo->m_strUserPort = mapValue["userport"];
        pMo->m_strShowPhone = mapValue["showphone"];
        pMo->m_uChannelId = atoi(mapValue["channelid"].data());
        pMo->m_iMsgType = MSGTYPE_ACCESS_MO_MSG;

        MONITOR_INIT( MONITOR_ACCESS_MO_SMS );
        MONITOR_VAL_SET("clientid",  pMo->m_strClientId);
        MONITOR_VAL_SET_INT("channelid", pMo->m_uChannelId);
        MONITOR_VAL_SET("moid",  pMo->m_strMoId );
        MONITOR_VAL_SET("phone", pMo->m_strPhone.substr(0, 20));
        MONITOR_VAL_SET("motime",  pMo->m_strTime );
        MONITOR_VAL_SET("userport",  pMo->m_strUserPort );
        MONITOR_VAL_SET("content",  pMo->m_strContent );
        MONITOR_VAL_SET_INT("smsfrom", uSmsFrom );
        MONITOR_VAL_SET("synctime", Comm::getCurrentTime_z(0));
        MONITOR_VAL_SET_INT("component_id", g_uComponentId);
        MONITOR_PUBLISH(g_pMQMonitorPublishThread);

        /////http mo
        if (uSmsFrom == SMS_FROM_ACCESS_HTTP)
        {
            map<string, SmsAccount>::iterator itr = m_mapAccount.find(pMo->m_strClientId);
            if (itr != m_mapAccount.end())
            {
                if (0 == itr->second.m_uNeedSignExtend)
                {
                    pMo->m_strSign = "";
                }
                else
                {
                    if (true == pMo->m_strSign.empty())
                    {
                        string strKey = "";
                        strKey = pMo->m_strClientId + "&" + pMo->m_strSignPort;
                        map<string,ClientIdSignPort>::iterator iter = m_mapClientSignPort.find(strKey);
                        if (iter != m_mapClientSignPort.end())
                        {
                            pMo->m_strSign = iter->second.m_strSign;
                        }
                        else
                        {
                            LogWarn("clientAndsignport[%s] is not find clientSignport",strKey.data());
                        }
                    }
                }
                pMo->m_strMoUrl = itr->second.m_strMoUrl;
            }
        }

        LogDebug("=after==clientid[%s],content[%s],moid[%s],phone[%s],sign[%s],signport[%s],userport[%s],time[%s].",
            pMo->m_strClientId.data(),pMo->m_strContent.data(),pMo->m_strMoId.data(),pMo->m_strPhone.data(),
            pMo->m_strSign.data(),pMo->m_strSignPort.data(),pMo->m_strUserPort.data(),pMo->m_strTime.data());

        InsertMoRecord(pMo);

        if (uSmsFrom >= SMS_FROM_ACCESS_CMPP3 && uSmsFrom <= SMS_FROM_ACCESS_SMGP)
        {
            //查询是否是退订，插入t_sms_black_list表及写入redis
            if(pMo->m_strContent.length() <= 2 && !pMo->m_strContent.empty())
            {
                string strCon = pMo->m_strContent;
                Comm::Toupper(strCon);
                string strUpStream = ";" + strCon + ";";
                if (m_strUnsubscribe.find(strUpStream) != string::npos)
                {
                    UInt64 uSeq = m_pSnManager->getSn();
                    models::BlacklistInfo* pBlacklist = new models::BlacklistInfo();
                    pBlacklist->m_strClientId = pMo->m_strClientId;
                    pBlacklist->m_strPhone = pMo->m_strPhone;
                    m_mapBlacklist[uSeq] = pBlacklist;
                    if (m_setBlackList.find(pMo->m_strClientId + "_" + pMo->m_strPhone) != m_setBlackList.end())
                    {
                       LogNotice("too many mo for clientid[%s],phone[%s]", pMo->m_strClientId.c_str(), pMo->m_strPhone.c_str());
                    }
                    else
                    {
                        m_setBlackList.insert(pMo->m_strClientId + "_" + pMo->m_strPhone);
                        TRedisReq* pRedis = new TRedisReq();
                        string strKey = "black_list:" + pMo->m_strPhone;
                        pRedis->m_strSessionID = "check blacklist";
                        pRedis->m_iSeq = uSeq;
                        pRedis->m_RedisCmd = "GET " + strKey;
                        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
                        pRedis->m_pSender = this;
                        pRedis->m_strKey = strKey;
                        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);
                    }

                }
            }
        }
        if (uSmsFrom == SMS_FROM_ACCESS_CMPP)
        {
            g_pCMPPServiceThread->PostMsg(pMo);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_CMPP3)
        {
            g_pCMPP3ServiceThread->PostMsg(pMo);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_SMGP)
        {
            g_pSMGPServiceThread->PostMsg(pMo);
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_HTTP)
        {
            LogError("http msg has no reason go there!");
        }
        else if (uSmsFrom == SMS_FROM_ACCESS_SGIP)
        {
            g_pSgipRtAndMoThread->PostMsg(pMo);
        }
        else
        {
            LogError("smsfrom[%u] is invalid type.",uSmsFrom);
            delete pMo;
            return;
        }

    }
    else if(strType == "3") ///from access to update db
    {
        publishMqC2sDbMsg("", smsp::Channellib::decodeBase64(mapValue["sql"]));
    }
    else
    {
        LogError("this report or mo type is invalid.");
    }

}

void ReportReceiveThread::InsertMoRecord(TMsg* pMsg)
{
    TReportReceiverMoMsg* pMo = (TReportReceiverMoMsg*)pMsg;

    if (NULL == pMo)
    {
        LogError("pMsg is NULL.");
        return;
    }

    time_t now = time(NULL);
    struct tm pTime = {0};
    char sztime[64] = {0};
    char szreachtime[64] = { 0 };
    if(now != 0 )
    {
        localtime_r((time_t*)&now,&pTime);
        strftime(szreachtime, sizeof(sztime), "%Y%m%d%H%M%S", &pTime);
    }

    string strToPhone = "";
    map<string, SmsAccount>::iterator itrUser = m_mapAccount.find(pMo->m_strClientId);
    if (m_mapAccount.end() != itrUser)
    {
        strToPhone = itrUser->second.m_strSpNum;
        strToPhone.append(pMo->m_strUserPort);
    }
    else
    {
        strToPhone = pMo->m_strUserPort;
    }
    string StrUUid = pMo->m_strMoId;
    UInt64 moId = m_pSnManager->CreateMoId(0);
    char cMoid[64] = {0};
    snprintf(cMoid,64,"%lu",moId);
    pMo->m_strMoId.assign(cMoid);
    //std::string msg = content;
    char contentSQL[2500] = {0};
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();
    if(MysqlConn != NULL)
    {
        mysql_real_escape_string(MysqlConn, contentSQL, pMo->m_strContent.data(), pMo->m_strContent.length());
    }
    char sql[4096]  = {0};
    char table[32];
    memset(table, 0, sizeof(table));
    snprintf(table, sizeof(table) - 1, "t_sms_access_molog_%04d%02d",
                                                        pTime.tm_year + 1900, pTime.tm_mon + 1);
    snprintf(sql,4096,
        "insert into %s(moid,channelid,receivedate,phone,tophone,content,sendmoid,clientid,userid) "
        "values('%s','%d','%s','%s','%s','%s','%s','%s','%s');",
        table,
        StrUUid.data(),
        pMo->m_uChannelId,
        szreachtime,
        pMo->m_strPhone.substr(0, 20).data(),
        strToPhone.substr(0, 20).data(),
        contentSQL,
        pMo->m_strMoId.data(),
        pMo->m_strClientId.data(),
        "1"
        );
    publishMqC2sDbMsg(StrUUid, sql);
}

void ReportReceiveThread::HandleReportReciveReturnOverMsg(TMsg* pMsg)
{
    //write done
    SessionMap::iterator it = m_SessionMap.find(pMsg->m_iSeq);
    if(it == m_SessionMap.end())
    {
        LogWarn("ERROR, can't find session,iSeq[%lu]", pMsg->m_iSeq);
        return;
    }

    if(NULL != it->second->m_wheelTimer)
    {
        delete it->second->m_wheelTimer;
    }

    //delete session.
    delete it->second;
    m_SessionMap.erase(it);
}

UInt32 ReportReceiveThread::GetSessionMapSize()
{
    return m_SessionMap.size();
}
