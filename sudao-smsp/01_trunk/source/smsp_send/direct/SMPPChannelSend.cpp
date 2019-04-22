#include "SMPPChannelSend.h"
#include "UTFString.h"
#include "SMPPConnectReq.h"
#include "SMPPConnectResp.h"
#include "SMPPSubmitReq.h"
#include "SMPPSubmitResp.h"
#include "SMPPHeartbeatReq.h"
#include "SMPPHeartbeatResp.h"
#include "SMPPDeliverReq.h"
#include "SMPPDeliverResp.h"
#include <sstream>
#include <cwctype>
#include <algorithm>
#include "main.h"
#include "Comm.h"

SMPPChannelSend::SMPPChannelSend()
{
    m_pSocket = NULL;
    m_uHeartbeatTick = 0;
    m_bHasReconnTask = false;
    m_wheelTime = NULL;
}

SMPPChannelSend::~SMPPChannelSend()
{

}

void SMPPChannelSend::init(models::Channel& chan)
{
    IChannel::init(chan);
    bindHandler();
    m_ChannelInfo = chan;
    
    //m_flowCtr.iWindowSize = chan.m_uSliderWindow;
    //m_flowCtr.lLastRspTime = time( NULL );

    if (m_wheelTime != NULL)
    {
        m_wheelTime->Destroy();
        m_wheelTime = NULL;
    }
    m_iState = CHAN_INIT;
}

void SMPPChannelSend::destroy()
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
        session->m_pSmsContext->m_strSubmit = SMPP_INTERNAL_ERROR;
        session->m_pSmsContext->m_strYZXErrCode = SMPP_INTERNAL_ERROR;
        channelStatusReport( iter->first, false );
        SAFE_DELETE( iter->second );
        _packetDB.erase(iter);
    }

    delete this;
}

UInt32 SMPPChannelSend::sendSms( smsDirectInfo &SmsInfo )
{
    if ( m_iState != CHAN_DATATRAN || NULL == m_pSocket )
    {
        UInt64 uSeq = m_SnMng.getDirectSequenceId();
        string strContent = "";
        saveSplitContent( &SmsInfo, strContent, 1, 1, uSeq, 1, 1 ); 
        LogError("==smpp error==[%s:%s:%d]connect is abnoramal.", 
           SmsInfo.m_strSmsId.data(), SmsInfo.m_strPhone.data(), m_ChannelInfo.channelID);
        channelStatusReport( uSeq );
        return CHAN_CLOSE;  
    }
    
    std::vector< string> contents;
    bool isIncludeChinese = Comm::IncludeChinese((char*)(SmsInfo.m_strSign + SmsInfo.m_strContent).data());

    string strSign = "";
    string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
    string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );


    if ( false == SmsInfo.m_strSign.empty() )
    {
        strSign = strLeft + SmsInfo.m_strSign + strRight;
    }

    SmsInfo.m_strSign = strSign;

    UInt32 cnt =1;
    int msgTotal = 1;
    
    cnt = parseSplitContent( SmsInfo.m_strContent, contents, SmsInfo.m_strSign, isIncludeChinese );
    msgTotal = contents.size();

    
    if (cnt > 1 && msgTotal == 1)
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendSmppLongMessage(&SmsInfo, contents,m_ChannelInfo._showsigntype,cnt,msgTotal,isIncludeChinese);
    }
    else ///short message
    {
        SmsInfo.m_uLongSms = 0;
        SmsInfo.m_uChannelCnt = 1;
        sendSmppShortMessage(&SmsInfo, contents,m_ChannelInfo._showsigntype,cnt,msgTotal, true);
    }

    return CHAN_DATATRAN;

}


void SMPPChannelSend::sendSmppLongMessage( smsDirectInfo *pSmsInfo, vector<string>& vecContent,
                         UInt32 uShowSignType, UInt32 uTotal, UInt32 uMsgTotal, bool isIncludeChinese)
{
    utils::UTFString utfHelper;
    for (std::vector<std::string>::iterator it = vecContent.begin(); it != vecContent.end(); it++)
    {
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

            memset(chtmp, 0x00, 1024);
            //拼content 6个头字段
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

            smsp::SMPPSubmitReq req;
            req._sequenceNum = m_SnMng.getDirectSequenceId();

            pSmsInfo->m_strShowPhone = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;

            UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
            if (uPortLength > 21)
            {
                uPortLength = 21;
            }
            
            if (pSmsInfo->m_strShowPhone.length() > uPortLength)
            {
                pSmsInfo->m_strShowPhone = pSmsInfo->m_strShowPhone.substr( 0, uPortLength );
            }

            req._esmClass = 0x40;
            req._phone = pSmsInfo->m_strPhone.substr(2,pSmsInfo->m_strPhone.length()-2);
            req._sourceAddr = m_ChannelInfo._accessID;
            req.setContent(chtmp, strlen+6);
            /////LogNotice("smpp long content[%s],seq[%u].", chtmp, req._sequenceNum);
            if(NULL == m_pSocket){
                LogError("socket is null!!");
                delete[] chtmp;
                chtmp = NULL;
                return;
            }
            pack::Writer writer(m_pSocket->Out());
            req.Pack(writer);

            LogNotice("==smpp submit==[%s:%s],iseq[%u] ", 
                     pSmsInfo->m_strSmsId.data(), pSmsInfo->m_strPhone.data(), req._sequenceNum );
            
            string strContent = "";
            utfHelper.u162u(strtmp,strContent);
            saveSplitContent(pSmsInfo, strContent, uTotal, index, req._sequenceNum, uMsgTotal, isIncludeChinese);
            index++;
        }
        delete[] chtmp;
        chtmp = NULL;
    }
}

void SMPPChannelSend::sendSmppShortMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent, 
                                UInt32 uShowSignType,UInt32 uTotal,UInt32 uMsgTotal, bool isIncludeChinese)
{
    int i = 0;
    utils::UTFString utfHelper;
    for (vector<string>::iterator it = vecContent.begin(); it != vecContent.end(); it++)
    {
        i++;
        string sendContent;

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

        smsp::SMPPSubmitReq req;
        UInt32 len = 0;
        if(isIncludeChinese)
        {
            len = utfHelper.u2u16((*it), sendContent);
            req._dataCoding = 8;
        }
        else
        {   
            
            len = sendContent.length(); //utf-8 > ascII, for englishWords
            req._dataCoding = 0;
        }
        
        ////LogDebug("s short change len[%d], content[%s].", len,(*it).data());

        pSmsInfo->m_strShowPhone = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;
        UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
        if (uPortLength > 21)
        {
            uPortLength = 21;
        }
        if (pSmsInfo->m_strShowPhone.length() > uPortLength)
        {
            pSmsInfo->m_strShowPhone = pSmsInfo->m_strShowPhone.substr(0,uPortLength);
        }
            
        req._sequenceNum = m_SnMng.getDirectSequenceId();
        req._phone = pSmsInfo->m_strPhone.substr(2,pSmsInfo->m_strPhone.length()-2);
        req._sourceAddr=m_ChannelInfo._accessID;
        req.setContent(sendContent,len);
        if(NULL == m_pSocket){
            LogError("socket is null!!");
            return;
        }
        pack::Writer writer(m_pSocket->Out());
        req.Pack(writer);

        LogNotice("==smpp submit==[%s:%s],iseq[%u]", 
                  pSmsInfo->m_strSmsId.data(), pSmsInfo->m_strPhone.data(),req._sequenceNum );
        
        saveSplitContent( pSmsInfo, (*it),uTotal, i,req._sequenceNum, uMsgTotal, isIncludeChinese );

    }

}


void SMPPChannelSend::bindHandler()
{
    _hander[BIND_TRANSCEIVER_RESP] = &SMPPChannelSend::onConnResp;
    _hander[SUBMIT_SM_RESP] = &SMPPChannelSend::onSubmitResp;
    _hander[ENQUIRE_LINK] = &SMPPChannelSend::onHeartbeat;
    _hander[ENQUIRE_LINK_RESP] = &SMPPChannelSend::onHeartbeat;
    _hander[DELIVER_SM] = &SMPPChannelSend::onDeliver;

}

void SMPPChannelSend::onSubmitResp(pack::Reader& reader, SMPPBase& resp)
{

    smsp::SMPPSubmitResp respSubmit(resp);

    if (respSubmit.Unpack(reader))
    {
        std::string retCode;
        char result[16] = { 0 };
        snprintf(result, 16,"%d", respSubmit._commandStatus);
        retCode = result;

        channelSessionMap::iterator it = _packetDB.find( resp._sequenceNum );
        if ( it != _packetDB.end() )
        {
            directSendSesssion *pSession = it->second;
            smsp::SmsContext   *pSms  = pSession->m_pSmsContext;
        
            LogNotice("==smpp submitResp== channelid:%d, smsID[%s], result[%s], iSeq[%u]", 
                        m_ChannelInfo.channelID, respSubmit._msgID.data(), result,resp._sequenceNum);

            pSms->m_strSubret = result;
            pSms->m_strChannelSmsId = respSubmit._msgID;
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
            channelStatusReport( resp._sequenceNum  );

        }
        else
        {
            LogWarn("channelid:%d,hasn't find Submit Packet,seq[%u], channelsmsid[%s]!", 
                     m_ChannelInfo.channelID, resp._sequenceNum, respSubmit._msgID.data());
        }

    }
}


void SMPPChannelSend::onDeliver(pack::Reader& reader, SMPPBase& resp)
{
    smsp::SMPPDeliverResp deliverResp(resp);
    if (deliverResp.Unpack(reader))
    {
        respDeliver(resp);

        LogElk("==smpp report== channelid:%d, msid[%s],status[%d].", 
                    m_ChannelInfo.channelID,deliverResp._msgId.data(),deliverResp._status);
        
        TDirectToDeliveryReportReqMsg* pMsg = new TDirectToDeliveryReportReqMsg();
        if (NULL == pMsg)
        {
            LogError("pMsg is NULL.");
            return;
        }

        pMsg->m_uChannelId = m_ChannelInfo.channelID;
        pMsg->m_uChannelIdentify = m_ChannelInfo.m_uChannelIdentify;
        pMsg->m_strChannelSmsId = deliverResp._msgId;
        pMsg->m_strPhone = deliverResp._sourceAddr;
        pMsg->m_lReportTime = time(NULL);
        pMsg->m_strDesc = deliverResp._remark;
        pMsg->m_strErrorCode = deliverResp._remark;
        pMsg->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (m_ChannelInfo.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
        pMsg->m_iMsgType = MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ;
        pMsg->m_iHttpMode = GJ_SMPP;

        UInt32 status = deliverResp._status;
        if(status != 0) //report failed
        {
            status = SMS_STATUS_CONFIRM_FAIL;
        }
        else    //report suc
        {
            status = SMS_STATUS_CONFIRM_SUCCESS;
        }
        pMsg->m_iStatus = status;
        g_CChannelThreadPool->smsTxPostMsg(pMsg);
    }
    else
    {
        LogError("deliverResp.Unpack is failed.");
    }

}
void SMPPChannelSend::respDeliver(SMPPBase& resp)
{

    smsp::SMPPDeliverReq req(resp);
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);

}
void SMPPChannelSend::onHeartbeat(pack::Reader& reader, SMPPBase& resp)
{
    if(resp._commandId==ENQUIRE_LINK)
    {
        ////LogDebug("***smpp this is ENQUIRE_LINK***");
        if(resp._commandLength > resp.bodySize())       //16 = head.length
        {   
            LogWarn("warn, ENQUIRE_LINK cmd.length[%d]>16", resp._commandLength);
            string strtmp;
            reader((UInt32)(resp._commandLength - resp.bodySize()), strtmp);
        }

        
        smsp::SMPPHeartbeatResp respHeartbeat;
        respHeartbeat._commandId = ENQUIRE_LINK_RESP;
        respHeartbeat._sequenceNum = resp._sequenceNum;
        if(NULL == m_pSocket){
            LogError("socket is null!!");
            return;
        }
        pack::Writer writer(m_pSocket->Out());
        respHeartbeat.Pack(writer);
    }
    else if(resp._commandId==ENQUIRE_LINK_RESP)
    {
        ////LogDebug("***smpp this is ENQUIRE_LINK_RESP***");
        m_uHeartbeatTick = 0;

        if(resp._commandLength > resp.bodySize())       //16 = head.length
        {   
            LogWarn("warn, ENQUIRE_LINK_RESP cmd.length[%d]>16", resp._commandLength);
            string strtmp;
            reader((UInt32)(resp._commandLength - resp.bodySize()), strtmp);
        }
    }


}

void SMPPChannelSend::sendHeartbeat()
{
    if (!isAlive())
    {
        return;
    }
    smsp::SMPPHeartbeatReq req;
    req._sequenceNum = m_SnMng.getDirectSequenceId();
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);
}

void SMPPChannelSend::onConnResp(pack::Reader& reader, SMPPBase& resp)
{
    smsp::SMPPConnectResp conResp(resp);
    if (conResp.Unpack(reader))
    {
        if (conResp.isLogin())
        {
            LogNotice("SMPP channelid[%d] Login success.", m_ChannelInfo.channelID);
            m_iState = CHAN_DATATRAN;
            m_bHasConn = true;
            int iloginStatus = conResp._isOK ? 1 : 0 ;
            InitFlowCtr();
            updateLoginState(m_uChannelLoginState, 1, Comm::int2str(iloginStatus));
            m_uChannelLoginState = 1;
            
            m_uContinueLoginFailedNum = 0 ;
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState   =  CHAN_DATATRAN;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
            m_pProcessThread->PostMsg(pmsg);
        }
        else
        {
            LogError("SMPP channelid[%d],status:%d, Login failed.", m_ChannelInfo.channelID,conResp._commandStatus);
            m_iState = CHAN_INIT;
            int iloginStatus = conResp._isOK?1:0;
            
            updateLoginState(m_uChannelLoginState, 2, Comm::int2str(iloginStatus));     
            m_uChannelLoginState = 2;
        }
    }
}

void SMPPChannelSend::conn()
{
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

    m_iState = CHAN_INIT;
    string ip = url.substr(0, pos);
    UInt16 port = atoi(url.substr(pos + 1).data());
    LogNotice("SMPPChannelSend::connect server [%s:%d] accout[%s]", ip.data(), port,m_ChannelInfo._clientID.data());
    Address addr(ip, port);
    m_pSocket->Connect(addr, 10000);
    smsp::SMPPConnectReq reqLogin;
    reqLogin._sequenceNum = m_SnMng.getDirectSequenceId();
    reqLogin.setAccout(m_ChannelInfo._clientID, m_ChannelInfo._password);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);
    m_uHeartbeatTick = 0;

    updateLoginState(m_uChannelLoginState, 0, "");
    m_uChannelLoginState = 0;
}

void SMPPChannelSend::reConn()
{
    if (m_bHasReconnTask == false)
    {
        m_bHasReconnTask = true;
    }
    else
    {
        return;
    }
    
    m_iState = CHAN_INIT;
    if(m_pSocket){
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
    m_bHasConn = false;
    m_uHeartbeatTick = 0;
}

bool SMPPChannelSend::onTransferData()
{
    SMPPBase resp;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return false;
    }
    pack::Reader reader(m_pSocket->In());
    if (resp.Unpack(reader))
    {
        //check length from head. 
        if(resp._commandLength > 5000) //add by fangjinxiong 20161117
        {
            //协议头中的 length过长。 断连接重连。
            LogError("resp._commandLength[%d]>5000, reconn. channelid[%d], channelName[%s]",
                        resp._commandLength, m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());

            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState = CHAN_RESET;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;
            m_pProcessThread->PostMsg(pmsg);
            reader.SetGood(false);
            return reader;
        }

        //check cmd  
        map<UInt32, HANDLER_FUN>::iterator it = _hander.find(resp._commandId);
        if (it != _hander.end())
        {   
            m_uHeartbeatTick = 0;   //add by fangjinxiong 20161111
            (this->*(*it).second)(reader, resp);
        }
        else if(resp._commandId == GENERIC_NACK
            || resp._commandId == BIND_RECEIVER || resp._commandId == BIND_RECEIVER_RESP
            || resp._commandId == BIND_TRANSMITTER || resp._commandId == BIND_TRANSMITTER_RESP
            || resp._commandId == QUERY_SM || resp._commandId == QUERY_SM_RESP
            || resp._commandId == SUBMIT_SM
            || resp._commandId == DELIVER_SM_RESP
            || resp._commandId == UNBIND || resp._commandId == UNBIND_RESP
            || resp._commandId == REPLACE_SM || resp._commandId == REPLACE_SM_RESP
            || resp._commandId == CANCEL_SM || resp._commandId == CANCEL_SM_RESP
            || resp._commandId == BIND_TRANSCEIVER
            || resp._commandId == OUTBIND
        )
        {
            //cmd we do not need
            char* buffer = new char[resp._commandLength- 16];
            reader(resp._commandLength- 16, buffer);
            delete[] buffer;
        }
        else
        {
            //cmd not belong to cmpp protocl
            LogError("resp.requestId_[%d] not belong to smpp, reconn. channelid[%d], channelName[%s]",
                     resp._commandId, m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
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

void SMPPChannelSend::OnEvent(int type, InternalSocket* socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        LogDebug("SMPPChannelSend::Connected");     
        m_iState = CHAN_INIT;
    }
    else if (type == EventType::Read)
    {
        m_uHeartbeatTick = 0;
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("SMPPChannelSend::Close.channelid[%d], channelName[%s]", 
                 m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_CLOSE;
        
        updateLoginState(m_uChannelLoginState, 2, "link is close");
        m_uChannelLoginState = 2;
    }
    else if (type == EventType::Error)
    {
        LogError("SMPPChannelSend::Error.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_EXCEPT;
        
        updateLoginState(m_uChannelLoginState, 2, "link is error");
        m_uChannelLoginState = 2;
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

void SMPPChannelSend::OnTimer()
{
    if (false == m_bHasConn)
    {
        conn();
        m_bHasReconnTask = false;
        continueLoginFailed(m_uContinueLoginFailedNum);
        m_uContinueLoginFailedNum++;
    }
    else if (true == m_bHasConn)
    {
        sendHeartbeat();
        if ( ++ m_uHeartbeatTick > 10 )
        {
            LogWarn("SMPPChannelSend heartbeat timeout,channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
            reConn();
        }
        else
        {
            FlowSliderResetCheck();
        }   
    }
    ///_packetDB keep 1min smsContent

    Int64 iNowTime = time(NULL);
    channelSessionMap::iterator iter = _packetDB.begin();
    for (;iter != _packetDB.end();)
    {
        
        directSendSesssion *session = iter->second ;
        if ((iNowTime - session->m_pSmsContext->m_lSubmitDate ) > ICHANNEL_SESSION_TIMEOUT_SECS )
        {
            session->m_pSmsContext->m_iStatus = SMS_STATUS_CONFIRM_FAIL;

            LogWarn("==smpp submitResp timeout==[%s:%s:%d]_packetDB size:%d.",
                session->m_pSmsContext->m_strSmsId.data(),
                session->m_pSmsContext->m_strPhone.data(),
                m_ChannelInfo.channelID, _packetDB.size());

            session->m_pSmsContext->m_strSubmit = SMPP_REPLY_TIME_OUT;
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

void SMPPChannelSend::onData()
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
            LogWarn("state[%d] is invalid.", m_iState);
            break;
        }
    }
}

