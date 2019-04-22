/*************************************************************************
copyright (C),2018-2030,ucpaas .Co.,Ltd.

FileName     : CChargeThread.cpp
Author       : pengzhen      Version : 1.0    Date: 2018/07/18
Description  : 计费处理线程类
Version      : 1.0
Function List :
**************************************************************************/

#include "UrlCode.h"
#include "UTFString.h"
#include "Comm.h"
#include "HttpParams.h"
#include "base64.h"
#include "CotentFilter.h"
#include "./CChargeThread.h"
#include "LogMacro.h"
#include "BusTypes.h"
#include "propertyutils.h"
#include "ErrorCode.h"
#include "HttpSender.h"
#include "main.h"

using namespace BusType;

CChargeThread::CChargeThread(const char* name) : CThread(name)
{
}

CChargeThread::~CChargeThread()
{
}

bool CChargeThread::Init()
{
    PropertyUtils::GetValue("smsp_charge_url", m_strChargeUrl);
    LogDebug("smsp_charge_url[%s].", m_strChargeUrl.data());

    if (m_strChargeUrl.empty())
    {
        cout << "m_strChargeUrl is empty." << endl;
        return false;
    }

    m_pInternalService = new InternalService();

    if (m_pInternalService == NULL)
    {
        printf("m_pInternalService is NULL.");
        return false;
    }

    m_pInternalService->Init();

    g_pRuleLoadThread->getAgentInfo(m_AgentInfoMap);
    g_pRuleLoadThread->getAgentAccount(m_AgentAcct);
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    return true;

}

void CChargeThread::HandleMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            TUpdateSmsAccontReq* pAccountUpdateReq = (TUpdateSmsAccontReq*)pMsg;

            if (pAccountUpdateReq)
            {
                m_AccountMap.clear();
                m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
                LogNotice("RuleUpdate account update size:%u!", m_AccountMap.size());
            }

            break;
        }

        case MSGTYPE_RULELOAD_AGENTINFO_UPDATE_REQ:
        {
            TUpdateAgentInfoReq* pReq = (TUpdateAgentInfoReq*)pMsg;

            if (pReq)
            {
                m_AgentInfoMap.clear();
                m_AgentInfoMap = pReq->m_AgentInfoMap;
                LogNotice("RuleUpdate AGENTINFO update. map.size[%d]", pReq->m_AgentInfoMap.size());
            }

            break;
        }

        case MSGTYPE_RULELOAD_AGENT_ACCOUNT_UPDATE_REQ:
        {
            TupdateAgentAcctInfoReq* pReq = (TupdateAgentAcctInfoReq*)(pMsg);

            if (pReq)
            {
                m_AgentAcct.clear();
                m_AgentAcct = pReq->m_AgentAcctMap;
                LogNotice("update t_sms_agent_account size:%d", m_AgentAcct.size());
            }

            break;
        }

        default:
            ;;
    }

    return;
}

/**
*   post charge func, check agent balance
**/
bool CChargeThread::smsChargePost(session_pt pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    UInt64  ullAgentCheck = 10000;
    float   fBalance    = 0.0;//可用余额

    LogDebug("[%s:%s:%s] PostCharge AgentId:[%d] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_uAgentId);

    SmsAccount* pAccount = GetAccountInfo(pSession->m_strClientId, m_AccountMap);

    if (!pAccount)
    {
        LogError("[%s:%s:%s] is not find in m_AccountMap.",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str());

        return false;
    }

    agentInfoMap::iterator itagent = m_AgentInfoMap.find(pSession->m_uAgentId);

    if (itagent == m_AgentInfoMap.end() && pSession->m_uPayType != 1)//后付费不检查
    {
        LogError("[%s:%s:%s] MOD PrePayType AgentId[%ld] not find ",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            pSession->m_uAgentId);

        pSession->m_uState = SMS_STATUS_SEND_FAIL;
        return false;
    }

    if (itagent == m_AgentInfoMap.end())
    {
        LogNotice("[%s:%s:%s] PostCharge AgentId:[%d] cannot find in the agentinfo",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            pSession->m_uAgentId);

        return true;
    }


    pSession->m_uAgentType = itagent->second.m_iAgent_type;
    pSession->m_iAgentPayType = itagent->second.m_iAgentPaytype;

    LogDebug("[%s:%s:%s] PostCharge AgentId:[%d],agenttype[%d],agentPayType[%d] ",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        pSession->m_uAgentId,
        pSession->m_uAgentType,
        itagent->second.m_iAgentPaytype);

    if (pSession->m_uAgentType == 5 && pSession->m_uPayType == 1)
    {

        /**  非代理商用户返回OK**/
        if (pSession->m_uAgentId < ullAgentCheck || itagent == m_AgentInfoMap.end())
        {
            LogWarn("[%s:%s:%s] AgentInfo[%d] Not Find",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentId);

            pSession->m_uState = SMS_STATUS_SEND_FAIL;
            return true;
        }

        /*如果没有找到代理商对应的账号信息直接返回OK*/
        agentAcctMap::iterator itAgentAcct = m_AgentAcct.find(pSession->m_uAgentId);

        if (itAgentAcct == m_AgentAcct.end())
        {
            LogWarn("[%s:%s:%s] AgentAccount[%d] Not Find",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentId);

            pSession->m_uState = SMS_STATUS_SEND_FAIL;
            return true;
        }

        fBalance = (itAgentAcct->second.m_fbalance <= 0.0) ? \
            itAgentAcct->second.m_fcurrent_credit : \
            (itAgentAcct->second.m_fbalance + itAgentAcct->second.m_fcurrent_credit);

        if (fBalance <= 0.0)
        {
            LogWarn("[%s:%s:%s] smsPostCharge AgentId:[%d] <**** No Balance *****>",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentId);

            return false;
        }
    }

    if (pSession->m_uPayType == 0)
    {
        if (pSession->m_uAgentType != 1 && pSession->m_uAgentType != 2 && pSession->m_uAgentType != 5)
        {
            LogError("[%s:%s:%s] pre charge, agentType[%d] is not 1,2,5",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentType);

            pSession->m_uState = SMS_STATUS_SEND_FAIL;
            return false;
        }
    }

    if (pSession->m_uPayType == 2 || pSession->m_uPayType == 3)
    {
        if (pSession->m_uAgentType != 5)
        {
            LogError("[%s:%s:%s] pre charge, agentType[%d] is not 5",
                pSession->m_strClientId.c_str(),
                pSession->m_strSmsId.c_str(),
                pSession->m_strPhone.c_str(),
                pSession->m_uAgentType);

            pSession->m_uState = SMS_STATUS_SEND_FAIL;
            return false;
        }
    }

    return true;

}

bool CChargeThread::smsChargePre(session_pt pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    ////host charge ip
    string strIp = "";
    UInt32 uIp = 0;

    if (true == m_SmspUtils.CheckIPFromUrl(m_strChargeUrl, strIp))
    {
        uIp = inet_addr(strIp.c_str());
    }
    else
    {
        LogError("[%s:%s:%s] chargeUrl[%s] hostIp is failed.",
            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str(),
            m_strChargeUrl.c_str());

        pSession->m_strErrorCode = HOSTIP_CHARGE_IP_FAILED;
        pSession->m_strYZXErrCode = HOSTIP_CHARGE_IP_FAILED;
        pSession->m_uSaleFee = 0;
        pSession->m_fCostFee = 0;
        pSession->m_uProductCost = 0;
        return false;
    }

    http::HttpSender* pHttpSender = new http::HttpSender();
    CHK_NULL_RETURN_FALSE(pHttpSender);

    if (false == pHttpSender->Init(m_pInternalService, pSession->m_uSessionId, g_pSessionThread))
    {
        LogError("[%s:%s:%s] pHttpSender->Init is failed.",
            pSession->m_strClientId.c_str(), pSession->m_strSmsId.c_str(), pSession->m_strPhone.c_str());

        SAFE_DELETE(pHttpSender);
        pSession->m_strErrorCode = HTTPSENDER_INIT_FAILED;
        pSession->m_strYZXErrCode = HTTPSENDER_INIT_FAILED;
        pSession->m_uSaleFee = 0;
        pSession->m_fCostFee = 0;
        pSession->m_uProductCost = 0;
        return false ;
    }

    pSession->m_uSendHttpType = SMS_TO_CHARGE;

    if (FOREIGN == pSession->m_uOperater)
    {
        pSession->m_uProductType = PRODUCT_TYPE_GOU_JI;
    }
    else
    {
        int nSmsType = atoi(pSession->m_strSmsType.c_str());

        switch (nSmsType)
        {
            case SMS_TYPE_YING_XIAO:
                pSession->m_uProductType = PRODUCT_TYPE_YING_XIAO;
                break;

            case SMS_TYPE_YAN_ZHENG_MA:
                pSession->m_uProductType = PRODUCT_TYPE_YAN_ZHENG_MA;
                break;

            default:
                pSession->m_uProductType = PRODUCT_TYPE_TONG_ZHI;
                break;
        }
    }

    int iTemplateType = atoi(pSession->m_strTemplateType.c_str());

    switch (iTemplateType)
    {
        case TEMPLATE_TYPE_USSD:
            pSession->m_uProductType = PRODUCT_TYPE_USSD;
            break;

        case TEMPLATE_TYPE_FLUSH_SMS:
            pSession->m_uProductType = PRODUCT_TYPE_FLUSH_SMS;
            break;

        case TEMPLATE_TYPE_HANG_UP_SMS:
            pSession->m_uProductType = PRODUCT_TYPE_HANG_UP_SMS;
            break;
    }

    string strPostData = "";
    ////clientid
    strPostData.append("clientid=");
    strPostData.append(pSession->m_strClientId);
    ////agent type
    strPostData.append("&agenttype=");
    strPostData.append(Comm::int2str(pSession->m_uAgentType));

    strPostData.append("&agent_id=");
    strPostData.append(Comm::int2str(pSession->m_uAgentId));
    strPostData.append("&paytype=");
    strPostData.append(Comm::int2str(pSession->m_uPayType));


    ////producttype
    strPostData.append("&producttype=");
    char cPro[25] = {0};

    if (pSession->m_uProductType == PRODUCT_TYPE_TONG_ZHI || pSession->m_uProductType == PRODUCT_TYPE_YAN_ZHENG_MA)
    {
        snprintf(cPro, 25, "%d,%u", PRODUCT_TYPE_HANG_YE, pSession->m_uProductType);
    }
    else
    {
        snprintf(cPro, 25, "%u", pSession->m_uProductType);
    }

    strPostData.append(cPro);

    //operator_code
    strPostData.append("&operator_code=");
    strPostData.append(Comm::int2str(pSession->m_uOperater));
    //area_id
    strPostData.append("&area_id=");
    strPostData.append(Comm::int2str(pSession->m_uArea));
    ////count
    strPostData.append("&count=");
    char cCount[25] = {0};
    snprintf(cCount, 25, "%u", pSession->m_uSmsNum);
    strPostData.append(cCount);
    strPostData.append("&fee=");//t_sms_client_tariff ---> fee
    ////fee
    char cFee[25] = {0};

    if (PRODUCT_TYPE_GOU_JI == pSession->m_uProductType)
    {
        snprintf(cFee, 25, "%lf", pSession->m_uSaleFee);
    }
    else
    {
        snprintf(cFee, 25, "%d", 0);
    }

    strPostData.append(cFee);
    strPostData.append("&smsid=");
    strPostData.append(pSession->m_strSmsId);
    strPostData.append("&phone=");
    strPostData.append(pSession->m_strPhone);
    ///AccountAscription
    char cAccountAscription[32] = {0};
    snprintf(cAccountAscription, 32, "%d", pSession->m_uClientAscription);
    strPostData.append("&client_ascription=");
    strPostData.append(cAccountAscription);

    pSession->m_lchargeTime = time(NULL);

    pHttpSender->setIPCache(uIp);
    pSession->m_pHttpSender = pHttpSender;

    //  clientid=b01409&agenttype=4&producttype=0&operator_code=1&area_id=176&count=1&fee=0
    //  &smsid=c78de009-24a4-4f50-88c8-9623955a17b3&phone=13581231503&client_ascription=1
    LogDebug("[%s:%s:%s]======send to charge======[%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        strPostData.c_str());

    pHttpSender->Post(m_strChargeUrl, strPostData);

    return true ;

}

UInt32 CChargeThread::smsChargeResult(session_pt pSession, UInt32 uResult, const string& strContent)
{
    bool chargeSuccess = false;

    if (!pSession)
    {
        LogError("session is NULL.");
        return ACCESS_ERROR_PARAM_NULL ;
    }

    LogNotice("[%s:%s:%s] charge cost time[%d] result[%s]",
        pSession->m_strClientId.c_str(),
        pSession->m_strSmsId.c_str(),
        pSession->m_strPhone.c_str(),
        time(NULL) - pSession->m_lchargeTime,
        strContent.data());

    if (200 == uResult)
    {
        // analyze charge resp content
        // result=0&orderid=1111111111111114444&sale_price=0&product_cost=0
        map<string, string> mapParam;
        Comm::splitExMap(strContent, string("&"), mapParam);

        string strReturn = mapParam["result"];
        string strOrderId = mapParam["orderid"];
        string strSaleFee = mapParam["sale_price"];
        string strCost = mapParam["product_cost"];
        string strProductType = mapParam["product_type"];

        if (0 != strReturn.compare("0"))
        {
            pSession->m_uProductType = 99;
            pSession->m_uSaleFee = 0;
            pSession->m_fCostFee = 0;
            pSession->m_uProductCost = 0;
            pSession->m_strErrorCode = CHARGE_IS_FAILED;
            pSession->m_strYZXErrCode = CHARGE_IS_FAILED;
        }
        else
        {
            pSession->m_strSubId = strOrderId;

            if (pSession->m_strSubId.empty())
            {
                pSession->m_strSubId = "0";
            }

            pSession->m_uSaleFee = atof(strSaleFee.data());
            pSession->m_uProductCost = atof(strCost.data());

            if (pSession->m_uProductType != 0)
            {
                pSession->m_uProductType = atoi(strProductType.data());
            }

            chargeSuccess = true;
        }
    }
    else
    {
        pSession->m_uSaleFee      = 0;
        pSession->m_fCostFee      = 0;
        pSession->m_uProductCost  = 0;
        pSession->m_uProductType  = 99;

        if (true != pSession->m_bOverRatePlanFlag)
        {
            pSession->m_strErrorCode = CHARGE_FAILED;
            pSession->m_strYZXErrCode = CHARGE_FAILED;
        }
        else
        {
            pSession->m_strErrorCode = OVER_RATE_CHARGE_FAIL_1;
            pSession->m_strYZXErrCode = OVER_RATE_CHARGE_FAIL_1;
        }
    }

    if (!chargeSuccess)
    {
        LogError("[%s:%s:%s] Charge is failed [ %s ] .",
            pSession->m_strClientId.c_str(),
            pSession->m_strSmsId.c_str(),
            pSession->m_strPhone.c_str(),
            pSession->m_strErrorCode.data());

        return ACCESS_ERROR_CHARGE;
    }

    return ACCESS_ERROR_NONE;

}

bool CChargeThread::smsChargeSelect()
{
    return (m_pInternalService->GetSelector()->Select() > 0) ;
}

