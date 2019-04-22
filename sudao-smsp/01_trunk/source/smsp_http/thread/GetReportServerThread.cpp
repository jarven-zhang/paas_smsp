#include "GetReportServerThread.h"
#include <stdlib.h>

#include<iostream>
#include "UrlCode.h"

#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "json/json.h"
#include "main.h"
#include "global.h"


GetReportServerThread::GetReportServerThread(const char *name):
    CThread(name)
{
    m_pSnManager = NULL;
}

GetReportServerThread::~GetReportServerThread()
{

}

bool GetReportServerThread::Init()
{
    if(false == CThread::Init())
    {
        return false;
    }

    m_pSnManager = new SnManager();
    if(NULL == m_pSnManager)
    {
        LogError("new SnManager() is failed.");
        return false;
    }

    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    return true;
}

void GetReportServerThread::HandleMsg(TMsg *pMsg)
{
    CHK_NULL_RETURN(pMsg);

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch (pMsg->m_iMsgType)
    {
    case MSGTYPE_REPORT_GET_REPORT_REQ:
    {
        HandleGetReportOrMoReqMsg(pMsg, GET_REPORT);
        break;
    }
    case MSGTYPE_REPORT_GET_MO_REQ:
    {
        HandleGetReportOrMoReqMsg(pMsg, GET_MO);
        break;
    }
    case MSGTYPE_TIMEOUT:
    {
        LogNotice("this is timeout iSeq[%ld]", pMsg->m_iSeq);
        break;
    }
    case MSGTYPE_REDISLIST_REPORT_RESP:
    {
        HandleRedisListReportMsg(pMsg);
        break;
    }
    case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
    {
        LogNotice("RuleUpdate account update!");
        TUpdateSmsAccontReq *pAccountUpdateReq = (TUpdateSmsAccontReq *)pMsg;
        if (pAccountUpdateReq)
        {
            m_AccountMap.clear();
            m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void GetReportServerThread::HandleGetReportOrMoReqMsg(TMsg *pMsg, UInt32 uGetType)
{
    THTTPRequest *pRequest = (THTTPRequest *)pMsg;
    CHK_NULL_RETURN(pRequest);

    LogNotice("get report or mo request recived, data[%s], iseq[%ld], type:[%d]", pRequest->m_strRequest.data(), pRequest->m_iSeq, uGetType);

    std::string strAccount = "", strPwdMd5 = "";
    //unpacket
    try
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js = "";
        if (json_format_string(pRequest->m_strRequest, js) < 0)
        {
            LogError("==err== json message error, req[%s]", pRequest->m_strRequest.data());
            //// failed reponse
            Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_PARAMERROR, ERR_MSG_GETREPORT_RETURN_PARAMERROR, "");
            return;
        }
        if(!reader.parse(js, root))
        {
            LogError("==err== json parse is failed, req[%s]", pRequest->m_strRequest.data());
            //// failed reponse
            Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_PARAMERROR, ERR_MSG_GETREPORT_RETURN_PARAMERROR, "");
            return;
        }

        strAccount = root["clientid"].asString();
        strPwdMd5 = root["password"].asString();
    }
    catch(...)
    {
        LogError("==err== access httpserver unpacet error, json err. Request[%s]", pRequest->m_strRequest.data());
        //// failed reponse
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_PARAMERROR, ERR_MSG_GETREPORT_RETURN_PARAMERROR, "");
        return;
    }


    if(strAccount.empty() || strPwdMd5.empty())
    {
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_PARAMERROR, ERR_MSG_GETREPORT_RETURN_PARAMERROR, "");		//fjx todo
        return;
    }

    ////check account
    AccountMap::iterator iterAccount = m_AccountMap.find(strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogError("account[%s] is not find in accountMap", strAccount.data());
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_AUTHENTICATIONFAIL, ERR_MSG_GETREPORT_RETURN_AUTHENTICATIONFAIL, "");
        return;
    }

    if (6 == iterAccount->second.m_uStatus)	//FORBBIDEN
    {
        LogError("account[%s],status[%d] is not ok!", strAccount.data(), iterAccount->second.m_uStatus);
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_FORBBIDEN, ERR_MSG_GETREPORT_RETURN_FORBBIDEN, "");
        return;
    }

    string strip = pRequest->m_socketHandle->GetServerIpAddress();
    LogNotice("strGustIP[%s]", strip.c_str());

    /////check ip
    if (false == iterAccount->second.m_bAllIpAllow)
    {
        bool IsIPInWhiteList = false;
        for(list<string>::iterator it = iterAccount->second.m_IPWhiteList.begin(); it != iterAccount->second.m_IPWhiteList.end(); it++)
        {
            if(string::npos != strip.find((*it), 0))
            {
                IsIPInWhiteList = true;
                break;
            }
        }

        if(false == IsIPInWhiteList)
        {
            LogError(" IP[%s] is not in whitelist. account[%s]", strip.data(), strAccount.data());
            Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_IPNOTALLOW, ERR_MSG_GETREPORT_RETURN_IPNOTALLOW, "");
            return;
        }
    }
    else
    {
        LogDebug("account[%s] allowed IP[%s]!", strAccount.data(), strip.data());
    }


    //check password
    string strMD5;
    unsigned char md5[16] = { 0 };
    MD5((const unsigned char *) iterAccount->second.m_strPWD.data(), iterAccount->second.m_strPWD.length(), md5);
    std::string HEX_CHARS = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
        strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
    }
    if((0 != strncasecmp(strMD5.data(), strPwdMd5.data(), 32)) || (strPwdMd5.length() != 32))
    {
        LogError("db Md5[%s], request Md5[%s] pwd md5 is not match.", strMD5.data(), strPwdMd5.data());
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_AUTHENTICATIONFAIL, ERR_MSG_GETREPORT_RETURN_AUTHENTICATIONFAIL, "");
        return;
    }


    if ((GET_REPORT == uGetType) && (iterAccount->second.m_uNeedReport != 3 ))
    {
        LogWarn("smsp push report to user, no need to get report.user[%s] gettype[%d] needreport[%d]", strAccount.data(), uGetType, iterAccount->second.m_uNeedReport);
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_NOTALLOWTOGETREPORT, ERR_MSG_GETREPORT_RETURN_NOTALLOWTOGETREPORT, "");
        return;
    }

    if ((GET_MO == uGetType) && (iterAccount->second.m_uNeedMo != 3))
    {
        LogWarn("smsp push mo to user, no need to get mo.user[%s] gettype[%d] needmo[%d]", strAccount.data(), uGetType, iterAccount->second.m_uNeedMo);
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_NOTALLOWTOGET_MO, ERR_MSG_GETREPORT_RETURN_NOTALLOWTOGET_MO, "");
        return;
    }

    if (GET_REPORT == uGetType)
    {
        UInt32 curInterval = time(NULL) - m_mapClientReportReqTimes[strAccount];
        if(curInterval < iterAccount->second.m_uGetRptInterval)
        {
            LogWarn("get report too offten, interval[%d], dbGetRptInterval[%d]", curInterval, iterAccount->second.m_uGetRptInterval);
            Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_TOO_OFFTEN, ERR_MSG_GETREPORT_RETURN_TOO_OFFTEN, "");
            m_mapClientReportReqTimes[strAccount] = time(NULL);
            return;
        }
        m_mapClientReportReqTimes[strAccount] = time(NULL);
        RedisListReq(strAccount, uGetType, pRequest->m_iSeq, iterAccount->second.m_uGetRptMaxSize);
    }
    else if (GET_MO == uGetType)
    {
        UInt32 curInterval = time(NULL) - m_mapClientMoReqTimes[strAccount];
        if(curInterval < iterAccount->second.m_uGetRptInterval)
        {
            LogWarn("get report too offten, interval[%d], dbGetRptInterval[%d]", curInterval, iterAccount->second.m_uGetRptInterval);
            Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_TOO_OFFTEN, ERR_MSG_GETREPORT_RETURN_TOO_OFFTEN, "");
            m_mapClientMoReqTimes[strAccount] = time(NULL);
            return;
        }
        m_mapClientMoReqTimes[strAccount] = time(NULL);
        RedisListReq(strAccount, uGetType, pRequest->m_iSeq, iterAccount->second.m_uGetRptMaxSize);
    }
    else
    {
        LogError("uGetReport:%d,is invalid.", uGetType);
        Response(pRequest->m_iSeq, ERR_CODE_GETREPORT_RETURN_INTERNALERROR, ERR_MSG_GETREPORT_RETURN_INTERNALERROR, "");
        return;
    }
    return;
}

void GetReportServerThread::Response(UInt64 iSeq, int iCode, string strMsg, string strData)
{
    char cCode[25] = {0};
    snprintf(cCode, 25, "%d", iCode);

    string strResponse = "";
    strResponse.append("{\"code\":");
    strResponse.append(cCode);
    strResponse.append(",\"data\":");
    if (strData.length() < 5)
    {
        strResponse.append("[]");
    }
    else
    {
        strResponse.append(strData);
    }

    strResponse.append(",\"msg\":");
    strResponse.append("\"");
    strResponse.append(strMsg);
    strResponse.append("\"}");

    LogDebug("iSeq[%ld], response[%s]", iSeq, strResponse.data());
    TMsg *pMsgResp = new TMsg();
    pMsgResp->m_iMsgType = MSGTYPE_HTTP_SERVICE_RESP;
    pMsgResp->m_iSeq = iSeq;
    pMsgResp->m_strSessionID = strResponse;
    g_pHttpServiceThread->PostMsg(pMsgResp);
}

bool GetReportServerThread::checkEmptyDayMap(string strClientID, string strDate)
{
    map<string, DATES>::iterator it = m_emptyRedisLists.find(strClientID);
    if(it != m_emptyRedisLists.end())
    {
        DATES dates = it->second;
        DATES::iterator itdate = dates.find(strDate);
        if(itdate != dates.end())
        {
            //find clientid&DATE SUC
            LogDebug("this has ensured has no record in redisList, strClientID[%s], strDate[%s]", strClientID.data(), strDate.data());
            return true;
        }
    }
    return false;
}

void GetReportServerThread::RedisListReq(string strClientID, UInt32 uGetType, UInt64 uSeq, UInt32 maxSize)
{
    string strTemp = "";
    if (GET_REPORT == uGetType)
    {
        strTemp.assign("active_report_cache:");
    }
    else
    {
        strTemp.assign("active_mo_cache:");
    }

    string strCmd = "rpop ";
    string strKey = "";
    strKey.append(strTemp);
    strKey.append(strClientID);

    strCmd.append(strKey);
    LogNotice("get report or mo redisList req. cmd[%s], maxSize[%d]", strCmd.data(), maxSize);

    TRedisReq *preq = new TRedisReq();
    preq->m_iMsgType = MSGTYPE_REDISLIST_REPORT_REQ;
    preq->m_RedisCmd.assign(strCmd);
    preq->m_strKey = strKey;
    preq->m_uMaxSize = maxSize;
    preq->m_iSeq = uSeq;
    preq->m_pSender = this;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, preq);
}


void GetReportServerThread::HandleRedisListReportMsg(TMsg *pMsg)
{
    TRedisListReportResp *pRedisResp =  (TRedisListReportResp *)(pMsg);
    if (!pRedisResp)
    {
        LogError("pRedisResp is NULL.");
        return;
    }

    switch(pRedisResp->m_iResult)
    {
    case 0:		// redis response suc
        //将结果应答给用户
        //add clinetid req timer
        Response(pRedisResp->m_iSeq, ERR_CODE_GETREPORT_RETURN_OK, ERR_MSG_GETREPORT_RETURN_OK, pRedisResp->m_strReport);
        break;
    case -1:	// this key has no data
        Response(pRedisResp->m_iSeq, ERR_CODE_GETREPORT_RETURN_OK, ERR_MSG_GETREPORT_RETURN_OK, "");
        break;
    case -2:	// redis link error
        //link error,报告给用户 inernalerror
        Response(pRedisResp->m_iSeq, ERR_CODE_GETREPORT_RETURN_INTERNALERROR, ERR_MSG_GETREPORT_RETURN_INTERNALERROR, "");
        break;
    default:
        LogWarn("no reason go here, key[%s]", pRedisResp->m_strKey.c_str());
        break;
    }
}

