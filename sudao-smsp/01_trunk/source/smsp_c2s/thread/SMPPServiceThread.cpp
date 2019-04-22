#include <mysql.h>
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "SMPPDeliverReq.h"
#include "main.h"
#include "MqManagerThread.h"

SMPPServiceThread::SMPPServiceThread(const char *name):CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
}

SMPPServiceThread::~SMPPServiceThread()
{
    delete m_pSnManager;
}

bool SMPPServiceThread::Init(const std::string ip, unsigned int port)
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

void SMPPServiceThread::MainLoop()
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

void SMPPServiceThread::HandleMsg(TMsg* pMsg)
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
            HandleSMPPAcceptSocketMsg(pMsg);
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
        case MSGTYPE_TCP_DELIVER_REQ:
        {
            HandleDeliverReq(pMsg);
            break;
        }
        case MSGTYPE_SMPP_HEART_BEAT_REQ:
        {
            HandleSMPPHeartBeatReq(pMsg);
            break;
        }
        case MSGTYPE_SMPP_HEART_BEAT_RESP:
        {
            HandleSMPPHeartBeatResp(pMsg);
            break;
        }
        case MSGTYPE_SMPP_LINK_CLOSE_REQ:
        {
            HandleSMPPLinkCloseReq(pMsg);
            break;
        }
        case MSGTYPE_REPORTRECEIVE_TO_SMS_REQ:
        {
            HandleReportReceiveMsg(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            HandleRedisRspSendReportToSMPP(pMsg);
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

void SMPPServiceThread::HandleDeliverReq(TMsg* pMsg)
{
    CTcpDeliverReqMsg* pDel = (CTcpDeliverReqMsg*)pMsg;
    SMPPLinkMap::iterator iterLink = m_LinkMap.find(pDel->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pDel->m_strLinkId.data());
        return;
    }

    smsp::SMPPDeliverReq reqDeliver;

    char cSubmitId[60] = {0};
    snprintf(cSubmitId,60,"%lu",pDel->m_uSubmitId);

    reqDeliver._msgId.assign(cSubmitId);

    reqDeliver._sourceAddr = pDel->m_strPhone;

    if (pDel->m_uResult == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
    {
        reqDeliver._remark = "DELIVRD";
        reqDeliver._commandStatus = SMPP_ESME_ROK;
    }
    else
    {
        reqDeliver._remark = "UNDELIV";
        reqDeliver._commandStatus = SMPP_ESME_RSYSERR;
    }
    reqDeliver._dataCoding = 8;////ucs2

    ///// build smpp deliver
    ////id:196051309453199355 sub:001 dlvrd:001 submit date:1605131145 done date:1605131145 stat:DELIVRD err:0000 text:
    char dataTime[64] = {0};
    time_t now = time(NULL);
    struct tm pTime = {0};
    localtime_r((time_t*)&now,&pTime);
    strftime(dataTime, sizeof(dataTime), "%y%m%d%H%M", &pTime);

    string strContent = "";
    strContent.append("id:");
    strContent.append(reqDeliver._msgId);
    strContent.append(" sub:001 dlvrd:001 submit date:");
    strContent.append(dataTime);
    strContent.append(" done date:");
    strContent.append(dataTime);
    strContent.append(" stat:");
    strContent.append(reqDeliver._remark);
    strContent.append(" err:");
    strContent.append(" text:");

    utils::UTFString utfHelper;
    utfHelper.u2Ascii(strContent, reqDeliver._shortMessage);
    reqDeliver._smLength = reqDeliver._shortMessage.size();
    LogDebug("====deliver report[%s].",strContent.data());

    reqDeliver._sequenceNum = m_pSnManager->getDirectSequenceId();
    reqDeliver._registeredDelivery = 1;
    reqDeliver._esmClass = 0x04;

    pack::Writer writer(iterLink->second->m_SMPPHandler->m_socket->Out());
    reqDeliver.Pack(writer);
}

void SMPPServiceThread::HandleSubmitResp(TMsg* pMsg)
{
    CTcpSubmitRespMsg* pResp = (CTcpSubmitRespMsg*)pMsg;
    SMPPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }
    iterLink->second->m_uNoHBRespCount =0;

    smsp::SMPPSubmitResp respSubmit;
    respSubmit._commandStatus = pResp->m_uResult;
    respSubmit._sequenceNum = pResp->m_uSequenceNum;
    respSubmit._uMsgID = pResp->m_uSubmitId;

    char msgId[25];
    memset(msgId, 0, sizeof(msgId));
    snprintf(msgId,25,"%lu",pResp->m_uSubmitId);
    respSubmit.m_strMsgId.assign(msgId);
    pack::Writer writer(iterLink->second->m_SMPPHandler->m_socket->Out());
    respSubmit.Pack(writer);
}

void SMPPServiceThread::HandleLoginResp(TMsg* pMsg)
{
    CTcpLoginRespMsg* pResp = (CTcpLoginRespMsg*)pMsg;
    SMPPLinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_strAccount = pResp->m_strAccount;
    iterLink->second->m_uNoHBRespCount =0;

    smsp::SMPPConnectResp LoginResp;
    LoginResp._commandStatus = pResp->m_uResult;
    LoginResp._sequenceNum = pResp->m_uSequenceNum;

    pack::Writer writer(iterLink->second->m_SMPPHandler->m_socket->Out());
    LoginResp.Pack(writer);
    if(0 == pResp->m_uResult)
    {
        iterLink->second->m_SMPPHandler->m_iLinkState = LINK_CONNECTED;
        LogElk("clientid:%s,linkId:%s,connect login sucess.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }
    else
    {
        LogElk("clientid:%s,linkId:%s,connect login failed.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }

    return;
}

void SMPPServiceThread::HandleSMPPLinkCloseReq(TMsg* pMsg)
{
    LogNotice("recv close link[%s] ", pMsg->m_strSessionID.data());
    CTcpCloseLinkReqMsg* pTLinkCloseReq = (CTcpCloseLinkReqMsg*)pMsg;

    SMPPLinkMap::iterator itLink = m_LinkMap.find(pTLinkCloseReq->m_strLinkId);
    if(itLink != m_LinkMap.end())
    {
        //主动断开连接
        LogElk("clientid:%s,linkId:%s, recv link close req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        if(itLink->second->m_SMPPHandler != NULL)
        {
            smsp::SMPPTerminateReq reqTerminate;
            reqTerminate._sequenceNum = m_pSnManager->getDirectSequenceId();
            pack::Writer writer(itLink->second->m_SMPPHandler->m_socket->Out());
            reqTerminate.Pack(writer);
            LogNotice("clientid:%s,linkId:%s,terminate req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        }
        CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
        pDis->m_strAccount = pTLinkCloseReq->m_strAccount;
        pDis->m_strLinkId = pTLinkCloseReq->m_strLinkId;
        pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
        g_pUnitypThread->PostMsg(pDis);
     

        //close handle
        if(itLink->second->m_SMPPHandler != NULL)
        {
            delete itLink->second->m_SMPPHandler;
            itLink->second->m_SMPPHandler = NULL;
        }
        delete itLink->second->m_HeartBeatTimer;
        delete itLink->second;
        m_LinkMap.erase(itLink);
    }
    else
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap", pTLinkCloseReq->m_strLinkId.data());
    }
}

void SMPPServiceThread::HandleSMPPAcceptSocketMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iLinkId = m_pSnManager->getSn();
    char charLinkId[64] = { 0 };
    snprintf(charLinkId, 64,"%ld", iLinkId);
    string strLinkId = charLinkId;

    SMPPSocketHandler *pSMPPSocketHandler = new SMPPSocketHandler(this, strLinkId);
    if(false == pSMPPSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("SMPPServiceThread::procSMServiceAcceptSocket is faield.")
        delete pSMPPSocketHandler;
        return;
    }

    LogDebug("add session,smservice  LinkId[%s]",strLinkId.data());

    SMPPLinkManager* pSession = new SMPPLinkManager();
    pSession->m_SMPPHandler = pSMPPSocketHandler;
    pSession->m_HeartBeatTimer = SetTimer(CMPP_HEART_BEAT_TIMER_ID, strLinkId, HEART_BEAT_TIMEOUT);

    m_LinkMap[strLinkId] = pSession;
}

void SMPPServiceThread::HandleSMPPHeartBeatReq(TMsg* pMsg)
{
    TSMPPHeartBeatReq *pHeartBeatReq = (TSMPPHeartBeatReq*)pMsg;

    SMPPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }

    if(NULL != pHeartBeatReq->m_socketHandle && NULL != pHeartBeatReq->m_socketHandle->m_socket)
    {
        it->second->m_uNoHBRespCount = 0;
        smsp::SMPPHeartbeatResp respHeartBeat;
        respHeartBeat._sequenceNum = pHeartBeatReq->m_reqHeartBeat->_sequenceNum;
        pack::Writer writer(pHeartBeatReq->m_socketHandle->m_socket->Out());
        respHeartBeat.Pack(writer);
    }
    delete pHeartBeatReq->m_reqHeartBeat;

}

void SMPPServiceThread::HandleSMPPHeartBeatResp(TMsg* pMsg)
{
    TSMPPHeartBeatResp *pHeartBeatResp = (TSMPPHeartBeatResp*)pMsg;
    SMPPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogWarn("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }
    delete pHeartBeatResp->m_respHeartBeat;
    it->second->m_uNoHBRespCount = 0;
}

void SMPPServiceThread::HandleReportReceiveMsg(TMsg* pMsg)
{
    TReportReceiveToSMSReq *pTempReportReq = (TReportReceiveToSMSReq*)pMsg;
    //copy and save report msg in map, use it later.
    TReportReceiveToSMSReq *pTReport2CmppReqSave = new TReportReceiveToSMSReq();
    pTReport2CmppReqSave->m_strSmsId = pTempReportReq->m_strSmsId;
    pTReport2CmppReqSave->m_strDesc = pTempReportReq->m_strDesc;
    pTReport2CmppReqSave->m_strPhone = pTempReportReq->m_strPhone;
    pTReport2CmppReqSave->m_strContent = pTempReportReq->m_strContent;
    pTReport2CmppReqSave->m_strUpstreamTime = pTempReportReq->m_strUpstreamTime;
    pTReport2CmppReqSave->m_strClientId = pTempReportReq->m_strClientId;
    pTReport2CmppReqSave->m_strSrcId = pTempReportReq->m_strSrcId;
    pTReport2CmppReqSave->m_iLinkId = pTempReportReq->m_iLinkId;
    pTReport2CmppReqSave->m_iType = pTempReportReq->m_iType;
    pTReport2CmppReqSave->m_iStatus = pTempReportReq->m_iStatus;
    pTReport2CmppReqSave->m_lReportTime = pTempReportReq->m_lReportTime;
    pTReport2CmppReqSave->m_uUpdateFlag = pTempReportReq->m_uUpdateFlag;
    pTReport2CmppReqSave->m_strReportDesc = pTempReportReq->m_strReportDesc;
    pTReport2CmppReqSave->m_strInnerErrcode = pTempReportReq->m_strInnerErrcode;
        
    string reportMapKey;
    reportMapKey = pTReport2CmppReqSave->m_strSmsId;
    reportMapKey = reportMapKey + '_';
    reportMapKey = reportMapKey + pTReport2CmppReqSave->m_strPhone;
    pTReport2CmppReqSave->m_pRedisRespWaitTimer = SetTimer(REDIS_RESP_WAIT_TIMER_ID, reportMapKey, REDIS_RESP_WAIT_TIMEOUT);
    m_ReportMsgMap[reportMapKey] = pTReport2CmppReqSave;

    //search smsid_phone in redis.
    TRedisReq* req = new TRedisReq();
    req->m_iSeq = pMsg->m_iSeq;
    req->m_strSessionID = reportMapKey;
    string key ="accesssms:" + reportMapKey;
    LogDebug("key[%s]", key.data());
    req->m_RedisCmd = "HGETALL " + key;
    req->m_strKey = key;
    req->m_iMsgType = MSGTYPE_REDIS_REQ;
    req->m_pSender = this;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,req);
}

void SMPPServiceThread::HandleRedisRspSendReportToSMPP(TMsg* pMsg)
{
    //todo...process mo.
    LogDebug("redis reply recved!");
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
        delete itReportMsg->second->m_pRedisRespWaitTimer;
        itReportMsg->second->m_pRedisRespWaitTimer = NULL;
        delete itReportMsg->second;
        m_ReportMsgMap.erase(itReportMsg);
        return;
    }

    string submitIds = "";
    string clientId  = "";
    string strIDs = "";
    string strDate = "";

    string strDestAddrTon;
    string strDestAddrNpi;
    string strSourceAddrTon;
    string strSourceAddrNpi;

    string strConnectLink;

    if ((NULL == pRedisResp->m_RedisReply)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pRedisResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pRedisResp->m_RedisReply->elements == 0)))
    {
        LogError("redis reply is error, type[%d], cmd[%s]", pRedisResp->m_RedisReply->type, pRedisResp->m_RedisCmd.c_str());
        delete itReportMsg->second->m_pRedisRespWaitTimer;
        itReportMsg->second->m_pRedisRespWaitTimer = NULL;
        delete itReportMsg->second;
        m_ReportMsgMap.erase(itReportMsg);
        freeReply(pRedisResp->m_RedisReply);
        return;
    }
    else
    {
        paserReply("submitid", pRedisResp->m_RedisReply, submitIds);
        paserReply("clientid", pRedisResp->m_RedisReply, clientId);
        paserReply("id", pRedisResp->m_RedisReply, strIDs);
        paserReply("date", pRedisResp->m_RedisReply, strDate);

        paserReply("destaddrton", pRedisResp->m_RedisReply, strDestAddrTon);
        paserReply("destaddrnpi", pRedisResp->m_RedisReply, strDestAddrNpi);
        paserReply("sourceaddrton", pRedisResp->m_RedisReply, strSourceAddrTon);
        paserReply("sourceaddrnpi", pRedisResp->m_RedisReply, strSourceAddrNpi);
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
        pRedisDel->m_RedisCmd.assign("DEL ");
        pRedisDel->m_RedisCmd.append(cmdkey);
        pRedisDel->m_strKey = cmdkey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisDel);
    }

    LogDebug("submitIds[%s], clientId[%s], ids[%s], DestAddrTon[%s], DestAddrNpi[%s], SourceAddrTon[%s], SourceAddrNpi[%s], cmd[%s], connectLink[%s]",
        submitIds.data(), clientId.data(), strIDs.data(), strSourceAddrTon.data(), strSourceAddrNpi.data(),
        strDestAddrTon.data(), strDestAddrNpi.data(), pRedisResp->m_RedisCmd.c_str(), strConnectLink.c_str());

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
    SMPPSocketHandler* SMPPHandler = NULL;
    SMPPLinkMap::iterator it = m_LinkMap.find(strConnectLink);
    if (it != m_LinkMap.end())
    {
        if(clientId == it->second->m_strAccount
            && NULL != it->second->m_SMPPHandler
            && NULL != it->second->m_SMPPHandler->m_socket)
        {
            SMPPHandler = it->second->m_SMPPHandler;
            LogDebug("clientId[%s], link[%s]", clientId.data(), it->first.data());
        }
    }

    if (SMPPHandler == NULL)
    {
        for (SMPPLinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
        {
            if(clientId == itLink->second->m_strAccount
                && NULL != itLink->second->m_SMPPHandler
                && NULL != itLink->second->m_SMPPHandler->m_socket)
            {
                SMPPHandler = itLink->second->m_SMPPHandler;
                strConnectLink = itLink->first;
                LogDebug("clientId[%s], link[%s]", clientId.data(), itLink->first.data());
                break;
            }
        }
    }
    if(NULL == SMPPHandler)
    {
        LogWarn("clientId[%s] find no SMPPHandler.", clientId.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(clientId);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogError("clientid[%s] is not find in accountMap.",clientId.data());
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    string realReport;
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
        smsp::SMPPDeliverReq reqDeliver;
        reqDeliver._msgId  = submitslist[i];
        LogNotice("[%s:%s],submitId[%s],strConnectLink=%s.",
            itReportMsg->second->m_strSmsId.data(),itReportMsg->second->m_strPhone.data()
            ,reqDeliver._msgId.data(), strConnectLink.data());

        reqDeliver._sourceAddr = itReportMsg->second->m_strPhone;
        ////reqDeliver._status = itReportMsg->second->m_iStatus;

        if (itReportMsg->second->m_iStatus == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
        {
            reqDeliver._remark = "DELIVRD";
            reqDeliver._commandStatus = SMPP_ESME_ROK;
            realReport = "000";
        }
        else
        {
            reqDeliver._remark = "UNDELIV";
            reqDeliver._commandStatus = SMPP_ESME_RSYSERR;
            realReport = strReportDesc;
        }
        reqDeliver._dataCoding = 8;////ucs2

        ///// build smpp deliver
        ////id:196051309453199355 sub:001 dlvrd:001 submit date:1605131145 done date:1605131145 stat:DELIVRD err:0000 text:
        char dataTime[64] = {0};
        time_t now = time(NULL);
        struct tm pTime = {0};
        localtime_r((time_t*)&now,&pTime);
        strftime(dataTime, sizeof(dataTime), "%y%m%d%H%M", &pTime);

        string strContent = "";
        strContent.append("id:");
        strContent.append(reqDeliver._msgId);
        strContent.append(" sub:001 dlvrd:001 submit date:");
        strContent.append(dataTime);
        strContent.append(" done date:");
        strContent.append(dataTime);
        strContent.append(" stat:");
        strContent.append(reqDeliver._remark);
        strContent.append(" err:");
        strContent.append(realReport);
        strContent.append(" text:");

        utils::UTFString utfHelper;
        utfHelper.u2Ascii(strContent, reqDeliver._shortMessage);
        reqDeliver._smLength = reqDeliver._shortMessage.size();
        LogElk("deliver report[%s].",strContent.data());

        reqDeliver._sequenceNum = m_pSnManager->getDirectSequenceId();
        reqDeliver._registeredDelivery = 1;
        reqDeliver._esmClass = 0x04;

        reqDeliver._destAddrTon = atoi(strDestAddrTon.data());
        reqDeliver._destAddrNpi = atoi(strDestAddrNpi.data());
        reqDeliver._sourceAddrTon = atoi(strSourceAddrTon.data());
        reqDeliver._sourceAddrNpi = atoi(strSourceAddrNpi.data());

        if((NULL != SMPPHandler) && (0 != uNeedReport))
        {
            pack::Writer writer(SMPPHandler->m_socket->Out());
            reqDeliver.Pack(writer);
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

void SMPPServiceThread::HandleTimerOutMsg(TMsg* pMsg)
{
    if(REDIS_RESP_WAIT_TIMER_ID == (UInt32)pMsg->m_iSeq)    //report query redis timeout.
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
    else if(CMPP_HEART_BEAT_TIMER_ID == (UInt32)pMsg->m_iSeq)   //send CMPP heartbeat timer.
    {
        SMPPLinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
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

        if(NULL == it->second->m_SMPPHandler || NULL == it->second->m_SMPPHandler->m_socket)
        {
            LogWarn("m_CMPPHandler or m_CMPPHandler->m_socket is NULL LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        it->second->m_uNoHBRespCount++;
        if(it->second->m_uNoHBRespCount > HEART_BEAT_MAX_COUNT)
        {
            if(it->second->m_SMPPHandler->m_iLinkState == LINK_CONNECTED)
            {
                CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
                pDis->m_strAccount = it->second->m_strAccount;
                pDis->m_strLinkId = pMsg->m_strSessionID;
                pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
                g_pUnitypThread->PostMsg(pDis);
            }
            
            LogError("HeartBeat timeout, close link[%s]", pMsg->m_strSessionID.data());
            delete it->second->m_HeartBeatTimer;
            it->second->m_HeartBeatTimer = NULL;
            delete it->second->m_SMPPHandler;
            it->second->m_SMPPHandler = NULL;
            delete it->second;
            m_LinkMap.erase(it);
            return;
        }

        smsp::SMPPHeartbeatReq reqHeartBeat;
        reqHeartBeat._sequenceNum = m_pSnManager->getDirectSequenceId();
        pack::Writer writer(it->second->m_SMPPHandler->m_socket->Out());
        reqHeartBeat.Pack(writer);

        delete it->second->m_HeartBeatTimer;
        it->second->m_HeartBeatTimer = NULL;
        it->second->m_HeartBeatTimer = SetTimer(CMPP_HEART_BEAT_TIMER_ID, pMsg->m_strSessionID, HEART_BEAT_TIMEOUT);
    }
    else
    {
        LogWarn("no this timerId[%ld].", pMsg->m_iSeq);
    }
}

void SMPPServiceThread::UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv)
{
    char sql[2048]  = {0};
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

UInt32 SMPPServiceThread::GetSessionMapSize()
{
    return m_LinkMap.size();
}

