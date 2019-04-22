#include "SGIPChannelSend.h"
#include "UTFString.h"
#include "SGIPUnBindReq.h"
#include "SGIPUnBindResp.h"
#include "SGIPConnectReq.h"
#include "SGIPConnectResp.h"
#include "SGIPSubmitReq.h"
#include "SGIPSubmitResp.h"
#include <sstream>
#include <cwctype>
#include <algorithm>
#include "main.h"
#include "Comm.h"

SGIPChannelSend::SGIPChannelSend()
{
    m_pSocket = NULL;
    m_uHeartbeatTick = 0;
    m_bHasReconnTask = false;
    m_wheelTime = NULL;
}

SGIPChannelSend::~SGIPChannelSend()
{
}

void SGIPChannelSend::destroy()
{
    LogWarn("==link==channelid:%d link is break.",m_ChannelInfo.channelID);
    
    updateLoginState(m_uChannelLoginState, 2, "channel is close");  
    m_uChannelLoginState = 2;
    
    m_iState = CHAN_CLOSE;
    if(m_pSocket){
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
    if (NULL != m_wheelTime)
    {
        m_wheelTime->Destroy();
        m_wheelTime = NULL;
    }
    
    channelSessionMap::iterator iter = _packetDB.begin();
    LogDebug("_packetDB.size()=%d", _packetDB.size());
    for (; iter != _packetDB.end(); ++iter )
    {
        directSendSesssion *session = iter->second;
        session->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        session->m_pSmsContext->m_strSubmit = SGIP_INTERNAL_ERROR;
        session->m_pSmsContext->m_strYZXErrCode = SGIP_INTERNAL_ERROR;
        channelStatusReport( iter->first, false );
        SAFE_DELETE( iter->second );
        _packetDB.erase(iter);
    }

    delete this;
}

UInt32 SGIPChannelSend::sendSms( smsDirectInfo &SmsInfo )
{
    if ( m_iState != CHAN_DATATRAN || NULL == m_pSocket )
    {
        UInt64 uSeq = m_SnMng.getDirectSequenceId();
        string strContent = "";
        saveSplitContent( &SmsInfo, strContent, 1, 1, uSeq, 1, 1 ); 
        LogError("==sgip  error==[%s:%s:%d]connect is abnoramal.", 
                 SmsInfo.m_strSmsId.data(), SmsInfo.m_strPhone.data(), m_ChannelInfo.channelID);
        channelStatusReport( uSeq );
        return CHAN_CLOSE;
    }

    std::vector<string> contents;
    UInt32 cnt =1;
    int msgTotal = 1;

    bool isIncludeChinese = Comm::IncludeChinese((char*)(SmsInfo.m_strSign + SmsInfo.m_strContent).data());
    
    string strSign = "";
    string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
    string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );

    if ( false == SmsInfo.m_strSign.empty() )
    {
        strSign = strLeft + SmsInfo.m_strSign + strRight;
    }

    SmsInfo.m_strSign = strSign;

    cnt = parseSplitContent(SmsInfo.m_strContent, contents, SmsInfo.m_strSign, isIncludeChinese);
    msgTotal = contents.size();
    
    ///long message
    if ( cnt > 1 && msgTotal == 1 )
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendSgipLongMessage(&SmsInfo, contents,cnt,msgTotal, isIncludeChinese);
    }
    else ///short message
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendSgipShortMessage(&SmsInfo, contents,cnt,msgTotal, isIncludeChinese);
    }

    return CHAN_DATATRAN;
}



void SGIPChannelSend::sendSgipShortMessage( smsDirectInfo *pSmsInfo, vector<string>& vecContent, 
             UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese)
{
    utils::UTFString utfHelper;
    int i = 0;
    for (std::vector<std::string>::iterator it = vecContent.begin(); it != vecContent.end(); it++)
    {
        i++;
        std::string sendContent;

        if (pSmsInfo->m_uShowSignType == 2)
        {
            (*it) = (*it) + pSmsInfo->m_strSign;
        }
        else if ((pSmsInfo->m_uShowSignType == 0) || (pSmsInfo->m_uShowSignType == 1))
        {
            (*it) = pSmsInfo->m_strSign + (*it);
        }
        else
        {
            ////no sign
            ;
        }

        sendContent = (*it);

        SGIPSubmitReq req;
        int len = 0;
        if(isIncludeChinese)
        {
            len = utfHelper.u2u16((*it), sendContent);
            req.msgCoding_ = 8;
        }
        else
        {   
            
            len = sendContent.length(); //utf-8 > ascII
            req.msgCoding_ = 0;
        }
        
        req.sequenceId_ = m_SnMng.getDirectSequenceId();
        ////req.spNum_ = m_ChannelInfo._accessID;
        //////extro no
        string strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;

        UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
        if (uPortLength > 21)
        {
            uPortLength = 21;
        }
        if (strAppend.length() > uPortLength)
        {
            strAppend = strAppend.substr(0,uPortLength);
        }
        
        req.spNum_.append(strAppend);
        pSmsInfo->m_strShowPhone = req.spNum_;
        req.corpId_ = m_ChannelInfo.spID;

        //////prefix add 86
        if (1 == m_ChannelInfo.m_uNeedPrefix)
        {
            req.userNum_.assign("86");
            req.userNum_.append(pSmsInfo->m_strPhone);
        }
        else
        {
            req.userNum_ = pSmsInfo->m_strPhone;
        }

        
        req.serviceType_ = m_ChannelInfo.m_strServerId;        
        req.setContent((char*)sendContent.data(),len);

        pack::Writer writer(m_pSocket->Out());
        req.Pack(writer);

        LogNotice("==sgip submit==[%s:%s:%d]iSeq[%u]",
                  pSmsInfo->m_strSmsId.data(),
                  pSmsInfo->m_strPhone.data(),
                  m_ChannelInfo.channelID,req.sequenceId_);
        
        saveSplitContent(pSmsInfo, (*it), uTotal, i, req.sequenceId_, uMsgTotal, isIncludeChinese);
        
    }
}

void SGIPChannelSend::sendSgipLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent
    ,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese)
{
    utils::UTFString utfHelper;
    for (std::vector<std::string>::iterator it = vecContent.begin(); it != vecContent.end(); it++)
    {
        std::string sendContent;

        if (pSmsInfo->m_uShowSignType == 2)
        {
            (*it) = (*it) + pSmsInfo->m_strSign;
        }
        else if ((pSmsInfo->m_uShowSignType == 0) || ( pSmsInfo->m_uShowSignType == 1))
        {
            (*it) = pSmsInfo->m_strSign + (*it);
        }
        else
        {
            ////no sign
            ;
        }

        sendContent = (*it);        
        int len = utfHelper.u2u16((*it), sendContent);
        int totalLen = len;
        string out;
        int lenSign = utfHelper.u2u16(pSmsInfo->m_strSign, out);
        
        int MAXLENGTH = m_ChannelInfo.m_uCnSplitLen * 2;
        if (!isIncludeChinese)
        {
             MAXLENGTH = m_ChannelInfo.m_uEnSplitLen * 2;
        }
       
        int size = (len + MAXLENGTH -1)/MAXLENGTH;
        int sizeWithSign = size;
        if (pSmsInfo->m_uShowSignType == 3)
        {
            if (m_ChannelInfo.m_uSplitRule)
            {
                sizeWithSign = (len + lenSign + MAXLENGTH -1)/MAXLENGTH;
                LogDebug("size=%d,sizeWithSign=%d", size, sizeWithSign);
            }
        }
        int index = 1;
        char* chtmp = new char[1024];

        UInt32 uCand = getRandom();
        bool ifLast = false;

        while(len > 0)
        {
            memset(chtmp,0,1024);
            std::string strtmp;
            int strlen = 0;

            if(len > MAXLENGTH)
            {
                strtmp = sendContent.substr(index*MAXLENGTH - MAXLENGTH, MAXLENGTH);
                strlen = MAXLENGTH;
                len = len -MAXLENGTH;
            }
            else
            {
                if (sizeWithSign == 2  && size != sizeWithSign)
                {
                    if (index == 1)
                    {
                        strtmp = sendContent.substr(0, len - 2);
                        strlen = len - 2;
                        len = 2;
                    }
                    else if (index == 2)
                    {
                        strtmp = sendContent.substr(totalLen - 2);
                        strlen = 2;
                        len = 0;
                    }
                }
                else
                {
                    if (size != sizeWithSign && sizeWithSign > 2)
                    {
                        if (ifLast)
                        {
                            strtmp = sendContent.substr(totalLen - 2);
                            strlen = 2;
                            len = 0;
                        }
                        else
                        {
                            strtmp = sendContent.substr(index*MAXLENGTH - MAXLENGTH, len - 2);
                            strlen = len - 2;
                            len = 2;
                            ifLast = true;
                        }
                    }
                    else
                    {
                        strtmp = sendContent.substr(index*MAXLENGTH - MAXLENGTH);
                        strlen = len;
                        len = 0;
                    }
                
                }
            }

            int a =5;
            memcpy(chtmp,(void*)&a,1);

            a = 0;
            memcpy(chtmp+1,(void*)&a,1);

            a = 3;
            memcpy(chtmp+2,(void*)&a,1);

            a = uCand;
            memcpy(chtmp+3,(void*)&a,1);

            a= size;
            memcpy(chtmp+4,(void*)&a,1);
            
            a =index;
            memcpy(chtmp+5,(void*)&a,1);

            //拼content内容部分
            memcpy(chtmp+6, strtmp.data(), strlen);

            SGIPSubmitReq req;
            req.sequenceId_ = m_SnMng.getDirectSequenceId();
            /////req.spNum_ = m_ChannelInfo._accessID;
            //////extro no
            string strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;

            UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
            if (uPortLength > 21)
            {
                uPortLength = 21;
            }
            if (strAppend.length() > uPortLength)
            {
                strAppend = strAppend.substr(0,uPortLength);
            }
        
            req.spNum_.append(strAppend);
            pSmsInfo->m_strShowPhone = req.spNum_;
            req.tpUdhi_ = 0x01;
            req.msgCoding_ = 8;
            req.corpId_ = m_ChannelInfo.spID;

            //////prefix add 86
            if (1 == m_ChannelInfo.m_uNeedPrefix)
            {
                req.userNum_.assign("86");
                req.userNum_.append(pSmsInfo->m_strPhone);
            }
            else
            {
                req.userNum_ = pSmsInfo->m_strPhone;
            }
            ///// 
            req.serviceType_ = m_ChannelInfo.m_strServerId;
            //////LogDebug("###serviceId[%s],strAppend[%s].",req.serviceType_.data(),strAppend.data());
            req.setContent(chtmp, strlen+6);
            
            pack::Writer writer(m_pSocket->Out());
            req.Pack(writer);

            LogNotice("==sgip submit==[%s:%s:%d]iSeq[%u]",
                pSmsInfo->m_strSmsId.data(),
                pSmsInfo->m_strPhone.data(),
                m_ChannelInfo.channelID,req.sequenceId_);
            
            string strContent = "";
            utfHelper.u162u(strtmp,strContent);
            saveSplitContent(pSmsInfo, strContent, uTotal, index, req.sequenceId_, uMsgTotal, isIncludeChinese);
            index++;
        }

        delete[] chtmp;
        chtmp = NULL;
    }
}

void SGIPChannelSend::bindHandler() 
{
    _hander[SGIP_BIND_RESP] = &SGIPChannelSend::onConnResp;
    _hander[SGIP_SUBMIT_RESP] = &SGIPChannelSend::onSubmitResp;
    _hander[SGIP_UNBIND_RESP] = &SGIPChannelSend::onUnBindResp;
}

void SGIPChannelSend::onSubmitResp(pack::Reader& reader, SGIPBase& resp)
{
    SGIPSubmitResp respSubmit(resp);
    if (respSubmit.Unpack(reader))
    {
        std::string retCode;
        char result[3] = {0};
        snprintf(result,3,"%d", respSubmit._result);
        retCode = result;

        channelSessionMap::iterator it = _packetDB.find( resp.sequenceId_);
        if (it != _packetDB.end())
        {
            directSendSesssion *pSession = it->second;
            smsp::SmsContext   *pSms  = pSession->m_pSmsContext;

            LogNotice("==sgip submitResp==[%s:%s:%d]channelsmsid:%s,result:%s,iSeq:%u",
                pSms->m_strSmsId.data(),pSms->m_strPhone.data(), m_ChannelInfo.channelID
                , respSubmit._msgId.data(),result, resp.sequenceId_);
            
            pSms->m_strSubret = result;
            pSms->m_strChannelSmsId = respSubmit._msgId;
            pSms->m_lSubretDate= time(NULL);
            
            if( retCode != "0" )    //submitResp failed
            {
                pSms->m_iStatus = SMS_STATUS_SEND_FAIL;  
            }
            else
            {
                pSms->m_iStatus = SMS_STATUS_SEND_SUCCESS;
            }

            /* 上报状态*/   
            channelStatusReport( resp.sequenceId_ );
            
        }
        else
        {
            LogWarn("channelid:%d,hasn't find Submit Packet,seq[%u], channelsmsid[%s]!", 
                      m_ChannelInfo.channelID, resp.sequenceId_, respSubmit._msgId.data());
        }

    }
}


void SGIPChannelSend::onUnBindResp(pack::Reader& reader, SGIPBase& resp)
{
    LogDebug("sgip onUnBindResp");

    if(resp.packetLength_ > resp.bodySize())        //20 = head.length
    {   
        LogWarn("warn, onUnBindResp.length[%d]>20", resp.packetLength_);
        string strtmp;
        reader((UInt32)(resp.packetLength_ - resp.bodySize()), strtmp);
    }
    
    m_iState = CHAN_INIT;
}


void SGIPChannelSend::onConnResp(pack::Reader& reader, SGIPBase& resp)
{
    m_uHeartbeatTick = 0;

    SGIPConnectResp respCon(resp);
    if (respCon.Unpack(reader))
    {
        if (respCon.isLogin())
        {
            m_iState = CHAN_DATATRAN;
            LogNotice("SGIP channelid[%d] Login success.", m_ChannelInfo.channelID);
            m_uContinueLoginFailedNum = 0;
            InitFlowCtr();
            updateLoginState(m_uChannelLoginState, 1, Comm::int2str(respCon.result_));
            m_uChannelLoginState = 1;
            
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState   =  CHAN_DATATRAN;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
            m_pProcessThread->PostMsg(pmsg);
        } 
        else
        {
            LogError("SGIP channelid[%d],status:%d, Login failed.", m_ChannelInfo.channelID,respCon.result_);
            m_iState = CHAN_INIT;   
            
            updateLoginState(m_uChannelLoginState, 2, Comm::int2str(respCon.result_));
            m_uChannelLoginState = 2;
        }
    }
}


void SGIPChannelSend::init(models::Channel& chan)
{
    IChannel::init(chan);   
    bindHandler();

    m_ChannelInfo = chan;
    //m_flowCtr.iWindowSize = chan.m_uSliderWindow;
    //m_flowCtr.lLastRspTime = time( NULL );
    
    SGIPBase::g_nNode = m_ChannelInfo.nodeID;
    conn();
}

void SGIPChannelSend::conn() 
{
    if (++m_uHeartbeatTick > 2)
    {
        updateLoginState(m_uChannelLoginState, 2, "reconnect 2 times, no response.");
        m_uChannelLoginState = 2;
        m_uHeartbeatTick = 0;
        LogWarn("sgip reconnect 2 times, no response.");
    }

    if (NULL != m_pSocket)
    {
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }

    m_pSocket = new InternalSocket();
    if(NULL == m_pSocket){
        LogError("mem error!!");
        return;
    }

    CDirectSendThread *DirectThread = static_cast< CDirectSendThread *>(m_pProcessThread);
    
    m_pSocket->Init( DirectThread->m_pInternalService, DirectThread->m_pLinkedBlockPool );

    m_pSocket->SetHandler(this);
    string url = m_ChannelInfo.url;
    string::size_type pos = url.find(":");
    if (pos == std::string::npos)
    {
        LogError("ip[%s] is invalid.", url.data());
        return;
    }

    string ip = url.substr(0, pos);
    UInt16 port = atoi(url.substr(pos + 1).data());
    LogNotice("SGIPChannelSend::connect server [%s:%d] accout[%s]", ip.data(), port,m_ChannelInfo._clientID.data());
    Address addr(ip, port);
    m_pSocket->Connect(addr, 10000);
    SGIPConnectReq reqLogin;
    reqLogin.sequenceId_ = m_SnMng.getDirectSequenceId();
    m_iState = CHAN_INIT;
    reqLogin.setAccout(m_ChannelInfo._clientID, m_ChannelInfo._password);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);

    //updateLoginState(m_uChannelLoginState, 0, "");
    m_uChannelLoginState = 0;
}

void SGIPChannelSend::disConn()
{
    SGIPUnBindReq req;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);
    m_iState = CHAN_INIT;
}

void SGIPChannelSend::reConn()
{
    conn();
}

bool SGIPChannelSend::onTransferData()
{
    SGIPBase resp;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return false;
    }
    pack::Reader reader(m_pSocket->In());
    if(reader.GetSize() < 20)
    {
        reader.SetGood(false);
        return reader;
    }
    
    if (resp.Unpack(reader))
    {
        if( reader.GetSize() < resp.packetLength_ - 20 )
        {
            reader.SetGood(false);
            return reader;
        }
        
        LogDebug("###commandID[0x%x],node[%d],time[%d],sequenceID[%u].",
                resp.requestId_,resp.sequenceIdNode_,resp.sequenceIdTime_,resp.sequenceId_);

        //check length from head. 
        if( resp.packetLength_ > 5000 )
        {
            //协议头中的 length过长。 断连接重连。
            LogError("resp.packetLength_[%d]>5000, reconn. channelid[%d], channelName[%s]", 
                    resp.packetLength_, m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState = CHAN_RESET;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;
            m_pProcessThread->PostMsg(pmsg);
            reader.SetGood(false);
            return reader;
        }

        
        //check cmd  
        map<UInt32, HANDLER_FUN>::iterator it = _hander.find(resp.requestId_);
        if (it != _hander.end())
        {   
            //cmd we need 
            (this->*(*it).second)(reader, resp);
        }
        else if(resp.requestId_ == SGIP_BIND
            || resp.requestId_ == SGIP_UNBIND
            || resp.requestId_ == SGIP_SUBMIT
            || resp.requestId_ == SGIP_REPORT || resp.requestId_ == SGIP_REPORT_RESP
            || resp.requestId_ == SGIP_DELIVER || resp.requestId_ == SGIP_DELIVER_RESP
            || resp.requestId_ == SGIP_ADDSP || resp.requestId_ == SGIP_ADDSP_RESP
            || resp.requestId_ == SGIP_MODIFYSP || resp.requestId_ == SGIP_MODIFYSP_RESP
            || resp.requestId_ == SGIP_DELETESP || resp.requestId_ == SGIP_DELETESP_RESP
            || resp.requestId_ == SGIP_QUERYROUTE || resp.requestId_ == SGIP_QUERYROUTE_RESP
            || resp.requestId_ == SGIP_ADDTELESEG || resp.requestId_ == SGIP_ADDTELESEG_RESP
            || resp.requestId_ == SGIP_MODIFYTELESEG || resp.requestId_ == SGIP_MODIFYTELESEG_RESP
            || resp.requestId_ == SGIP_DELETETELESEG || resp.requestId_ == SGIP_DELETETELESEG_RESP
            || resp.requestId_ == SGIP_ADDSMG || resp.requestId_ == SGIP_ADDSMG_RESP
            || resp.requestId_ == SGIP_MODIFYSMG || resp.requestId_ == SGIP_MODIFYSMG_RESP
            || resp.requestId_ == SGIP_DELETESMG || resp.requestId_ == SGIP_DELETESMG_RESP
            || resp.requestId_ == SGIP_CHECKUSER || resp.requestId_ == SGIP_CHECKUSER_RESP
            || resp.requestId_ == SGIP_USERRPT || resp.requestId_ == SGIP_USERRPT_RESP
            || resp.requestId_ == SGIP_TRACE || resp.requestId_ == SGIP_TRACE_RESP
        )
        {
            //cmd we do not need
            char* buffer = new char[resp.packetLength_ - 20];
            reader(resp.packetLength_ - 20, buffer);
            delete[] buffer;
        }
        else
        {
            //cmd not belong to sgip
            LogError("resp.requestId_[%d] not belong to sgip, reconn. channelid[%d], channelName[%s]", 
                    resp.requestId_, m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;           
            pmsg->m_uState = CHAN_RESET;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
            m_pProcessThread->PostMsg(pmsg);
            reader.SetGood(false);
        }
    }
    return reader;
}

void SGIPChannelSend::OnEvent(int type, InternalSocket *socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        m_iState = CHAN_INIT;
        LogDebug("SGIPChannelSend::Connected");
    }
    else if (type == EventType::Read)
    {
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("SGIPChannelSend::Close.channelid[%d], channelName[%s]", 
                m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_CLOSE;

        //updateLoginState(m_uChannelLoginState, 2, "link is close");
        //m_uChannelLoginState = 2;
    }
    else if (type == EventType::Error)
    {
        LogError("SGIPChannelSend::Error.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_EXCEPT;

        //updateLoginState(m_uChannelLoginState, 2, "link is error");
        //m_uChannelLoginState = 2;
    }

    /* 通道异常*/
    if( m_iState >= CHAN_CLOSE )
    {
        TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
        pmsg->m_uChannelId = m_ChannelInfo.channelID;
        pmsg->m_uState   =  m_iState;
        pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
        m_pProcessThread->PostMsg(pmsg);
    }

}

void SGIPChannelSend::OnTimer() 
{
    ////disConn();
    
    if (CHAN_DATATRAN != m_iState)
    {
        continueLoginFailed(m_uContinueLoginFailedNum);
        m_uContinueLoginFailedNum++;
        reConn();
    }
    else
    {
        /* 检测滑动窗口*/
        FlowSliderResetCheck();
    }

    ///_packetDB keep 1min smsContent
    Int64 iNowTime = time(NULL);
    
    channelSessionMap::iterator iter = _packetDB.begin();
    for (;iter != _packetDB.end();)
    {
        directSendSesssion *session = iter->second ;

        if (( iNowTime - session->m_pSmsContext->m_lSubmitDate) > ICHANNEL_SESSION_TIMEOUT_SECS )
        {
            LogWarn("==sgip submitResp timeout==[%s:%s:%d]_packetDB size:%d.",
                    session->m_pSmsContext->m_strSmsId.data(),
                    session->m_pSmsContext->m_strPhone.data(),
                    m_ChannelInfo.channelID,_packetDB.size());
                    
            session->m_pSmsContext->m_strSubmit = SGIP_REPLY_TIME_OUT;
            session->m_pSmsContext->m_strChannelSmsId = "";
            session->m_pSmsContext->m_lSubretDate= time(NULL);
            session->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_SUCCESS;
            channelStatusReport( iter->first, false );
            SAFE_DELETE( iter->second );
            _packetDB.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }
}

void SGIPChannelSend::onData()
{
    switch (m_iState)
    {
        case CHAN_DATATRAN:
        case CHAN_INIT:
        {
            while (onTransferData())
            {
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
