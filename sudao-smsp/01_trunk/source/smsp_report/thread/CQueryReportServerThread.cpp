#include "CQueryReportServerThread.h"
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "json/value.h"
#include "json/writer.h"
#include "main.h"
#include "Comm.h"



CQueryReportServerThread::CQueryReportServerThread(const char *name):
    CThread(name)
{
    m_pSnManager = NULL;

    // QUERYREPORT_INTERVAL default: 1
    m_uQueryReportTimeIntva = 1;
}


CQueryReportServerThread::~CQueryReportServerThread()
{

}


bool CQueryReportServerThread::Init()
{   
    //printf("init GetBalanceServerThread  \r\n");
    if(false == CThread::Init())
    {
        return false;
    }

    m_setStateUnknown.insert(0);
    m_setStateUnknown.insert(1);

    m_setStateSuccess.insert(3);

    for (int i = 4; i <= 10; i++)
    {
        m_setStateFail.insert(i);
    }

    m_mapReportStatus["UNKNOWN"] = m_setStateUnknown;
    m_mapReportStatus["SUCCESS"] = m_setStateSuccess;
    m_mapReportStatus["FAIL"] = m_setStateFail;
    
    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;
}

void CQueryReportServerThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "QUERYREPORT_INTERVAL";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if ((0 > nTmp) || (100 < nTmp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);

            break;
        }

        m_uQueryReportTimeIntva = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).",
        strSysPara.c_str(), m_uQueryReportTimeIntva);
}

void CQueryReportServerThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_REPORTRECIVE_REQ:
        {
            HandleReportReciveReqMsg(pMsg);
            break;      
        }
        case MSGTYPE_TIMEOUT:
        {   
            LogNotice("this is timeout iSeq[%ld]",pMsg->m_iSeq);
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            LogNotice("RuleUpdate account update!");
            TUpdateSmsAccontReq* pAccountUpdateReq= (TUpdateSmsAccontReq*)pMsg;
            m_AccountMap.clear();
            m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
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
        case MSGTYPE_DB_QUERY_RESP:
        {
            HandleDBResponseMsg(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}

void CQueryReportServerThread::HandleReportReciveReqMsg(TMsg* pMsg)
{
    THttpServerRequest *pRequest = (THttpServerRequest*)pMsg;
    LogDebug("request recived, data[%s], iseq[%ld]", pRequest->m_strRequest.data(), pRequest->m_iSeq);

    //ver=2.0&password=1bbd886460827015e5d605ed44252251&mobile=18612341234&sid=08faf6-5728-438d-95ed-e0e0cec4fd37&senddate=20170101
    
    string strAccount;
    string strVer;
    string strPwdMd5;
    string strPhone;
    string strSid;
    string strSendDate;

    //unpacket          
    web::HttpParams param;
    param.Parse(pRequest->m_strRequest);
    
    strAccount = pRequest->m_strSessionID;
    strVer = param._map["ver"];
    strPwdMd5 = param._map["password"];
    strPhone = param._map["mobile"];
    strSid = param._map["sid"];
    strSendDate = param._map["senddate"];   
    
    if(strAccount.empty() || strPwdMd5.empty())
    {
        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_PARAMERROR;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_PARAMERROR;
        Response(pRequest->m_iSeq, response);   
        return;
    }

    ////check account
    AccountMap::iterator iterAccount = m_AccountMap.find(strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogError("account[%s] is not find in accountMap", strAccount.data());
        
        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_AUTHENTICATIONFAIL;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_AUTHENTICATIONFAIL;
        Response(pRequest->m_iSeq, response);   
        return;
    }

    if (6 == iterAccount->second.m_uStatus)
    {
        LogError("account[%s],status[%d] is not ok!", strAccount.data(),iterAccount->second.m_uStatus);

        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_FORBBIDEN;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_FORBBIDEN;
        Response(pRequest->m_iSeq, response);   
        return;
    }
    
    string strip = pRequest->m_socketHandler->GetClientAddress();
    
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
            LogError("IP[%s] is not in whitelist. account[%s]",strip.data(), strAccount.data());

            TQueryReportResponseInfo response = {0};
            response.code = ERR_CODE_QUERYREPORT_RETURN_IPNOTALLOW;
            response.msg = ERR_MSG_QUERYREPORT_RETURN_IPNOTALLOW;
            Response(pRequest->m_iSeq, response);   
            return;
        }
    }
    else
    {
        LogDebug("account[%s] allowed IP[%s]!",strAccount.data(),strip.data());
    }

    //check password
    string strMD5;
    unsigned char md5[16] = { 0 };
    MD5((const unsigned char*) iterAccount->second.m_strPWD.data(), iterAccount->second.m_strPWD.length(), md5);
    std::string HEX_CHARS = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
        strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
    }

    if((0 != strncasecmp(strMD5.data(), strPwdMd5.data(), 32)) || (strPwdMd5.length() !=32))
    {
        LogError("db Md5[%s], request Md5[%s] pwd md5 is not match.", strMD5.data(), strPwdMd5.data());

        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_AUTHENTICATIONFAIL;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_AUTHENTICATIONFAIL;
        Response(pRequest->m_iSeq, response);   
        return;
    }

    //根据（clientid + sid + mobile)条件，限制每秒最多查询 m_uQueryReportTimeIntva 次   
    UInt64 uCurrTime = time(NULL);
    
    std::string strKey = strAccount + strSid + strPhone;
    stl_map_query_frequency::iterator itor = m_ClientReqTimes.find(strKey);
    if (itor == m_ClientReqTimes.end())
    {
        TQueryFrequency queryFrequency;
        queryFrequency.uCurrTimeStamp = uCurrTime;
        queryFrequency.uQueryCount = 1;
        m_ClientReqTimes[strKey] = queryFrequency;
    }
    else
    {
        TQueryFrequency& query = m_ClientReqTimes[strKey];
        if (query.uCurrTimeStamp != uCurrTime)
        {
            query.uCurrTimeStamp = uCurrTime;
            query.uQueryCount = 1;
        }
        else
        {
            query.uQueryCount++;
            if (query.uQueryCount > m_uQueryReportTimeIntva)
            {
                LogWarn("query too offten, uQueryCount[%d], m_uQueryReportTimeIntva[%d]", query.uQueryCount, m_uQueryReportTimeIntva);

                TQueryReportResponseInfo response = {0};
                response.code = ERR_CODE_QUERYREPORT_RETURN_TOO_OFFTEN;
                response.msg = ERR_MSG_QUERYREPORT_RETURN_TOO_OFFTEN;
                Response(pRequest->m_iSeq, response);   
                return;
            }
        }
    }
    
    //request db
    char sql_condition[1024] = { 0 };
    snprintf(sql_condition, 1024, "select * from t_sms_access_{{index}}_%s where clientid = '%s' and smsid = '%s' and phone = '%s'",
        strSendDate.c_str(), strAccount.c_str(), strSid.c_str(), strPhone.c_str());

    string strSql;
    for (int i = 0; i < 9; i++)
    {
        strSql.append(sql_condition).append(" union all ");
    }
    strSql.append(sql_condition).append(";");

    for (int i = 0; i < 10; i++)
    {
        strSql = strSql.replace(strSql.find("{{index}}"), 9, Comm::int2str(i));
    }

    LogDebug("Exec run db sql[%s]", strSql.c_str());
    
    TDBQueryReq* pMsgDB = new TDBQueryReq();
    pMsgDB->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
    pMsgDB->m_pSender = this;
    pMsgDB->m_iSeq = pRequest->m_iSeq;      //share linkSession
    pMsgDB->m_SQL.assign(strSql);
    g_pRunDBThreadPool->PostMsg(pMsgDB);
}

void CQueryReportServerThread::Response(UInt64 iSeq, const TQueryReportResponseInfo& response)
{   
    LogDebug("iseq[%ld], code[%d], msg[%s], sid[%s], uid[%s], mobile[%s], report_status[%s], desc[%s], user_receive_time[%s],", 
        iSeq, response.code, response.msg.c_str(), response.sid.c_str(), response.uid.c_str(), response.mobile.c_str(), 
        response.report_status.c_str(), response.desc.c_str(), response.user_receive_time.c_str());

    Json::Value root;
    root["code"] = Json::Value(response.code);
    root["msg"] = Json::Value(response.msg);

    if (ERR_CODE_QUERYREPORT_RETURN_OK == response.code)
    {
        root["sid"] = Json::Value(response.sid);
        root["uid"] = Json::Value(response.uid);
        root["mobile"] = Json::Value(response.mobile);
        root["report_status"] = Json::Value(response.report_status);
        root["desc"] = Json::Value(response.desc);
        root["user_receive_time"] = Json::Value(response.user_receive_time);
    }
    
    Json::FastWriter fast_writer;
    string strResponse = fast_writer.write(root);

    LogDebug("iSeq[%ld], response[%s]", iSeq, strResponse.data());
    TMsg* pMsgResp = new TMsg();
    pMsgResp->m_iMsgType = MSGTYPE_HTTP_SERVICE_RESP;
    pMsgResp->m_iSeq = iSeq;
    pMsgResp->m_strSessionID = strResponse;
    g_pReportReceiveThread->PostMsg(pMsgResp);
}

UInt32 CQueryReportServerThread::GetSessionMapSize()
{
    return 0;
}

void CQueryReportServerThread::HandleDBResponseMsg(TMsg* pMsg)
{
    TDBQueryResp* pResult = (TDBQueryResp*)pMsg;
    if(0 != pResult->m_iResult)
    {
        LogError("sql query error!");
        delete pResult->m_recordset;

        //internal error
        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_INTERNALERROR;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_INTERNALERROR;
        Response(pResult->m_iSeq, response);    
        return;
    }
    
    //
    RecordSet* rs = pResult->m_recordset;
    UInt64 RecordCount = rs->GetRecordCount();
    
    if(0 == RecordCount)
    {
        delete rs;
        //ok, no packet
        TQueryReportResponseInfo response = {0};
        response.code = ERR_CODE_QUERYREPORT_RETURN_NO_RESULT;
        response.msg = ERR_MSG_QUERYREPORT_RETURN_NO_RESULT;
        Response(pResult->m_iSeq, response);    
        return;
    }
    LogDebug("RecordCount[%d].", RecordCount);

    std::string smsid;
    std::string uid;
    std::string phone;
    int state;
    std::string date;
    std::string errorcode;
    std::string report;

    for (UInt64 i = 0; i < 1; ++i)    //长短信只回复一条
    {
        smsid = (*rs)[i]["smsid"];
        uid = (*rs)[i]["uid"];
        phone = (*rs)[i]["phone"];
        state = atoi((*rs)[i]["state"].c_str());
        date = (*rs)[i]["date"];
        errorcode = (*rs)[i]["errorcode"];
        report = (*rs)[i]["report"];
    }
    
    //get db response over
    TQueryReportResponseInfo response = {0};
    response.code = ERR_CODE_QUERYREPORT_RETURN_OK;
    response.msg = ERR_MSG_QUERYREPORT_RETURN_OK;
    response.sid = smsid;
    response.uid = uid;
    response.mobile = phone;
    response.user_receive_time = date;
    
    for (stl_map_str_set_int::iterator itor_mapReport = m_mapReportStatus.begin(); itor_mapReport != m_mapReportStatus.end(); ++itor_mapReport)
    {
        stl_set_int& setState = itor_mapReport->second;
        if (setState.find(state) != setState.end())
        {
            response.report_status = itor_mapReport->first;
        }
    }

    for (int i = 0; i < 1; i++)
    {
        if (m_setStateUnknown.find(state) != m_setStateUnknown.end())
        {
            response.desc = ERR_MSG_QUERYREPORT_RETURN_UNKNOWN;
            break;
        }

        if (m_setStateSuccess.find(state) != m_setStateSuccess.end())
        {
            response.desc = ERR_MSG_QUERYREPORT_RETURN_OK;
            break;
        }

        std::string desc;
        desc = (6 == state) ? report : errorcode;
        std::string::size_type index = desc.find("*");
        if (std::string::npos != index)
        {
            desc = desc.substr(0, index);
        }

        response.desc = desc;
        break;
    }
    
    
    Response(pResult->m_iSeq, response);
    delete rs;
}


