
#include "ReportReceiveThread.h"
#include <stdlib.h>

#include<iostream>
#include "UrlCode.h"

#include "http/httprequest.h"
#include "http/httpresponse.h"
#include "main.h"
#include "Comm.h"

ReportReceiveThread::ReportReceiveThread(const char *name):
    CThread(name)
{
    m_pInternalService = NULL;
    m_pSnManager = NULL;
}


ReportReceiveThread::~ReportReceiveThread()
{
    //m_pServerSocket->Close();
}


bool ReportReceiveThread::Init(const std::string ip, unsigned int port)
{
    //printf("init ReportReceiveThread  \r\n");
    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        return false;
    }

    m_pInternalService->Init();
    if(NULL == m_pInternalService)
    {
        return false;
    }

    m_pServerSocekt = m_pInternalService->CreateServerSocket(this);
    if (false == m_pServerSocekt->Listen(Address(ip, port)))
    {
        printf("m_pServerSocekt->Listen is failed.\n");
        return false;
    }

    m_pSnManager = new SnManager();
    if(NULL == m_pSnManager)
    {
        LogError("new SnManager() is failed.");
        return false;
    }

    if(false == CThread::Init())
    {
        return false;
    }

    m_pLibMng = new ChannelMng();
    if (NULL == m_pLibMng)
    {
        LogError("m_pLibMng is NULL.");
        return false;
    }



    m_chlSchedule = new ChannelScheduler();
    m_chlSchedule->init();

    m_pLinkedBlockPool = new LinkedBlockPool();

    return true;
}

void ReportReceiveThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        int count = m_pInternalService->GetSelector()->Select();
        //m_pInternalService->GetTaskManager()->Process();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL && 0 == count)
        {
            //LogDebug("usleep 100");
            usleep(g_uSecSleep);
        }
        else if(pMsg != NULL)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void ReportReceiveThread::HandleMsg(TMsg* pMsg)
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
            HandleAcceptSocketReqMsg(pMsg);
            break;
        }
        case MSGTYPE_REPORTRECIVE_REQ:
        {
            HandleReportReciveReqMsg(pMsg);
            break;
        }
        case MSGTYPE_HTTP_SERVICE_RESP:     //for other thread return httpresponse
        {
            HttpResponse(pMsg->m_iSeq, pMsg->m_strSessionID);
            break;
        }
        case MSGTYPE_SOCKET_WRITEOVER:
        {
            //socket 写完了
            HandleReportReciveReturnOverMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            LogNotice("this is timeout iSeq[%ld]",pMsg->m_iSeq);
            HandleReportReciveReturnOverMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            TUpdateChannelReq* msg = (TUpdateChannelReq*)pMsg;

            //// remove is delete channel
            for ( ChannelMap::iterator itr = m_chlSchedule->m_ChannelMap.begin();
                    itr != m_chlSchedule->m_ChannelMap.end(); itr++ )
            {
                ChannelMap::iterator iter = msg->m_ChannelMap.find( itr->first );
                if ( iter == msg->m_ChannelMap.end()
                    && ( HTTP_GET == itr->second.httpmode
                        || HTTP_POST == itr->second.httpmode ))
                {
                    LogWarn(" HttpChanne delete channelId[%d],channelName[%s].",
                            itr->second.channelID, itr->second.channelname.data());
                    /* 关闭通道链接库*/
                    m_pLibMng->delChannelLib( itr->second.m_strChannelLibName );
                    m_pLibMng->delChannelLib( itr->second.channelname );
                }
            }
            m_chlSchedule->m_ChannelMap.clear();
            m_chlSchedule->m_ChannelMap = msg->m_ChannelMap;
            LogDebug("m_ChannelMap size[%d].", msg->m_ChannelMap.size());
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ:
        {
            HandleChannelErrorCodeUpdateMsg(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}

void ReportReceiveThread::HandleChannelErrorCodeUpdateMsg(TMsg* pMsg)
{
    TUpdateChannelErrorCodeReq* pRecv = (TUpdateChannelErrorCodeReq*)pMsg;
    if (NULL == pRecv)
    {
        LogError("pRecv is NULL.");
        return;
    }
}

void ReportReceiveThread::HandleAcceptSocketReqMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iSeq = m_pSnManager->getSn();

    //new sockethandle
    ReportSocketHandler *pReportSocketHandler = new ReportSocketHandler(this, iSeq);

    //new intenalsocket
    if(false == pReportSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("ReportSocketHandler init is failed.");
        if(NULL != pReportSocketHandler)
        {
            delete pReportSocketHandler;
            pReportSocketHandler = NULL;
        }
        return;
    }

    //add sessionmap, set timer
    ReportReceiveSession* pSession = new ReportReceiveSession();
    pSession->m_webHandler = pReportSocketHandler;
    pSession->m_wheelTimer = SetTimer(iSeq, "", 6*1000);    //SMS_session 6s超时
    m_SessionMap[iSeq] = pSession;

}

void ReportReceiveThread::HandleReportReciveReqMsg(TMsg* pMsg)
{
    TReportReciveRequest *pReportReciveRequest = (TReportReciveRequest*)pMsg;
    LogDebug("ReportRecive, data[%s]", pReportReciveRequest->m_strRequest.data());

    string response = "OK";
    string query = http::UrlCode::UrlDecode(pReportReciveRequest->m_strRequest);
    StateReportVector report;
    UpstreamSmsVector upsms;

    //get channelID
    Channel channel;
    UInt32 uChannelType = 0;
    UInt32 channelID = GetChannelIDfromName(pReportReciveRequest->m_strChannel,uChannelType, channel);

    LogDebug("channelname: %s, channelLibName:%s", channel.channelname.c_str(), channel.m_strChannelLibName.c_str());
    smsp::Channellib *mylib = m_pLibMng->loadChannelLibByNames(channel.m_strChannelLibName, channel.channelname );

    if(mylib)
    {
        if(mylib->parseReport(query, report, upsms, response))
        {

            LogNotice("parseReport,channelid[%d],report.size[%d],upsms.size[%d],query[%s],channellibname[%s]",channelID,report.size(),upsms.size(),query.data(), channel.m_strChannelLibName.c_str());
            for(StateReportVector::iterator it = report.begin(); it != report.end(); it++)
            {
                LogDebug("_status: %d, smsid: %s, phone: %s", it->_status, it->_smsId.c_str(), it->_phone.c_str());
                TReportToDispatchReqMsg* req = new TReportToDispatchReqMsg();
                req->m_iStatus = it->_status;
                if (req->m_iStatus != Int32(SMS_STATUS_CONFIRM_SUCCESS))
                {
                    req->m_strDesc = it->_desc;
                }
                else
                {
                    req->m_strDesc = it->_desc;
                }
                req->m_lReportTime = it->_reportTime;
                req->m_strChannelSmsId = it->_smsId;
                req->m_uChannelId = channelID;
                req->m_strChannelLibName = channel.m_strChannelLibName;
                req->m_uChannelIdentify = channel.m_uChannelIdentify;
                req->m_uClusterType  = channel.m_uClusterType;
                req->m_strPhone      = Comm::trim(it->_phone);
                req->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (channel.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
                req->m_iMsgType = MSGTYPE_REPORT_TO_DISPATCH_REQ;
                g_pDispatchThread->PostMsg(req);
            }

            for ( UpstreamSmsVector::iterator iter = upsms.begin(); iter != upsms.end(); ++iter)
            {
                LogDebug("_appendid: %s, _content: %s, phone: %s",
                                    iter->_appendid.c_str(), iter->_content.c_str(), iter->_phone.c_str());
                TUpStreamToDispatchReqMsg* pUpStream = new TUpStreamToDispatchReqMsg();
                if (NULL == pUpStream)
                {
                    LogError("pUpStream is NULL.");
                    continue;
                }
                pUpStream->m_iChannelId = channelID;
                pUpStream->m_strChannelLibName = channel.m_strChannelLibName;
                pUpStream->m_lUpTime = iter->_upsmsTimeInt;
                pUpStream->m_strAppendId = iter->_appendid;

                pUpStream->m_strMoPrefix = channel.m_strMoPrefix;

                if (channel.channelname == "MAS")
                {
                    if (pUpStream->m_strAppendId.size() >= channel._accessID.size())
                    {
                        string strID =  pUpStream->m_strAppendId.substr(channel._accessID.size());
                        //LogError("pUpStream strID = %s,size=%d, channel._accessID=%s, size=%d.",strID.data(),strID.size(), channel._accessID.data(), channel._accessID.size());
                        pUpStream->m_strAppendId = strID;
                    }
                }

                pUpStream->m_strContent = iter->_content;
                pUpStream->m_strPhone = Comm::trim(iter->_phone);
                pUpStream->m_strUpTime = iter->_upsmsTimeStr;
                pUpStream->m_uChannelType = uChannelType;
                pUpStream->m_iMsgType = MSGTYPE_UPSTREAM_TO_DISPATCH_REQ;
                g_pDispatchThread->PostMsg(pUpStream);
            }
        }
        else
        {
            LogWarn("channel[%s] parseReportFailed. query[%s]", pReportReciveRequest->m_strChannel.data(), query.data());
            response = "failed";
        }
    }
    else
    {
        LogError("mylib is NULL.");
        response = "failed";
    }

    //给通道应答

    if (pReportReciveRequest->m_strChannel == "MAS")
    {
        if (response == "failed")
        {
            LogError("mylib MAS parse report failed,need to ask for reSend.");
        }
        response = "ok";     //ask for return ok whenever success or fail

    }

    HttpResponse(pReportReciveRequest->m_iSeq, response);
    /*
    SessionMap::iterator it = m_SessionMap.find(pReportReciveRequest->m_iSeq);
    if(it == m_SessionMap.end())
    {
        LogWarn("get session failed, parse over, return to user");
        return;
    }

    if(NULL != it->second->m_webHandler && NULL != it->second->m_webHandler->m_socket)
    {
        std::string content;
        http::HttpResponse respone;
        respone.SetStatusCode(200);
        respone.SetContent(response);
        respone.Encode(content);
        it->second->m_webHandler->m_socket->Out()->Write(content.data(), content.size());
        it->second->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }
    */

}

void ReportReceiveThread::HttpResponse(UInt64 iSeq, string& response)
{


    //给应答
    SessionMap::iterator it = m_SessionMap.find(iSeq);
    if(it == m_SessionMap.end())
    {
        LogWarn("get session failed, parse over, return to user");
        return;
    }

    if(NULL != it->second->m_webHandler && NULL != it->second->m_webHandler->m_socket)
    {
        std::string content;
        http::HttpResponse respone;
        respone.SetStatusCode(200);
        respone.SetContent(response);
        respone.Encode(content);

        LogDebug("======content======[%s]", content.data());
        it->second->m_webHandler->m_socket->Out()->Write(content.data(), content.size());
        it->second->m_webHandler->m_socket->Out()->Flush();
    }
    else
    {
        LogWarn("webhandle->m_socket or webhandler is NULL");
    }
}

void ReportReceiveThread::HandleReportReciveReturnOverMsg(TMsg* pMsg)
{
    //write done
    SessionMap::iterator it = m_SessionMap.find(pMsg->m_iSeq);
    if(it == m_SessionMap.end())
    {
        //error can't find session
        LogWarn("can't find session,iSeq[%lu]", pMsg->m_iSeq);
        return;
    }

    //delete timer
    if(NULL != it->second->m_wheelTimer)
    {
        delete it->second->m_wheelTimer;
    }

    //关socket
    if(NULL != it->second->m_webHandler)
    {
        it->second->m_webHandler->Destroy();
        delete it->second->m_webHandler;
    }

    //删除session
    delete it->second;
    m_SessionMap.erase(it);
}

UInt32 ReportReceiveThread::GetChannelIDfromName(string channelname,UInt32& uChannelType, Channel& channel)
{
    ChannelMap::iterator it = m_chlSchedule->m_ChannelMap.begin();

    for(; it != m_chlSchedule->m_ChannelMap.end(); it++)
    {
        if(it->second.channelname == channelname)
        {
            uChannelType = it->second.m_uChannelType;
            channel = it->second;
            return it->second.channelID;
        }
    }

    LogWarn("channelName[%s],Len[%d] is not find m_ChannelMap size[%d].",channelname.data(),channelname.length(),m_chlSchedule->m_ChannelMap.size());
    return 0;
}

UInt32 ReportReceiveThread::GetSessionMapSize()
{
    return m_SessionMap.size();
}
