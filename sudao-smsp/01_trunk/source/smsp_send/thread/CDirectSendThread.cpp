#include "main.h"
#include "Uuid.h"
#include "Comm.h"
#include "base64.h"

bool CDirectSendThread::Init( UInt32 uThreadId )
{
    if ( false == CThread::Init())
    {
        LogError("CDirectSendThread::Init CThread::Init is failed.");
        return false;
    }

    pthread_rwlock_init(&m_rwlock, NULL);

    m_pInternalService = new InternalService();
    if( m_pInternalService == NULL )
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }

    m_pInternalService->Init();

    m_pLinkedBlockPool = new LinkedBlockPool();

    m_uThreadId = uThreadId;
    return true;
}

/*添加消息*/
bool CDirectSendThread::PostChannelMsg( TMsg* pMsg, int ichannelid )
{
    bool ret = true;

    pthread_mutex_lock(&m_mutex);

    /* 获取通道队列的队列*/
    map<int, TMsgQueue*>::iterator it = m_ChannelmsgQueueMap.find( ichannelid );
    if( it == m_ChannelmsgQueueMap.end())
    {
        TDispatchToDirectSendReqMsg* msg = (TDispatchToDirectSendReqMsg*)(pMsg);
        LogWarn("[%s:%s:%d] can not find channelMsgQueue",
            msg->m_smsParam.m_strSmsId.c_str(), msg->m_smsParam.m_strPhone.c_str(), ichannelid);
        SAFE_DELETE( pMsg );
        ret = false;
    }
    else
    {
        it->second->AddMsg(pMsg);
    }

    pthread_mutex_unlock(&m_mutex);

    return ret;

}

void CDirectSendThread::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    TMsg* pMsg = NULL;
    TMsg* pChannelMsg = NULL;
    int   iSend = 0;

    while( true )
    {
        UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();

        pChannelMsg = NULL;
        iSend = 0;

    /* 遍历所有队列*/
        for( map<int, TMsgQueue*>::iterator itchlQueue = m_ChannelmsgQueueMap.begin(); itchlQueue != m_ChannelmsgQueueMap.end();)
        {
            ChannelInfoMap::iterator itr = m_mapChannelInfo.find( itchlQueue->first );
            if ( itr == m_mapChannelInfo.end())
            {
                LogError("channelid:%d is not find in m_mapChannelInfo.", itchlQueue->first );
                if( itchlQueue->second->GetMsgSize() <= 0 )
                {
                    SAFE_DELETE ( itchlQueue->second );
                    m_ChannelmsgQueueMap.erase(itchlQueue++);
                }
                continue;
            }

            /* 获取通道消息*/
            pthread_mutex_lock(&m_mutex);
            pChannelMsg = itchlQueue->second->GetMsg();
            pthread_mutex_unlock(&m_mutex);

            if ( NULL != pChannelMsg )
            {
                HandleMsg(pChannelMsg);
                SAFE_DELETE( pChannelMsg );
                iSend++;
            }
            itchlQueue++;
        }

        //get thread msg
        pthread_mutex_lock(&m_mutex);
        pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if ( NULL != pMsg )
        {
            HandleMsg(pMsg);
            SAFE_DELETE( pMsg );
            iSend++;
        }

        if( pMsg == NULL && 0 == iSelect && iSend == 0 )
        {
            usleep(g_uSecSleep);
        }

    }

    m_pInternalService->GetSelector()->Destroy();
}


void CDirectSendThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    switch(pMsg->m_iMsgType)
    {
        /*发送请求*/
        case MSGTYPE_DISPATCH_TO_DIRECTSEND_REQ:
        {
            HandleDispatchToDirectSendReq(pMsg);
            break;
        }
        case MSGTYPE_CHANNEL_SLIDE_WINDOW_UPDATE:
        {
            HandleChannelSlideWindowUpdate(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_DEL_REQ:
        {
            HandleChannelDel(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_ADD_REQ:
        {
            HandleChannelAdd(pMsg);
            break;
        }
        case MSGTYPE_CONTINUE_LOGIN_FAIL_VALUE_UPDATE:
        {
            HandleContinueLoginFailedValueChanged(pMsg);
            break;
        }
        case MSGTYPE_CHANNEL_CLOSE_WAIT_TIME_UPDATE:
        {
            HandleChannelCloseWaitTimeUpdate(pMsg);
            break;
        }
        /*通道重连*/
        case MSGTYPE_CHANNEL_CONN_STATUS:
        {
            TDirectChannelConnStatusMsg* connMsg = (TDirectChannelConnStatusMsg*)(pMsg);
            ChannelInfoMap::iterator itrTemp = m_mapChannelInfo.find(connMsg->m_uChannelId);
            if( itrTemp != m_mapChannelInfo.end())
            {
                LogNotice("Direct Channel [%u] Change State:[%u]",connMsg->m_uChannelId, connMsg->m_uState );
                switch( connMsg->m_uState )
                {
                    case CHAN_INIT:
                    case CHAN_DATATRAN:
                    case CHAN_UNLOGIN:
                    case CHAN_CLOSE:
                    case CHAN_MAXSLIDINGWINDOW:
                    case CHAN_EXCEPT:
                        break;
                    case CHAN_RESET:
                        itrTemp->second->reConn();
                        break;
                }
            }
            break;
        }
        /*处理超时*/
        case MSGTYPE_TIMEOUT:
        {
            if (0 == pMsg->m_strSessionID.compare("HeartTimeout"))
            {
                HandleHeartTimeOut(pMsg);
            }
            else if(0 == pMsg->m_strSessionID.compare("InitHeartTimeout"))
            {
                HandleHeartTimeOut(pMsg);

            }
            else if(0 == pMsg->m_strSessionID.compare("CloseChannelTimeOut"))
            {
                HandleChannelCloseTimeOut(pMsg);
            }

            break;
        }
        /*默认处理*/
        default:
        {
            LogError("MsgType[%d] is invalid.", pMsg->m_iMsgType);
            break;
        }
    }
}

void CDirectSendThread::HandleContinueLoginFailedValueChanged(TMsg* pMsg)
{
    TContinueLoginFailedValueMsg * p = (TContinueLoginFailedValueMsg*)pMsg;

    IChannel * pChannel = NULL;
    for(ChannelInfoMap::iterator iter = m_mapChannelInfo.begin(); iter != m_mapChannelInfo.end(); iter++)
    {
        pChannel = iter->second;
        pChannel->UpdateContinueLoginFailedValue(p->m_uValue);
    }
}

void CDirectSendThread::HandleChannelCloseWaitTimeUpdate(TMsg* pMsg)
{
    TChannelCloseWaitTimeMsg * p = (TChannelCloseWaitTimeMsg*)pMsg;

    LogNotice("m_uChannelCloseWaitTime Update[%d -> %d]", m_uChannelCloseWaitTime, p->m_uValue);
    m_uChannelCloseWaitTime = p->m_uValue;
}

UInt32 CDirectSendThread::HandleDispatchToDirectSendReq( TMsg* pMsg )
{
    UInt32 ret = CC_TX_RET_SUCCESS;

    TDispatchToDirectSendReqMsg* pDMsg = (TDispatchToDirectSendReqMsg*)pMsg;

    ChannelInfoMap::iterator itr = m_mapChannelInfo.find( pDMsg->m_smsParam.m_iChannelId );
    if ( itr == m_mapChannelInfo.end())
    {
        LogError("[%s:%s:%d] is not find in m_mapChannelInfo.",
                pDMsg->m_smsParam.m_strSmsId.data(),
                pDMsg->m_smsParam.m_strPhone.data(),
                pDMsg->m_smsParam.m_iChannelId);

        goto TO_REBACK;
    }

    if(itr->second->m_bCloseChannel)
    {
        LogWarn("[%s:%s:%d] will be close soon! Transfer the message to Reback!",
            pDMsg->m_smsParam.m_strSmsId.data(),
            pDMsg->m_smsParam.m_strPhone.data(),
            pDMsg->m_smsParam.m_iChannelId);

        goto TO_REBACK;
    }

    pDMsg->m_smsParam.m_strSessionId = pMsg->m_strSessionID;
    setMoRelation(itr->second, &pDMsg->m_smsParam);
    ret = itr->second->sendSms( pDMsg->m_smsParam );

    return ret ;

TO_REBACK:
    /* 找不到通道需要重新reback */
    DirectSendSimpleStatusReport( pMsg,  NO_CHANNEL );
    ret = CC_TX_RET_CHANNEL_NOT_EXSIT;
    return ret;
}

void CDirectSendThread::setMoRelation(IChannel* pChannel, smsDirectInfo* pSmsInfo)
{
    if (NULL == pChannel)
    {
        LogError("pChannel is NULL.");
        return;
    }

    /***  '通道类型，
            0 :   自签平台用户端口
            1 :   固签无自扩展，
            2：固签有自扩展，
            3：自签通道用户端口'
    ****/


    string strShowPhone = pChannel->m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort +
    pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;

    UInt32 uPortLength = pChannel->m_ChannelInfo._accessID.length() + pChannel->m_ChannelInfo.m_uExtendSize;
    if (uPortLength > 21)
    {
        uPortLength = 21;
    }
    if (strShowPhone.length() > uPortLength)
    {
        strShowPhone = strShowPhone.substr(0,uPortLength);
    }

    if ((1 == pChannel->m_ChannelInfo.m_uChannelType) || (2 == pChannel->m_ChannelInfo.m_uChannelType))
    {
        TRedisReq* req = new TRedisReq();
        const int bufsize = 2048;
        char cmd[bufsize] = {0};
        char cmdkey[1024] = {0};

        string strSign = "null";
        string strSignPort = "null";
        string strDisplayNum = "null";

        if (false == pSmsInfo->m_strSign.empty())
        {
           strSign = pSmsInfo->m_strSign;
        }

        if (false == pSmsInfo->m_strSignPort.empty())
        {
           strSignPort = pSmsInfo->m_strSignPort;
        }

        if (false == pSmsInfo->m_strDisplayNum.empty())
        {
           strDisplayNum = pSmsInfo->m_strDisplayNum;
        }
        /////HMSET sign_mo_port:channelid_showphone:phone clientid * sign * signport * userport * mourl * smsfrom*
        snprintf(cmdkey, 1024, "sign_mo_port:%d_%s:%s", pSmsInfo->m_iChannelId, strShowPhone.data(),pSmsInfo->m_strPhone.data());

        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
                cmdkey,
                pSmsInfo->m_strClientId.data(),
                strSign.data(),
                strSignPort.data(),
                strDisplayNum.data(),
                pSmsInfo->m_uC2sId,
                pSmsInfo->m_uSmsFrom);

        req->m_RedisCmd = cmd;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        req->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, req );

        LogDebug("==http sign_mo_port==smsid:%s,phone:%s,redisCmd:%s.", pSmsInfo->m_strSmsId.data(), pSmsInfo->m_strPhone.data(), cmd );

        TRedisReq* pDel = new TRedisReq();
        memset(cmd, 0x00, sizeof(cmd));
        snprintf(cmd, bufsize, "EXPIRE %s %u", cmdkey, pSmsInfo->m_uSignMoPortExpire);
        pDel->m_RedisCmd = cmd;
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDel );

        return;
    }



    if (LT_SGIP == pChannel->m_ChannelInfo.httpmode)
    {
        TRedisReq* req = new TRedisReq();
        const int bufsize = 2048;
        char cmd[bufsize] = {0};
        char cmdkey[1024] = {0};

        string strSign = "null";
        string strSignPort = "null";
        string strDisplayNum = "null";

        if (false == pSmsInfo->m_strSign.empty())
        {
            strSign = pSmsInfo->m_strSign;
        }

        if (false == pSmsInfo->m_strSignPort.empty())
        {
            strSignPort = pSmsInfo->m_strSignPort;
        }

        if (false == pSmsInfo->m_strDisplayNum.empty())
        {
            strDisplayNum = pSmsInfo->m_strDisplayNum;
        }

        /////HMSET moport:channelid_showphone client* sign* signport* userport* mourl* smsfrom*

        snprintf(cmdkey, 1024,"moport:%d_%s", pSmsInfo->m_iChannelId, strShowPhone.data());
        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
        cmdkey,
        pSmsInfo->m_strClientId.data(),
        strSign.data(),
        strSignPort.data(),
        strDisplayNum.data(),
        pSmsInfo->m_uC2sId,
        pSmsInfo->m_uSmsFrom);

        LogDebug("==sgip moport==smsid:%s,phone:%s,redisCmd:%s.", pSmsInfo->m_strSmsId.data(), pSmsInfo->m_strPhone.data(), cmd);

        req->m_RedisCmd = cmd;
        req->m_strKey = cmdkey;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,req);

        TRedisReq* pDel = new TRedisReq();
        pDel->m_RedisCmd = "EXPIRE " + string(cmdkey) + " 259200";
        pDel->m_strKey = cmdkey;
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);
    }
    else if ((YD_CMPP== pChannel->m_ChannelInfo.httpmode) || (DX_SMGP == pChannel->m_ChannelInfo.httpmode) || (YD_CMPP3 == pChannel->m_ChannelInfo.httpmode))
    {
        TDirectSendToDriectMoUrlReqMsg* pToMsg = new TDirectSendToDriectMoUrlReqMsg();

        pToMsg->m_MoParmInfo.m_strClientId.assign(pSmsInfo->m_strClientId);
        pToMsg->m_MoParmInfo.m_strExtendPort = pSmsInfo->m_strDisplayNum;
        pToMsg->m_MoParmInfo.m_uC2sId = pSmsInfo->m_uC2sId;
        pToMsg->m_MoParmInfo.m_strSign = pSmsInfo->m_strSign;
        pToMsg->m_MoParmInfo.m_strSignPort = pSmsInfo->m_strSignPort;
        pToMsg->m_MoParmInfo.m_strShowPhone = strShowPhone;
        pToMsg->m_MoParmInfo.m_uSmsFrom = pSmsInfo->m_uSmsFrom;

        pToMsg->m_iMsgType = MSGTYPE_DIRECT_SEND_TO_DIRECT_MO_REQ;
        g_pDirectMoThread->PostMsg(pToMsg);
    }
    else
    {
        return;
    }
}

void CDirectSendThread::HandleChannelDel( TMsg* pMsg )
{
    static Int64 sequence_id = 0;

    TDirectChannelUpdateMsg* delMsg = ( TDirectChannelUpdateMsg* )pMsg;

    ChannelInfoMap::iterator iter = m_mapChannelInfo.find(delMsg->m_iChannelId);
    if(iter != m_mapChannelInfo.end())
    {
        IChannel * pChannel = iter->second;

        //释放本地资源
        m_mapChannelInfo.erase(iter);

        pChannel->m_bCloseChannel = true;

        map<int, TMsgQueue*>::iterator iterQueue = m_ChannelmsgQueueMap.find(delMsg->m_iChannelId);
        if(iterQueue != m_ChannelmsgQueueMap.end())
        {
            //如果队列有数据，先将数据转移到reback
            TMsg* pChannelMsg = NULL;
            bool  loop = false;

            do{
                pthread_mutex_lock(&m_mutex);
                pChannelMsg = iterQueue->second->GetMsg();
                pthread_mutex_unlock(&m_mutex);

                if(pChannelMsg != NULL)
                {
                    HandleMsg(pChannelMsg);
                    SAFE_DELETE( pChannelMsg );
                    loop = true;
                }
                else
                {
                    loop = false;
                }
            }while(loop);

            //清除消息队列
            SAFE_DELETE(iterQueue->second);
            m_ChannelmsgQueueMap.erase(iterQueue);

            //定时释放通道
            Int64 deltaTime = time( NULL ) - delMsg->m_lDelTime;
            LogNotice("Channel[%d] will be closed in %ld seconds", delMsg->m_iChannelId, (Int64)m_uChannelCloseWaitTime - deltaTime);
            if(deltaTime < (Int64)m_uChannelCloseWaitTime)
            {
                ChannelWaitDelete Value;

                Value.m_ChannelDelMsg.m_Channel = delMsg->m_Channel;
                Value.m_ChannelDelMsg.m_iChannelId = delMsg->m_iChannelId;
                Value.m_ChannelDelMsg.m_iMsgType = delMsg->m_iMsgType;
                Value.m_ChannelDelMsg.m_lDelTime = delMsg->m_lDelTime;
                Value.m_ChannelDelMsg.m_bDelAll = delMsg->m_bDelAll;

                Value.m_pChannel = pChannel;

                m_mapChannelWaitDel.insert(make_pair(sequence_id, Value));

                //设定时器，时长为时间差
                pChannel->m_timerCloseChannel = SetTimer(sequence_id++, "CloseChannelTimeOut", ((Int64)m_uChannelCloseWaitTime - deltaTime) * 1000);
            }
            else
            {
                if(pChannel)
                {
                    //直接释放通道
                    pChannel->UpdateChannelInfo(delMsg->m_Channel);
                    pChannel->m_bDelAll = delMsg->m_bDelAll;
                    pChannel->destroy();
                }
            }
        }
        else
        {
            LogWarn("Channel[%d] is not found in m_ChannelmsgQueueMap", delMsg->m_iChannelId);
        }
    }
    else
    {
        LogWarn("Channel[%d] is not found in m_mapChannelInfo", delMsg->m_iChannelId);
    }
}

void CDirectSendThread::HandleChannelAdd( TMsg* pMsg )
{
    TDirectChannelUpdateMsg* addMsg = ( TDirectChannelUpdateMsg* )pMsg;
    InitChannelInfo(addMsg->m_Channel);
}

void CDirectSendThread::HandleChannelCloseTimeOut(TMsg* pMsg)
{
    map<Int64, ChannelWaitDelete>::iterator iter = m_mapChannelWaitDel.find(pMsg->m_iSeq);
    if(iter != m_mapChannelWaitDel.end())
    {
        LogNotice("Sequence[%lu] TimeOut, Close Channel[%d]", pMsg->m_iSeq, iter->second.m_ChannelDelMsg.m_iChannelId);

        IChannel * pChannel = iter->second.m_pChannel;

        if(pChannel)
        {
            //释放通道
            SAFE_DELETE(pChannel->m_timerCloseChannel);
            pChannel->UpdateChannelInfo(iter->second.m_ChannelDelMsg.m_Channel);
            pChannel->m_bDelAll = iter->second.m_ChannelDelMsg.m_bDelAll;
            pChannel->destroy();

            //协议内部已释放
            pChannel = NULL;
        }

        //释放本地资源
        m_mapChannelWaitDel.erase(iter);
    }
    else
    {
        LogWarn("Sequence[%lu] is not found in m_mapChannelWaitDel", pMsg->m_iSeq);
    }
}


/* 处理通道定时事件*/
void CDirectSendThread::HandleHeartTimeOut( TMsg* pMsg )
{
    ChannelInfoMap::iterator itr = m_mapChannelInfo.find(pMsg->m_iSeq);
    if (itr == m_mapChannelInfo.end())
    {
        LogError("iSeq[%lu],is not find in m_mapChannelInfo.", pMsg->m_iSeq);
        return;
    }

    itr->second->OnTimer();

    //print ChannelQueueMapSize
    PrintChannelQueueSize();

    SAFE_DELETE( itr->second->m_wheelTime );
    itr->second->m_wheelTime = SetTimer(pMsg->m_iSeq, "HeartTimeout", 3000 );
}

void CDirectSendThread::PrintChannelQueueSize()
{
    if( time(NULL) - m_uTimeForPrintChlMapSize > 60 )
    {
        for(map<int, TMsgQueue*>::iterator itchlQueue = m_ChannelmsgQueueMap.begin();
                itchlQueue != m_ChannelmsgQueueMap.end(); itchlQueue++ )
        {
            LogFatal("CDirectSendThread[%u] ChannelId[%d] QueueSize[%d].",
                     m_uThreadId, itchlQueue->first, itchlQueue->second->GetMsgSize());
        }

        m_uTimeForPrintChlMapSize = time(NULL);
    }
}

void CDirectSendThread::InitContinueLoginFailedValue(UInt32 uValue)
{
    m_uContinueLoginFailedValue = uValue;
}

void CDirectSendThread::InitChannelCloseWaitTime(UInt32 uValue)
{
    m_uChannelCloseWaitTime = uValue;
}

/* 直连协议增加通道*/
bool CDirectSendThread::InitChannelInfo( models::Channel& chan )
{
    LogNotice("Init Channel[%u],httpmode[%s:%u],protocoltype[%u]",
        chan.channelID,Comm::protocolToString(chan.httpmode).data(),chan.httpmode,chan.m_uProtocolType);

    if ( YD_CMPP == chan.httpmode )
    {
        if(CMPP_FUJIAN == chan.m_uProtocolType)
        {
            if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
            {
                CMPPProvinceChannelSend* pCmppChannel = new CMPPProvinceChannelSend();
                if(false ==  InsertChannelMap(pCmppChannel,chan))
                    return false;
            }
            else
            {
                CMPPProvinceChannel* pCmppChannel = new CMPPProvinceChannel();
                if(false ==  InsertChannelMap(pCmppChannel,chan))
                    return false;
            }
        }
        else
        {
            if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
            {
                CMPPChannelSend* pCmppChannel = new CMPPChannelSend();
                if(false == InsertChannelMap(pCmppChannel,chan))
                    return false;
            }
            else
            {
                CMPPChannel* pCmppChannel = new CMPPChannel();
                if(false == InsertChannelMap(pCmppChannel,chan))
                    return false;
            }
        }
    }
    else if ( GJ_SMPP == chan.httpmode )
    {
        if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
        {
            SMPPChannelSend* pSmppChannel = new SMPPChannelSend();
            if(false == InsertChannelMap(pSmppChannel,chan))
                return false;
        }
        else
        {
            SMPPChannel* pSmppChannel = new SMPPChannel();
            if(false == InsertChannelMap(pSmppChannel,chan))
                return false;
        }
    }
    else if ( LT_SGIP == chan.httpmode )
    {
        if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
        {
            SGIPChannelSend* pSgipChannel = new SGIPChannelSend();
            if(false == InsertChannelMap(pSgipChannel,chan))
                return false;
        }
        else
        {
            SGIPChannel* pSgipChannel = new SGIPChannel();
            if(false == InsertChannelMap(pSgipChannel,chan))
                return false;
        }

    }
    else if (DX_SMGP == chan.httpmode)
    {
        if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
        {
            SMGPChannelSend* pSmgpChannel = new SMGPChannelSend();
            if(false == InsertChannelMap(pSmgpChannel,chan))
                return false;
        }
        else
        {
            SMGPChannel* pSmgpChannel = new SMGPChannel();
            if(false == InsertChannelMap(pSmgpChannel,chan))
                return false;
        }

    }
    else if(YD_CMPP3 == chan.httpmode)
    {
        if (chan.m_uExValue & CHANNEL_OWN_SPLIT_FLAG)
        {
            CMPP3ChannelSend* pCmppChannel = new CMPP3ChannelSend();
            if(false == InsertChannelMap(pCmppChannel,chan))
                return false;
        }
        else
        {
            CMPP3Channel* pCmppChannel = new CMPP3Channel();
            if(false == InsertChannelMap(pCmppChannel,chan))
                return false;
        }

    }
    else
    {
        LogError("httpmode[%d] is not support direct.", chan.httpmode);
        return false;
    }

    //add by fangjinxiong:20161124 每条通道有各自自己的队列
    map<int, TMsgQueue*>::iterator itupdateq = m_ChannelmsgQueueMap.find(chan.channelID);
    if(itupdateq == m_ChannelmsgQueueMap.end())
    {
        TMsgQueue* pqueue = new TMsgQueue();
        m_ChannelmsgQueueMap[chan.channelID] = pqueue;      //add channelQueue to channelQueueMap
        LogDebug("add ChannelQueue[%d]", chan.channelID);
    }

    return true;
}

bool CDirectSendThread::InsertChannelMap( IChannel *pChannelInfo, models::Channel& chan )
{
    if(NULL == pChannelInfo)
    {
        LogWarn("param is NULL!!");
        return false;
    }

    pChannelInfo->m_uNodeId = m_uThreadId;
    pChannelInfo->m_pProcessThread = this;
    pChannelInfo->init(chan);
    pChannelInfo->UpdateContinueLoginFailedValue(m_uContinueLoginFailedValue);
    pChannelInfo->m_wheelTime = SetTimer(chan.channelID, "InitHeartTimeout", 3000);//稳定HeartTimeout，初始化InitHeartTimeout

    m_mapChannelInfo[chan.channelID] = pChannelInfo;
    return true;
}

void CDirectSendThread::HandleChannelSlideWindowUpdate(TMsg* pMsg)
{
    TDirectChannelUpdateMsg* p = (TDirectChannelUpdateMsg*)pMsg;
    LogDebug("Channel[%d] update!", p->m_iChannelId);
    ChannelInfoMap::iterator iter = m_mapChannelInfo.find(p->m_iChannelId);
    if(iter != m_mapChannelInfo.end())
    {
        IChannel * pChannel = iter->second;
        pChannel->UpdateChannelInfo(p->m_Channel);
    }
    else
    {
        LogWarn("Channel[%d] is not found in m_mapChannelInfo", p->m_iChannelId);
    }
}

/*短信状态上报处理*/
UInt32 CDirectSendThread::DirectSendSimpleStatusReport( TMsg* pMsg, string errCode )
{
    TDispatchToDirectSendReqMsg* pDMsg = (TDispatchToDirectSendReqMsg*)pMsg;
    CSMSTxStatusReportMsg *txReport = new CSMSTxStatusReportMsg();
    txReport->report.m_status        = SMS_STATUS_SUBMIT_FAIL;
    txReport->report.m_strSubmit     = errCode ;
    txReport->report.m_strYZXErrCode = errCode;
    txReport->report.m_strSmsid      = pDMsg->m_smsParam.m_strSmsId;
    txReport->report.m_uTotalCount   = 1;
    txReport->report.m_iIndexNum     = 1;
    txReport->m_strSessionID         = pMsg->m_strSessionID;
    txReport->m_iMsgType             = MSGTYPE_CHANNEL_SEND_STATUS_REPORT;
    g_CChannelThreadPool->smsTxPostMsg(txReport);
    return CC_TX_RET_SUCCESS;
}

/* 获取线程流控状态*/
UInt32 CDirectSendThread::GetFlowState( UInt32 uChannelId, int &flowCnt )
{
    UInt32 ret = CHANNEL_FLOW_STATE_CLOSE;

    pthread_rwlock_rdlock(&m_rwlock);

    ChannelInfoMap::iterator channelItr =  m_mapChannelInfo.find( uChannelId );
    if (channelItr != m_mapChannelInfo.end())
    {
        ret = channelItr->second->GetChannelFlowCtr( flowCnt );
    }

    pthread_rwlock_unlock(&m_rwlock);

    return ret ;
}
