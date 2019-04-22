#include "SMGPChannel.h"
#include "UTFString.h"
#include "SMGPConnectReq.h"
#include "SMGPConnectResp.h"
#include "SMGPSubmitReq.h"
#include "SMGPSubmitResp.h"
#include "SMGPDeliverResp.h"
#include "SMGPDeliverReq.h"
#include "SMGPHeartbeatReq.h"
#include "SMGPHeartbeatResp.h"
#include <sstream>
#include <cwctype>
#include <algorithm>
#include "ErrorCode.h"
#include "main.h"
#include "Comm.h"

SMGPChannel::SMGPChannel() 
{
    m_pSocket = NULL;
    m_uHeartbeatTick = 0;
    m_bHasReconnTask = false;
    m_wheelTime = NULL;
}

SMGPChannel::~SMGPChannel()
{
}


UInt32 SMGPChannel::sendSms( smsDirectInfo &SmsInfo )
{
    if (  m_iState != CHAN_DATATRAN || NULL == m_pSocket )
    {
        UInt64 uSeq = m_SnMng.getDirectSequenceId();
        string strContent = "";
        saveSmsContent( &SmsInfo, strContent, 1, 1, uSeq, 1 );  
        LogError("==smgp error==[%s:%s:%d]connect is abnoramal", 
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

    if (false == SmsInfo.m_strSign.empty())
    {
        strSign = strLeft + SmsInfo.m_strSign + strRight;
    }
    
    SmsInfo.m_strSign = strSign;
    
    cnt = parseMsgContent(SmsInfo.m_strContent, contents, SmsInfo.m_strSign, isIncludeChinese);
    msgTotal = contents.size();
    
    ///long message
    if (cnt > 1 && msgTotal == 1)
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendSmgpLongMessage(&SmsInfo, contents,cnt,msgTotal);
    }
    else ///short message
    {
        SmsInfo.m_uLongSms = 0;
        SmsInfo.m_uChannelCnt = 1;
        sendSmgpShortMessage(&SmsInfo, contents,cnt,msgTotal, isIncludeChinese);
    }

    return CHAN_DATATRAN;
} 



void SMGPChannel::sendSmgpShortMessage( smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,
                        UInt32 uMsgTotal, bool isIncludeChinese)
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
   
        }

        sendContent = (*it);

        SMGPSubmitReq req;
        int len = 0;
        if(isIncludeChinese)
        {
            len = utfHelper.u2u16((*it), sendContent);
            req.msgFormat_ = 8;
        }
        else
        {   
            
            len = sendContent.length(); //utf-8 > ascII
            req.msgFormat_ = 0;
        }
        
        if (len > 140)
        {
            LogWarn("short message length is too long len[%d]",len);
        }
        
        req.sequenceId_ = m_SnMng.getDirectSequenceId();
        ////req.srcTermId_ = m_ChannelInfo._accessID;
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
        
        req.srcTermId_.append(strAppend);
        pSmsInfo->m_strShowPhone = req.srcTermId_;
        req.phone_ = pSmsInfo->m_strPhone;
        ///// 
        req.serviceId_ = m_ChannelInfo.m_strServerId;
        ////LogDebug("###serviceId[%s],strAppend[%s].",req.serviceId_.data(),strAppend.data());
        req.setContent((char*)sendContent.data(),len);
        
        pack::Writer writer(m_pSocket->Out());
        req.Pack(writer);
        
        LogNotice("==smgp submit==[%s:%s:%d]sequence:%d",
            pSmsInfo->m_strSmsId.data(),
            pSmsInfo->m_strPhone.data(),
            m_ChannelInfo.channelID,req.sequenceId_);
        saveSmsContent(pSmsInfo, (*it),uTotal, i,req.sequenceId_, uMsgTotal);

    }
}

void SMGPChannel::sendSmgpLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal)
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

        int len = utfHelper.u2u16((*it), sendContent);
        ////LogDebug("smgp long change len[%d], content[%s].", len,(*it).data());
        int MAXLENGTH = 134;
        //int size = ((len - len%MAXLENGTH)/MAXLENGTH) +1;
        int size = (len + MAXLENGTH -1)/MAXLENGTH;
        int index = 1;
        char* chtmp = new char[1024];

        ////int iSequence = 0;
        UInt32 uCand = getRandom();

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
                strtmp = sendContent.substr(index*MAXLENGTH - MAXLENGTH);
                strlen = len;
                len = 0;
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

            SMGPSubmitReq req;
            req.m_uFlag = 1;
            req.msgFormat_ = 8;
            req.sequenceId_ = m_SnMng.getDirectSequenceId();
            ////req.srcTermId_ = m_ChannelInfo._accessID;
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
            
            req.srcTermId_.append(strAppend);
            pSmsInfo->m_strShowPhone = req.srcTermId_;
            req.phone_ = pSmsInfo->m_strPhone;
            req.serviceId_ = m_ChannelInfo.m_strServerId;
            req.setContent(chtmp, strlen+6);
            pack::Writer writer(m_pSocket->Out());
            req.Pack(writer);
            
            LogNotice("==smgp submit==[%s:%s:%d]sequence:%d",
                  pSmsInfo->m_strSmsId.data(),
                  pSmsInfo->m_strPhone.data(),
                  m_ChannelInfo.channelID,req.sequenceId_);
            
            string strContent = "";
            utfHelper.u162u(strtmp,strContent);
            saveSmsContent(pSmsInfo, strContent, uTotal, index, req.sequenceId_, uMsgTotal);
            index++;
        }

        delete[] chtmp;
        chtmp = NULL;
    }
}

void SMGPChannel::bindHandler() 
{
    _hander[SMGP_LOGIN_RESP] = &SMGPChannel::onConnResp;
    _hander[SMGP_SUBMIT_RESP] = &SMGPChannel::onSubmitResp;
    _hander[SMGP_DELIVER] = &SMGPChannel::onDeliver;
    _hander[SMGP_ACTIVE_TEST_RESP] = &SMGPChannel::onHeartbeatResp;
    _hander[SMGP_ACTIVE_TEST]      = &SMGPChannel::onHeartbeatReq;
}

void SMGPChannel::onSubmitResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen) 
{       
    SMGPSubmitResp respSubmit;
    respSubmit.packetLength_ = packetLen; 
    if (respSubmit.Unpack(reader))
    {
        std::string retCode;
        char result[3] = {0};
        snprintf(result,3,"%d", respSubmit.status_);
        retCode = result;

        m_uHeartbeatTick = 0;

        channelSessionMap::iterator it = _packetDB.find( sequenceId );
        if ( it != _packetDB.end())
        {
            directSendSesssion *pSession = it->second;
            smsp::SmsContext   *pSms  = pSession->m_pSmsContext;
        
            LogNotice("==smgp submitResp== channelid:%d,sequence:%d,channelsmsid:%s,result:%s,smsid:%s,phone:%s.",
                    m_ChannelInfo.channelID,sequenceId,respSubmit.msgId_.data(),result,
                    pSms->m_strSmsId.data(),pSms->m_strPhone.data());
    
            pSms->m_strSubret = result;
            pSms->m_strChannelSmsId = respSubmit.msgId_;
            pSms->m_lSubretDate= time(NULL);

            
            if(retCode != "0")//submitResp failed
            {    
                pSms->m_iStatus = SMS_STATUS_SEND_FAIL;
            }
            else //submitResp suc
            {
                pSms->m_iStatus = SMS_STATUS_SEND_SUCCESS;
            }
            
            channelStatusReport( sequenceId );
            
        }
        else
        {
            LogWarn("channelid:%d,hasn't find Submit Packet,seq[%u], channelsmsid[%s]!", 
                     m_ChannelInfo.channelID, sequenceId, respSubmit.msgId_.data());
        }
    }
}


void SMGPChannel::onDeliver(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen) 
{
    SMGPDeliverReq req;
    req.packetLength_ = packetLen;  //add by fangjinxiong 20161117
    if (req.Unpack(reader)) 
    {   
        respDeliver(sequenceId, req._msgId);

        m_uHeartbeatTick = 0;

        if (1 == req._isReport)
        {
            LogElk("==smgp report== channelid:%d,channelsmsid[%s],status[%d],destphone[%s],srcPhone[%s],desc[%s].",
                m_ChannelInfo.channelID,
                req._msgId.data(),
                req._status,
                req._destTermId.data(),
                req._srcTermId.data(),
                req.m_strErrorCode.data());

            TDirectToDeliveryReportReqMsg* pMsg = new TDirectToDeliveryReportReqMsg();
            if (NULL == pMsg)
            {
                LogError("pMsg is NULL.");
                return;
            }

            UInt32 status = req._status;
            if( status != 0 )   //report failed
            {
                status = SMS_STATUS_CONFIRM_FAIL;
            }
            else    //report suc
            {
                status = SMS_STATUS_CONFIRM_SUCCESS;
            }
            pMsg->m_strDesc = req.m_strErrorCode;
            pMsg->m_strErrorCode = req.m_strErrorCode;
            pMsg->m_uChannelId = m_ChannelInfo.channelID;
            pMsg->m_uChannelIdentify = m_ChannelInfo.m_uChannelIdentify;
            pMsg->m_iStatus = status;
            pMsg->m_strChannelSmsId = req._msgId;
            pMsg->m_lReportTime = time(NULL);
            pMsg->m_iMsgType = MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ;
            pMsg->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (m_ChannelInfo.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
            pMsg->m_strPhone = req._srcTermId.data();
            pMsg->m_iHttpMode = DX_SMGP;
            g_CChannelThreadPool->smsTxPostMsg( pMsg );

        }
        else if (0 == req._isReport)
        {
            string strContent = "";
            ContentCoding(req._msgContent,req._msgFmt,strContent);
            LogElk("==smgp mo== destphone[%s],srcPhone[%s],content[%s].",
                    req._destTermId.data(),req._srcTermId.data(),strContent.data());
            TToDirectMoReqMsg* pMo = new TToDirectMoReqMsg();
            pMo->m_iMsgType = MSGTYPE_TO_DIRECTMO_REQ;
            pMo->m_strContent = strContent;
            pMo->m_strPhone = req._srcTermId.data();
            pMo->m_strDstSp = req._destTermId.data();
            pMo->m_strSp = m_ChannelInfo._accessID.data();
            pMo->m_uChannelId = m_ChannelInfo.channelID;
            pMo->m_uMoType = DX_SMGP;
            pMo->m_uChannelType = m_ChannelInfo.m_uChannelType;
            pMo->m_uChannelMoId = strtoul(req._msgId.data(),0,0);   
            g_pDirectMoThread->PostMsg(pMo);
        }
        else
        {
            LogError("this is invalid flag[%d].",req._isReport);
        }
    }
}

void SMGPChannel::respDeliver(UInt32 seq, string msgId) 
{
    SMGPDeliverResp resp;
    resp.sequenceId_ = seq;
    resp._msgId = msgId;
    resp._result = 0;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    resp.Pack(writer);
}

void SMGPChannel::onHeartbeatResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    //////LogDebug("Active_heart_resp  sequenceId[%ld].",sequenceId);
    m_uHeartbeatTick = 0;

    if(packetLen > 12)      //12 = head.length
    {   
        LogWarn("warn, HeartbeatResp.length[%d]>12", packetLen);
        string strtmp;
        reader((UInt32)(packetLen-12), strtmp);
    }
}

void SMGPChannel::onHeartbeatReq(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    //////LogDebug("Active_heart_req  sequenceId[%ld].",sequenceId);

    if(packetLen > 12)      //12 = head.length
    {   
        LogWarn("warn, HeartbeatReq.length[%d]>12", packetLen);
        string strtmp;
        reader((UInt32)(packetLen-12), strtmp);
    }

    
    SMGPHeartbeatResp resp;
    resp.sequenceId_ = sequenceId;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    resp.Pack(writer);
}

void SMGPChannel::sendHeartbeat() 
{
    if (!isAlive())
    {
        return;
    }
    SMGPHeartbeatReq req;
    req.sequenceId_ = m_SnMng.getDirectSequenceId();
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);
}

void SMGPChannel::onConnResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    SMGPConnectResp resp;
    resp.packetLength_ = packetLen;  //add by fangjinxiong 20161117
    if (resp.Unpack(reader)) 
    {
        if (resp.isLogin())
        {
            LogNotice("SMGP channelid[%d] Login success.", m_ChannelInfo.channelID);
            m_iState = CHAN_DATATRAN;
            m_bHasConn = true;

            m_uContinueLoginFailedNum = 0;
            InitFlowCtr();
            updateLoginState(m_uChannelLoginState, 1, Comm::int2str(resp.status_));     
            m_uChannelLoginState = 1;
            
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState   =  CHAN_DATATRAN;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
            m_pProcessThread->PostMsg(pmsg);
            
        }
        else
        {
            LogNotice("SMGP channelid[%d],status:%d, Login failed.", m_ChannelInfo.channelID,resp.status_);
            m_iState = CHAN_INIT;
            
            updateLoginState(m_uChannelLoginState, 2, Comm::int2str(resp.status_));     
            m_uChannelLoginState = 2;
        }
    }
}

void SMGPChannel::init(models::Channel& chan)
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

void SMGPChannel::conn()
{
    if (NULL != m_pSocket)
    {
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
    
    m_pSocket = new InternalSocket();
    if(NULL == m_pSocket){
        LogError("mem error");
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
    LogNotice("SMGPChannel::connect server [%s:%d] accout[%s]", ip.data(), port,m_ChannelInfo._clientID.data());
    Address addr(ip, port);
    m_pSocket->Connect(addr, 10000);
    SMGPConnectReq reqLogin;
    reqLogin.sequenceId_ = m_SnMng.getDirectSequenceId();
    m_iState = CHAN_INIT;
    reqLogin.setAccout(m_ChannelInfo._clientID, m_ChannelInfo._password);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);
    m_uHeartbeatTick = 0;

    updateLoginState(m_uChannelLoginState, 0, "");
    m_uChannelLoginState = 0;
}

void SMGPChannel::reConn()
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

void SMGPChannel::destroy()
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
    for (; iter != _packetDB.end(); ++iter)
    {
        directSendSesssion *session = iter->second;
        
        session->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        session->m_pSmsContext->m_strSubmit = SMGP_INTERNAL_ERROR;
        session->m_pSmsContext->m_strYZXErrCode = SMGP_INTERNAL_ERROR;
        channelStatusReport( iter->first, false );
        SAFE_DELETE( iter->second );
        _packetDB.erase(iter);
    }

    delete this;
}

bool SMGPChannel::onTransferData() 
{
    SMGPBase resp;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return false;
    }
    pack::Reader reader(m_pSocket->In());

    if(reader.GetSize() < 12)
    {
        reader.SetGood(false);
        return reader;
    }
    
    if (resp.Unpack(reader))
    {
        if(reader.GetSize() < resp.packetLength_ - 12)
        {
            reader.SetGood(false);
            return reader;
        }
        
        //////LogDebug("receive totallength[%d] commandid:0x[%x],sequenceid[%u].",resp.packetLength_,resp.requestId_,resp.sequenceId_);

        //check length from head. 
        if(resp.packetLength_ > 5000) //add by fangjinxiong 20161117
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
            m_uHeartbeatTick = 0;   //add by fangjinxiong 20161111
            (this->*(*it).second)(reader, resp.sequenceId_, resp.packetLength_);//add by fangjinxiong 20161117
        }
        else if(resp.requestId_ == SMGP_LOGIN
            || resp.requestId_ == SMGP_SUBMIT
            || resp.requestId_ == SMGP_DELIVER_RESP
            || resp.requestId_ == SMGP_TERMINATE || resp.requestId_ == SMGP_TERMINATE_RESP
            || resp.requestId_ == SMGP_QUERY || resp.requestId_ == SMGP_QUERY_RESP
            || resp.requestId_ == Forward || resp.requestId_ == Forward_Resp
            || resp.requestId_ == Query_TE_Route || resp.requestId_ == Query_TE_Route_Resp
            || resp.requestId_ == Query_SP_Route || resp.requestId_ == Query_SP_Route_Resp
            || resp.requestId_ == Payment_Request || resp.requestId_ == Payment_Request_Resp
            || resp.requestId_ == Payment_Affirm || resp.requestId_ == Payment_Affirm_Resp
            || resp.requestId_ == Query_UserState || resp.requestId_ == Query_UserState_Resp
            || resp.requestId_ == Get_All_TE_Route || resp.requestId_ == Get_All_TE_Route_Resp
            || resp.requestId_ == Get_All_SP_Route || resp.requestId_ == Get_All_SP_Route_Resp
            || resp.requestId_ == Update_TE_Route || resp.requestId_ == Update_TE_Route_Resp
            || resp.requestId_ == Update_SP_Route || resp.requestId_ == Update_SP_Route_Resp
            || resp.requestId_ == Push_Update_TE_Route || resp.requestId_ == Push_Update_TE_Route_Resp
            || resp.requestId_ == Push_Update_SP_Route || resp.requestId_ == Push_Update_SP_Route_Resp)
        {
            //cmd we do not need
            char* buffer = new char[resp.packetLength_ - 12];
            reader(resp.packetLength_ - 12, buffer);
            delete[] buffer;
        }
        else
        {
            //cmd not belong to smgp Protocal
            LogError("resp.requestId_[%d] not belong to smgp, reconn. channelid[%d], channelName[%s]",
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

void SMGPChannel::OnEvent(int type, InternalSocket* socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        m_iState = CHAN_INIT;
        LogDebug("SMGPChannel::Connected");
    }
    else if (type == EventType::Read)
    {
        m_uHeartbeatTick = 0;
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("SMGPChannel::Close.channelid[%d], channelName[%s]", 
                m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_CLOSE;
        
        updateLoginState(m_uChannelLoginState, 2, "link is close");
        m_uChannelLoginState = 2;
    }
    else if (type == EventType::Error)
    {
        LogError("SMGPChannel::Error.channelid[%d], channelName[%s]", 
                m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
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

void SMGPChannel::OnTimer()
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
        if ( ++m_uHeartbeatTick > 10)       //modify 3 to 10, fangjinxiong20161111
        {
            LogWarn("SMGPChannel heartbeat timeout,channelid[%d], channelName[%s]", 
                    m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
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
        if (( iNowTime - session->m_pSmsContext->m_lSubmitDate) > ICHANNEL_SESSION_TIMEOUT_SECS )
        {
            session->m_pSmsContext->m_iStatus = SMS_STATUS_CONFIRM_FAIL;
            LogWarn("==smgp submitResp timeout==[%s:%s:%d]_packetDB size:%d",
                    session->m_pSmsContext->m_strSmsId.data(),
                    session->m_pSmsContext->m_strPhone.data(),
                    m_ChannelInfo.channelID,_packetDB.size());
            
            session->m_pSmsContext->m_strSubmit = SMGP_REPLY_TIME_OUT;
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

void SMGPChannel::onData()
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
