#include "GetBalanceServerThread.h"
#include <stdlib.h>

#include <iostream>
#include "UrlCode.h"
#include "RsaManager.h"

#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "json/json.h"
#include "main.h"
#include "Comm.h"
#include "global.h"


GetBalanceServerThread::GetBalanceServerThread(const char *name):
    CThread(name)
{
    m_pSnManager = NULL;

    // GETBALANCE_INTERVAL default: 5
    m_uGetBlcTimeIntva = 5;
}


GetBalanceServerThread::~GetBalanceServerThread()
{
    //m_pServerSocket->Close();
}


bool GetBalanceServerThread::Init()
{
    //printf("init GetBalanceServerThread  \r\n");
    if(false == CThread::Init())
    {
        return false;
    }

    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);
    g_pRuleLoadThread->getAgentInfo(m_AgentInfoMap);
    g_pRuleLoadThread->getSysAuthentication(m_mapSysAuthentication);

    ////
    map<std::string, std::string> sysParamMap;
    g_pRuleLoadThread->getSysParamMap(sysParamMap);
    GetSysPara(sysParamMap);

    return true;
}

void GetBalanceServerThread::GetSysPara(const std::map<std::string, std::string>& mapSysPara)
{
    string strSysPara;
    std::map<std::string, std::string>::const_iterator iter;

    do
    {
        strSysPara = "GETBALANCE_INTERVAL";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        const std::string& strTmp = iter->second;

        int nTmp = atoi(strTmp.c_str());
        if ((0 > nTmp) || (5 * 60 < nTmp))
        {
            LogError("Invalid system parameter(%s) value(%s, %d).",
                strSysPara.c_str(), strTmp.c_str(), nTmp);
            break;
        }

        m_uGetBlcTimeIntva = nTmp;
    }
    while (0);

    LogNotice("System parameter(%s) value(%u).", strSysPara.c_str(), m_uGetBlcTimeIntva);

    do
    {
        strSysPara = "SMSP_RSA_PRIVATE_KEY";
        iter = mapSysPara.find(strSysPara);
        if (iter == mapSysPara.end())
        {
            LogError("Can not find system parameter(%s).", strSysPara.c_str());
            break;
        }

        m_strSmspRsaPriKey = iter->second;
    }
    while (0);

    LogNotice("System parameter(%s) value(%s).", strSysPara.c_str(), m_strSmspRsaPriKey.data());
}

void GetBalanceServerThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_AUTHENTICATION_REQ:
        {
            handleAuthentication(pMsg);
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
        case MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ:
        {
            TUpdateAgentInfoReq* msg = (TUpdateAgentInfoReq*)pMsg;
            LogNotice("RuleUpdate AGENTINFO update. map.size[%d]", msg->m_AgentInfoMap.size());
            m_AgentInfoMap.clear();
            m_AgentInfoMap = msg->m_AgentInfoMap;
            break;
        }
        case MSGTYPE_RULELOAD_SYS_AUTHENTICATION_UPDATE_REQ:
        {
            DbUpdateReq<SysAuthenticationMap>* pReq = (DbUpdateReq<SysAuthenticationMap>*)pMsg;
            m_mapSysAuthentication = pReq->m_map;
            LogNotice("RuleUpdate SysAuthentication update. map.size[%u]", m_mapSysAuthentication.size());
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

void GetBalanceServerThread::HandleReportReciveReqMsg(TMsg* pMsg)
{
    THttpServerRequest *pRequest = (THttpServerRequest*)pMsg;
    LogDebug("request recived, data[%s], iseq[%ld]", pRequest->m_strRequest.data(), pRequest->m_iSeq);

    string strAccount;
    string strPwdMd5;
    //unpacket
    try
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js;

        if (json_format_string(pRequest->m_strRequest, js) < 0)
        {
            LogError("==err== json message error, req[%s]", pRequest->m_strRequest.data());
            //// failed reponse
            Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_PARAMERROR, ERR_MSG_GETBALANCE_RETURN_PARAMERROR, 0, 0, 0, 0, 0, 0, 0);
            return;
        }
        if(!reader.parse(js,root))
        {
            LogError("==err== json parse is failed, req[%s]", pRequest->m_strRequest.data());
            //// failed reponse
            Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_PARAMERROR, ERR_MSG_GETBALANCE_RETURN_PARAMERROR, 0, 0, 0, 0, 0, 0, 0);
            return;
        }

        strAccount = root["clientid"].asString();
        strPwdMd5 = root["password"].asString();
    }
    catch(...)
    {
        LogError("==err== access httpserver unpacet error, json err. Request[%s]", pRequest->m_strRequest.data());
        //// failed reponse
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_PARAMERROR, ERR_MSG_GETBALANCE_RETURN_PARAMERROR, 0, 0, 0, 0, 0, 0, 0);
        return;
    }


    if(strAccount.empty() || strPwdMd5.empty())
    {
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_PARAMERROR, ERR_MSG_GETBALANCE_RETURN_PARAMERROR, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    ////check account
    AccountMap::iterator iterAccount = m_AccountMap.find(strAccount);
    if (iterAccount == m_AccountMap.end())
    {
        LogError("account[%s] is not find in accountMap", strAccount.data());
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_AUTHENTICATIONFAIL, ERR_MSG_GETBALANCE_RETURN_AUTHENTICATIONFAIL, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    if (6 == iterAccount->second.m_uStatus)
    {
        LogError("account[%s],status[%d] is not ok!", strAccount.data(),iterAccount->second.m_uStatus);
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_FORBBIDEN, ERR_MSG_GETBALANCE_RETURN_FORBBIDEN, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    if (1 == iterAccount->second.m_uPayType)
    {
        //after pay
        LogError("account[%s],m_uPaytype[%d] is not ok!", strAccount.data(), iterAccount->second.m_uPayType);
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_NOFEEINFO, ERR_MSG_GETBALANCE_RETURN_NOFEEINFO, 0, 0, 0, 0, 0, 0, 0);
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
            LogError(" IP[%s] is not in whitelist. account[%s]",strip.data(), strAccount.data());
            Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_IPNOTALLOW, ERR_MSG_GETBALANCE_RETURN_IPNOTALLOW, 0, 0, 0, 0, 0, 0, 0);
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
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_AUTHENTICATIONFAIL, ERR_MSG_GETBALANCE_RETURN_AUTHENTICATIONFAIL, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    //check interval
    UInt32 curInterval = time(NULL) - m_ClientReqTimes[strAccount];
    LogDebug("fjx test, curInterval[%d], db.interval[%d]", curInterval, iterAccount->second.m_uGetRptInterval);
    if(curInterval < m_uGetBlcTimeIntva)
    {
        LogWarn("get balance too offten, currInterval[%d], db.m_uGetBlcTimeIntva[%d]", curInterval, iterAccount->second.m_uGetRptInterval);
        Response(pRequest->m_iSeq, ERR_CODE_GETBALANCE_RETURN_TOO_OFFTEN, ERR_MSG_GETBALANCE_RETURN_TOO_OFFTEN, 0, 0, 0, 0, 0, 0, 0);
        m_ClientReqTimes[strAccount] = time(NULL);
        return;
    }

    //modify strAccount request time
    m_ClientReqTimes[strAccount] = time(NULL);

    //get anentType
    int agentType = getAgentTypeByID(iterAccount->second.m_uAgentId);

    //request db
    char sql[4096] = { 0 };
    TDBQueryReq* pMsgDB = new TDBQueryReq();
    pMsgDB->m_iMsgType = MSGTYPE_DB_QUERY_REQ;
    pMsgDB->m_pSender = this;
    pMsgDB->m_iSeq = pRequest->m_iSeq;      //share linkSession
    pMsgDB->m_strSessionID = Comm::int2str(agentType);
    /////////////////////
    if(agentType == OEM_DLS)
    {
        //oem
        snprintf(sql,4096 ,"select * from t_sms_oem_client_pool where client_id='%s' and status=0 and due_time>=NOW();", strAccount.data());
    }
    else if(agentType == XIAOSHOU_DLS || agentType == PINPAI_DLS)
    {
        //pingpai,agent
        snprintf(sql,4096 ,"select * from t_sms_client_order where client_id='%s' and status=1 and end_time>=NOW() and NOW()>=effective_time;", strAccount.data());
    }
    else
    {
        LogError("agentType[%d] not find", agentType);
    }
    /////////////////////////
    pMsgDB->m_SQL.assign(sql);
    g_pDisPathDBThreadPool->PostMsg(pMsgDB);

}

void GetBalanceServerThread::Response(UInt64 iSeq, int iCode, string strMsg, UInt32 uHANGYE, UInt32 uYINGXIAO, float fGUOJI,UInt32 uIdCode,UInt32 uNotice,
    int agentType, UInt32 uUssd_Remain, UInt32 uShanXin_Remain, UInt32 uGuaJiDuanXin_Remain)
{
    LogDebug("iseq[%ld], iCode[%d], strMsg[%s], uHANGYE[%d], uYINGXIAO[%d], fGUOJI[%f]", iSeq, iCode, strMsg.data(), uHANGYE, uYINGXIAO, fGUOJI);

    char strTypeToString[20] = {0};

    Json::Value jsonData;

    if(iCode == ERR_CODE_GETBALANCE_RETURN_OK)
    {
        //hangye
        Json::Value jsonHY;
        jsonHY["product_type"] = Json::Value(0);
        snprintf(strTypeToString, 20, "%u", uHANGYE);
        jsonHY["remain_quantity"] = Json::Value(string(strTypeToString));
        jsonData.append(jsonHY);

        //yingxiao
        Json::Value jsonYX;
        jsonYX["product_type"] = Json::Value(1);
        snprintf(strTypeToString, 20, "%u", uYINGXIAO);
        jsonYX["remain_quantity"] = Json::Value(string(strTypeToString));
        jsonData.append(jsonYX);

        //guoji
        Json::Value jsonGJ;
        jsonGJ["product_type"] = Json::Value(2);
        snprintf(strTypeToString, 20, "%0.2f", fGUOJI);
        jsonGJ["remain_quantity"] = Json::Value(string(strTypeToString));
        jsonData.append(jsonGJ);

        //IdCode
        Json::Value jsonIdCode;
        jsonIdCode["product_type"] = Json::Value(3);
        snprintf(strTypeToString, 20, "%u", uIdCode);
        jsonIdCode["remain_quantity"] = Json::Value(string(strTypeToString));
        jsonData.append(jsonIdCode);

        //notice
        Json::Value jsonNotice;
        jsonNotice["product_type"] = Json::Value(4);
        snprintf(strTypeToString, 20, "%u", uNotice);
        jsonNotice["remain_quantity"] = Json::Value(string(strTypeToString));
        jsonData.append(jsonNotice);


        if(agentType == OEM_DLS)
        {
            //do nothing
        }
        else if(agentType == XIAOSHOU_DLS || agentType == PINPAI_DLS)
        {
            //ussd
            Json::Value jsonUSSD;
            jsonUSSD["product_type"] = Json::Value(7);
            snprintf(strTypeToString, 20, "%u", uUssd_Remain);
            jsonUSSD["remain_quantity"] = Json::Value(string(strTypeToString));
            jsonData.append(jsonUSSD);

            //ShanXin
            Json::Value jsonShanXin;
            jsonShanXin["product_type"] = Json::Value(8);
            snprintf(strTypeToString, 20, "%u", uShanXin_Remain);
            jsonShanXin["remain_quantity"] = Json::Value(string(strTypeToString));
            jsonData.append(jsonShanXin);

            //GuaJiDuanXin
            Json::Value jsonGuaJiDuanXin;
            jsonGuaJiDuanXin["product_type"] = Json::Value(9);
            snprintf(strTypeToString, 20, "%u", uGuaJiDuanXin_Remain);
            jsonGuaJiDuanXin["remain_quantity"] = Json::Value(string(strTypeToString));
            jsonData.append(jsonGuaJiDuanXin);
        }
        else
        {
            LogWarn("should not go here. agentType[%d] no correct", agentType);
        }
    }

    Json::Value root;
    root["code"] = Json::Value(iCode);
    root["msg"] = Json::Value(strMsg);
    root["data"] = Json::Value(jsonData);

    Json::FastWriter fast_writer;
    string strResponse = fast_writer.write(root);

    LogDebug("iSeq[%ld], response[%s]", iSeq, strResponse.data());
    TMsg* pMsgResp = new TMsg();
    pMsgResp->m_iMsgType = MSGTYPE_HTTP_SERVICE_RESP;
    pMsgResp->m_iSeq = iSeq;
    pMsgResp->m_strSessionID = strResponse;
    g_pReportReceiveThread->PostMsg(pMsgResp);

}


UInt32 GetBalanceServerThread::GetSessionMapSize()
{
    return 0;
}

void GetBalanceServerThread::HandleDBResponseMsg(TMsg* pMsg)
{
    TDBQueryResp* pResult = (TDBQueryResp*)pMsg;
    if(0 != pResult->m_iResult)
    {
        LogError("sql query error!");
        delete pResult->m_recordset;

        //internal error
        Response(pResult->m_iSeq, ERR_CODE_GETBALANCE_RETURN_INTERNALERROR, ERR_MSG_GETBALANCE_RETURN_INTERNALERROR, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    //
    RecordSet* rs = pResult->m_recordset;
    CHK_NULL_RETURN(rs);

    UInt64 RecordCount = rs->GetRecordCount();

    if(0 == RecordCount)
    {
        delete rs;
        //ok, no packet
        Response(pResult->m_iSeq, ERR_CODE_GETBALANCE_RETURN_OK, ERR_MSG_GETBALANCE_RETURN_OK, 0, 0, 0, 0, 0, 0, 0);
        return;
    }
    LogDebug("RecordCount[%d].", RecordCount);

    UInt32 uHANGYE_Remain = 0;
    UInt32 uYINGXIAO_Remain = 0;
    float fGUOJI_Remain = 0;
    UInt32 uNotice_Remain = 0;
    UInt32 uIdCode_Remain = 0;
    UInt32 uUssd_Remain = 0;
    UInt32 uShanXin_Remain = 0;
    UInt32 uGuaJiDuanXin_Remain = 0;

    int agentType = atoi(pResult->m_strSessionID.c_str());
    if(agentType == OEM_DLS)
    {
        // t_sms_oem_client_pool
        for (UInt64 i = 0; i < RecordCount; ++i)
        {
            int iPdt_type= atoi((*rs)[i]["product_type"].data());
            int iRemain_quantity = atoi((*rs)[i]["remain_number"].data());
            float fRemain_amount = atof((*rs)[i]["remain_amount"].data());

            switch(iPdt_type)
            {
                case PRODUCT_TYPE_HANG_YE:      //HANGYE
                {
                    uHANGYE_Remain +=iRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_YING_XIAO:        //YINGXIAO
                {
                    uYINGXIAO_Remain += iRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_GOU_JI:       //GUO JI
                {
                    fGUOJI_Remain += fRemain_amount;
                    break;
                }
                case PRODUCT_TYPE_YAN_ZHENG_MA:
                {
                    uIdCode_Remain += iRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_TONG_ZHI:
                {
                    uNotice_Remain += iRemain_quantity;
                    break;
                }
                default:
                {
                    LogWarn("client order err, product_type[%d]", iPdt_type);
                    break;
                }
            }
        }
    }//endif  oem orders
    else if(agentType == XIAOSHOU_DLS || agentType == PINPAI_DLS)
    {
        // t_sms_client_orders
        for (UInt64 i = 0; i < RecordCount; ++i)
        {
            int iPdt_type= atoi((*rs)[i]["product_type"].data());
            float fRemain_quantity = atof((*rs)[i]["remain_quantity"].data());

            switch(iPdt_type)
            {
                case PRODUCT_TYPE_HANG_YE:      //HANGYE
                {
                    uHANGYE_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_YING_XIAO:        //YINGXIAO
                {
                    uYINGXIAO_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_GOU_JI:       //GUO JI
                {
                    fGUOJI_Remain += fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_YAN_ZHENG_MA:
                {
                    uIdCode_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_TONG_ZHI:
                {
                    uNotice_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_USSD:     //USSD
                {
                    uUssd_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_FLUSH_SMS:        // shanxin
                {
                    uShanXin_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                case PRODUCT_TYPE_HANG_UP_SMS:      //   guajiduanxin
                {
                    uGuaJiDuanXin_Remain += (UInt32)fRemain_quantity;
                    break;
                }
                default:
                {
                    LogWarn("client order err, product_type[%d]", iPdt_type);
                    break;
                }
            }
        }
    }
    else
    {
        LogWarn("agentTpye[%d] not correct", agentType);
        Response(pResult->m_iSeq, ERR_CODE_GETBALANCE_RETURN_PARAMERROR, ERR_MSG_GETBALANCE_RETURN_PARAMERROR, 0, 0, 0, 0, 0, 0, 0);
        return;
    }


    //get db response over
    Response(pResult->m_iSeq, ERR_CODE_GETBALANCE_RETURN_OK, ERR_MSG_GETBALANCE_RETURN_OK, uHANGYE_Remain, uYINGXIAO_Remain, fGUOJI_Remain,
        uIdCode_Remain,uNotice_Remain,agentType, uUssd_Remain, uShanXin_Remain, uGuaJiDuanXin_Remain);
    delete rs;
}

int GetBalanceServerThread::getAgentTypeByID(UInt64 uAgentID)
{
    map<UInt64, AgentInfo>::iterator itagent = m_AgentInfoMap.find(uAgentID);
    if(itagent == m_AgentInfoMap.end())
    {
        // pre pay account get agentType failed
        LogError(" m_uAgentId[%ld] is not find in m_AgentInfoMap.", uAgentID);
        return AGENT_OTHER;
    }
    else
    {
        return itagent->second.m_iAgent_type;
    }
}

void GetBalanceServerThread::handleAuthentication(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    ////////////////////////////////////////////////////////////////////////

    string strExpireTimestamp;
    string strPubKey;
    Int32 iCode = 0;

    if (!processAuthentication(pMsg, strExpireTimestamp, strPubKey))
    {
        LogError("Call processAuthentication failed.");
        iCode = -1;
    }

    if (strPubKey.empty())
    {
        LogError("The RSA Public Key of Peer is empty. Can't response to Peer.");
        return;
    }

    ////////////////////////////////////////////////////////////////////////

    Json::Value root;
    root["code"] = Json::Value(iCode);
    root["expire_timestamp"] = Json::Value(strExpireTimestamp);

    Json::FastWriter fast_writer;
    string strResponse = fast_writer.write(root);
    LogDebug("strResponse[%s].", strResponse.data());

    ////////////////////////////////////////////////////////////////////////

    RsaManager rsaManager;

    if (!rsaManager.initByRSAPublicKey(strPubKey))
    {
        LogError("Call processAuthentication failed.");
        return;
    }

    string strAfterEncrypt;
    if (!rsaManager.encrypt(strResponse, strAfterEncrypt))
    {
        LogError("Call processAuthentication failed.");
        return;
    }

    ////////////////////////////////////////////////////////////////////////

    TMsg* pRsp = new TMsg();
    CHK_NULL_RETURN(pRsp);
    pRsp->m_strSessionID = http::UrlCode::UrlEncode(strAfterEncrypt);
    pRsp->m_iSeq = pMsg->m_iSeq;
    pRsp->m_iMsgType = MSGTYPE_HTTP_SERVICE_RESP;
    g_pReportReceiveThread->PostMsg(pRsp);
}

bool GetBalanceServerThread::processAuthentication(TMsg* pMsg,
                                                            string& strExpireTimestamp,
                                                            string& strPubKey)
{
    CHK_NULL_RETURN_FALSE(pMsg);
    THttpServerRequest* pReq = (THttpServerRequest*)pMsg;

    ////////////////////////////////////////////////////////////////////////////////////////

    RsaManager rsaManager;
    if (!rsaManager.initByRSAPrivateKey(m_strSmspRsaPriKey))
    {
        LogError("Call initByRSAPrivateKey failed.");
        return false;
    }

    string strAfterDecrypt;
    if (!rsaManager.decrypt(http::UrlCode::UrlDecode(pReq->m_strRequest), strAfterDecrypt))
    {
        LogError("Call decrypt failed.");
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    string serial_num;
    string component_type;
    string component_md5;
    string timestamp;

    try
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js;

        if (json_format_string(strAfterDecrypt, js) < 0)
        {
            LogError("Call json_format_string failed. Data[%s].", strAfterDecrypt.data());
            return false;
        }

        if(!reader.parse(js,root))
        {
            LogError("Call json_format_string failed. Data[%s].", strAfterDecrypt.data());
            return false;
        }

        serial_num = root["serial_num"].asString();
        component_type = Comm::int2str(root["component_type"].asUInt());
        component_md5 = root["component_md5"].asString();
        timestamp = Comm::int2str(root["timestamp"].asUInt());
        strPubKey = root["pub_key"].asString();
    }
    catch(...)
    {
        LogError("Catch exception when parse json. Data[%s].", strAfterDecrypt.data());
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    string strKey = serial_num + ":" + component_type;

    SysAuthenticationMap::iterator iter;
    CHK_MAP_FIND_STR_RET_FALSE(m_mapSysAuthentication, iter, strKey);
    SysAuthentication& sysAuthen = iter->second;

    string strClientIp = pReq->m_socketHandler->GetClientAddress();

    if (!sysAuthen.checkIp(strClientIp))
    {
        LogError("Call checkIp failed. ClientIp[%s], DB[%s].",
            strClientIp.data(), sysAuthen.m_bind_ip.data());
        return false;
    }

    if (component_md5 != sysAuthen.m_component_md5)
    {
        LogError("The Md5[%s] not match with DB[%s].",
            component_md5.data(), sysAuthen.m_component_md5.data());
        return false;
    }

    UInt32 uiCurrentTimestamp = (UInt32)time(NULL);
    UInt32 uiExpireTimestamp = Comm::strToUint32(sysAuthen.m_expire_timestamp);

    if (uiCurrentTimestamp > uiExpireTimestamp)
    {
        LogError("CurrentTimestamp[%u] > ExpireTimestamp[%u] in DB.",
            uiCurrentTimestamp, uiExpireTimestamp);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    if (sysAuthen.m_first_timestamp.empty() || (sysAuthen.m_first_timestamp == "0"))
    {
        char sql[512] = {0};
        snprintf(sql, sizeof(sql),
            "update t_sms_sys_authentication set first_timestamp=%u where id='%s';",
            uiCurrentTimestamp,
            sysAuthen.m_id.data());

        LogNotice("sql[%s].", sql);

        TDBQueryReq* pMsg = new TDBQueryReq();
        CHK_NULL_RETURN_FALSE(pMsg);
        pMsg->m_SQL.assign(sql);
        pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
        g_pDisPathDBThreadPool->PostMsg(pMsg);
    }

    LogNotice("==Authentication Pass== serial_num[%s], component_type[%s], component_md5[%s], timestamp[%s].",
        serial_num.data(),
        component_type.data(),
        component_md5.data(),
        timestamp.data());

    strExpireTimestamp = sysAuthen.m_expire_timestamp;
    return true;
}

