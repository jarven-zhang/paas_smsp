#include "SGIPRtAndMoThread.h"
#include <stdlib.h>
#include <iostream>
#include "SGIPReportReq.h"
#include "SGIPDeliverReq.h"
#include "main.h"
#include "MqManagerThread.h"

const int CHECK_CLIENT_RPTMO_LINK_TIMERID = 1231123;
const int CHECK_CLIENT_RPTMO_LINK_TIME = 5*60*1000; //5MIN


CSgipRtAndMoThread::CSgipRtAndMoThread(const char *name):CThread(name)
{
    m_pInternalService = NULL;
}

CSgipRtAndMoThread::~CSgipRtAndMoThread()
{

}

bool CSgipRtAndMoThread::Init()
{
    if (false == CThread::Init())
    {
        LogError("CSgipRtAndMoThread::Init CThread::Init is failed.");
        return false;
    }

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }

    m_AccountMap.clear();
    g_pRuleLoadThread->getSmsAccountMap(m_AccountMap);

    m_pInternalService->Init();

    m_pLinkedBlockPool = new LinkedBlockPool();

    //timer to connet sgipRptMoServer, check if has report in redis and need to send
    m_pTimer_CheckLink = SetTimer(CHECK_CLIENT_RPTMO_LINK_TIMERID, "timer_to_check_clt_rpt_link", CHECK_CLIENT_RPTMO_LINK_TIME);


    return true;
}

void CSgipRtAndMoThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    TMsg* pMsg = NULL;
    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if ((pMsg != NULL) && ((MSGTYPE_SGIP_RT_REQ == pMsg->m_iMsgType) || (MSGTYPE_SGIP_MO_REQ == pMsg->m_iMsgType)))
        {
            RtAndMoLinkMap::iterator itr = m_mapRtAndMoLinkMap.find(pMsg->m_strSessionID);
            if (itr == m_mapRtAndMoLinkMap.end())
            {
                LogError("clientid[%s] is not find in m_mapRtAndMoLinkMap.",pMsg->m_strSessionID.data());
                delete pMsg;
                continue;
            }
            else
            {
                if (CONNECT_STATUS_OK != itr->second->m_uState)
                {
                    itr->second->m_vecRtAndMo.push_back(pMsg);
                    continue;
                }
            }
        }

        if(pMsg == NULL && 0 == iSelect)
        {
            usleep(g_uSecSleep);
        }

        if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }

    }

    m_pInternalService->GetSelector()->Destroy();
    return;
}

void CSgipRtAndMoThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_REPORTRECEIVE_TO_SMS_REQ:
        {
            procRtReqMsg(pMsg);
            break;
        }
        case MSGTYPE_ACCESS_MO_MSG:
        {
            procMoReqMsg(pMsg);
            break;
        }
        case MSGTYPE_TCP_DELIVER_REQ:
        {
            HandleDeliverReq(pMsg);
            break;
        }
        case MSGTYPE_REDIS_RESP:
        {
            procSgipReportRedisResp(pMsg);
            break;
        }
        case MSGTYPE_SGIP_RT_REQ:
        {
            procRtPushReqMsg(pMsg);
            break;
        }
        case MSGTYPE_SGIP_MO_REQ:
        {
            procMoPushReqMsg(pMsg);
            break;
        }
        case MSGTYPE_SGIP_LINK_CLOSE_REQ:
        {
            procSgipCloseReqMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_SMSACCOUNT_UPDATE_REQ:
        {
            TUpdateSmsAccontReq* pAccountUpdateReq = (TUpdateSmsAccontReq*)pMsg;
            m_AccountMap.clear();
            m_AccountMap = pAccountUpdateReq->m_SmsAccountMap;
            LogNotice("m_AccountMap size[%d].",m_AccountMap.size());
            break;
        }
        case MSGTYPE_GET_LINKED_CLIENT_REQ:
        {
            HandleGetLinkedClientReq(pMsg);
            break;
        }
        case MSGTYPE_REPORT_GET_AGAIN_REQ:
        {
            procRtPushReqMsg(pMsg);
            break;
        }
        case MSGTYPE_MO_GET_AGAIN_REQ:
        {
            procMoPushReqMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            procTimeOut(pMsg);
            break;
        }
        default:
        {
            LogError("MsgType[%d] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}

void CSgipRtAndMoThread::HandleDeliverReq(TMsg* pMsg)
{
    CSgipFailRtReqMsg* pFail = (CSgipFailRtReqMsg*)pMsg;

    AccountMap::iterator iteror = m_AccountMap.find(pFail->m_strClientId);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s],length[%d],accountMapsize[%d] is not find in accountMap.",
            pFail->m_strClientId.data(),pFail->m_strClientId.length(),m_AccountMap.size());
        return;
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    if (0 == uNeedReport)
    {
        LogDebug("clientid[%s] reportFlag[%d] is must not report.",pFail->m_strClientId.data(),uNeedReport);
        return;
    }


    RtAndMoLinkMap::iterator itrLink = m_mapRtAndMoLinkMap.find(pFail->m_strClientId);
    if (itrLink == m_mapRtAndMoLinkMap.end())
    {
        CSgipPush* pPush = new CSgipPush();
        pPush->init(iteror->second.m_strSgipRtMoIp,iteror->second.m_uSgipRtMoPort,iteror->second.m_strAccount,iteror->second.m_strPWD);

        pPush->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,pFail->m_strClientId, SGIP_CONNECT_TIMEOUT_NUM);

        CSgipRtReqMsg* pRt = new CSgipRtReqMsg();
        pRt->m_iMsgType = MSGTYPE_SGIP_RT_REQ;
        pRt->m_strSessionID.assign(pFail->m_strClientId);
        pRt->m_uSequenceId = pFail->m_uSequenceId;
        pRt->m_uSequenceIdNode = pFail->m_uSequenceIdNode;
        pRt->m_uSequenceIdTime = pFail->m_uSequenceIdTime;
        pRt->m_strPhone = pFail->m_strPhone;
        pRt->m_strClientId = pFail->m_strClientId;
        if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(pFail->m_uState))
        {
            pRt->m_uState = 0;
            pRt->m_uErrorCode = 0;
        }
        else
        {
            pRt->m_uState = 2;
            pRt->m_uErrorCode = 99;
        }
        pPush->m_vecRtAndMo.push_back(pRt);

        m_mapRtAndMoLinkMap[pFail->m_strClientId] = pPush;
    }
    else
    {
        if (CONNECT_STATUS_OK != itrLink->second->m_uState)
        {
            CSgipRtReqMsg* pRt = new CSgipRtReqMsg();
            pRt->m_iMsgType = MSGTYPE_SGIP_RT_REQ;
            pRt->m_strSessionID.assign(pFail->m_strClientId);
            pRt->m_uSequenceId = pFail->m_uSequenceId;
            pRt->m_uSequenceIdNode = pFail->m_uSequenceIdNode;
            pRt->m_uSequenceIdTime = pFail->m_uSequenceIdTime;
            pRt->m_strPhone = pFail->m_strPhone;
            pRt->m_strClientId = pFail->m_strClientId;
            if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(pFail->m_uState))
            {
                pRt->m_uState = 0;
                pRt->m_uErrorCode = 0;
            }
            else
            {
                pRt->m_uState = 2;
                pRt->m_uErrorCode = 99;
            }
            itrLink->second->m_vecRtAndMo.push_back(pRt);
        }
        else
        {
            SGIPReportReq reportReq;
            reportReq.sequenceId_ = m_SnManager.getDirectSequenceId();
            reportReq.subimtSeqNumNode_ = pFail->m_uSequenceIdNode;
            reportReq.subimtSeqNumTime_ = pFail->m_uSequenceIdTime;
            reportReq.subimtSeqNum_ = pFail->m_uSequenceId;
            reportReq.userNumber_.assign(pFail->m_strPhone);
            if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(pFail->m_uState))
            {
                reportReq.state_ = 0;
                reportReq.errorCode_ = 0;
            }
            else
            {
                reportReq.state_ = 2;
                reportReq.errorCode_ = 99;
            }

            pack::Writer writer(itrLink->second->m_pSocket->Out());
            reportReq.Pack(writer);
            LogElk("client submit error response clientid[%s],phone[%s].",pFail->m_strClientId.data(),pFail->m_strPhone.data());

        }
    }



}

void CSgipRtAndMoThread::procSgipCloseReqMsg(TMsg* pMsg)
{
    CSgipCloseReqMsg* pClose = (CSgipCloseReqMsg*)pMsg;
    LogNotice("clientid[%s] is close.",pClose->m_strClientId.data());
    RtAndMoLinkMap::iterator iterCon = m_mapRtAndMoLinkMap.find(pClose->m_strClientId);
    if (iterCon == m_mapRtAndMoLinkMap.end())
    {
        LogError("clientid[%s] is not find in m_mapRtAndMoLinkMap.",pClose->m_strClientId.data());
        return;
    }

    delete iterCon->second->m_ExpireTimer;
    iterCon->second->destroy();
    m_mapRtAndMoLinkMap.erase(iterCon);
    return;
}

void CSgipRtAndMoThread::procRtPushReqMsg(TMsg* pMsg)
{
    CSgipRtReqMsg* pRt = (CSgipRtReqMsg*)pMsg;

    RtAndMoLinkMap::iterator iterRt = m_mapRtAndMoLinkMap.find(pRt->m_strClientId);
    if (iterRt == m_mapRtAndMoLinkMap.end())
    {
        LogElk("clientid[%s] is not find in m_mapRtAndMoLinkMap.",pRt->m_strClientId.data());
        return;
    }

    if(iterRt->second->m_ExpireTimer != NULL)
    {
        delete iterRt->second->m_ExpireTimer;
        iterRt->second->m_ExpireTimer = NULL;
    }

    iterRt->second->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,pRt->m_strClientId, SGIP_CONNECT_TIMEOUT_NUM);

    SGIPReportReq reportReq;
    reportReq.sequenceId_ = m_SnManager.getDirectSequenceId();
    reportReq.subimtSeqNumNode_ = pRt->m_uSequenceIdNode;
    reportReq.subimtSeqNumTime_ = pRt->m_uSequenceIdTime;
    reportReq.subimtSeqNum_ = pRt->m_uSequenceId;
    reportReq.userNumber_.assign(pRt->m_strPhone);
    reportReq.state_ = pRt->m_uState;
    reportReq.errorCode_ = pRt->m_uErrorCode;

    pack::Writer writer(iterRt->second->m_pSocket->Out());
    reportReq.Pack(writer);
    LogElk("push report clientid[%s],phone[%s].",pRt->m_strClientId.data(),pRt->m_strPhone.data());
}

void CSgipRtAndMoThread::procMoPushReqMsg(TMsg* pMsg)
{
    CSgipMoReqMsg* pMo = (CSgipMoReqMsg*)pMsg;
    RtAndMoLinkMap::iterator iterMo = m_mapRtAndMoLinkMap.find(pMo->m_strClientId);
    if (iterMo == m_mapRtAndMoLinkMap.end())
    {
        LogError("clientid[%s] is not find in m_mapRtAndMoLinkMap.",pMo->m_strClientId.data());
        return;
    }

    if(iterMo->second->m_ExpireTimer != NULL)
    {
        delete iterMo->second->m_ExpireTimer;
        iterMo->second->m_ExpireTimer =NULL;
    }

    iterMo->second->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,pMo->m_strClientId, SGIP_CONNECT_TIMEOUT_NUM);

    SGIPDeliverReq deliverReq;
    utils::UTFString utfHelper;
    string strOut = "";
    int len = utfHelper.u2u16(pMo->m_strContent, strOut);
    if (len > 140)
    {
        LogElk("short message length is too long len[%d]",len);
    }
    deliverReq.setContent((char*)strOut.data(),len);
    deliverReq.m_strSpNum.assign(pMo->m_strSpNum);
    deliverReq.m_strPhone.assign(pMo->m_strPhone);
    deliverReq.sequenceId_ = m_SnManager.getDirectSequenceId();

    pack::Writer writer(iterMo->second->m_pSocket->Out());
    deliverReq.Pack(writer);
    LogElk("push MO clientid[%s],content[%s],phone[%s].",pMo->m_strClientId.data(),pMo->m_strContent.data(),pMo->m_strPhone.data());
}

void CSgipRtAndMoThread::procSgipReportRedisResp(TMsg* pMsg)
{
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
    if ((NULL == pRedisResp->m_RedisReply)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pRedisResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pRedisResp->m_RedisReply->type == REDIS_REPLY_ARRAY) && (pRedisResp->m_RedisReply->elements == 0)))
    {
        LogError("redis reply type is error[%d], cmd[%s]", pRedisResp->m_RedisReply->type, pRedisResp->m_RedisCmd.c_str());

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
        freeReply(pRedisResp->m_RedisReply);

        string cmdkey = "accesssms:";
        cmdkey += reportKey;

        TRedisReq* pRedisDel = new TRedisReq();
        pRedisDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pRedisDel->m_RedisCmd.assign("DEL  ");
        pRedisDel->m_RedisCmd.append(cmdkey);
        pRedisDel->m_strKey = cmdkey;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pRedisDel);
    }

    LogDebug("submitIds[%s], clientId[%s], ids[%s], cmd[%s]", submitIds.data(), clientId.data(), strIDs.data(), pRedisResp->m_RedisCmd.c_str());
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
            LogError("submitId count error.");
            delete itReportMsg->second->m_pRedisRespWaitTimer;
            itReportMsg->second->m_pRedisRespWaitTimer = NULL;
            delete itReportMsg->second;
            m_ReportMsgMap.erase(itReportMsg);
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
        if(vecIDs.size() != submitsize)
        {
            LogError("ID count error.");
            delete itReportMsg->second->m_pRedisRespWaitTimer;
            itReportMsg->second->m_pRedisRespWaitTimer = NULL;
            delete itReportMsg->second;
            m_ReportMsgMap.erase(itReportMsg);
            return;
        }
    }

    AccountMap::iterator iteror = m_AccountMap.find(clientId);
    UInt32 uNeedReport = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s],length[%d],accountMapSize[%d] is not find in accountMap.",
            clientId.data(),clientId.length(),m_AccountMap.size());
        delete itReportMsg->second->m_pRedisRespWaitTimer;
        itReportMsg->second->m_pRedisRespWaitTimer = NULL;
        delete itReportMsg->second;
        m_ReportMsgMap.erase(itReportMsg);
        return;
    }
    else
    {
        uNeedReport = iteror->second.m_uNeedReport;
    }

    for(UInt32 i = 0; i < submitsize; i++)
    {
        if (0 == itReportMsg->second->m_uUpdateFlag)
        {
            UpdateRecord(submitslist[i], vecIDs[i], iteror->second.m_uIdentify, strDate, itReportMsg->second);
        }

        if (0 != uNeedReport)
        {
            string strSgipSequence = submitslist.at(i);
            vector<string> vecSequence;
            Comm comm;
            comm.splitExVector(strSgipSequence, "|", vecSequence);
            UInt32 uSequenceIdNode = 0;
            UInt32 uSequenceIdTime = 0;
            UInt32 uSequenceId = 0;
            if(vecSequence.size() != 3)
            {
                LogError("strSgipSequence[%s] is invalid.",strSgipSequence.data());
            }
            else
            {
                uSequenceIdNode = atoi(vecSequence.at(0).data());
                uSequenceIdTime= atoi(vecSequence.at(1).data());
                uSequenceId = atoi(vecSequence.at(2).data());
            }

            RtAndMoLinkMap::iterator itrLink = m_mapRtAndMoLinkMap.find(clientId);
            if (itrLink == m_mapRtAndMoLinkMap.end())
            {
                CSgipPush* pPush = new CSgipPush();
                pPush->init(iteror->second.m_strSgipRtMoIp,iteror->second.m_uSgipRtMoPort,iteror->second.m_strAccount,iteror->second.m_strPWD);

                pPush->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,clientId, SGIP_CONNECT_TIMEOUT_NUM);

                CSgipRtReqMsg* pRt = new CSgipRtReqMsg();
                pRt->m_iMsgType = MSGTYPE_SGIP_RT_REQ;
                pRt->m_strSessionID.assign(clientId);
                pRt->m_uSequenceId = uSequenceId;
                pRt->m_uSequenceIdNode = uSequenceIdNode;
                pRt->m_uSequenceIdTime = uSequenceIdTime;
                pRt->m_strPhone = itReportMsg->second->m_strPhone;
                pRt->m_strClientId = clientId;
                if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(itReportMsg->second->m_iStatus))
                {
                    pRt->m_uState = 0;
                    pRt->m_uErrorCode = 0;
                }
                else
                {
                    pRt->m_uState = 2;
                    if(itReportMsg->second->m_strReportDesc.length() <= 3 &&
                        atoi(itReportMsg->second->m_strReportDesc.data()) < 256 &&
                        atoi(itReportMsg->second->m_strReportDesc.data()) >= 0)
                    {
                        pRt->m_uErrorCode = atoi(itReportMsg->second->m_strReportDesc.data());
                    }
                    else
                    {
                        pRt->m_uErrorCode = 99;
                    }
                }
                pPush->m_vecRtAndMo.push_back(pRt);

                m_mapRtAndMoLinkMap[clientId] = pPush;
            }
            else
            {
                if (CONNECT_STATUS_OK != itrLink->second->m_uState)
                {
                    CSgipRtReqMsg* pRt = new CSgipRtReqMsg();
                    pRt->m_iMsgType = MSGTYPE_SGIP_RT_REQ;
                    pRt->m_strSessionID.assign(clientId);
                    pRt->m_uSequenceId = uSequenceId;
                    pRt->m_uSequenceIdNode = uSequenceIdNode;
                    pRt->m_uSequenceIdTime = uSequenceIdTime;
                    pRt->m_strPhone = itReportMsg->second->m_strPhone;
                    pRt->m_strClientId = clientId;
                    if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(itReportMsg->second->m_iStatus))
                    {
                        pRt->m_uState = 0;
                        pRt->m_uErrorCode = 0;
                    }
                    else
                    {
                        pRt->m_uState = 2;
                        if(itReportMsg->second->m_strReportDesc.length() <= 3 &&
                            atoi(itReportMsg->second->m_strReportDesc.data()) < 256 &&
                            atoi(itReportMsg->second->m_strReportDesc.data()) >= 0)
                        {
                            pRt->m_uErrorCode = atoi(itReportMsg->second->m_strReportDesc.data());
                        }
                        else
                        {
                            pRt->m_uErrorCode = 99;
                        }
                    }
                    itrLink->second->m_vecRtAndMo.push_back(pRt);
                }
                else
                {
                    SGIPReportReq reportReq;
                    reportReq.sequenceId_ = m_SnManager.getDirectSequenceId();
                    reportReq.subimtSeqNumNode_ = uSequenceIdNode;
                    reportReq.subimtSeqNumTime_ = uSequenceIdTime;
                    reportReq.subimtSeqNum_ = uSequenceId;
                    reportReq.userNumber_.assign(itReportMsg->second->m_strPhone);
                    if (SMS_STATUS_CONFIRM_SUCCESS == UInt32(itReportMsg->second->m_iStatus))
                    {
                        reportReq.state_ = 0;
                        reportReq.errorCode_ = 0;
                    }
                    else
                    {
                        reportReq.state_ = 2;
                        if(itReportMsg->second->m_strReportDesc.length() < 3 &&
                            atoi(itReportMsg->second->m_strReportDesc.data()) < 256 &&
                            atoi(itReportMsg->second->m_strReportDesc.data()) >= 0)
                        {
                            reportReq.errorCode_ = atoi(itReportMsg->second->m_strReportDesc.data());
                        }
                        else
                        {
                            reportReq.errorCode_ = 99;
                        }

                    }

                    pack::Writer writer(itrLink->second->m_pSocket->Out());
                    reportReq.Pack(writer);
                    LogElk("push report clientid[%s],phone[%s].",clientId.data(),itReportMsg->second->m_strPhone.data());

                }
            }
        }
    }

    delete itReportMsg->second->m_pRedisRespWaitTimer;
    itReportMsg->second->m_pRedisRespWaitTimer = NULL;
    delete itReportMsg->second;
    m_ReportMsgMap.erase(itReportMsg);
}

void CSgipRtAndMoThread::UpdateRecord(string& strSubmitId,string strID, UInt32 uIdentify, string strDate, TReportReceiveToSMSReq *rpRecv)
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


void CSgipRtAndMoThread::procRtReqMsg(TMsg* pMsg)
{
    TReportReceiveToSMSReq *pTempReportReq = (TReportReceiveToSMSReq*)pMsg;
    TReportReceiveToSMSReq *pTReport2SgipReqSave = new TReportReceiveToSMSReq();
    pTReport2SgipReqSave->m_strSmsId = pTempReportReq->m_strSmsId;
    pTReport2SgipReqSave->m_strDesc = pTempReportReq->m_strDesc;
    pTReport2SgipReqSave->m_strPhone = pTempReportReq->m_strPhone;
    pTReport2SgipReqSave->m_strContent = pTempReportReq->m_strContent;
    pTReport2SgipReqSave->m_strUpstreamTime = pTempReportReq->m_strUpstreamTime;
    pTReport2SgipReqSave->m_strClientId = pTempReportReq->m_strClientId;
    pTReport2SgipReqSave->m_strSrcId = pTempReportReq->m_strSrcId;
    pTReport2SgipReqSave->m_iLinkId = pTempReportReq->m_iLinkId;
    pTReport2SgipReqSave->m_iType = pTempReportReq->m_iType;
    pTReport2SgipReqSave->m_iStatus = pTempReportReq->m_iStatus;
    pTReport2SgipReqSave->m_lReportTime = pTempReportReq->m_lReportTime;
    pTReport2SgipReqSave->m_uUpdateFlag = pTempReportReq->m_uUpdateFlag;
    pTReport2SgipReqSave->m_strReportDesc = pTempReportReq->m_strReportDesc;
    pTReport2SgipReqSave->m_strInnerErrcode = pTempReportReq->m_strInnerErrcode;
    
    string reportMapKey;
    reportMapKey = pTReport2SgipReqSave->m_strSmsId;
    reportMapKey = reportMapKey + '_';
    reportMapKey = reportMapKey + pTReport2SgipReqSave->m_strPhone;
    pTReport2SgipReqSave->m_pRedisRespWaitTimer = SetTimer(SGIP_REDIS_TIMEOUT_ID, reportMapKey, REDIS_RESP_WAIT_TIMEOUT);
    m_ReportMsgMap[reportMapKey] = pTReport2SgipReqSave;

    //search smsid_phone in redis.
    TRedisReq* req = new TRedisReq();
    req->m_iSeq = pMsg->m_iSeq;
    req->m_strSessionID = reportMapKey;
    string key ="accesssms:" + reportMapKey;
    LogDebug("key[%s], iseq[%d]", key.data(), pMsg->m_iSeq);
    req->m_RedisCmd =  "HGETALL " + key;
    req->m_strKey = key;
    req->m_iMsgType = MSGTYPE_REDIS_REQ;
    req->m_uReqTime = time(NULL);
    req->m_pSender = this;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,req);
}

void CSgipRtAndMoThread::procMoReqMsg(TMsg* pMsg)
{
    TReportReceiverMoMsg* pMoMsg = (TReportReceiverMoMsg*)pMsg;

    AccountMap::iterator iteror = m_AccountMap.find(pMoMsg->m_strClientId);
    UInt32 uNeedMo = 0;
    if (iteror == m_AccountMap.end())
    {
        LogElk("clientid[%s] is not find in accountMap.",pMoMsg->m_strClientId.data());
        return;
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

    if (1 == uNeedMo)
    {
        RtAndMoLinkMap::iterator itrLink = m_mapRtAndMoLinkMap.find(pMoMsg->m_strClientId);
        if (itrLink == m_mapRtAndMoLinkMap.end())
        {
            CSgipPush* pPush = new CSgipPush();
            pPush->init(iteror->second.m_strSgipRtMoIp,iteror->second.m_uSgipRtMoPort,iteror->second.m_strAccount,iteror->second.m_strPWD);

            pPush->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,pMoMsg->m_strClientId, SGIP_CONNECT_TIMEOUT_NUM);

            CSgipMoReqMsg* pMo = new CSgipMoReqMsg();
            pMo->m_iMsgType = MSGTYPE_SGIP_MO_REQ;
            pMo->m_strSessionID.assign(pMoMsg->m_strClientId);
            pMo->m_strClientId.assign(pMoMsg->m_strClientId);
            pMo->m_strContent.assign(pMoMsg->m_strContent);
            pMo->m_strPhone.assign(strPhone);
            pMo->m_strSpNum.assign(strTemp);
            pPush->m_vecRtAndMo.push_back(pMo);

            m_mapRtAndMoLinkMap[pMoMsg->m_strClientId] = pPush;
        }
        else
        {
            if (CONNECT_STATUS_OK != itrLink->second->m_uState)
            {
                CSgipMoReqMsg* pMo = new CSgipMoReqMsg();
                pMo->m_iMsgType = MSGTYPE_SGIP_MO_REQ;
                pMo->m_strSessionID.assign(pMoMsg->m_strClientId);
                pMo->m_strClientId.assign(pMoMsg->m_strClientId);
                pMo->m_strContent.assign(pMoMsg->m_strContent);
                pMo->m_strPhone.assign(strPhone);
                pMo->m_strSpNum.assign(strTemp);

                itrLink->second->m_vecRtAndMo.push_back(pMo);
            }
            else
            {
                SGIPDeliverReq deliverReq;

                utils::UTFString utfHelper;
                string strOut = "";
                int len = utfHelper.u2u16(pMoMsg->m_strContent, strOut);
                if (len > 140)
                {
                    LogElk("short message length is too long len[%d]",len);
                }
                deliverReq.setContent((char*)strOut.data(),len);
                deliverReq.m_strSpNum.assign(strTemp);
                deliverReq.m_strPhone.assign(strPhone);
                deliverReq.sequenceId_ = m_SnManager.getDirectSequenceId();

                if(itrLink->second->m_ExpireTimer != NULL)
                {
                    delete itrLink->second->m_ExpireTimer;
                    itrLink->second->m_ExpireTimer = NULL;
                }

                itrLink->second->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,pMoMsg->m_strClientId, SGIP_CONNECT_TIMEOUT_NUM);

                pack::Writer writer(itrLink->second->m_pSocket->Out());
                deliverReq.Pack(writer);
                LogElk("push MO clientid[%s],content[%s],phone[%s].",
                    pMoMsg->m_strClientId.data(),pMoMsg->m_strContent.data(),strPhone.data());
            }
        }
    }
}

void CSgipRtAndMoThread::procTimeOut(TMsg* pMsg)
{
    if (SGIP_REDIS_TIMEOUT_ID == pMsg->m_iSeq)
    {
        ReportMsgMap::iterator itReportMsg = m_ReportMsgMap.find(pMsg->m_strSessionID);
        if(itReportMsg == m_ReportMsgMap.end())
        {
            LogWarn("m_strSessionID[%s] is not find m_ReportMsgMap", pMsg->m_strSessionID.data());
            return;
        }

        LogDebug("Sessionid[%s] smsid:%s,phone:%s,SGIP_REDIS_TIMEOUT.",
            pMsg->m_strSessionID.data(),itReportMsg->second->m_strSmsId.data(),itReportMsg->second->m_strPhone.data());

        delete itReportMsg->second->m_pRedisRespWaitTimer;
        delete itReportMsg->second;
        m_ReportMsgMap.erase(itReportMsg);
    }
    else if (SGIP_CONNECT_TIMEOUT_ID == pMsg->m_iSeq)
    {
        LogDebug("clientid[%s],SGIP_CONNECT_TIMEOUT.",pMsg->m_strSessionID.data());
        RtAndMoLinkMap::iterator iterCon = m_mapRtAndMoLinkMap.find(pMsg->m_strSessionID);
        if (iterCon == m_mapRtAndMoLinkMap.end())
        {
            LogError("clientid[%s] is not find in m_mapRtAndMoLinkMap.",pMsg->m_strSessionID.data());
            return;
        }

        delete iterCon->second->m_ExpireTimer;
        iterCon->second->destroy();
        m_mapRtAndMoLinkMap.erase(iterCon);
    }
    else if ((UInt64)CHECK_CLIENT_RPTMO_LINK_TIMERID == pMsg->m_iSeq)
    {   
        //check unlinked sgip up link
        for(AccountMap::iterator it = m_AccountMap.begin(); it != m_AccountMap.end(); it++)
        {
            RtAndMoLinkMap::iterator itlink = m_mapRtAndMoLinkMap.find(it->first);
            if((itlink == m_mapRtAndMoLinkMap.end()) && (it->second.m_uSmsFrom == SMS_FROM_ACCESS_SGIP))
            {
                //m_AccountMap ¨®D¡ê? m_mapRtAndMoLinkMap ??¨®D?¡ê then new a up link
                CSgipPush* pPush = new CSgipPush();
                pPush->init(it->second.m_strSgipRtMoIp,it->second.m_uSgipRtMoPort,it->second.m_strAccount,it->second.m_strPWD);
                pPush->m_ExpireTimer = SetTimer(SGIP_CONNECT_TIMEOUT_ID,it->first, SGIP_CONNECT_TIMEOUT_NUM);

                LogDebug("ontimer connect up link, clientid[%s]", it->first.c_str());
                m_mapRtAndMoLinkMap[it->first] = pPush;
            }
        }

        if(m_pTimer_CheckLink != NULL)
        {
            delete m_pTimer_CheckLink;
            m_pTimer_CheckLink = NULL;
        }
        m_pTimer_CheckLink = SetTimer(CHECK_CLIENT_RPTMO_LINK_TIMERID, "timer_to_check_clt_rpt_link", CHECK_CLIENT_RPTMO_LINK_TIME);
    }
    else
    {
        LogWarn("iSeq[%lu] is invalid.",pMsg->m_iSeq);
    }
}

void CSgipRtAndMoThread::HandleGetLinkedClientReq(TMsg* pMsg)
{
    //AccountMap::iterator itrAccount = m_AccountMap.find(pReq->m_strAccount);
    CLinkedClientListRespMsg* presp = new CLinkedClientListRespMsg();
    for(RtAndMoLinkMap::iterator itrAccount = m_mapRtAndMoLinkMap.begin(); itrAccount != m_mapRtAndMoLinkMap.end(); itrAccount++)
    {
        LogDebug("add link client[%s]", itrAccount->first.c_str());
        presp->m_list_Client.push_back(itrAccount->first);
    }

    if(presp->m_list_Client.size() ==0)
    {
        ////LogDebug("no client link");
        delete presp;
        presp = NULL;
    }
    else
    {
        LogDebug("linked client sum[%u]", presp->m_list_Client.size());
        presp->m_iMsgType = MSGTYPE_GET_LINKED_CLIENT_RESQ;
        pMsg->m_pSender->PostMsg(presp);
    }
}


