#include <mysql.h>
#include "SMGPServiceThread.h"
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"
#include "SMGPDeliverReq.h"
#include "base64.h"
#include "MqManagerThread.h"



SMGPServiceThread::SMGPServiceThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
}

SMGPServiceThread::~SMGPServiceThread()
{
    delete m_pSnManager;
}

bool SMGPServiceThread::Init(const std::string ip, unsigned int port)
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
    if (false ==m_pServerSocekt->Listen(Address(ip, port)))
    {
        printf("m_pServerSocekt->Listen is failed.\n");
        return false;
    }

    m_pLinkedBlockPool = new LinkedBlockPool();

    return true;
}

void SMGPServiceThread::MainLoop()
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

void SMGPServiceThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_ACCEPTSOCKET_REQ:  //come from InternalServerSocket
        {
            HandleSMGPAcceptSocketMsg(pMsg);
            break;
        }
        case MSGTYPE_TCP_LOGIN_RESP:    //come from unitythread
        {
            HandleLoginResp(pMsg);
            break;
        }
        case MSGTYPE_TCP_SUBMIT_RESP:   //come from SMGPSocketHandler
        {
            HandleSubmitResp(pMsg);
            break;
        }
        case MSGTYPE_TCP_DELIVER_REQ:   //come from unitythread where  message was failed
        {
            HandleDeliverReq(pMsg);
            break;
        }
        case MSGTYPE_SMGP_HEART_BEAT_REQ:   //come from SMGPSocketHander
        {
            HandleSMGPHeartBeatReq(pMsg);
            break;
        }
        case MSGTYPE_SMGP_HEART_BEAT_RESP:
        {
            HandleSMGPHeartBeatResp(pMsg);
            break;
        }
        case MSGTYPE_SMGP_LINK_CLOSE_REQ:
        {
            HandleSMGPLinkCloseReq(pMsg);
            break;
        }
        case MSGTYPE_REPORTRECEIVE_TO_SMS_REQ:  //come from reportReceiveThread
        {
            HandleReportReceiveMsg(pMsg);
            break;
        }
        case MSGTYPE_ACCESS_MO_MSG:
        {
            HandleMoMsg(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisRspSendReportToSMGP(pMsg);
            break;
        }
        case MSGTYPE_REPORT_GET_AGAIN_REQ:
        {
            HandleReportGetAgain(pMsg);
            break;
        }
        case MSGTYPE_MO_GET_AGAIN_REQ:
        {
            HandleMOGetAgain(pMsg);
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
            break;
        }
    }

}

void SMGPServiceThread::HandleDeliverReq(TMsg* pMsg)
{
    CTcpDeliverReqMsg* pDel = (CTcpDeliverReqMsg*)pMsg;
    SMGPLinkMap::iterator iterLink = m_LinkMap.find(pDel->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pDel->m_strLinkId.data());
        return;
    }

    smsp::SMGPDeliverReq reqDeliver;
    reqDeliver.m_uSubmitId = pDel->m_uSubmitId;
    reqDeliver._status = pDel->m_uResult;
    reqDeliver._srcTermId = pDel->m_strPhone;
    /////LogDebug("msgid:[%d]:srcTermId:[%s]",pDel->m_uSubmitId,pDel->m_strPhone);

    if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
    {
        reqDeliver._remark = "DELIVRD";
        reqDeliver._err = "000";
    }
    else
    {
        reqDeliver._remark = "UNDELIV";
        reqDeliver._err = "999";
    }
    reqDeliver.sequenceId_ = m_pSnManager->getDirectSequenceId();
    reqDeliver._isReport = 1;

    pack::Writer writer(iterLink->second->m_SMGPHandler->m_socket->Out());
    reqDeliver.Pack(writer);

    LogElk("==smgp push report==[%s:%s]",iterLink->second->m_strAccount.data(), pDel->m_strPhone.data());

    return;
}

void SMGPServiceThread::HandleSubmitResp(TMsg* pMsg)
{
    CTcpSubmitRespMsg* pResp = (CTcpSubmitRespMsg*)pMsg;
    SMGPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_uNoHBRespCount =0;

    smsp::SMGPSubmitResp respSubmit;
    respSubmit.status_ = pResp->m_uResult;
    ////respSubmit.msgId_.assign(cMsgId);
    respSubmit.m_uSubmitId = pResp->m_uSubmitId;
    respSubmit.sequenceId_ = pResp->m_uSequenceNum;
    pack::Writer writer(iterLink->second->m_SMGPHandler->m_socket->Out());
    respSubmit.Pack(writer);
    LogElk("==smgp push submitResp==  submitid:%lu,clientid:%s,result:%d.",
        pResp->m_uSubmitId,iterLink->second->m_strAccount.data(),pResp->m_uResult);
}

void SMGPServiceThread::HandleLoginResp(TMsg* pMsg)
{
    CTcpLoginRespMsg* pResp = (CTcpLoginRespMsg*)pMsg;
    SMGPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_strAccount = pResp->m_strAccount;
    iterLink->second->m_uNoHBRespCount =0;

    smsp::SMGPConnectResp LoginResp;
    LoginResp.status_ = pResp->m_uResult;
    LoginResp.serverVersion_ = 3;
    LoginResp.sequenceId_= pResp->m_uSequenceNum;

    pack::Writer writer(iterLink->second->m_SMGPHandler->m_socket->Out());
    LoginResp.Pack(writer);
    if(0 == pResp->m_uResult)
    {
        iterLink->second->m_SMGPHandler->m_iLinkState = LINK_CONNECTED;
        LogElk("clientid:%s,linkId:%s,connect login sucess.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }
    else
    {
        LogElk("clientid:%s,linkId:%s,connect login failed.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }

    return;
}

void SMGPServiceThread::HandleSMGPLinkCloseReq(TMsg* pMsg)
{
    /////LogDebug("recv close link[%s] ", pMsg->m_strSessionID.data());
    CTcpCloseLinkReqMsg *pTLinkCloseReq = (CTcpCloseLinkReqMsg*)pMsg;

    SMGPLinkMap::iterator itLink = m_LinkMap.find(pTLinkCloseReq->m_strLinkId);
    if(itLink != m_LinkMap.end())
    {
        LogElk("clientid:%s,linkId:%s, recv link close req.",pTLinkCloseReq->m_strAccount.data(),pMsg->m_strSessionID.data());
        //主动断开连接
        if(itLink->second->m_SMGPHandler != NULL)
        {
            smsp::SMGPTerminateReq reqTerminate;
            reqTerminate.sequenceId_ = m_pSnManager->getDirectSequenceId();
            pack::Writer writer(itLink->second->m_SMGPHandler->m_socket->Out());
            reqTerminate.Pack(writer);
            LogNotice("clientid:%s,linkId:%s,terminate req."
                ,pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        }
        CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
        pDis->m_strAccount = pTLinkCloseReq->m_strAccount;
        pDis->m_strLinkId = pTLinkCloseReq->m_strLinkId;
        pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
        g_pUnitypThread->PostMsg(pDis);
      

        //close handle
        if(itLink->second->m_SMGPHandler != NULL)
        {
            delete itLink->second->m_SMGPHandler;
            itLink->second->m_SMGPHandler = NULL;
        }
        delete itLink->second->m_HeartBeatTimer;
        delete itLink->second;
        m_LinkMap.erase(itLink);
    }
    else
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap!", pTLinkCloseReq->m_strLinkId.data());
    }
}

void SMGPServiceThread::HandleSMGPAcceptSocketMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iLinkId = m_pSnManager->getSn();
    char charLinkId[64] = { 0 };
    snprintf(charLinkId,64, "%ld", iLinkId);
    string strLinkId = charLinkId;

    SMGPSocketHandler *pSMGPSocketHandler = new SMGPSocketHandler(this, strLinkId);
    if(false == pSMGPSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("SMGPServiceThread::procSMServiceAcceptSocket is faield.")
        delete pSMGPSocketHandler;
        return;
    }

    LogDebug("add session,smservice  LinkId[%s]",strLinkId.data());
    SMGPLinkManager* pSession = new SMGPLinkManager();
    pSession->m_SMGPHandler = pSMGPSocketHandler;
    pSession->m_HeartBeatTimer = SetTimer(SMGP_HEART_BEAT_TIMER_ID, strLinkId, HEART_BEAT_TIMEOUT);

    m_LinkMap[strLinkId] = pSession;
}

void SMGPServiceThread::HandleSMGPHeartBeatReq(TMsg* pMsg)
{
    TSMGPHeartBeatReq *pHeartBeatReq = (TSMGPHeartBeatReq*)pMsg;
    //LogDebug("handle client's heartbeat req  sequenceId:[%d]",pHeartBeatReq->m_reqHeartBeat->sequenceId_);

    SMGPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }

    if(NULL != pHeartBeatReq->m_socketHandle && NULL != pHeartBeatReq->m_socketHandle->m_socket)
    {
        it->second->m_uNoHBRespCount = 0;
        smsp::SMGPHeartbeatResp respHeartBeat;
        respHeartBeat.sequenceId_  = pHeartBeatReq->m_reqHeartBeat->sequenceId_;
        pack::Writer writer(pHeartBeatReq->m_socketHandle->m_socket->Out());
        respHeartBeat.Pack(writer);
    }
    delete pHeartBeatReq->m_reqHeartBeat;

}

void SMGPServiceThread::HandleSMGPHeartBeatResp(TMsg* pMsg)
{
    TSMGPHeartBeatResp *pHeartBeatResp = (TSMGPHeartBeatResp*)pMsg;
    //LogDebug("handle client's heartbeat resp sequenceId[%d]",pHeartBeatResp->m_respHeartBeat->sequenceId_);
    SMGPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }
    delete pHeartBeatResp->m_respHeartBeat;
    it->second->m_uNoHBRespCount = 0;
}

void SMGPServiceThread::HandleMoMsg(TMsg* pMsg)
{
    //////LogDebug("here is Mo!");
    TReportReceiverMoMsg* pMoMsg = (TReportReceiverMoMsg*)pMsg;

    //search client link, and send delivery request.
    SMGPSocketHandler* SMGPHandler = NULL;
    for (SMGPLinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(pMoMsg->m_strClientId == itLink->second->m_strAccount
            && NULL != itLink->second->m_SMGPHandler
            && NULL != itLink->second->m_SMGPHandler->m_socket)
        {
            SMGPHandler = itLink->second->m_SMGPHandler;
            break;
        }
    }

    if(NULL == SMGPHandler)
    {
        LogError("clientId[%s] find no SMGPHandler.", pMoMsg->m_strClientId.data());
        //return;
    }

    AccountMap::iterator iteror = m_AccountMap.find(pMoMsg->m_strClientId);
    UInt32 uNeedMo = 0;
    if (iteror == m_AccountMap.end())
    {
        LogError("clientid[%s] is not find in accountMap.",pMoMsg->m_strClientId.data());
        return;
    }
    else
    {
        uNeedMo = iteror->second.m_uNeedMo;
    }

    string strTemp = iteror->second.m_strSpNum + pMoMsg->m_strUserPort;
    //////LogDebug("spNUm = %s;UserPort = %s.",iteror->second.m_strSpNum.data(),pMoMsg->m_strUserPort.data());
    smsp::SMGPDeliverReq moDeliver;
    moDeliver._srcTermId = pMoMsg->m_strPhone;
    moDeliver._destTermId = strTemp;
    /////LogDebug("srcPhone = %s.",pMoMsg->m_strPhone.data());
    /////LogDebug("destPhone = %s.",moDeliver._destTermId.data());
    moDeliver._msgFmt = 8;
    moDeliver._isReport = 0;
    moDeliver.sequenceId_ = m_pSnManager->getDirectSequenceId();
    moDeliver._msgId = pMoMsg->m_strMoId.substr(0,10);
    utils::UTFString utfHelper;
    string strOut = "";
    int len = utfHelper.u2u16(pMoMsg->m_strContent, strOut);
    if (len > 140)
    {
        LogElk("short message length is too long len[%d]",len);
    }
    moDeliver.setContent((char*)strOut.data(),len);

    if((NULL != SMGPHandler) && (0 != uNeedMo))
    {
        pack::Writer writer(SMGPHandler->m_socket->Out());
        moDeliver.Pack(writer);
        LogElk("push MO clientid[%s],content[%s],phone[%s].",pMoMsg->m_strClientId.data(),pMoMsg->m_strContent.data(),pMoMsg->m_strPhone.data());
    }
    else if((NULL == SMGPHandler) && (0 != uNeedMo))
    {
        //set to redis
        LogDebug("can not find socketHandle to send Report. save cache to Redis");
        //lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*

        //setRedis
        string strKey = "report_cache:" + pMoMsg->m_strClientId;
        string strCmd = "lpush " + strKey + " clientid=" + pMoMsg->m_strClientId +"&ismo=1&destid=" + strTemp + "&phone=" + pMoMsg->m_strPhone +
            "&content=" + Base64::Encode(pMoMsg->m_strContent) + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SMGP);

        TRedisReq* pRedis = new TRedisReq();
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_RedisCmd.assign(strCmd);
        LogNotice("cmd[%s]", strCmd.c_str());
        pRedis->m_strKey= strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

        //set expire
        TRedisReq* pRedisExpire = new TRedisReq();
        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";  //259200 = 72*3600
        LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
        pRedisExpire->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);
    }
}

void SMGPServiceThread::HandleReportReceiveMsg(TMsg* pMsg)
{

    //////LogDebug("here is report!");
    TReportReceiveToSMSReq *pTempReportReq = (TReportReceiveToSMSReq*)pMsg;
    //copy and save report msg in map, use it later.
    TReportReceiveToSMSReq *pTReport2SMGPReqSave = new TReportReceiveToSMSReq();
    pTReport2SMGPReqSave->m_strSmsId = pTempReportReq->m_strSmsId;
    pTReport2SMGPReqSave->m_strDesc = pTempReportReq->m_strDesc;
    pTReport2SMGPReqSave->m_strPhone = pTempReportReq->m_strPhone;
    pTReport2SMGPReqSave->m_strContent = pTempReportReq->m_strContent;
    pTReport2SMGPReqSave->m_strUpstreamTime = pTempReportReq->m_strUpstreamTime;
    pTReport2SMGPReqSave->m_strClientId = pTempReportReq->m_strClientId;
    pTReport2SMGPReqSave->m_strSrcId = pTempReportReq->m_strSrcId;
    pTReport2SMGPReqSave->m_iLinkId = pTempReportReq->m_iLinkId;
    pTReport2SMGPReqSave->m_iType = pTempReportReq->m_iType;
    pTReport2SMGPReqSave->m_iStatus = pTempReportReq->m_iStatus;
    pTReport2SMGPReqSave->m_lReportTime = pTempReportReq->m_lReportTime;
    pTReport2SMGPReqSave->m_uUpdateFlag = pTempReportReq->m_uUpdateFlag;
    pTReport2SMGPReqSave->m_strReportDesc = pTempReportReq->m_strReportDesc;
    pTReport2SMGPReqSave->m_strInnerErrcode = pTempReportReq->m_strInnerErrcode;
    
    string reportMapKey;
    reportMapKey = pTReport2SMGPReqSave->m_strSmsId;
    reportMapKey = reportMapKey + '_';
    reportMapKey = reportMapKey + pTReport2SMGPReqSave->m_strPhone;
    pTReport2SMGPReqSave->m_pRedisRespWaitTimer = SetTimer(REDIS_RESP_WAIT_TIMER_ID, reportMapKey, REDIS_RESP_WAIT_TIMEOUT);
    m_ReportMsgMap[reportMapKey] = pTReport2SMGPReqSave;

    //search smsid_phone in redis.
    TRedisReq* req = new TRedisReq();
    req->m_iSeq = pMsg->m_iSeq;
    req->m_strSessionID = reportMapKey;
    string key ="accesssms:" + reportMapKey;
    LogDebug("key[%s], iseq[%d]", key.data(), pMsg->m_iSeq);
    req->m_RedisCmd = "HGETALL " + key;
    req->m_strKey = key;
    req->m_iMsgType = MSGTYPE_REDIS_REQ;
    req->m_uReqTime = time(NULL);
    req->m_pSender = this;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,req);
}

void SMGPServiceThread::HandleRedisRspSendReportToSMGP(TMsg* pMsg)
{
    //todo...process mo.
    /////LogDebug("redis reply recved!");
    TRedisResp* pRedisResp = (TRedisResp*)pMsg;

    string reportKey = pRedisResp->m_strSessionID;
    ReportMsgMap::iterator itReportMsg = m_ReportMsgMap.find(reportKey);
    if(itReportMsg == m_ReportMsgMap.end())
    {
        LogWarn("m_strSessionID[%s] is not find m_ReportMsgMap", pRedisResp->m_strSessionID.data());
        return;
    }

    if( NULL == pRedisResp->m_RedisReply)
    {
        LogError("redis reply is NULL!");
        return;
    }

    string submitIds = "";
    string clientId  = "";
    string strIDs = "";
    string strDate = "";
    string strConnectLink;
    if ((NULL == pRedisResp->m_RedisReply)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pRedisResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pRedisResp->m_RedisReply->elements == 0)))
    {
        LogError("redis reply is error[%d], cmd[%s]", pRedisResp->m_RedisReply->type, pRedisResp->m_RedisCmd.c_str());
        delete itReportMsg->second->m_pRedisRespWaitTimer;
        itReportMsg->second->m_pRedisRespWaitTimer = NULL;
        delete itReportMsg->second;
        m_ReportMsgMap.erase(itReportMsg);
        freeReply(pRedisResp->m_RedisReply);
        return;
    }
    else
    {
        //LogDebug("Redis costtime[%d], iseq[%d]", time(NULL) - pRedisResp->m_uReqTime, pRedisResp->m_iSeq);
        paserReply("submitid", pRedisResp->m_RedisReply, submitIds);
        paserReply("clientid", pRedisResp->m_RedisReply, clientId);
        paserReply("id", pRedisResp->m_RedisReply, strIDs);
        paserReply("date", pRedisResp->m_RedisReply, strDate);
        paserReply("connectLink", pRedisResp->m_RedisReply, strConnectLink);
        freeReply(pRedisResp->m_RedisReply);

        string cmdkey = "accesssms:";
        cmdkey += reportKey;
        TRedisReq* pRedisDel = new TRedisReq();
        if (NULL == pRedisDel)
        {
            LogError("pRedisDel is NULL.");
            return;
        }
        pRedisDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisDel->m_RedisCmd.assign("DEL  ");
        pRedisDel->m_RedisCmd.append(cmdkey);
        pRedisDel->m_strKey = cmdkey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisDel);
    }

    LogDebug("submitIds[%s], clientId[%s], ids[%s], cmd[%s], connectLink[%s]", submitIds.data(), clientId.data(), strIDs.data()
    , pRedisResp->m_RedisCmd.c_str(), strConnectLink.data());
    UInt32 submitsize;
    std::vector<string> submitslist;
    std::string::size_type nPos = submitIds.find("&");
    if (std::string::npos == nPos)
    {
        submitsize = 1;
        submitslist.push_back(submitIds);
    }
    else
    {
        Comm comm;
        comm.splitExVector(submitIds, "&", submitslist);
        submitsize = submitslist.size();

        if(submitsize == 0)
        {
            LogWarn("submitId count error.");
            return;
        }
    }

    //get all ids
    std::vector<string> vecIDs;
    std::string::size_type nIDPos = strIDs.find(",");
    if (std::string::npos == nIDPos)
    {
        vecIDs.push_back(strIDs);
    }
    else
    {
        Comm comm;
        comm.splitExVector(strIDs, ",", vecIDs);
        if(vecIDs.size() != submitsize)     //id num must the same with submitsize
        {
            LogWarn("ID count error.");
            return;
        }
    }

    //search client link, and send delivery request.
    SMGPSocketHandler* SMGPHandler = NULL;
    SMGPLinkMap::iterator it = m_LinkMap.find(strConnectLink);
    if (it != m_LinkMap.end())
    {
        if(clientId == it->second->m_strAccount
            && NULL != it->second->m_SMGPHandler
            && NULL != it->second->m_SMGPHandler->m_socket)
        {
            SMGPHandler = it->second->m_SMGPHandler;
            LogDebug("clientId[%s], link[%s]", clientId.data(), it->first.data());
        }
    }

    if (SMGPHandler == NULL)
    {
        for (SMGPLinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
        {
            if(clientId == itLink->second->m_strAccount
                && NULL != itLink->second->m_SMGPHandler
                && NULL != itLink->second->m_SMGPHandler->m_socket)
            {
                SMGPHandler = itLink->second->m_SMGPHandler;
                strConnectLink = itLink->first;
                LogDebug("clientId[%s], link[%s]", clientId.data(), itLink->first.data());
                break;
            }
        }
    }
    if(NULL == SMGPHandler)
    {
        LogElk("clientId[%s] find no SMGPHandler.", clientId.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(clientId);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap.",clientId.data());
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    ////a li xuyao report xiangxi
    string strReportDesc = "999";
    if (2 == iteror->second.m_uNeedReport)
    {
        UInt32 reportLen = itReportMsg->second->m_strReportDesc.length();
        if(reportLen <= 3)
        {
            UInt32 flag = 1;
            char oneChar ;
            for(UInt32 i = 0;i < reportLen;i ++)
            {
                oneChar = itReportMsg->second->m_strReportDesc.at(i);
                if(oneChar > '9' || oneChar < '0')
                {
                    flag = 0;
                    break;
                }
            }
            if(flag == 1)
            {
                strReportDesc.assign(itReportMsg->second->m_strReportDesc);
            }
        }
    }

    for(UInt32 i = 0; i < submitsize; i++)
    {
        smsp::SMGPDeliverReq reqDeliver;
        /////LogNotice("smsid[%s].", itReportMsg->second->m_strSmsId.data());
        ////reqDeliver._msgId  = atol(submitslist[i].data());
        reqDeliver.m_uSubmitId = strtoul(submitslist[i].data(), NULL, 0);
        /////LogDebug("phone[%s]", itReportMsg->second->m_strPhone.data());
        //reqDeliver._destId = itReportMsg->second->m_strPhone; //?_srcTerminalId
        reqDeliver._status = itReportMsg->second->m_iStatus;
        if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
        {
            reqDeliver._remark = "DELIVRD";
            reqDeliver._err = "000";
        }
        else
        {
            reqDeliver._remark = "UNDELIV";
            reqDeliver._err = strReportDesc;
        }

        reqDeliver._srcTermId = itReportMsg->second->m_strPhone;
        reqDeliver.sequenceId_ = m_pSnManager->getDirectSequenceId();
        reqDeliver._isReport = 1;

        if((NULL != SMGPHandler) && (0 != uNeedReport))
        {
            pack::Writer writer(SMGPHandler->m_socket->Out());
            reqDeliver.Pack(writer);
            LogElk("push report clientid:%s,phone:%s,smsid:%s,state:%d,strConnectLink=%s.",
                clientId.data(),itReportMsg->second->m_strPhone.data(),itReportMsg->second->m_strSmsId.data()
                ,itReportMsg->second->m_iStatus, strConnectLink.c_str());
        }
        else if((NULL == SMGPHandler) && (0 != uNeedReport))
        {
            LogDebug("can not find socketHandle to send Report. save cache to Redis");
            //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*

            //setRedis
            string strKey = "report_cache:" + clientId;
            string strCmd = "lpush " + strKey + " clientid=" + clientId +"&msgid=" + submitslist[i] +"&status=" + Comm::int2str(reqDeliver._status)
                + "&remark=" + Base64::Encode(reqDeliver._remark) + "&phone=" + reqDeliver._srcTermId + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SMGP);

            TRedisReq* pRedis = new TRedisReq();
            pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRedis->m_RedisCmd.assign(strCmd);
            LogNotice("cmd[%s]", strCmd.c_str());
            pRedis->m_strKey = strKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

            //set expire
            TRedisReq* pRedisExpire = new TRedisReq();
            pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
            pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";  //259200 = 72*3600
            LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
            pRedisExpire->m_strKey = strKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);

        }

        if (0 == itReportMsg->second->m_uUpdateFlag)
        {
            UpdateRecord(submitslist[i], vecIDs[i], iteror->second.m_uIdentify, strDate,itReportMsg->second);
        }

    }

    delete itReportMsg->second->m_pRedisRespWaitTimer;
    itReportMsg->second->m_pRedisRespWaitTimer = NULL;
    delete itReportMsg->second;
    m_ReportMsgMap.erase(itReportMsg);
}

void SMGPServiceThread::HandleReportGetAgain(TMsg* pMsg)
{
    TReportRepostMsg* preq = (TReportRepostMsg*)(pMsg);

    //search client link, and send delivery request.
    SMGPSocketHandler* SMGPHandler = NULL;
    for (SMGPLinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(preq->m_strCliendid == itLink->second->m_strAccount
            && NULL != itLink->second->m_SMGPHandler
            && NULL != itLink->second->m_SMGPHandler->m_socket)
        {
            SMGPHandler = itLink->second->m_SMGPHandler;
            break;
        }
    }

    if(NULL == SMGPHandler)
    {
        LogElk("clientId[%s] find no SMGPHandler.", preq->m_strCliendid.data());
    }


    //get uNeedReport flag
    AccountMap::iterator iteror = m_AccountMap.find(preq->m_strCliendid);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogError("clientid[%s] is not find in accountMap.",preq->m_strCliendid.data());
        return;
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    if((NULL != SMGPHandler) && (0 != uNeedReport))
    {
        smsp::SMGPDeliverReq reqDeliver;
        reqDeliver._srcTermId = preq->m_strPhone;
        reqDeliver.m_uSubmitId = strtoul(preq->m_strMsgid.data(), NULL, 0);
        reqDeliver._status = preq->m_istatus;
        reqDeliver._isReport = 1;
        reqDeliver.sequenceId_ = m_pSnManager->getDirectSequenceId();

        if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
        {
            reqDeliver._remark = "DELIVRD";
            reqDeliver._err = "000";
        }
        else
        {
            reqDeliver._remark = "UNDELIV";
            reqDeliver._err = preq->m_strRemark;
        }

        pack::Writer writer(SMGPHandler->m_socket->Out());
        reqDeliver.Pack(writer);

        LogElk("push report again, clientid:%s,phone:%s,smsid:%s,state:%d.",
            preq->m_strCliendid.data(),preq->m_strPhone.data(),preq->m_strMsgid.data(),preq->m_istatus);
    }
    else if((NULL == SMGPHandler) && (0 != uNeedReport))
    {
        LogDebug("can not find socketHandle to send Report. save cache to Redis again");
        //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*

        //setRedis
        string strKey = "report_cache:" + preq->m_strCliendid;
        string strCmd = "lpush " + strKey + " clientid=" + preq->m_strCliendid +"&msgid=" + preq->m_strMsgid.data() +"&status=" + Comm::int2str(preq->m_istatus)
            + "&remark=" + Base64::Encode(preq->m_strRemark) + "&phone=" + preq->m_strPhone + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SMGP);

        TRedisReq* pRedis = new TRedisReq();
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_RedisCmd.assign(strCmd);
        LogNotice("cmd[%s]", strCmd.c_str());
        pRedis->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

        //set expire
        TRedisReq* pRedisExpire = new TRedisReq();
        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";  //259200 = 72*3600
        LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
        pRedisExpire->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);
    }
}

void SMGPServiceThread::HandleMOGetAgain(TMsg* pMsg)
{
    TMoRepostMsg* pMoMsg = (TMoRepostMsg*)(pMsg);

    SMGPSocketHandler* SMGPHandler = NULL;
    for (SMGPLinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(pMoMsg->m_strCliendid == itLink->second->m_strAccount
            && NULL != itLink->second->m_SMGPHandler
            && NULL != itLink->second->m_SMGPHandler->m_socket)
        {
            SMGPHandler = itLink->second->m_SMGPHandler;
            break;
        }
    }

    if(NULL == SMGPHandler)
    {
        LogError("clientId[%s] find no smgpHandler.", pMoMsg->m_strCliendid.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(pMoMsg->m_strCliendid);
    UInt32 uNeedMo = 0;
    if (iteror == m_AccountMap.end())
    {
        LogError("clientid[%s] is not find in accountMap.",pMoMsg->m_strCliendid.data());
    }
    else
    {
        uNeedMo = iteror->second.m_uNeedMo;
    }

    ///////////
    smsp::SMGPDeliverReq moDeliver;
    moDeliver._srcTermId = pMoMsg->m_strPhone;
    moDeliver._destTermId = pMoMsg->m_strDestid;
    moDeliver._msgFmt = 8;
    moDeliver._isReport = 0;
    moDeliver.sequenceId_ = m_pSnManager->getDirectSequenceId();

    utils::UTFString utfHelper;
    string strOut = "";
    int len = utfHelper.u2u16(pMoMsg->m_strContent, strOut);
    if (len > 140)
    {
        LogElk("hort message length is too long len[%d]",len);
    }
    moDeliver.setContent((char*)strOut.data(),len);


    if((NULL != SMGPHandler) && (0 != uNeedMo))
    {
        pack::Writer writer(SMGPHandler->m_socket->Out());
        moDeliver.Pack(writer);
        LogElk("push MO again clientid[%s],content[%s],phone[%s].",pMoMsg->m_strCliendid.data(),pMoMsg->m_strContent.data(),pMoMsg->m_strPhone.data());
    }
    else if((NULL == SMGPHandler) && (0 != uNeedMo))
    {
        //set to redis
        LogDebug("can not find socketHandle to send Report. save cache to Redis");
        //lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*

        //setRedis
        string strKey = "report_cache:" + pMoMsg->m_strCliendid;
        string strCmd = "lpush " + strKey + " clientid=" + pMoMsg->m_strCliendid +"&ismo=1&destid=" + moDeliver._destTermId + "&phone=" + pMoMsg->m_strPhone +
            "&content=" + Base64::Encode(pMoMsg->m_strContent) + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_SMGP);

        TRedisReq* pRedis = new TRedisReq();
        pRedis->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedis->m_RedisCmd.assign(strCmd);
        LogNotice("cmd[%s]", strCmd.c_str());
        pRedis->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedis);

        //set expire
        TRedisReq* pRedisExpire = new TRedisReq();
        pRedisExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisExpire->m_RedisCmd = "EXPIRE " + strKey + " " +"259200";  //259200 = 72*3600
        LogNotice("expire cmd[%s]", pRedisExpire->m_RedisCmd.c_str());
        pRedisExpire->m_strKey = strKey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);
    }
}


void SMGPServiceThread::HandleTimerOutMsg(TMsg* pMsg)
{
    if(REDIS_RESP_WAIT_TIMER_ID == (UInt32)pMsg->m_iSeq)   //report query redis timeout.
    {
        ReportMsgMap::iterator itReportMsg = m_ReportMsgMap.find(pMsg->m_strSessionID);
        if(itReportMsg == m_ReportMsgMap.end())
        {
            LogDebug("m_ReportMsgMap[%s] not exist.", pMsg->m_strSessionID.data());
            return;
        }
        else
        {
            LogDebug("m_ReportMsgMap[%s] report timeout.", pMsg->m_strSessionID.data());
            delete itReportMsg->second->m_pRedisRespWaitTimer;
            delete itReportMsg->second;
            m_ReportMsgMap.erase(itReportMsg);
        }
    }
    else if(SMGP_HEART_BEAT_TIMER_ID == (UInt32)pMsg->m_iSeq)   //send SMGP heartbeat timer.
    {
        SMGPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
        if(it == m_LinkMap.end())
        {
            LogWarn("can't find session,LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        if(NULL == it->second->m_HeartBeatTimer)
        {
            LogWarn("m_HeartBeatTimer is NULL LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        if(NULL == it->second->m_SMGPHandler || NULL == it->second->m_SMGPHandler->m_socket)
        {
            LogWarn("m_SMGPHandler or m_SMGPHandler->m_socket is NULL LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        it->second->m_uNoHBRespCount++;
        if(it->second->m_uNoHBRespCount > HEART_BEAT_MAX_COUNT)
        {
            if(it->second->m_SMGPHandler->m_iLinkState == LINK_CONNECTED)
            {
                CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
                pDis->m_strAccount = it->second->m_strAccount;
                pDis->m_strLinkId = pMsg->m_strSessionID;
                pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
//              CHK_ALL_THREAD_INIT_OK_RETURN();
                g_pUnitypThread->PostMsg(pDis);
            }
            
            LogError("HeartBeat timeout, close linkId:%s,clientId:%s.", pMsg->m_strSessionID.data(),it->second->m_strAccount.data());
            delete it->second->m_HeartBeatTimer;
            it->second->m_HeartBeatTimer = NULL;
            delete it->second->m_SMGPHandler;
            it->second->m_SMGPHandler = NULL;
            delete it->second;
            m_LinkMap.erase(it);
            return;
        }

        smsp::SMGPHeartbeatReq reqHeartBeat;
        reqHeartBeat.sequenceId_ = m_pSnManager->getDirectSequenceId();
        //LogError("smgp service's heartbeat req sequcenceId:[%d]",reqHeartBeat.sequenceId_);
        pack::Writer writer(it->second->m_SMGPHandler->m_socket->Out());
        reqHeartBeat.Pack(writer);

        delete it->second->m_HeartBeatTimer;
        it->second->m_HeartBeatTimer = NULL;
        it->second->m_HeartBeatTimer = SetTimer(SMGP_HEART_BEAT_TIMER_ID, pMsg->m_strSessionID, HEART_BEAT_TIMEOUT);
    }
    else
    {
        LogWarn("no this timerId[%ld].", pMsg->m_iSeq);
    }

    return;
}

//update SMGP report
void SMGPServiceThread::UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv)
{
    char sql[1024]  = {0};
    char szReportTime[64] = {0};
    
    Int32 iStatus = rpRecv->m_iStatus;
    Int64 lReportTime = rpRecv->m_lReportTime;
    string& strDesc = rpRecv->m_strDesc;
    UInt32 channelCount = rpRecv->m_uChannelCount;

    if(lReportTime != 0 )
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&lReportTime,&pTime);
        strftime(szReportTime, sizeof(szReportTime), "%Y%m%d%H%M%S", &pTime);
    }

    snprintf(sql, 1024,"UPDATE t_sms_access_%d_%s SET state = '%d', report = '%s', reportdate = '%s',channelcnt = '%d'"
        ",innerErrorcode='%s' where id = '%s' and date='%s';",
        uIdentify, strDate.substr(0, 8).data(), iStatus, strDesc.data(), szReportTime,channelCount
        , rpRecv->m_strInnerErrcode.c_str(), strID.data(), strDate.data());

    publishMqC2sDbMsg(strID, sql);
}

UInt32 SMGPServiceThread::GetSessionMapSize()
{
    return m_LinkMap.size();
}

