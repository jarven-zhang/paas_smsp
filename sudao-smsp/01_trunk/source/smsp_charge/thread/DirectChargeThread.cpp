#include "DirectChargeThread.h"
#include "LogMacro.h"
#include "RuleLoadThread.h"
#include "HttpServerThread.h"
#include "CDBThread.h"
#include "CDBThreadPool.h"
#include "Fmt.h"
#include "Comm.h"
#include "enum.h"
#include "global.h"

#include "boost/foreach.hpp"


#define foreach BOOST_FOREACH

DirectChargeThread* g_pDirectChargeThread = NULL;

extern CDBThreadPool* g_pDisPathDBThreadPool;


DirectChargeThread::Session::Session(DirectChargeThread* pThread, UInt64 ulSequence)
{
    m_eChargeType = ChargeType_Init;
    m_pGivePool = NULL;
    m_uiRetCode = 0;
    m_dSalePrice = 0;
    m_dProductCost = 0;

    m_pThread = pThread;
    m_ulSequence = (0!=ulSequence)?ulSequence:m_pThread->m_snManager.getSeq64();
    m_pThread->m_mapSession[m_ulSequence] = this;

    LogDebug("add session[%lu].", m_ulSequence);
}

DirectChargeThread::Session::~Session()
{
    if (0 != m_ulSequence)
    {
        m_pThread->m_mapSession.erase(m_ulSequence);
        LogDebug("del session[%lu].", m_ulSequence);
    }
}

DirectChargeThread::DirectChargeThread(const char* name) : CThread(name)
{
    m_vecPrice.resize(8);
    m_vecPrice[BusType::YIDONG] = "yd_sms_price";
    m_vecPrice[BusType::LIANTONG] = "lt_sms_price";
    m_vecPrice[BusType::DIANXIN] = "dx_sms_price";
    m_vecPrice[BusType::FOREIGN] = "gj_sms_discount";
    m_vecPrice[BusType::XNYD] = "yd_sms_price";
    m_vecPrice[BusType::XNLT] = "lt_sms_price";
    m_vecPrice[BusType::XNDX] = "dx_sms_price";
}

DirectChargeThread::~DirectChargeThread()
{
}

bool DirectChargeThread::start()
{
    g_pDirectChargeThread = new DirectChargeThread("DirectChargeThd");
    INIT_CHK_NULL_RET_FALSE(g_pDirectChargeThread);
    INIT_CHK_FUNC_RET_FALSE(g_pDirectChargeThread->init());
    INIT_CHK_FUNC_RET_FALSE(g_pDirectChargeThread->CreateThread());

    return true;
}

bool DirectChargeThread::init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    GET_RULELOAD_DATA_MAP(t_sms_agent_info, m_mapAgentInfo);
    GET_RULELOAD_DATA_VECMAP(t_sms_direct_client_give_pool, m_mapVecGivePool);
    //GET_RULELOAD_DATA_VEC(t_sms_user_property_log, m_vecUserPropertyLog);
	GET_RULELOAD_DATA_VECMAP(t_sms_user_property_log, m_mapVecUserPropertyLog);

    //initSmsSalePrice();
    return true;
}

bool DirectChargeThread::updateClientGivePool()
{
    for (SessionMapIter iter = m_mapSession.begin(); iter != m_mapSession.end(); iter++)
    {
        Session* pSession = iter->second;
        CHK_NULL_RETURN_FALSE(pSession);

        pSession->m_pGivePool = NULL;
    }

    return true;
}

#if 0
bool DirectChargeThread::initSmsSalePrice()
{
    m_mapSmsSalePrice.clear();

    foreach (AppData* pApp, m_vecUserPropertyLog)
    {
        TSmsUserPropertyLog* pAppData = (TSmsUserPropertyLog*)pApp;
        if (NULL == pAppData) continue;

        string strKey = pAppData->m_strClientId + ":" + pAppData->m_strProperty;

        SmsSalePriceMapIter iter1 = m_mapSmsSalePrice.find(strKey);

        if (m_mapSmsSalePrice.end() == iter1)
        {
            SmsSalePrice s;
            s.m_strDate = pAppData->m_strEffectDate;
            s.m_dPrice = to_double(pAppData->m_strValue);

            vector<SmsSalePrice> v;
            v.push_back(s);

            m_mapSmsSalePrice[strKey] = v;

//            LogDebug("insert %s %s %lf.", strKey.data(), s.m_strDate.data(), s.m_dPrice);
        }
        else
        {
            SmsSalePrice s;
            s.m_strDate = pAppData->m_strEffectDate;
            s.m_dPrice = to_double(pAppData->m_strValue);

            SmsSalePriceVec& v = iter1->second;
            v.push_back(s);

//            LogDebug("update %s %s %lf.", strKey.data(), s.m_strDate.data(), s.m_dPrice);
        }
    }


//    for (SmsSalePriceMapIter iter = m_mapSmsSalePrice.begin(); iter != m_mapSmsSalePrice.end(); ++iter)
//    {
//        const string& strKey = iter->first;
//        const SmsSalePriceVec& v = iter->second;
//
//        LogDebug("%s", strKey.data());
//
//        for (SmsSalePriceVec::const_iterator iterV = v.begin(); iterV != v.end(); ++iterV)
//        {
//            const SmsSalePrice& s = *iterV;
//
//            LogDebug(" --> %s - %lf", s.m_strDate.data(), s.m_dPrice);
//        }
//    }

    return true;
}
#endif

void DirectChargeThread::initWorkFuncMap()
{
    REG_MSG_CALLBACK(DirectChargeThread, MSGTYPE_TO_CHARGE_REQ, handleChargeReq);
    REG_MSG_CALLBACK(DirectChargeThread, MSGTYPE_DB_QUERY_RESP, handleChargeDbRes);
    REG_MSG_CALLBACK(DirectChargeThread, MSGTYPE_RULELOAD_DB_UPDATE_REQ, handleDbUpdateReq);
}

void DirectChargeThread::handleDbUpdateReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    DbUpdateReq* pReq = (DbUpdateReq*)pMsg;

    switch (pReq->m_uiTableId)
    {
        CASE_UPDATE_TABLE_MAP(t_sms_agent_info, m_mapAgentInfo);
        CASE_UPDATE_TABLE_VECMAP_FUNC(t_sms_direct_client_give_pool, m_mapVecGivePool, updateClientGivePool());
        //CASE_UPDATE_TABLE_VEC_FUNC(t_sms_user_property_log, m_vecUserPropertyLog, initSmsSalePrice());
		CASE_UPDATE_TABLE_VECMAP(t_sms_user_property_log, m_mapVecUserPropertyLog);

        default:{LogError("Invalid TableId:%u.", pReq->m_uiTableId);break;}
    }
}

void DirectChargeThread::handleChargeReq(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    DirectChargeReq* pReq = (DirectChargeReq*)pMsg;

    Session* pSession = new Session(this, pReq->m_iSeq);
    CHK_NULL_RETURN(pSession);
    pSession->setReq(*pReq);

    AppDataMapCIter iter = m_mapAgentInfo.find(pSession->m_req.m_strAgendId);
    if (m_mapAgentInfo.end() == iter)
    {
        LogError("[%s:%s:%s] Can't find AgendId[%s] in m_mapAgentInfo.",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data(),
            pSession->m_req.m_strAgendId.data());

        pSession->m_uiRetCode = HTTP_RESPONSE_NO_FEE;
        pSession->m_strOrderId.clear();
        pSession->m_dSalePrice = 0;
        pSession->m_dProductCost = 0;

        responseToClient(pSession);
        return;
    }

    TSmsAgentInfo* pSmsAgentInfo = (TSmsAgentInfo*)(iter->second);

    LogDebug("[%s:%s:%s] Agent-PayType[%u], Account-PayType[%u].",
        pSession->m_req.m_strClientID.data(),
        pSession->m_req.m_strSmsID.data(),
        pSession->m_req.m_strPhone.data(),
        pSmsAgentInfo->m_uiPayType,
        pSession->m_req.m_uiPayType);

    if (PayType_ChargeAgent == pSession->m_req.m_uiPayType)
    {
        if (PayType_Pre == pSmsAgentInfo->m_uiPayType)
        {
            processChargeAgentOrAccount(pSession);
        }
    }
    else if (PayType_ChargeAccount == pSession->m_req.m_uiPayType)
    {
        if ((PayType_Pre == pSmsAgentInfo->m_uiPayType) || (PayType_Post == pSmsAgentInfo->m_uiPayType))
        {
            processChargeAgentOrAccount(pSession);
        }
    }
    else
    {
        LogError("[%s:%s:%s] Agent-PayType[%u], Account-PayType[%u].",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data(),
            pSmsAgentInfo->m_uiPayType,
            pSession->m_req.m_uiPayType);
    }
}

bool DirectChargeThread::processChargeAgentOrAccount(Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    if (processChargeGivePool(pSession))
    {
        return true;
    }

    processChargeBalance(pSession);

    return true;
}

bool DirectChargeThread::processChargeGivePool(Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    AppDataVecMapIter iter = m_mapVecGivePool.find(pSession->m_req.m_strClientID);
    if (m_mapVecGivePool.end() == iter)
    {
        LogDebug("[%s:%s:%s] No give sms.",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data());

        return false;
    }

    AppDataVec& vecGivePool = iter->second;

    string strCurrentTime = Comm::getCurrentTime();
    LogDebug("strCurrentTime[%s].", strCurrentTime.data());

    foreach (AppData* pAppData, vecGivePool)
    {
        TSmsDirectClientGivePool* pGivePool = (TSmsDirectClientGivePool*)pAppData;
        CHK_NULL_RETURN_FALSE(pGivePool);

        LogDebug("[%s:%s:%s] give pool[id:%lu,clientid:%s,duetime:%s,remain:%u].",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data(),
            pGivePool->m_ulGivePoolId,
            pGivePool->m_strClientId.data(),
            pGivePool->m_strDueTime.data(),
            pGivePool->m_uiRemainNumber);

        if (pGivePool->m_uiRemainNumber < pSession->m_req.m_uiCount) continue;
        if (pGivePool->m_strDueTime < strCurrentTime) continue;

        // 保存赠送池id,获取销售价
        pSession->m_strOrderId = to_string(pGivePool->m_ulGivePoolId);
        pSession->m_dSalePrice = pGivePool->m_dUnitPrice;

        string strSql = Fmt<1024>("UPDATE t_sms_direct_client_give_pool "
                                  "SET remain_number=remain_number-%u,update_time='%s' "
                                  "WHERE give_pool_id=%lu AND remain_number>=%u;",
                                  pSession->m_req.m_uiCount,
                                  strCurrentTime.data(),
                                  pGivePool->m_ulGivePoolId,
                                  pSession->m_req.m_uiCount);

        LogNotice("[%s:%s:%s] sql[%s].",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data(),
            strSql.data());

        TDBQueryReq* pReq = new TDBQueryReq();
        pReq->m_SQL = strSql;
        pReq->m_strKey = to_string(pGivePool->m_ulGivePoolId);
        pReq->m_iSeq = pSession->m_ulSequence;
        pReq->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
        pReq->m_pSender = this;
        g_pDisPathDBThreadPool->PostMsg(pReq);

        pSession->m_eChargeType = Session::ChargeType_GivePool;
        pSession->m_pGivePool = pGivePool;
        return true;
    }

    return false;
}

bool DirectChargeThread::processChargeBalance(Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    // 清空赠送池id,获取销售价
    pSession->m_strOrderId.clear();
    getSalePrice(pSession);

    // 发送金额
    double dFeeSum = pSession->m_dSalePrice * pSession->m_req.m_uiCount;
    string strKey;
    string strSql;

    if (PayType_ChargeAgent == pSession->m_req.m_uiPayType)
    {
        strKey = pSession->m_req.m_strAgendId;

        strSql = Fmt<1024>("UPDATE t_sms_agent_account "
                           "SET "
                           "current_credit=(CASE WHEN (balance>0) THEN (CASE WHEN (balance<%lf) THEN (current_credit-(%lf-balance)) ELSE current_credit END) ELSE (current_credit-%lf) END),"
                           "no_back_payment=(CASE WHEN (balance>0) THEN (CASE WHEN (balance<%lf) THEN (%lf-balance) ELSE no_back_payment END) ELSE (no_back_payment+%lf) END),"
                           "balance=(balance-%lf) "
                           "WHERE agent_id='%s' AND (((balance>0) AND (balance+current_credit>%lf)) OR ((balance<=0) AND (current_credit>=%lf)));",
                           dFeeSum,dFeeSum,dFeeSum,
                           dFeeSum,dFeeSum,dFeeSum,
                           dFeeSum,
                           pSession->m_req.m_strAgendId.data(),dFeeSum,dFeeSum);
    }
    else if (PayType_ChargeAccount == pSession->m_req.m_uiPayType)
    {
        strKey = pSession->m_req.m_strClientID;

        strSql = Fmt<1024>("UPDATE t_sms_direct_client_account "
                           "SET balance=balance-%lf "
                           "WHERE clientid='%s' AND balance>=%lf;",
                           dFeeSum,
                           pSession->m_req.m_strClientID.data(),
                           dFeeSum);
    }
    else
    {
        LogError("[%s:%s:%s] Invalid PayType[%u].",
            pSession->m_req.m_strClientID.data(),
            pSession->m_req.m_strSmsID.data(),
            pSession->m_req.m_strPhone.data(),
            pSession->m_req.m_uiPayType);

        return false;
    }

    LogNotice("[%s:%s:%s] sql[%s].",
        pSession->m_req.m_strClientID.data(),
        pSession->m_req.m_strSmsID.data(),
        pSession->m_req.m_strPhone.data(),
        strSql.data());

    TDBQueryReq* pReq = new TDBQueryReq();
    pReq->m_SQL = strSql;
    pReq->m_strKey = strKey;
    pReq->m_iSeq = pSession->m_ulSequence;
    pReq->m_iMsgType = MSGTYPE_DB_UPDATE_QUERY_REQ;
    pReq->m_pSender = this;
    g_pDisPathDBThreadPool->PostMsg(pReq);

    pSession->m_eChargeType = Session::ChargeType_Balance;
    return true;
}

void DirectChargeThread::handleChargeDbRes(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TDBQueryResp* pRsp = (TDBQueryResp*)pMsg;

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pMsg->m_iSeq);
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    LogDebug("[%s:%s:%s] Result[%d], AffectRow[%d].",
        pSession->m_req.m_strClientID.data(),
        pSession->m_req.m_strSmsID.data(),
        pSession->m_req.m_strPhone.data(),
        pRsp->m_iResult,
        pRsp->m_iAffectRow);

    if ((pRsp->m_iResult == 0) && (pRsp->m_iAffectRow == 1))
    {
        if ((Session::ChargeType_GivePool == pSession->m_eChargeType)
        && (NULL != pSession->m_pGivePool)
        && (pSession->m_pGivePool->m_uiRemainNumber >= pSession->m_req.m_uiCount))
        {
            // 更新内存记录
            LogDebug("RemainNumber[%u],Count[%u].",
                pSession->m_pGivePool->m_uiRemainNumber,
                pSession->m_req.m_uiCount);

            pSession->m_pGivePool->m_uiRemainNumber -= pSession->m_req.m_uiCount;
        }

        responseToClient(pSession);
    }
    else
    {
        if (Session::ChargeType_GivePool == pSession->m_eChargeType)
        {
            processChargeBalance(pSession);
            return;
        }

        pSession->m_uiRetCode = HTTP_RESPONSE_NO_FEE;
        responseToClient(pSession);
    }
}

#if 0
void DirectChargeThread::getSalePrice(Session* pSession)
{
    CHK_NULL_RETURN(pSession);

    double dSalePrice = 0;
    string strKey = pSession->m_req.m_strClientID + ":" + m_vecPrice[pSession->m_req.m_uiOperater];
    SmsSalePriceMapIter iter1 = m_mapSmsSalePrice.find(strKey);

    if (m_mapSmsSalePrice.end() != iter1)
    {
        const SmsSalePriceVec& v = iter1->second;
        const string strTime = getCurrentDateTime("%Y-%m-%d");

        for (SmsSalePriceVec::const_iterator iterV = v.begin(); iterV != v.end(); ++iterV)
        {
            const SmsSalePrice& s = *iterV;
            LogDebug(" --> %s - %lf", s.m_strDate.data(), s.m_dPrice);

            if (s.m_strDate <= strTime) {dSalePrice = s.m_dPrice;}
            else break;
        }
    }

    if (BusType::FOREIGN == pSession->m_req.m_uiOperater)
    {
        // 值=前缀对应客户国际费率(access组件传过来的fee字段) * (clientid + gj_sms_discount)对应的生效折扣
        // 涉及t_sms_client_tariff、t_sms_user_property_log
        pSession->m_dSalePrice = (pSession->m_req.m_dFee * 0.000001) * (dSalePrice * 0.1);
        pSession->m_dProductCost = pSession->m_req.m_dFee;
    }
    else
    {
        // 值 = (clientid + 手机号码所属运营商)对应的生效单价(t_sms_user_property_log)
        pSession->m_dSalePrice = dSalePrice;
        pSession->m_dProductCost = 0;
    }

    LogDebug("[%s:%s:%s] SalePrice[%lf], ProductCost[%lf].",
        pSession->m_req.m_strClientID.data(),
        pSession->m_req.m_strSmsID.data(),
        pSession->m_req.m_strPhone.data(),
        pSession->m_dSalePrice,
        pSession->m_dProductCost);
}
#endif

void DirectChargeThread::getSalePrice(Session* pSession)
{
	CHK_NULL_RETURN(pSession);

	double dSalePrice = 0;
	string strKey = pSession->m_req.m_strClientID + ":" + m_vecPrice[pSession->m_req.m_uiOperater];
	AppDataVecMapIter itrMap = m_mapVecUserPropertyLog.find(strKey);

	if (m_mapVecUserPropertyLog.end() != itrMap)
	{
		const vector<AppData*>& v = itrMap->second;
		const string strTime = getCurrentDateTime("%Y-%m-%d %H:%M:%S");
		
		for (vector<AppData*>::const_iterator iter = v.begin(); iter != v.end(); ++iter)
		{
			const TSmsUserPropertyLog* pUserProperty = (TSmsUserPropertyLog*)(*iter);
			if (pUserProperty->m_strEffectDate <= strTime)
			{
				dSalePrice = to_double(pUserProperty->m_strValue);
			}
			else
			{
				break;
			}
		}
	}

	if (BusType::FOREIGN == pSession->m_req.m_uiOperater)
	{
		// 值=前缀对应客户国际费率(access组件传过来的fee字段) * (clientid + gj_sms_discount)对应的生效折扣
		// 涉及t_sms_client_tariff、t_sms_user_property_log
		pSession->m_dSalePrice = (pSession->m_req.m_dFee * 0.000001) * (dSalePrice * 0.1);
		pSession->m_dProductCost = pSession->m_req.m_dFee;
	}
	else
	{
		// 值 = (clientid + 手机号码所属运营商)对应的生效单价(t_sms_user_property_log)
		pSession->m_dSalePrice = dSalePrice;
		pSession->m_dProductCost = 0;
	}

	LogDebug("[%s:%s:%s] SalePrice[%lf], ProductCost[%lf].",
		pSession->m_req.m_strClientID.data(),
		pSession->m_req.m_strSmsID.data(),
		pSession->m_req.m_strPhone.data(),
		pSession->m_dSalePrice,
		pSession->m_dProductCost);
}

bool DirectChargeThread::responseToClient(Session* pSession)
{
    CHK_NULL_RETURN_FALSE(pSession);

    THttpQueryResp* pRsp = new THttpQueryResp();
    CHK_NULL_RETURN_FALSE(pRsp);

    pRsp->m_iSeq = pSession->m_req.m_iSeq;
    pRsp->m_uiReturn = pSession->m_uiRetCode;
    pRsp->m_strSubID = pSession->m_strOrderId;
    pRsp->m_dSlaePrice = pSession->m_dSalePrice * 1000000;
    pRsp->m_dProductCost = pSession->m_dProductCost;
    pRsp->m_iMsgType = MSGTYPE_DISPATCH_TO_SMSERVICE_RESP;
    g_pHttpServiceThread->PostMsg(pRsp);

    return true;
}

