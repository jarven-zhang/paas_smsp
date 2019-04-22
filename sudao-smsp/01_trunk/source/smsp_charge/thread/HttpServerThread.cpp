#include "HttpServerThread.h"
#include "CRuleLoadThread.h"
#include "LogMacro.h"
#include "ChargeThread.h"
#include "DirectChargeThread.h"
#include "global.h"
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "Comm.h"
#include "enum.h"
#include "Fmt.h"


extern UInt32 g_uSecSleep;

HttpServerThread* g_pHttpServiceThread = NULL;

const UInt64 HTTPSERVER_SESSION_TIMER_ID = 12;
const int HTTPSERVER_SESSION_TIMEOUT = 10*1000;


#define CHK_UINT_RET(var, min, max)                    \
    if (((var) < (min)) || ((var) > (max)))            \
    {                                                  \
        LogError("Invalid parameter(%s) value(%u).",   \
            VNAME(var), var);                          \
        return false;                                   \
    }


#define CHK_DOUBLE_RET(var, min, max)                   \
    if (((var) < (min)) || ((var) > (max)))             \
    {                                                   \
        LogError("Invalid parameter(%s) value(%lf).",   \
            VNAME(var), var);                           \
        return false;                                    \
    }

HttpServerThread::Session::Session(HttpServerThread* pThread, UInt64 ulSequence)
{
    m_webHandler = NULL;
    m_wheelTimer = NULL;

    m_uiAgentType = 0;
    m_uiPayType = 0;
    m_uiOperaterCode = 0;
    m_uiArea = 0;
    m_uiCount = 0;
    m_dFee = 0;
    m_uiClientType = 0;

    m_pThread = pThread;
    m_ulSequence = (0!=ulSequence)?ulSequence:m_pThread->m_snManager.getSn();
    m_pThread->m_mapSession[m_ulSequence] = this;

    LogDebug("add session[%lu].", m_ulSequence);
}

HttpServerThread::Session::~Session()
{
    SAFE_DELETE(m_wheelTimer);

    if (m_webHandler)
    {
        m_webHandler->Destroy();
        delete m_webHandler;
    }

    if (0 != m_ulSequence)
    {
        m_pThread->m_mapSession.erase(m_ulSequence);
        LogDebug("del session[%lu].", m_ulSequence);
    }
}

bool HttpServerThread::Session::parseReq(cs_t strData)
{
    web::HttpParams param;
    param.Parse(strData);

    m_strClientid = param._map["clientid"];
    m_strProductType = param._map["producttype"];
    m_strSmsID = param._map["smsid"];
    m_strPhone = param._map["phone"];
    m_strAgendId = param._map["agent_id"];

    string strCount = param._map["count"];
    string strPayType = param._map["paytype"];
    string strAgentType = param._map["agenttype"];
    string strFee = param._map["fee"];
    string strClientType = param._map["client_ascription"];
    string strOperaterCode = param._map["operator_code"];
    string strArea = param._map["area_id"];

    if (m_strClientid.empty()
     || strPayType.empty()
     || strOperaterCode.empty()
     || strCount.empty()
     || m_strSmsID.empty()
     || m_strPhone.empty()
     || strClientType.empty())
    {
        LogError("Invalid parameter value. Clientid[%s],PayType[%s],"
            "OperaterCode[%s],Count[%s],SmsID[%s],Phone[%s],ClientType[%s].",
            m_strClientid.data(),
            strPayType.data(),
            strOperaterCode.data(),
            strCount.data(),
            m_strSmsID.data(),
            m_strPhone.data(),
            strClientType.data());

        return false;
    }

     if (strOperaterCode=="4" && strFee.empty())
     {
         LogError("Invalid parameter value. Clientid[%s],"
             "OperaterCode[%s],SmsID[%s],Phone[%s],strFee[%s].",
             m_strClientid.data(),
             strOperaterCode.data(),
             m_strSmsID.data(),
             m_strPhone.data(),
             strFee.data());

         return false;
     }

    m_uiCount = to_uint32(strCount);
    m_uiPayType = to_uint32(strPayType);
    m_uiAgentType = to_uint32(strAgentType);
    m_dFee = to_double(strFee);
    m_uiClientType = to_uint32(strClientType);
    m_uiOperaterCode = to_uint32(strOperaterCode);
    m_uiArea = to_uint32(strArea);

    CHK_UINT_RET(m_uiCount, 0, 10);
    CHK_UINT_RET(m_uiPayType, 0, 3);
    CHK_UINT_RET(m_uiAgentType, 1, 5);
//    CHK_DOUBLE_RET(m_dFee, 0, 1000);

    if (((PayType_ChargeAgent == m_uiPayType) || (PayType_ChargeAccount == m_uiPayType))
    && (5 != m_uiAgentType))
    {
        LogError("[%s:%s:%s] PayType[%u], but AgentType[%u].",
            m_strClientid.data(),
            m_strSmsID.data(),
            m_strPhone.data(),
            m_uiPayType,
            m_uiAgentType);

        return false;
    }

    return true;
}

HttpServerThread::HttpServerThread(const char* name) : CThread(name)
{
    m_pInternalService = NULL;
    m_pServerSocekt = NULL;
    m_pLinkedBlockPool = NULL;
}

HttpServerThread::~HttpServerThread()
{
}

bool HttpServerThread::start()
{
    g_pHttpServiceThread = new HttpServerThread("HttpServerThd");
    INIT_CHK_NULL_RET_FALSE(g_pHttpServiceThread);
    INIT_CHK_FUNC_RET_FALSE(g_pHttpServiceThread->Init());
    INIT_CHK_FUNC_RET_FALSE(g_pHttpServiceThread->CreateThread());

    return true;
}

bool HttpServerThread::Init()
{
    INIT_CHK_FUNC_RET_FALSE(CThread::Init());

    ComponentConfig componentCfg;
    g_pRuleLoadThread->getComponentConfig(componentCfg);

    ListenPortMap mapListenPort;
    g_pRuleLoadThread->getListenPort(mapListenPort);

    ListenPortMapIter iter;
    INIT_CHK_MAP_FIND_STR_RET_FALSE(mapListenPort, iter, string("charge"));
    const ListenPort& listenPort = iter->second;

    m_pInternalService = new InternalService();
    INIT_CHK_NULL_RET_FALSE(m_pInternalService);
    INIT_CHK_FUNC_RET_FALSE(m_pInternalService->Init());

    m_pServerSocekt = m_pInternalService->CreateServerSocket(this);
    INIT_CHK_FUNC_RET_FALSE(m_pServerSocekt->Listen(Address(componentCfg.m_strHostIp, listenPort.m_uPortValue)));

    m_pLinkedBlockPool = new LinkedBlockPool();
    INIT_CHK_NULL_RET_FALSE(m_pLinkedBlockPool);

    return true;
}

void HttpServerThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while (true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if (pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }
        else if (pMsg != NULL)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void HttpServerThread::initWorkFuncMap()
{
    REG_MSG_CALLBACK(HttpServerThread, MSGTYPE_ACCEPTSOCKET_REQ, handleAcceptSocketReqMsg);
    REG_MSG_CALLBACK(HttpServerThread, MSGTYPE_HTTP_SERVICE_REQ, handleHttpServiceReqMsg);
    REG_MSG_CALLBACK(HttpServerThread, MSGTYPE_SOCKET_WRITEOVER, handleReturnOverMsg);
    REG_MSG_CALLBACK(HttpServerThread, MSGTYPE_DISPATCH_TO_SMSERVICE_RESP, handleHttpRspMsg);
    REG_MSG_CALLBACK(HttpServerThread, MSGTYPE_TIMEOUT, HandleTimerOutMsg);
}

void HttpServerThread::handleAcceptSocketReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    TAcceptSocketMsg* pReq = (TAcceptSocketMsg*)pMsg;

    Session* pSession = new Session(this);
    CHK_NULL_RETURN(pSession);

    HttpSocketHandler* pSocketHandler = new HttpSocketHandler(this, pSession->m_ulSequence);
    CHK_NULL_RETURN(pSocketHandler);

    if (!pSocketHandler->Init(m_pInternalService, pReq->m_iSocket, pReq->m_address))
    {
        LogError("Call pSocketHandler->Init failed.")
        delete pSocketHandler;
        return;
    }

    pSession->m_webHandler = pSocketHandler;
    pSession->m_wheelTimer = SetTimer(pSession->m_ulSequence, "", HTTPSERVER_SESSION_TIMEOUT);
}

void HttpServerThread::handleHttpServiceReqMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    THttpRequest* pHttpReq = (THttpRequest*)pMsg;

    LogNotice("==chargeReq== body:%s.", pHttpReq->m_strRequest.data());

    //  clientid=b01409&agenttype=4&producttype=0&operator_code=1&area_id=176&count=1&fee=0
    //  &smsid=c78de009-24a4-4f50-88c8-9623955a17b3&phone=13581231503&client_ascription=1


    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pHttpReq->m_iSeq)
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    if (!pSession->parseReq(pHttpReq->m_strRequest))
    {
        LogError("Call checkPara failed.");

        THttpQueryResp* pRsp = new THttpQueryResp;
        pRsp->m_strSmsID = pSession->m_strSmsID;
        pRsp->m_strPhone = pSession->m_strPhone;
        pRsp->m_uiReturn = HTTP_RESPONSE_NO_PARMA_ERROR;
        pRsp->m_iSeq = pHttpReq->m_iSeq;

        handleHttpRspMsg(pRsp);
        delete pRsp;
        return;
    }

    if (PayType_Pre == pSession->m_uiPayType)
    {
        THttpQueryReq* pReq = new THttpQueryReq();
        pReq->m_iMsgType = MSGTYPE_TO_CHARGE_REQ;
        pReq->m_iSeq = pHttpReq->m_iSeq;
        pReq->m_strClientID = pSession->m_strClientid;
        pReq->m_strProductType = pSession->m_strProductType;
        pReq->m_uAgentType = pSession->m_uiAgentType;
        pReq->m_uOperater = pSession->m_uiOperaterCode;
        pReq->m_uArea = pSession->m_uiArea;
        pReq->m_uCount = pSession->m_uiCount;
        pReq->m_strFee = to_string(pSession->m_dFee);
        pReq->m_strSmsID = pSession->m_strSmsID;
        pReq->m_strPhone = pSession->m_strPhone;
        pReq->m_uClientAscription = pSession->m_uiClientType;
        g_pChargeThread->PostMsg(pReq);
    }
    else if ((PayType_ChargeAgent == pSession->m_uiPayType) || (PayType_ChargeAccount == pSession->m_uiPayType))
    {
        DirectChargeReq* pReq = new DirectChargeReq();
        pReq->m_iMsgType = MSGTYPE_TO_CHARGE_REQ;
        pReq->m_iSeq = pHttpReq->m_iSeq;
        pReq->m_strClientID = pSession->m_strClientid;
        pReq->m_uiAgentType = pSession->m_uiAgentType;
        pReq->m_strAgendId = pSession->m_strAgendId;
        pReq->m_uiPayType = pSession->m_uiPayType;
        pReq->m_strProductType = pSession->m_strProductType;
        pReq->m_uiOperater = pSession->m_uiOperaterCode;
        pReq->m_uiArea = pSession->m_uiArea;
        pReq->m_uiCount = pSession->m_uiCount;
        pReq->m_dFee = pSession->m_dFee;
        pReq->m_strSmsID = pSession->m_strSmsID;
        pReq->m_strPhone = pSession->m_strPhone;
        pReq->m_uiClientAscription = pSession->m_uiClientType;
        g_pDirectChargeThread->PostMsg(pReq);
    }
}

void HttpServerThread::handleHttpRspMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);
    THttpQueryResp* pRsp = (THttpQueryResp*)pMsg;

    LogDebug("Sequence[%lu].", pRsp->m_iSeq);

    SessionMapIter iter;
    CHK_MAP_FIND_UINT_RET(m_mapSession, iter, pRsp->m_iSeq)
    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    string strResponse;
    strResponse.append("result=");
    strResponse.append(Comm::int2str(pRsp->m_uiReturn));
    strResponse.append("&orderid=");
    strResponse.append(pRsp->m_strSubID);
    strResponse.append("&sale_price=");
    strResponse.append(Comm::double2str(pRsp->m_dSlaePrice));
    strResponse.append("&product_cost=");
    strResponse.append(Comm::double2str(pRsp->m_dProductCost));
    strResponse.append("&product_type=");
    strResponse.append((INVALID_UINT32==pRsp->m_uiProductType)?"":Comm::int2str(pRsp->m_uiProductType));

    LogNotice("[%s:%s:%s] ==chargeResp== result[%s].",
        pSession->m_strClientid.data(),
        pSession->m_strSmsID.data(),
        pSession->m_strPhone.data(),
        strResponse.data());

    string strContent;
    http::HttpResponse respone;
    respone.SetStatusCode(200);
    respone.SetContent(strResponse);
    respone.Encode(strContent);

    CHK_NULL_RETURN(pSession->m_webHandler);
    CHK_NULL_RETURN(pSession->m_webHandler->m_socket);

    pSession->m_webHandler->m_socket->Out()->Write(strContent.data(), strContent.size());
    pSession->m_webHandler->m_socket->Out()->Flush();

    SAFE_DELETE(pSession);
}

void HttpServerThread::handleReturnOverMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    SessionMapIter iter = m_mapSession.find(pMsg->m_iSeq);
    if (m_mapSession.end() == iter)
    {
        return;
    }

    Session* pSession = iter->second;
    CHK_NULL_RETURN(pSession);

    SAFE_DELETE(pSession);
}

void HttpServerThread::HandleTimerOutMsg(TMsg* pMsg)
{
    CHK_NULL_RETURN(pMsg);

    LogNotice("Session[id:%lu,name:%s], Session.size[%u]",
        pMsg->m_iSeq,
        pMsg->m_strSessionID.data(),
        m_mapSession.size());

    handleReturnOverMsg(pMsg);
}

