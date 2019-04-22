#include "ChargeThread.h"
#include "CRuleLoadThread.h"
#include "HttpServerThread.h"
#include "CDBThreadPool.h"
#include "ErrorCode.h"
#include "LogMacro.h"
#include "Uuid.h"
#include "UrlCode.h"
#include "Comm.h"
#include "AgentInfo.h"
#include "enum.h"
#include "Fmt.h"

#include <time.h>


ChargeThread* g_pChargeThread = NULL;

extern CDBThreadPool* g_pDisPathDBThreadPool;


ChargeThread::ChargeThread(const char* name): CThread(name)
{
}

ChargeThread::~ChargeThread()
{
}

bool ChargeThread::start()
{
    g_pChargeThread = new ChargeThread("ChargeThread");
    INIT_CHK_NULL_RET_FALSE(g_pChargeThread);
    INIT_CHK_FUNC_RET_FALSE(g_pChargeThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pChargeThread->CreateThread());

    return true;
}

bool ChargeThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    /*t_sms_client_order*/
    m_CustomerOrderMap.clear();
    m_CustomerOrderSetKey.clear();
    g_pRuleLoadThread->getCustomerOrderMap(m_CustomerOrderMap, m_CustomerOrderSetKey);

    /*t_sms_oem_client_pool*/
    m_OEMClientPoolMap.clear();
    m_OEMClientPoolSetKey.clear();
    g_pRuleLoadThread->getOEMClientPoolsMap(m_OEMClientPoolMap, m_OEMClientPoolSetKey);

    return true;
}

void ChargeThread::initWorkFuncMap()
{
    REG_MSG_CALLBACK(ChargeThread, MSGTYPE_TIMEOUT, handleTimeoutReqMsg);
    REG_MSG_CALLBACK(ChargeThread, MSGTYPE_TO_CHARGE_REQ, handleChargeReqMsg);
    REG_MSG_CALLBACK(ChargeThread, MSGTYPE_DB_QUERY_RESP, handleDbRspMsg);
    REG_MSG_CALLBACK(ChargeThread, MSGTYPE_RULELOAD_CUSTOMERORDER_UPDATE_REQ, handleDbUpdateReqMsg);
    REG_MSG_CALLBACK(ChargeThread, MSGTYPE_RULELOAD_OEMCLIENTPOOL_UPDATE_REQ, handleDbUpdateReqMsg);
}

void ChargeThread::handleDbUpdateReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    switch (pMsg->m_iMsgType)
    {
        case MSGTYPE_RULELOAD_CUSTOMERORDER_UPDATE_REQ:
        {
            TUpdateCustomerOrderReq* msg = (TUpdateCustomerOrderReq*)(pMsg);
            LogNotice("t_sms_client_order table update. m_CustomerOrderMap.size[%d] m_CustomerOrderSetKey.size[%d]",
                msg->m_CustomerOrderMap.size(), msg->m_CustomerOrderSetKey.size());

            m_CustomerOrderMap.clear();
            m_CustomerOrderSetKey.clear();
            m_CustomerOrderMap = msg->m_CustomerOrderMap;
            m_CustomerOrderSetKey = msg->m_CustomerOrderSetKey;
            break;
        }
        case MSGTYPE_RULELOAD_OEMCLIENTPOOL_UPDATE_REQ:
        {
            TUpdateOEMClientPoolReq* msg = (TUpdateOEMClientPoolReq*)(pMsg);
            LogNotice("t_sms_oem_client_pool table update. m_OEMClientPoolMap.size[%d], m_OEMClientPoolSetKey.size[%d]",
                msg->m_OEMClientPoolMap.size(), msg->m_OEMClientPoolSetKey.size());

            m_OEMClientPoolMap.clear();
            m_OEMClientPoolSetKey.clear();
            m_OEMClientPoolMap = msg->m_OEMClientPoolMap;
            m_OEMClientPoolSetKey = msg->m_OEMClientPoolSetKey;
            break;
        }
        default:
        {
            LogWarn("msgType[%ld] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}

void ChargeThread::handleChargeReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    THttpQueryReq* pReq = (THttpQueryReq*)(pMsg);

    if (pReq->m_uAgentType == OEM_DLS)
    {
        OemCharging(pReq, m_SnMng.getSeq64());
    }
    else if (pReq->m_uAgentType == XIAOSHOU_DLS || pReq->m_uAgentType == PINPAI_DLS)
    {
        BrandOrSalesCharging(pReq, m_SnMng.getSeq64());
    }
    else
    {
        LogWarn("agentType error ,type[%d]", pReq->m_uAgentType);

        HttpResponse(HTTP_RESPONSE_NO_PRODUCT, pReq->m_iSeq, pReq->m_strSmsID, pReq->m_strPhone, "", 0, 0);
    }
}

void ChargeThread::OemCharging(THttpQueryReq* pReq, UInt64 uSeq, UInt64 uLastChargeFailClientPoolId)
{
    if (uLastChargeFailClientPoolId != 0)
    {
        set<UInt64>::iterator itorKey = m_OEMClientPoolSetKey.find(uLastChargeFailClientPoolId);

        if (itorKey == m_OEMClientPoolSetKey.end())
        {
            LogDebug("Can not find %lu in m_OEMClientPoolSetKey", uLastChargeFailClientPoolId);
            uLastChargeFailClientPoolId = 0;
        }
    }

    map<string, OEMClientPoolLIST>::iterator it = m_OEMClientPoolMap.find(pReq->m_strClientID);
    string strCurrentTime = Comm::getCurrentTime();
    OEMClientPool Order;

    if (it == m_OEMClientPoolMap.end())
    {
        LogNotice("[%s:%s]find no OEM product! clientID[%s]",
            pReq->m_strSmsID.data(), pReq->m_strPhone.data(), pReq->m_strClientID.data());

        goto CHARGE_NO_PRODUCT;
    }
    else
    {
        OEMClientPoolLIST list = it->second;

        OEMClientPoolLIST::iterator itor = list.begin();

        for (; itor != list.end(); itor++)
        {
            Order = *itor;

            /*检查上次扣费失败的点*/
            if (uLastChargeFailClientPoolId > 0)
            {
                if (Order.m_uClientPoolId == uLastChargeFailClientPoolId)
                {
                    LogDebug("find uLastChargeFailClientPoolId[%lu]", uLastChargeFailClientPoolId);
                    uLastChargeFailClientPoolId = 0;
                }

                continue;
            }

            /*检查产品类型*/
            if (string::npos == pReq->m_strProductType.find(Comm::int2str(Order.m_uProductType)))
            {
                continue;
            }

            /*检查到期时间*/
            if (Order.m_strDeadTime < strCurrentTime)
            {
                continue;
            }

            if (Order.m_uProductType == PRODUCT_TYPE_GUOJI)
            {
                /*检查运营商*/
                if (pReq->m_uOperater != OPERATOR_INTERNATIONAL || Order.m_uOperator != OPERATOR_INTERNATIONAL)
                {
                    continue;
                }

                if (Order.m_fRemainAmount * 1000000.0 - atof(pReq->m_strFee.data()) >= 0.000001) //pReq->m_strFee 单位:元*10^6
                {
                    goto CHARGE_SUCCESS;
                }
                else
                {
                    LogNotice("[%s:%s] ClientPoolId[%lu] balance[%lf] is not enough, need charge[%s] clientID[%s]",
                        pReq->m_strSmsID.data(),
                        pReq->m_strPhone.data(),
                        Order.m_uClientPoolId,
                        Order.m_fRemainAmount * 1000000,
                        pReq->m_strFee.data(),
                        pReq->m_strClientID.data());

                    continue;
                }
            }
            else
            {
                /*检查运营商*/
                if ((pReq->m_uOperater != Order.m_uOperator && Order.m_uOperator != OPERATOR_ALL_NETCOM) ||
                    pReq->m_uOperater == OPERATOR_INTERNATIONAL ||
                    Order.m_uOperator == OPERATOR_INTERNATIONAL)
                {
                    continue;
                }

                LogDebug("RemainCount:%d need:%d", Order.m_uRemainCount, pReq->m_uCount);

                if (Order.m_uRemainCount - (int)pReq->m_uCount >= 0)
                {
                    goto CHARGE_SUCCESS;
                }
                else
                {
                    LogNotice("[%s:%s]ClientPoolId[%lu] balance[%d] is not enough, need charge[%d] clientID[%s]",
                        pReq->m_strSmsID.data(), pReq->m_strPhone.data(), Order.m_uClientPoolId, Order.m_uRemainCount,
                        pReq->m_uCount, pReq->m_strClientID.data());
                    continue;
                }
            }
        }

        if (itor == list.end())
        {
            goto CHARGE_NO_FEE;
        }
    }

CHARGE_NO_FEE:
    HttpResponse(HTTP_RESPONSE_NO_FEE, pReq->m_iSeq, pReq->m_strSmsID, pReq->m_strPhone, "", 0, 0);
    OemClearHttpSession(uSeq);
    return;

CHARGE_NO_PRODUCT:
    HttpResponse(HTTP_RESPONSE_NO_PRODUCT, pReq->m_iSeq, pReq->m_strSmsID, pReq->m_strPhone, "", 0, 0);
    OemClearHttpSession(uSeq);
    return;

CHARGE_SUCCESS:
    map<UInt64, OEMChgSession>::iterator itSession = m_OEMSesssionMap.find(uSeq);

    if (itSession == m_OEMSesssionMap.end())
    {
        OEMChgSession session;
        session.m_pTimer = SetTimer(uSeq, OEM_SESSION_TIMER_STRING, 100 * 1000);
        session.m_strClientID = pReq->m_strClientID;
        session.m_uProductType = Comm::int2str(Order.m_uProductType);
        session.m_uAgentType = pReq->m_uAgentType;
        session.m_uCount = pReq->m_uCount;
        session.m_strFee = pReq->m_strFee;
        session.m_uSeqRet = pReq->m_iSeq;
        session.m_strSubID = Comm::int2str(Order.m_uClientPoolId);
        session.m_lchargetime = time(NULL);
        session.m_strSmsID = pReq->m_strSmsID;
        session.m_strPhone = pReq->m_strPhone;
        session.m_fUintPrice = Order.m_fUnitPrice;
        session.m_ChargeReq = *pReq;

        m_OEMSesssionMap[uSeq] = session;
    }
    else
    {
        itSession->second.m_uProductType = Comm::int2str(Order.m_uProductType);
        itSession->second.m_fUintPrice = Order.m_fUnitPrice;
        itSession->second.m_strSubID = Comm::int2str(Order.m_uClientPoolId);
        itSession->second.m_lchargetime = time(NULL);
    }

    LogDebug("m_OEMSesssionMap Key:%lu Size:%d", uSeq, m_OEMSesssionMap.size());

    char sql[1024]  = {0};

    if (Order.m_uProductType == PRODUCT_TYPE_GUOJI)
    {
        double ftmp = pReq->m_uCount * atof(pReq->m_strFee.data()) / 1000000.0;

        snprintf(sql, 1024,
            "update t_sms_oem_client_pool t set t.remain_amount=t.remain_amount-%lf ,t.update_time = '%s' "
            "WHERE t.client_pool_id=%lu and t.remain_amount >= %lf",
            ftmp,
            strCurrentTime.data(),
            Order.m_uClientPoolId,
            ftmp);
    }
    else
    {
        snprintf(sql, 1024,
            "update t_sms_oem_client_pool t set t.remain_number=t.remain_number-%d,t.remain_test_number=t.remain_test_number-%d ,t.update_time = '%s' "
            "WHERE t.client_pool_id=%lu and t.remain_number>=%d",
            pReq->m_uCount,
            pReq->m_uCount,
            strCurrentTime.data(),
            Order.m_uClientPoolId,
            pReq->m_uCount);
    }

    LogNotice("[%s:%s]update sql[%s]", pReq->m_strSmsID.data(), pReq->m_strPhone.data(), sql);
    TDBQueryReq* query = new TDBQueryReq();
    query->m_SQL = string(sql);
    query->m_strKey = to_string(Order.m_uClientPoolId);
    query->m_iSeq = uSeq;
    query->m_strSessionID = Comm::int2str(pReq->m_uAgentType);
    query->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    query->m_pSender = this;
    g_pDisPathDBThreadPool->PostMsg(query);
}

void ChargeThread::OemClearHttpSession(UInt64 uKey)
{
    map<UInt64, OEMChgSession>::iterator itor = m_OEMSesssionMap.find(uKey);

    if (itor != m_OEMSesssionMap.end())
    {
        LogDebug("m_OEMSesssionMap Delete Session[%lu]", uKey);

        SAFE_DELETE(itor->second.m_pTimer);
        m_OEMSesssionMap.erase(itor);
    }
    else
    {
        LogWarn("m_OEMSesssionMap can not find Session[%lu]", uKey);
    }
}

void ChargeThread::BrandOrSalesCharging(THttpQueryReq* pReq, UInt64 uSeq, UInt64 uLastChargeFailSubId)
{
    if (uLastChargeFailSubId != 0)
    {
        set<UInt64>::iterator itorKey = m_CustomerOrderSetKey.find(uLastChargeFailSubId);

        if (itorKey == m_CustomerOrderSetKey.end())
        {
            LogDebug("Can not find %lu in m_CustomerOrderSetKey", uLastChargeFailSubId);
            uLastChargeFailSubId = 0;
        }
    }

    map<string, CustomerOrderLIST>::iterator it = m_CustomerOrderMap.find(pReq->m_strClientID);
    string strCurrentTime = Comm::getCurrentTime();
    CustomerOrder Order;

    if (it == m_CustomerOrderMap.end())
    {
        LogNotice("[%s:%s]find no t_sms_client_order product! clientID[%s]",
            pReq->m_strSmsID.data(), pReq->m_strPhone.data(), pReq->m_strClientID.data());

        goto CHARGE_NO_PRODUCT;
    }
    else
    {
        CustomerOrderLIST list = it->second;

        CustomerOrderLIST::iterator itor = list.begin();

        for (; itor != list.end(); itor++)
        {
            Order = *itor;

            /*检查上次扣费失败的点*/
            if (uLastChargeFailSubId > 0)
            {
                if (Order.m_uSubID == uLastChargeFailSubId)
                {
                    LogDebug("find uLastChargeFailSubId[%lu]", uLastChargeFailSubId);
                    uLastChargeFailSubId = 0;
                }

                continue;
            }

            /*检查产品类型*/
            if (string::npos == pReq->m_strProductType.find(Comm::int2str(Order.m_uProductType)))
            {
                continue;
            }

            /*检查到期时间*/
            if (Order.m_strDeadTime < strCurrentTime)
            {
                continue;
            }

            if (Order.m_uProductType == PRODUCT_TYPE_GUOJI)
            {
                /*检查运营商*/
                if (pReq->m_uOperater != OPERATOR_INTERNATIONAL || Order.m_uOperator != OPERATOR_INTERNATIONAL)
                {
                    continue;
                }

                if (Order.m_fRemainAmount * 1000000.0 - atof(pReq->m_strFee.data()) >= 0.000001)
                {
                    goto CHARGE_SUCCESS;
                }
                else
                {
                    LogNotice("[%s:%s]ClientPoolId[%lu] balance[%lf] is not enough, need charge[%s] clientID[%s]",
                        pReq->m_strSmsID.data(), pReq->m_strPhone.data(), Order.m_uSubID, Order.m_fRemainAmount * 1000000,
                        pReq->m_strFee.data(), pReq->m_strClientID.data());
                    continue;
                }
            }
            else
            {
                /*检查运营商*/
                if ((pReq->m_uOperater != Order.m_uOperator && Order.m_uOperator != OPERATOR_ALL_NETCOM) ||
                    pReq->m_uOperater == OPERATOR_INTERNATIONAL ||
                    Order.m_uOperator == OPERATOR_INTERNATIONAL)
                {
                    continue;
                }

                if (Order.m_iRemainCount - (int)pReq->m_uCount >= 0)
                {
                    goto CHARGE_SUCCESS;
                }
                else
                {
                    LogNotice("[%s:%s]ClientPoolId[%lu] balance[%d] is not enough, need charge[%u] clientID[%s]",
                        pReq->m_strSmsID.data(), pReq->m_strPhone.data(), Order.m_uSubID, Order.m_iRemainCount,
                        pReq->m_uCount, pReq->m_strClientID.data());
                    continue;
                }
            }
        }

        if (itor == list.end())
        {
            goto CHARGE_NO_FEE;
        }
    }

CHARGE_NO_FEE:
    HttpResponse(HTTP_RESPONSE_NO_FEE, pReq->m_iSeq, pReq->m_strSmsID, pReq->m_strPhone, "", 0, 0);
    BrandOrSaleClearHttpSession(uSeq);
    return;

CHARGE_NO_PRODUCT:
    HttpResponse(HTTP_RESPONSE_NO_PRODUCT, pReq->m_iSeq, pReq->m_strSmsID, pReq->m_strPhone, "", 0, 0);
    BrandOrSaleClearHttpSession(uSeq);
    return;

CHARGE_SUCCESS:
    map<UInt64, ChgSession>::iterator itSession = m_SesssionMap.find(uSeq);

    if (itSession == m_SesssionMap.end())
    {
        ChgSession session;
        session.m_pTimer = SetTimer(uSeq, BRAND_OR_SALE_SESSION_TIMER_STRING, 100 * 1000);
        session.m_strClientID = pReq->m_strClientID;
        session.m_uProductType = Comm::int2str(Order.m_uProductType);
        session.m_uAgentType = pReq->m_uAgentType;
        session.m_uCount = pReq->m_uCount;
        session.m_strFee = pReq->m_strFee;
        session.m_uSeqRet = pReq->m_iSeq;
        session.m_strSubID = Order.m_uSubID;
        session.m_lchargetime = time(NULL);
        session.m_strSmsID = pReq->m_strSmsID;
        session.m_strPhone = pReq->m_strPhone;
        session.m_fUintPrice = Order.m_fUnitPrice;
        session.m_fProductCost = Order.m_fProductCost;
        session.m_fQuantity = Order.m_fQuantity;
        session.m_ChargeReq = *pReq;
        session.m_fSalePrice = Order.m_fSalePrice;

        m_SesssionMap[uSeq] = session;
    }
    else
    {
        itSession->second.m_uProductType = Comm::int2str(Order.m_uProductType);
        itSession->second.m_fUintPrice = Order.m_fUnitPrice;
        itSession->second.m_fProductCost = Order.m_fProductCost;
        itSession->second.m_fQuantity = Order.m_fQuantity;
        itSession->second.m_strSubID = Order.m_uSubID;
        itSession->second.m_lchargetime = time(NULL);
        itSession->second.m_fSalePrice = Order.m_fSalePrice;
    }

    LogDebug("m_SesssionMap Key:%lu Size:%d", uSeq, m_SesssionMap.size());

    //update db
    char sql[1024]  = {0};

    if (Order.m_uProductType == PRODUCT_TYPE_GUOJI)
    {
        double ftmp = Order.m_fSalePrice * pReq->m_uCount * atof(pReq->m_strFee.data()) / 1000000.0; //单位:元

        snprintf(sql, 1024, "update t_sms_client_order t set t.remain_quantity=t.remain_quantity-%lf, t.update_time = '%s' "
            "WHERE t.sub_id=%lu and t.remain_quantity >= %lf",
            ftmp,
            strCurrentTime.data(),
            Order.m_uSubID,
            ftmp);
    }
    else
    {
        snprintf(sql, 1024,
            "update t_sms_client_order t set t.remain_quantity=t.remain_quantity-%d, t.update_time = '%s' "
            "WHERE t.sub_id=%lu and t.remain_quantity >= %d",
            pReq->m_uCount,
            strCurrentTime.data(),
            Order.m_uSubID,
            pReq->m_uCount);
    }

    LogNotice("[%s:%s]update sql[%s]", pReq->m_strSmsID.data(), pReq->m_strPhone.data(), sql);
    TDBQueryReq* query = new TDBQueryReq();
    query->m_SQL = string(sql);
    query->m_strKey = to_string(Order.m_uSubID);
    query->m_iSeq = uSeq;
    query->m_strSessionID = Comm::int2str(pReq->m_uAgentType);
    query->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    query->m_pSender = this;
    g_pDisPathDBThreadPool->PostMsg(query);
}

void ChargeThread::BrandOrSaleClearHttpSession(UInt64 uKey)
{
    map<UInt64, ChgSession>::iterator itor = m_SesssionMap.find(uKey);

    if (itor != m_SesssionMap.end())
    {
        LogDebug("m_SesssionMap Delete Session[%lu]", uKey);

        SAFE_DELETE(itor->second.m_pTimer);
        m_SesssionMap.erase(itor);
    }
    else
    {
        LogWarn("m_SesssionMap can not find Session[%lu]", uKey);
    }
}

////iReturn-httpreturnCode  uSeqRet-httpSessionSeq   uSubID-orderid
UInt32 ChargeThread::HttpResponse(UInt32 iReturn, UInt64 uSeqRet, string strSmsID, string strPhone, string strSubID, double uSlaePrice, double uProductCost, UInt32 uProductType)
{
    THttpQueryResp* phttpResp = new THttpQueryResp();
    phttpResp->m_uiReturn = iReturn;
    phttpResp->m_iSeq = uSeqRet;
    phttpResp->m_strSmsID = strSmsID;
    phttpResp->m_strPhone = strPhone;
    phttpResp->m_dSlaePrice = uSlaePrice;
    phttpResp->m_dProductCost = uProductCost;
    phttpResp->m_strSubID = strSubID;
    phttpResp->m_uiProductType = uProductType;
    phttpResp->m_iMsgType = MSGTYPE_DISPATCH_TO_SMSERVICE_RESP;
    g_pHttpServiceThread->PostMsg(phttpResp);

    return 0;
}

void ChargeThread::handleDbRspMsg(TMsg* pMsg)
{
    //if failed, should retry another product
    TDBQueryResp* pResp = (TDBQueryResp*)(pMsg);

    //check session type
    int agentType = atoi(pResp->m_strSessionID.c_str());

    if (agentType == OEM_DLS)
    {
        OemDBResp(pResp);
    }
    else if (agentType == XIAOSHOU_DLS || agentType == PINPAI_DLS)
    {
        BrandOrSalesDBResp(pResp);
    }
    else
    {
        LogError("should not go here, agentType[%d]", agentType);
    }
}

// t_sms_client_order response
void ChargeThread::BrandOrSalesDBResp(TDBQueryResp* pResp)
{
    map<UInt64, ChgSession>::iterator itSession = m_SesssionMap.find(pResp->m_iSeq);

    if (itSession == m_SesssionMap.end())
    {
        LogWarn("session find failed. seq[%ld]", pResp->m_iSeq);
        return;
    }

    LogNotice("charge cost time[%d] from db. [%s:%s]", time(NULL) - itSession->second.m_lchargetime, itSession->second.m_strSmsID.data(), itSession->second.m_strPhone.data());

    if (pResp->m_iResult == 0)
    {
        if (pResp->m_iAffectRow >= 1)
        {
            LogNotice("[%s:%s]charge suc.  m_strSubID[%lu], clientid[%s], type[%s]", itSession->second.m_strSmsID.data(), itSession->second.m_strPhone.data(),
                itSession->second.m_strSubID, itSession->second.m_strClientID.data(), itSession->second.m_uProductType.data());

            double uSalePrice   = 0;
            double uProductCost = 0;

            if (atoi(itSession->second.m_uProductType.data()) == PRODUCT_TYPE_GUOJI)
            {
                uSalePrice = atof(itSession->second.m_strFee.data()) * itSession->second.m_fSalePrice;
                uProductCost = atof(itSession->second.m_strFee.data()) * itSession->second.m_fProductCost;
            }
            else
            {
                uSalePrice = itSession->second.m_fUintPrice * 1000000.0;
                uProductCost = itSession->second.m_fProductCost * 1000000.0 / itSession->second.m_fQuantity;
            }

            HttpResponse(HTTP_RESPONSE_OK, itSession->second.m_uSeqRet, itSession->second.m_strSmsID,
                itSession->second.m_strPhone, Comm::int2str(itSession->second.m_strSubID), uSalePrice, uProductCost,
                atoi(itSession->second.m_uProductType.data()));

            goto CLEAR_SESSION;
        }
        else
        {
            LogDebug("DB Charge Fail!!! ReCharge...uSep[%lu] uLastChargeFailClientPoolId[%lu]", itSession->first, itSession->second.m_strSubID);
            THttpQueryReq Req = itSession->second.m_ChargeReq;
            BrandOrSalesCharging(&Req, itSession->first, itSession->second.m_strSubID);
            return;
        }
    }
    else
    {
        HttpResponse(HTTP_RESPONSE_NO_FEE, itSession->second.m_uSeqRet, itSession->second.m_strSmsID, itSession->second.m_strPhone, "", 0, 0);
        goto CLEAR_SESSION;
    }

CLEAR_SESSION:
    LogDebug("m_SesssionMap Delete Session[%lu]", itSession->first);

    if (itSession->second.m_pTimer != NULL)
    {
        delete itSession->second.m_pTimer;
        itSession->second.m_pTimer = NULL;
    }

    m_SesssionMap.erase(itSession);
}

// t_sms_oem_client_pool response
void ChargeThread::OemDBResp(TDBQueryResp* pResp)
{
    //get session
    map<UInt64, OEMChgSession>::iterator itSession = m_OEMSesssionMap.find(pResp->m_iSeq);

    if (itSession == m_OEMSesssionMap.end())
    {
        LogWarn("session find failed. seq[%lu]", pResp->m_iSeq);
        return;
    }

    LogNotice("charge cost time[%d] from db. [%s:%s]", time(NULL) - itSession->second.m_lchargetime, itSession->second.m_strSmsID.data(), itSession->second.m_strPhone.data());

    if (pResp->m_iResult == 0) //sql执行成功
    {
        if (pResp->m_iAffectRow >= 1) //修改成功
        {
            //update suc ,http response
            LogNotice("[%s:%s]charge suc.  m_strSubID[%s], clientid[%s], type[%s]", itSession->second.m_strSmsID.data(), itSession->second.m_strPhone.data(),
                itSession->second.m_strSubID.c_str(), itSession->second.m_strClientID.data(), itSession->second.m_uProductType.data());

            double uSalePrice = 0;
            double uProductCost = 0;

            if (atoi(itSession->second.m_uProductType.data()) == PRODUCT_TYPE_GUOJI)
            {
                uSalePrice = atof(itSession->second.m_strFee.data());
                uProductCost = atof(itSession->second.m_strFee.data());
            }
            else
            {
                uSalePrice = itSession->second.m_fUintPrice * 1000.0 * 1000.0;  //单位:元*10^6
                uProductCost = itSession->second.m_fUintPrice * 1000.0 * 1000.0;
            }

            HttpResponse(HTTP_RESPONSE_OK, itSession->second.m_uSeqRet, itSession->second.m_strSmsID, itSession->second.m_strPhone,
                itSession->second.m_strSubID.c_str(), uSalePrice, uProductCost, atoi(itSession->second.m_uProductType.data()));

            goto CLEAR_SESSION;
        }
        else
        {
            //sql执行成功，影响的行数是0，就是实际扣费失败
            THttpQueryReq Req = itSession->second.m_ChargeReq;
            LogDebug("DB Charge Fail!!! ReCharge...uSep[%lu] uLastChargeFailClientPoolId[%s]", itSession->first, itSession->second.m_strSubID.data());
            OemCharging(&Req, itSession->first, strtoul(itSession->second.m_strSubID.data(), NULL, 0));
            return;
        }
    }
    else//sql执行失败
    {
        HttpResponse(HTTP_RESPONSE_NO_FEE, itSession->second.m_uSeqRet, itSession->second.m_strSmsID, itSession->second.m_strPhone, "", 0, 0);

        goto CLEAR_SESSION;
    }

CLEAR_SESSION:
    LogDebug("m_OEMSesssionMap Delete Session[%lu]", itSession->first);

    if (itSession->second.m_pTimer != NULL)
    {
        delete itSession->second.m_pTimer;
        itSession->second.m_pTimer = NULL;
    }

    m_OEMSesssionMap.erase(itSession);
}

void ChargeThread::handleTimeoutReqMsg(TMsg* pMsg)
{
    if (pMsg->m_strSessionID == BRAND_OR_SALE_SESSION_TIMER_STRING)
    {
        map<UInt64, ChgSession>::iterator it = m_SesssionMap.find(pMsg->m_iSeq);

        if (it == m_SesssionMap.end())
        {
            LogDebug("can not find session, m_iSeq[%ld]", pMsg->m_iSeq);
            return;
        }

        ////delete session
        if (it->second.m_pTimer != NULL)
        {
            delete it->second.m_pTimer;
            it->second.m_pTimer = NULL;
        }

        m_SesssionMap.erase(it);
    }
    else if (pMsg->m_strSessionID == OEM_SESSION_TIMER_STRING)
    {
        map<UInt64, OEMChgSession>::iterator it = m_OEMSesssionMap.find(pMsg->m_iSeq);

        if (it == m_OEMSesssionMap.end())
        {
            LogDebug("can not find session, m_iSeq[%ld]", pMsg->m_iSeq);
            return;
        }

        if (it->second.m_pTimer != NULL)
        {
            delete it->second.m_pTimer;
            it->second.m_pTimer = NULL;
        }

        m_OEMSesssionMap.erase(it);
    }
    else
    {
        LogWarn("this timer sessionId is error %s", pMsg->m_strSessionID.data());
    }

}

