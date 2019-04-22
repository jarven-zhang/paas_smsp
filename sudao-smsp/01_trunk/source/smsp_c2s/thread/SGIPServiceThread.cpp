#include <mysql.h>
#include "SGIPServiceThread.h"
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"

SGIPServiceThread::SGIPServiceThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
}

SGIPServiceThread::~SGIPServiceThread()
{
    delete m_pSnManager;
}

bool SGIPServiceThread::Init(const std::string ip, unsigned int port)
{
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
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

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }

    m_pInternalService->Init();

    m_pServerSocekt = m_pInternalService->CreateServerSocket(this);
    if (false == m_pServerSocekt->Listen(Address(ip, port)))
    {
        printf("m_pServerSocekt->Listen is failed.\n");
        return false;
    }

    m_pLinkedBlockPool = new LinkedBlockPool();

    return true;
}

void SGIPServiceThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();
        TMsg* pMsg = NULL;
        pthread_mutex_lock(&m_mutex);
        pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }
        else if(pMsg != NULL)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
    return;
}

void SGIPServiceThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_ACCEPTSOCKET_REQ:
        {
            HandleSGIPAcceptSocketMsg(pMsg);
            break;
        }
        case MSGTYPE_TCP_LOGIN_RESP:
        {
            HandleLoginResp(pMsg);
            break;
        }
        case MSGTYPE_TCP_SUBMIT_RESP:
        {
            HandleSubmitResp(pMsg);
            break;
        }
        case MSGTYPE_SGIP_LINK_CLOSE_REQ:
        {
            HandleSGIPLinkCloseReq(pMsg);
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
        case MSGTYPE_TIMEOUT:
        {
            HandleTimerOutMsg(pMsg);
            break;
        }
        default:
        {
            LogWarn("msgType[0x%x] is invalied.",pMsg->m_iMsgType);
            break;
        }
    }

}

void SGIPServiceThread::HandleSGIPAcceptSocketMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iLinkId = m_pSnManager->getSn();
    char charLinkId[64] = {0};
    snprintf(charLinkId,64, "%ld", iLinkId);
    string strLinkId = charLinkId;

    SGIPSocketHandler *pSGIPSocketHandler = new SGIPSocketHandler(this, strLinkId);
    if(false == pSGIPSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("SGIPServiceThread::HandleSGIPAcceptSocketMsg is faield.")
        delete pSGIPSocketHandler;
        return;
    }

    LogDebug("add session,smservice  LinkId[%s]",strLinkId.data());
    SGIPLinkManager* pSession = new SGIPLinkManager();
    pSession->m_SGIPHandler = pSGIPSocketHandler;
    pSession->m_ExpireTimer = SetTimer(EXPIRE_TIMER_ID,strLinkId,EXPIRE_TIMER_TIMEOUT);

    m_LinkMap[strLinkId] = pSession;
}

void SGIPServiceThread::HandleSubmitResp(TMsg* pMsg)
{
    CTcpSubmitRespMsg* pResp = (CTcpSubmitRespMsg*)pMsg;
    SGIPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogElk("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_uNoHBRespCount =0;


    delete iterLink->second->m_ExpireTimer;
    iterLink->second->m_ExpireTimer = SetTimer(EXPIRE_TIMER_ID,pResp->m_strLinkId,EXPIRE_TIMER_TIMEOUT);

    SGIPSubmitResp respSubmit;
    respSubmit._result = pResp->m_uResult;
    respSubmit.sequenceIdNode_ = pResp->m_uSequenceIdNode;
    respSubmit.sequenceIdTime_ = pResp->m_uSequenceIdTime;
    respSubmit.sequenceId_ = pResp->m_uSequenceId;

    pack::Writer writer(iterLink->second->m_SGIPHandler->m_socket->Out());
    respSubmit.Pack(writer);

    LogElk("==sgip push submitResp== sequenceIdNode:%d,sequenceIdTime:%d,sequenceId:%d,clientid:%s,result:%d.",
        pResp->m_uSequenceIdNode,pResp->m_uSequenceIdTime,pResp->m_uSequenceId,iterLink->second->m_strAccount.data(),pResp->m_uResult);
    return;
}

void SGIPServiceThread::HandleLoginResp(TMsg* pMsg)
{
    CTcpLoginRespMsg* pResp = (CTcpLoginRespMsg*)pMsg;
    SGIPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_strAccount = pResp->m_strAccount;
    iterLink->second->m_uNoHBRespCount =0;

    delete iterLink->second->m_ExpireTimer;
    iterLink->second->m_ExpireTimer = SetTimer(EXPIRE_TIMER_ID,pResp->m_strLinkId,EXPIRE_TIMER_TIMEOUT);

    SGIPConnectResp LoginResp;
    LoginResp.result_ = pResp->m_uResult;
    LoginResp.sequenceIdNode_ = pResp->m_uSequenceIdNode;
    LoginResp.sequenceIdTime_ = pResp->m_uSequenceIdTime;
    LoginResp.sequenceId_ = pResp->m_uSequenceId;

    pack::Writer writer(iterLink->second->m_SGIPHandler->m_socket->Out());
    LoginResp.Pack(writer);
    if(0 == pResp->m_uResult)
    {
        iterLink->second->m_SGIPHandler->m_iLinkState = LINK_CONNECTED;
        LogElk("clientid:%s,linkId:%s,connect login sucess.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }
    else
    {
        LogElk("clientid:%s,linkId:%s,connect login failed.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }

    return;
}

void SGIPServiceThread::HandleSGIPLinkCloseReq(TMsg* pMsg)
{
    LogDebug("recv close link[%s] ", pMsg->m_strSessionID.data());
    
    CTcpCloseLinkReqMsg *pTLinkCloseReq = (CTcpCloseLinkReqMsg*)pMsg;

    SGIPLinkMap::iterator itLink = m_LinkMap.find(pTLinkCloseReq->m_strLinkId);
    if(itLink != m_LinkMap.end())
    {
        //主动断开连接
        LogElk("clientid:%s,linkId:%s, recv link close req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        if(itLink->second->m_SGIPHandler != NULL)
        {
            SGIPUnBindReq req;
            req.sequenceId_ = m_pSnManager->getDirectSequenceId();
            pack::Writer writer(itLink->second->m_SGIPHandler->m_socket->Out());
            req.Pack(writer);
            LogNotice("clientid:%s,linkId:%s,terminate req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        }
        CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
        pDis->m_strAccount = pTLinkCloseReq->m_strAccount;
        pDis->m_strLinkId = pTLinkCloseReq->m_strLinkId;
        pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
        g_pUnitypThread->PostMsg(pDis);

        //close handle
        if(itLink->second->m_SGIPHandler != NULL)
        {
            delete itLink->second->m_SGIPHandler;
            itLink->second->m_SGIPHandler = NULL;
        }
        delete itLink->second->m_ExpireTimer;
        delete itLink->second;
        m_LinkMap.erase(itLink);
    }
    else
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap!", pTLinkCloseReq->m_strLinkId.data());
    }
}

void SGIPServiceThread::HandleTimerOutMsg(TMsg* pMsg)
{
    if(EXPIRE_TIMER_ID == (UInt32)pMsg->m_iSeq) //send CMPP heartbeat timer.
    {
        SGIPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
        if(it == m_LinkMap.end())
        {
            LogWarn("can't find session,LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        if(it->second->m_SGIPHandler->m_iLinkState == LINK_CONNECTED)
        {
            CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
            pDis->m_strAccount = it->second->m_SGIPHandler->m_strAccount;
            pDis->m_strLinkId = pMsg->m_strSessionID;
            pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
            g_pUnitypThread->PostMsg(pDis);
        }

        if (NULL != it->second->m_SGIPHandler)
        {
            LogWarn("account[%s] over 30 is no data stream.",it->second->m_SGIPHandler->m_strAccount.data());
            delete it->second->m_SGIPHandler;
        }

        if(NULL != it->second->m_ExpireTimer)
        {
            delete it->second->m_ExpireTimer;
        }

        delete it->second;
        m_LinkMap.erase(it);
    }
    else
    {
        LogWarn("no this timerId[%ld].", pMsg->m_iSeq);
    }

    return;
}

UInt32 SGIPServiceThread::GetSessionMapSize()
{
    return m_LinkMap.size();
}
