#include "CSgipReportThread.h"
#include <stdlib.h>
#include <iostream>
#include "main.h"
#include "Comm.h"

CSgipReportThread::CSgipReportThread(const char *name):CThread(name)
{
    m_pInternalService = NULL;
}

CSgipReportThread::~CSgipReportThread()
{
}


bool CSgipReportThread::Init(const std::string ip, unsigned int port)
{   
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }
    
    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }

    g_pRuleLoadThread->getChannelErrorCode(m_mapChannelErrorCode);

    ChannelMap mapSgipChannel;
    g_pRuleLoadThread->getSgipChannlelMap(mapSgipChannel);

    for (ChannelMap::iterator iter = mapSgipChannel.begin(); iter != mapSgipChannel.end(); ++iter)
    {
        const models::Channel& channel = iter->second;

        if (5 == channel.httpmode)
        {
            m_mapChannelInfo[channel.channelID] = channel;

            LogNotice("Add Channel[id:%d, httpmode:%u, name:%s, url:%s].",
                channel.channelID,
                channel.httpmode,
                channel.channelname.data(),
                channel.url.data());
        }
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

void CSgipReportThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
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
}

void CSgipReportThread::HandleMsg(TMsg* pMsg)
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
            HandleSgipReportAcceptSocketMsg(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_ERROR_CODE_UPDATE_REQ:
        {
            TUpdateChannelErrorCodeReq* pSgipMsg = (TUpdateChannelErrorCodeReq*)pMsg;
            m_mapChannelErrorCode.clear();
            m_mapChannelErrorCode = pSgipMsg->m_mapChannelErrorCode;
            LogNotice("update t_sms_channel_error_desc size:%d.",m_mapChannelErrorCode.size());
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            HandleChannelUpdate(pMsg);
            break;
        }
        case MSGTYPE_SOCKET_WRITEOVER:
        {
            HandleSgipReportOverMsg(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}

void CSgipReportThread::HandleChannelUpdate(TMsg* pMsg)
{
    TUpdateChannelReq* pSgip = (TUpdateChannelReq*)pMsg;
    m_mapChannelInfo.clear();

    ChannelMap::iterator iter = pSgip->m_ChannelMap.begin();

    for (; iter != pSgip->m_ChannelMap.end(); ++iter)
    {
        const models::Channel& channel = iter->second;

        if (5 == channel.httpmode)
        {
            m_mapChannelInfo[channel.channelID] = channel;

            LogNotice("Add Channel[id:%d, httpmode:%u, name:%s, url:%s].",
                channel.channelID,
                channel.httpmode,
                channel.channelname.data(),
                channel.url.data());
        }
    }
}

void CSgipReportThread::HandleSgipReportOverMsg(TMsg* pMsg)
{
    map<UInt64,SgipReportSocketHandler*>::iterator iter = m_mapSession.find(pMsg->m_iSeq);
    if(iter == m_mapSession.end())
    {
        LogWarn("ERROR, can't find session,iSeq[%lu]", pMsg->m_iSeq);
        return;
    }

    //¹Øsocket
    if(NULL != iter->second)
    {
        LogDebug("delete SgipReportSocketHandler  iSeq[%lu].",pMsg->m_iSeq);

        iter->second->Destroy();
        delete iter->second;
    }

    //É¾³ýsession
    m_mapSession.erase(iter);
}

void CSgipReportThread::HandleSgipReportAcceptSocketMsg(TMsg* pMsg)
{
    TAcceptSocketMsg *pTAcceptSocketMsg = (TAcceptSocketMsg*)pMsg;
    UInt64 iSeq = m_SnManager.getSn();

    SgipReportSocketHandler* pReportSocketHandler = new SgipReportSocketHandler(this, iSeq);
    if(false == pReportSocketHandler->Init(m_pInternalService, pTAcceptSocketMsg->m_iSocket, pTAcceptSocketMsg->m_address))
    {
        LogError("CSMServiceThread::HandleSgipReportAcceptSocketMsg is failed.")
        delete pReportSocketHandler;
        return;
    }

    LogDebug("add SgipReportSocketHandler  iSeq[%lu].",iSeq);

    m_mapSession[iSeq] = pReportSocketHandler;
}




