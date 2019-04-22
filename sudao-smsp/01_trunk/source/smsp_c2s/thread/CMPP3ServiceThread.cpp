#include <mysql.h>
#include "CMPP3ServiceThread.h"
#include <stdlib.h>
#include <iostream>
#include "UrlCode.h"
#include "HttpParams.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"
#include "CMPP3DeliverReq.h"
#include "ErrorCode.h"
#include "base64.h"
#include "MqManagerThread.h"



CMPP3ServiceThread::CMPP3ServiceThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
}

CMPP3ServiceThread::~CMPP3ServiceThread()
{
    delete m_pSnManager;
}

bool CMPP3ServiceThread::Init(const std::string ip, unsigned int port)
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

void CMPP3ServiceThread::MainLoop()
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

void CMPP3ServiceThread::HandleMsg(TMsg* pMsg)
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
            HandleCMPPAcceptSocketMsg(pMsg);
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
        case MSGTYPE_CMPP_HEART_BEAT_REQ:
        {
            HandleCMPPHeartBeatReq(pMsg);
            break;
        }
        case MSGTYPE_CMPP_HEART_BEAT_RESP:
        {
            HandleCMPPHeartBeatResp(pMsg);
            break;
        }
        case MSGTYPE_CMPP_LINK_CLOSE_REQ:
        {
            HandleCMPPLinkCloseReq(pMsg);
            break;
        }
        case MSGTYPE_REPORTRECEIVE_TO_SMS_REQ:
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
            HandleRedisRspSendReportToCMPP(pMsg);
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

void CMPP3ServiceThread::HandleDeliverReq(TMsg* pMsg)
{
    CTcpDeliverReqMsg* pDel = (CTcpDeliverReqMsg*)pMsg;
    CMPP3LinkMap::iterator iterLink = m_LinkMap.find(pDel->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pDel->m_strLinkId.data());
        return;
    }

    cmpp3::CMPPDeliverReq reqDeliver;
    reqDeliver._msgId  = pDel->m_uSubmitId;
    reqDeliver._status = pDel->m_uResult;
    reqDeliver._srcTerminalId = pDel->m_strPhone;

    if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
    {
        reqDeliver._remark = "DELIVRD";
    }
    else
    {
        reqDeliver._remark = pDel->m_strYZXErrCode;
    }
    reqDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
    reqDeliver._registeredDelivery = 1;
    pack::Writer writer(iterLink->second->m_CMPPHandler->m_socket->Out());
    reqDeliver.Pack(writer);

    LogElk("push report[%s:%s]",iterLink->second->m_strAccount.data(), pDel->m_strPhone.data());
    return;
}

void CMPP3ServiceThread::HandleSubmitResp(TMsg* pMsg)
{
    CTcpSubmitRespMsg* pResp = (CTcpSubmitRespMsg*)pMsg;
    CMPP3LinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_uNoHBRespCount =0;

    cmpp3::CMPPSubmitResp respSubmit;
    respSubmit._result = pResp->m_uResult;
    respSubmit._msgID  = pResp->m_uSubmitId;
    respSubmit._sequenceId = pResp->m_uSequenceNum;
    pack::Writer writer(iterLink->second->m_CMPPHandler->m_socket->Out());
    respSubmit.Pack(writer);

    LogElk("push submitResp submitid:%lu, clientid:%s, result:%d.",
        pResp->m_uSubmitId, iterLink->second->m_strAccount.data(), pResp->m_uResult);
    return;
}

void CMPP3ServiceThread::HandleLoginResp(TMsg* pMsg)
{
    CTcpLoginRespMsg* pResp = (CTcpLoginRespMsg*)pMsg;
    CMPP3LinkMap::iterator iterLink = m_LinkMap.find(pResp->m_strLinkId);
    if (iterLink == m_LinkMap.end())
    {
        LogError("LinkId[%s] is not find in LinkMAP!",pResp->m_strLinkId.data());
        return;
    }

    iterLink->second->m_strAccount = pResp->m_strAccount;
    iterLink->second->m_uNoHBRespCount =0;

    cmpp3::CMPPConnectResp LoginResp;
    LoginResp._status = pResp->m_uResult;
    LoginResp._version= 0X30;
    LoginResp._sequenceId= pResp->m_uSequenceNum;

    pack::Writer writer(iterLink->second->m_CMPPHandler->m_socket->Out());
    LoginResp.Pack(writer);
    if(0 == pResp->m_uResult)
    {
        iterLink->second->m_CMPPHandler->m_iLinkState = LINK_CONNECTED;
        LogElk("clientid:%s,linkId:%s,connect login sucess.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }
    else
    {
        LogElk("clientid:%s,linkId:%s,connect login failed.",pResp->m_strAccount.data(),pResp->m_strLinkId.data());
    }

    return;
}

void CMPP3ServiceThread::HandleCMPPLinkCloseReq(TMsg* pMsg)
{
    CTcpCloseLinkReqMsg *pTLinkCloseReq = (CTcpCloseLinkReqMsg*)pMsg;

    CMPP3LinkMap::iterator itLink = m_LinkMap.find(pTLinkCloseReq->m_strLinkId);
    if(itLink != m_LinkMap.end())
    {
        LogElk("clientid:%s,linkId:%s, recv link close req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        //主动断开连接
        if(itLink->second->m_CMPPHandler != NULL)
        {
            smsp::CMPPTerminateReq reqTerminate;
            reqTerminate._sequenceId = m_pSnManager->getDirectSequenceId();
            pack::Writer writer(itLink->second->m_CMPPHandler->m_socket->Out());
            reqTerminate.Pack(writer);
            LogNotice("clientid:%s,linkId:%s,terminate req.",pTLinkCloseReq->m_strAccount.data(),pTLinkCloseReq->m_strLinkId.data());
        }
        
        CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
        pDis->m_strAccount = pTLinkCloseReq->m_strAccount;
        pDis->m_strLinkId = pTLinkCloseReq->m_strLinkId;
        pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
        g_pUnitypThread->PostMsg(pDis);
      

        //close handle
        if(itLink->second->m_CMPPHandler != NULL)
        {
            delete itLink->second->m_CMPPHandler;
            itLink->second->m_CMPPHandler = NULL;
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

void CMPP3ServiceThread::HandleCMPPAcceptSocketMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iLinkId = m_pSnManager->getSn();
    char charLinkId[64] = { 0 };
    snprintf(charLinkId,64, "%ld", iLinkId);
    string strLinkId = charLinkId;

    CMPP3SocketHandler *pCMPPSocketHandler = new CMPP3SocketHandler(this, strLinkId);
    if(false == pCMPPSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("CMPP3ServiceThread::procSMServiceAcceptSocket is faield.")
        delete pCMPPSocketHandler;
        return;
    }

    LogElk("accept one cmpp socket LinkId[%s].",strLinkId.data());
    CMPP3LinkManager* pSession = new CMPP3LinkManager();
    pSession->m_CMPPHandler = pCMPPSocketHandler;
    pSession->m_HeartBeatTimer = SetTimer(CMPP_HEART_BEAT_TIMER_ID, strLinkId, HEART_BEAT_TIMEOUT);

    m_LinkMap[strLinkId] = pSession;
}

void CMPP3ServiceThread::HandleCMPPHeartBeatReq(TMsg* pMsg)
{
    TCMPP3HeartBeatReq *pHeartBeatReq = (TCMPP3HeartBeatReq*)pMsg;

    CMPP3LinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogElk("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }

    if(NULL != pHeartBeatReq->m_socketHandle && NULL != pHeartBeatReq->m_socketHandle->m_socket)
    {
        it->second->m_uNoHBRespCount = 0;
        cmpp3::CMPPHeartbeatResp respHeartBeat;
        respHeartBeat._sequenceId  = pHeartBeatReq->m_reqHeartBeat->_sequenceId;
        pack::Writer writer(pHeartBeatReq->m_socketHandle->m_socket->Out());
        respHeartBeat.Pack(writer);
    }
    delete pHeartBeatReq->m_reqHeartBeat;

}

void CMPP3ServiceThread::HandleCMPPHeartBeatResp(TMsg* pMsg)
{
    TCMPP3HeartBeatResp *pHeartBeatResp = (TCMPP3HeartBeatResp*)pMsg;
    CMPP3LinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
    if(it == m_LinkMap.end())
    {
        LogElk("LinkId[%s] is not find in m_LinkMap", pMsg->m_strSessionID.data());
        return;
    }
    delete pHeartBeatResp->m_respHeartBeat;
    it->second->m_uNoHBRespCount = 0;
}

void CMPP3ServiceThread::HandleMoMsg(TMsg* pMsg)
{
    TReportReceiverMoMsg* pMoMsg = (TReportReceiverMoMsg*)pMsg;

    //search client link, and send delivery request.
    CMPP3SocketHandler* CMPPHandler = NULL;
    for (CMPP3LinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(pMoMsg->m_strClientId == itLink->second->m_strAccount
            && NULL != itLink->second->m_CMPPHandler
            && NULL != itLink->second->m_CMPPHandler->m_socket)
        {
            CMPPHandler = itLink->second->m_CMPPHandler;
            break;
        }
    }

    if(NULL == CMPPHandler)
    {
        LogElk("clientId[%s] find no CMPPHandler.", pMoMsg->m_strClientId.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(pMoMsg->m_strClientId);
    UInt32 uNeedMo = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap.",pMoMsg->m_strClientId.data());
    }
    else
    {
        uNeedMo = iteror->second.m_uNeedMo;
    }

    string strTemp = iteror->second.m_strSpNum + pMoMsg->m_strUserPort;

    string strPhone = "";
    if (0 == pMoMsg->m_strPhone.compare(0,2,"86"))
    {
        strPhone = pMoMsg->m_strPhone.substr(2);
    }
    else
    {
        strPhone = pMoMsg->m_strPhone;
    }
    string strOut = "";
    utils::UTFString utfHelper;
    UInt32 len = utfHelper.u2u16(pMoMsg->m_strContent, strOut);
    UInt64 iMoid = strtoul(pMoMsg->m_strMoId.data(), 0, 0);
    if((NULL != CMPPHandler) && (0 != uNeedMo))
    {
        if(len <= 140 ||
            (1 == iteror->second.m_uSupportLongMo && len < 256))
        {
            cmpp3::CMPPDeliverReq moDeliver;
            moDeliver._srcTerminalId = strPhone;
            moDeliver._destId = strTemp;
            moDeliver._msgFmt = 8;
            moDeliver._registeredDelivery = 0;
            moDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
            moDeliver._msgId = iMoid;
            moDeliver.setContent((char*)strOut.data(),len);
            pack::Writer writer(CMPPHandler->m_socket->Out());
            moDeliver.Pack(writer);
            LogElk("push MO clientid[%s],content[%s],phone[%s].",pMoMsg->m_strClientId.data(),pMoMsg->m_strContent.data(),strPhone.data());
        }
        else
        {
            sendCmppLongMo(strOut,strTemp,strPhone,CMPPHandler);
            LogElk("push Long MO clientid[%s],content[%s],phone[%s].len[%u]",
                pMoMsg->m_strClientId.data(),pMoMsg->m_strContent.data(),strPhone.data(),len);
        }

    }
    else if((NULL == CMPPHandler) && (0 != uNeedMo))
    {
        //set to redis
        LogDebug("can not find socketHandle to send Report. save cache to Redis");
        //lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*

        //setRedis
        string strKey = "report_cache:" + pMoMsg->m_strClientId;
        string strCmd = "lpush " + strKey + " clientid=" + pMoMsg->m_strClientId +"&ismo=1&destid=" + strTemp + "&phone=" + strPhone +
            "&content=" + Base64::Encode(pMoMsg->m_strContent) + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_CMPP3);

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


void CMPP3ServiceThread::sendCmppLongMo(string strContent,string strDestId,string strPhone,CMPP3SocketHandler* CMPPHandler)
{
    if(NULL == CMPPHandler)
    {
        LogWarn("param is null!!!");
        return;
    }
    int len = strContent.size();
    int MAXLENGTH = 134;
    int size = (len + MAXLENGTH -1)/MAXLENGTH;
    int index = 1;
    int send_len = 0;//
    UInt32 uCand = getRandom();
    while(len > 0)
    {
        UInt64 uMoId = m_pSnManager->CreateSubmitId(0x12);
        char chtmp[1024];
        memset(chtmp,0,sizeof(chtmp));
        std::string strtmp;
        int strlen = 0;
        strlen = ( len > MAXLENGTH ) ? MAXLENGTH : len;

        strtmp = strContent.substr(send_len, strlen);
        len = len - strlen;
        send_len += strlen;

        memset(chtmp, 0x00, 1024);

        int a =5;
        memcpy(chtmp,(void*)&a,1);
        a = 0;
        memcpy(chtmp+1,(void*)&a,1);
        a = 3;
        memcpy(chtmp+2,(void*)&a,1);
        a = uCand;
        memcpy(chtmp+3,(void*)&a,1);
        a = size;
        memcpy(chtmp+4,(void*)&a,1);
        a = index;
        memcpy(chtmp+5,(void*)&a,1);

        memcpy(chtmp+6, strtmp.data(), strlen);

        cmpp3::CMPPDeliverReq moDeliver;
        moDeliver._srcTerminalId = strPhone;
        moDeliver._destId = strDestId;
        moDeliver._msgFmt = 8;
        moDeliver._registeredDelivery = 0;
        moDeliver._tpUdhi = 1;
        moDeliver._msgId = uMoId;
        moDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
        moDeliver.setContent(chtmp, strlen+6);
        pack::Writer writer(CMPPHandler->m_socket->Out());
        moDeliver.Pack(writer);
        index++;
    }
}


void CMPP3ServiceThread::HandleReportReceiveMsg(TMsg* pMsg)
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
    pTReport2CmppReqSave->m_uChannelCount= pTempReportReq->m_uChannelCount;
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

    req->m_RedisCmd = "HGETALL " + key;
    LogDebug("key[%s], iseq[%d], cmd[%s]", key.data(), pMsg->m_iSeq, req->m_RedisCmd.c_str());
    req->m_iMsgType = MSGTYPE_REDIS_REQ;
    req->m_uReqTime = time(NULL);
    req->m_pSender = this;
    req->m_strKey = key;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,req);
}

void CMPP3ServiceThread::HandleRedisRspSendReportToCMPP(TMsg* pMsg)
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
        //LogDebug("Redis costtime[%d], iseq[%d]", time(NULL) - pRedisResp->m_uReqTime, pRedisResp->m_iSeq);
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
        //response suc
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

    LogDebug("submitIds[%s], clientId[%s], ids[%s], m_RedisCmd[%s]", submitIds.data(), clientId.data(), strIDs.data(), pRedisResp->m_RedisCmd.c_str());
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
    CMPP3SocketHandler* CMPPHandler = NULL;
    CMPP3LinkMap::iterator it = m_LinkMap.find(strConnectLink);
    if (it != m_LinkMap.end())
    {
        if(clientId == it->second->m_strAccount
            && NULL != it->second->m_CMPPHandler
            && NULL != it->second->m_CMPPHandler->m_socket)
        {
            CMPPHandler = it->second->m_CMPPHandler;
            LogDebug("clientId[%s], link[%s]", clientId.data(), it->first.data());
        }
    }
    if (CMPPHandler == NULL)
    {
        for (CMPP3LinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
        {
            if(clientId == itLink->second->m_strAccount
                && NULL != itLink->second->m_CMPPHandler
                && NULL != itLink->second->m_CMPPHandler->m_socket)
            {
                CMPPHandler = itLink->second->m_CMPPHandler;
                break;
            }
        }
    }


    if(NULL == CMPPHandler)
    {
        LogElk("clientId[%s] find no CMPPHandler.", clientId.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(clientId);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap",clientId.data());
        return;
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    ////a li xuyao report xiangxi
    string strReportDesc = "UNDELIV";
    if (2 == iteror->second.m_uNeedReport)
    {
        int iDescLen = itReportMsg->second->m_strReportDesc.length();
        if ((UInt32(itReportMsg->second->m_iStatus) != SMS_STATUS_CONFIRM_SUCCESS)
            && (0 != strncmp(itReportMsg->second->m_strReportDesc.data(),"ET",2)))
        {
            if(7 == iDescLen)
            {
                strReportDesc.assign(itReportMsg->second->m_strReportDesc);
            }
            else if(7 > iDescLen && iDescLen > 0)
            {
                string tempDesc;
                tempDesc.append((7 - iDescLen ),' ');
                tempDesc.append(itReportMsg->second->m_strReportDesc.data());
                strReportDesc.assign(tempDesc);
            }
            else if(7 < iDescLen)
            {
                strReportDesc.assign(itReportMsg->second->m_strReportDesc.data(),7);
            }
        }
        if((UInt32(itReportMsg->second->m_iStatus) != SMS_STATUS_CONFIRM_SUCCESS)
            && (0 == strncmp(itReportMsg->second->m_strReportDesc.data(),"ET",2)))
        {
            string yzxReport;
            yzxReport.assign("YX");
            yzxReport.append(itReportMsg->second->m_strReportDesc,2,itReportMsg->second->m_strReportDesc.length() - 2);
            strReportDesc.assign(yzxReport);
        }
    }

    for(UInt32 i = 0; i < submitsize; i++)
    {
        cmpp3::CMPPDeliverReq reqDeliver;
        /////LogNotice("smsid[%s].", itReportMsg->second->m_strSmsId.data());
        reqDeliver._msgId  = strtoul(submitslist[i].data(), NULL, 0);
        /////LogDebug("phone[%s]", itReportMsg->second->m_strPhone.data());
        //reqDeliver._destId = itReportMsg->second->m_strPhone; //?_srcTerminalId
        reqDeliver._status = itReportMsg->second->m_iStatus;
        if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
        {
            reqDeliver._remark = "DELIVRD";
        }
        else
        {
            reqDeliver._remark = strReportDesc;
        }
        reqDeliver._srcTerminalId = itReportMsg->second->m_strPhone;
        reqDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
        reqDeliver._registeredDelivery = 1;

        if((NULL != CMPPHandler) && (0 != uNeedReport))
        {
            pack::Writer writer(CMPPHandler->m_socket->Out());
            reqDeliver.Pack(writer);

            LogElk("push report clientid:%s,phone:%s,smsid:%s,state:%d,strConnectLink=%s.",
                clientId.data(),itReportMsg->second->m_strPhone.data(),itReportMsg->second->m_strSmsId.data()
                ,itReportMsg->second->m_iStatus, strConnectLink.data());
        }
        else if((NULL == CMPPHandler) && (0 != uNeedReport))
        {
            LogDebug("can not find socketHandle to send Report. save cache to Redis");
            //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*

            //setRedis
            string strKey = "report_cache:" + clientId;
            string strCmd = "lpush " + strKey + " clientid=" + clientId +"&msgid=" + submitslist[i] +"&status=" + Comm::int2str(reqDeliver._status)
                + "&remark=" + Base64::Encode(reqDeliver._remark) + "&phone=" + reqDeliver._srcTerminalId + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_CMPP3);

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
            pRedisExpire->m_strKey= strKey;
            SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisExpire);

        }


        if (0 == itReportMsg->second->m_uUpdateFlag)
        {
            UpdateRecord(submitslist[i], vecIDs[i], iteror->second.m_uIdentify, strDate, itReportMsg->second);
        }

    }

    delete itReportMsg->second->m_pRedisRespWaitTimer;
    itReportMsg->second->m_pRedisRespWaitTimer = NULL;
    delete itReportMsg->second;
    m_ReportMsgMap.erase(itReportMsg);
}

void CMPP3ServiceThread::HandleReportGetAgain(TMsg* pMsg)
{
    TReportRepostMsg* preq = (TReportRepostMsg*)(pMsg);

    //search client link, and send delivery request.
    CMPP3SocketHandler* CMPPHandler = NULL;
    for (CMPP3LinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(preq->m_strCliendid == itLink->second->m_strAccount
            && NULL != itLink->second->m_CMPPHandler
            && NULL != itLink->second->m_CMPPHandler->m_socket)
        {
            CMPPHandler = itLink->second->m_CMPPHandler;
            break;
        }
    }

    if(NULL == CMPPHandler)
    {
        LogWarn("clientId[%s] find no CMPPHandler.", preq->m_strCliendid.data());
    }

    //get cmpphandler suc

    //get uNeedReport flag
    AccountMap::iterator iteror = m_AccountMap.find(preq->m_strCliendid);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap.",preq->m_strCliendid.data());
        return;
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    if((NULL != CMPPHandler) && (0 != uNeedReport))
    {
        cmpp3::CMPPDeliverReq reqDeliver;
        reqDeliver._msgId  = strtoul(preq->m_strMsgid.data(), NULL, 0);
        reqDeliver._status = preq->m_istatus;

        if (reqDeliver._status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)
        {
            reqDeliver._remark = "DELIVRD";
        }
        else
        {
            reqDeliver._remark = "UNDELIV";
            // Pass-through
            if (2 == uNeedReport)
            {
                int iDescLen = preq->m_strRemark.length();
                if (0 != strncmp(preq->m_strRemark.data(),"ET",2))
                {
                    if(7 == iDescLen)
                    {
                        reqDeliver._remark.assign(preq->m_strRemark);
                    }
                    else if(7 > iDescLen && iDescLen > 0)
                    {
                        int descLen = preq->m_strRemark.length();
                        string tempDesc;
                        tempDesc.append((7 - descLen ),' ');
                        tempDesc.append(preq->m_strRemark.data());
                        reqDeliver._remark.assign(tempDesc);
                    }
                    else if(7 < iDescLen)
                    {
                        reqDeliver._remark.assign(preq->m_strRemark.data(),7);
                    }
                }
                if(0 == strncmp(preq->m_strRemark.data(),"ET",2))
                {
                    string yzxReport;
                    yzxReport.assign("YX");
                    yzxReport.append(preq->m_strRemark,2,preq->m_strRemark.length() - 2);
                    reqDeliver._remark.assign(yzxReport);
                }
            }
        }

        ////reqDeliver._remark = preq->m_strRemark;
        reqDeliver._srcTerminalId = preq->m_strPhone;
        reqDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
        reqDeliver._registeredDelivery = 1;

        pack::Writer writer(CMPPHandler->m_socket->Out());
        reqDeliver.Pack(writer);

        LogElk("push report again, clientid:%s,phone:%s,smsid:%s,state:%d.",
            preq->m_strCliendid.data(),preq->m_strPhone.data(),preq->m_strMsgid.data(),preq->m_istatus);
    }
    else if((NULL == CMPPHandler) && (0 != uNeedReport))
    {
        LogDebug("can not find socketHandle to send Report. save cache to Redis again");
        //lpush report_cache:clientId clientid=*&msgid=*&status=*&remark=base64*&phone=*&usmsfrom=*&err=base64*

        //setRedis
        string strKey = "report_cache:" + preq->m_strCliendid;
        string strCmd = "lpush " + strKey + " clientid=" + preq->m_strCliendid +"&msgid=" + preq->m_strMsgid.data() +"&status=" + Comm::int2str(preq->m_istatus)
            + "&remark=" + Base64::Encode(preq->m_strRemark) + "&phone=" + preq->m_strPhone + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_CMPP3);

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

void CMPP3ServiceThread::HandleMOGetAgain(TMsg* pMsg)
{
    TMoRepostMsg* pMoMsg = (TMoRepostMsg*)(pMsg);

    CMPP3SocketHandler* CMPPHandler = NULL;
    for (CMPP3LinkMap::iterator itLink = m_LinkMap.begin(); itLink != m_LinkMap.end(); ++itLink)
    {
        if(pMoMsg->m_strCliendid == itLink->second->m_strAccount
            && NULL != itLink->second->m_CMPPHandler
            && NULL != itLink->second->m_CMPPHandler->m_socket)
        {
            CMPPHandler = itLink->second->m_CMPPHandler;
            break;
        }
    }

    if(NULL == CMPPHandler)
    {
        LogError("clientId[%s] find no CMPPHandler.", pMoMsg->m_strCliendid.data());
    }

    AccountMap::iterator iteror = m_AccountMap.find(pMoMsg->m_strCliendid);
    UInt32 uNeedMo = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap.",pMoMsg->m_strCliendid.data());
    }
    else
    {
        uNeedMo = iteror->second.m_uNeedMo;
    }

    //get cmpphandler suc
    utils::UTFString utfHelper;
    string strOut = "";
    UInt32 len = utfHelper.u2u16(pMoMsg->m_strContent, strOut);
    if((NULL != CMPPHandler) && (0 != uNeedMo))
    {
        UInt64 uMoId = m_pSnManager->CreateSubmitId(0x12);
        if(len <= 140 ||
            (1 == iteror->second.m_uSupportLongMo && len < 256))
        {
            cmpp3::CMPPDeliverReq moDeliver;
            moDeliver._srcTerminalId = pMoMsg->m_strPhone;
            moDeliver._destId = pMoMsg->m_strDestid;
            moDeliver._msgFmt = 8;
            moDeliver._registeredDelivery = 0;
            moDeliver._sequenceId = m_pSnManager->getDirectSequenceId();
            moDeliver._msgId = uMoId;
            moDeliver.setContent((char*)strOut.data(),len);
            pack::Writer writer(CMPPHandler->m_socket->Out());
            moDeliver.Pack(writer);
            LogElk("push MO again clientid[%s],content[%s],phone[%s].",pMoMsg->m_strCliendid.data(),pMoMsg->m_strContent.data(),pMoMsg->m_strPhone.data());
        }
        else
        {

            sendCmppLongMo(strOut,pMoMsg->m_strDestid,pMoMsg->m_strPhone,CMPPHandler);
            LogElk("push Long MO clientid[%s],content[%s],phone[%s].len[%u]",
                pMoMsg->m_strCliendid.data(),pMoMsg->m_strContent.data(),pMoMsg->m_strPhone.data(),len);
        }
    }
    else if((NULL == CMPPHandler) && (0 != uNeedMo))
    {
        //set to redis
        LogDebug("can not find socketHandle to send Report. save cache to Redis");
        //lpush report_cache:clientId clientid=*&ismo=1&destid=*&phone=*&content=*&usmsfrom=*

        //setRedis
        string strKey = "report_cache:" + pMoMsg->m_strCliendid;
        string strCmd = "lpush " + strKey + " clientid=" + pMoMsg->m_strCliendid +"&ismo=1&destid=" + pMoMsg->m_strDestid + "&phone=" + pMoMsg->m_strPhone +
            "&content=" + Base64::Encode(pMoMsg->m_strContent) + "&usmsfrom=" + Comm::int2str(SMS_FROM_ACCESS_CMPP3);

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


void CMPP3ServiceThread::HandleTimerOutMsg(TMsg* pMsg)
{
    if(REDIS_RESP_WAIT_TIMER_ID == (UInt32)pMsg->m_iSeq)   //report query redis timeout.
    {
        ReportMsgMap::iterator itReportMsg = m_ReportMsgMap.find(pMsg->m_strSessionID);
        if(itReportMsg == m_ReportMsgMap.end())
        {
            LogWarn("m_ReportMsgMap[%s] not exist.", pMsg->m_strSessionID.data());
            return;
        }
        else
        {
            LogWarn("m_ReportMsgMap[%s] report timeout.", pMsg->m_strSessionID.data());
            delete itReportMsg->second->m_pRedisRespWaitTimer;
            delete itReportMsg->second;
            m_ReportMsgMap.erase(itReportMsg);
        }
    }
    else if(CMPP_HEART_BEAT_TIMER_ID == (UInt32)pMsg->m_iSeq)   //send CMPP heartbeat timer.
    {
        CMPP3LinkMap::iterator it = m_LinkMap.find(pMsg->m_strSessionID);
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

        if(NULL == it->second->m_CMPPHandler || NULL == it->second->m_CMPPHandler->m_socket)
        {
            LogWarn("m_CMPPHandler or m_CMPPHandler->m_socket is NULL LinkId[%s]", pMsg->m_strSessionID.data());
            return;
        }

        it->second->m_uNoHBRespCount++;
        if(it->second->m_uNoHBRespCount > HEART_BEAT_MAX_COUNT)
        {
            if(it->second->m_CMPPHandler->m_iLinkState == LINK_CONNECTED)
            {
                CTcpDisConnectLinkReqMsg* pDis = new CTcpDisConnectLinkReqMsg();
                pDis->m_strAccount = it->second->m_strAccount;
                pDis->m_strLinkId = pMsg->m_strSessionID;
                pDis->m_iMsgType = MSGTYPE_TCP_DISCONNECT_REQ;
                g_pUnitypThread->PostMsg(pDis);
            }
            
            LogError("HeartBeat timeout, close linkId:%s,clientId:%s.", pMsg->m_strSessionID.data(),it->second->m_strAccount.data());
            delete it->second->m_HeartBeatTimer;
            it->second->m_HeartBeatTimer = NULL;
            delete it->second->m_CMPPHandler;
            it->second->m_CMPPHandler = NULL;
            delete it->second;
            m_LinkMap.erase(it);
            return;
        }

        cmpp3::CMPPHeartbeatReq reqHeartBeat;
        reqHeartBeat._sequenceId = m_pSnManager->getDirectSequenceId();
        pack::Writer writer(it->second->m_CMPPHandler->m_socket->Out());
        reqHeartBeat.Pack(writer);

        delete it->second->m_HeartBeatTimer;
        it->second->m_HeartBeatTimer = NULL;
        it->second->m_HeartBeatTimer = SetTimer(CMPP_HEART_BEAT_TIMER_ID, pMsg->m_strSessionID, HEART_BEAT_TIMEOUT);
    }
    else
    {
        LogWarn("no this timerId[%ld].", pMsg->m_iSeq);
    }

    return;
}

//update cmpp report
void CMPP3ServiceThread::UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv)
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
        uIdentify, strDate.substr(0, 8).data(), iStatus, strDesc.data(), szReportTime,channelCount, rpRecv->m_strInnerErrcode.c_str()
        , strID.data(), strDate.data());


    publishMqC2sDbMsg(strID, sql);
}

UInt32 CMPP3ServiceThread::GetSessionMapSize()
{
    return m_LinkMap.size();
}
UInt32 CMPP3ServiceThread::getRandom()
{
    m_uRandom++;
    if(m_uRandom >=255)
    {
        m_uRandom = 0;
    }

    return m_uRandom;
}


