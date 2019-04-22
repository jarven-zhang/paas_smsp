#include "CMPP3Channel.h"
#include "UTFString.h"
#include "CMPP3ConnectReq.h"
#include "CMPP3ConnectResp.h"
#include "CMPP3SubmitReq.h"
#include "CMPP3SubmitResp.h"
#include "CMPP3DeliverResp.h"
#include "CMPP3DeliverReq.h"
#include "CMPP3HeartbeatReq.h"
#include "CMPP3HeartbeatResp.h"
#include "CMPP3QueryReq.h"
#include "CMPP3QueryResp.h"

#include <sstream>
#include <cwctype>
#include <algorithm>
#include "ErrorCode.h"
#include "main.h"
#include "Comm.h"


CMPP3Channel::CMPP3Channel()
{
    m_pSocket = NULL;
    m_uHeartbeatTick = 0;
    m_bHasReconnTask = false;
    m_wheelTime = NULL;
    
}

CMPP3Channel::~CMPP3Channel()
{
}

UInt32 CMPP3Channel::sendSms( smsDirectInfo &SmsInfo )
{

    if ( m_iState != CHAN_DATATRAN || NULL == m_pSocket )
    {       
        UInt32 uSeq = m_SnMng.getDirectSequenceId();
        string strContent = "";
        saveSmsContent(&SmsInfo, strContent, 1, 1, uSeq, 1 );   
        channelStatusReport( uSeq );
        LogError("==cmpp3.0 error==[%s:%s:%d]connect is abnoramal.", 
                 SmsInfo.m_strSmsId.data(), SmsInfo.m_strPhone.data(), m_ChannelInfo.channelID);
        return CHAN_CLOSE;
    }

    std::vector<string> contents;

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
    cnt = parseMsgContent( SmsInfo.m_strContent, contents, SmsInfo.m_strSign, isIncludeChinese );
    msgTotal = contents.size();

    if ( cnt > 1 && msgTotal == 1 )
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendCmppLongMessage(&SmsInfo,contents,cnt,msgTotal);
    }
    else ///short message
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendCmppShortMessage(&SmsInfo,contents,cnt,msgTotal, isIncludeChinese);
    }
    
    return CHAN_DATATRAN;
    
}

void CMPP3Channel::sendCmppShortMessage( smsDirectInfo *pSmsInfo, vector<string>& vecContent, 
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
        
        cmpp3::CMPPSubmitReq req;
        
        int len = 0;
        if(isIncludeChinese)
        {
            len = utfHelper.u2u16((*it), sendContent);
            req._msgFmt = 8;
        }
        else
        {   
            
            len = sendContent.length(); //utf-8 > ascII
            req._msgFmt = 0;
        }
            
        if (len > 140)
        {
            LogWarn("short message length is too long len[%d]",len);
        }

        req._sequenceId = m_SnMng.getDirectSequenceId();        
        ////req._srcId = m_ChannelInfo._accessID;
        //////extro no
        string strAppend = "";
        if(CHAN_GEGULAR_SIGN_NO_EXTEND_PORT_TYPE == m_ChannelInfo.m_uChannelType)
        {
            strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort;
        }
        else if(CHAN_GEGULAR_SIGN_EXTEND_PORT_TYPE == m_ChannelInfo.m_uChannelType)
        {
            strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strDisplayNum;
        }
        else
        {
            strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;
        }

        UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
        if (uPortLength > 21)
        {
            uPortLength = 21;
        }
        if (strAppend.length() > uPortLength)
        {
            strAppend = strAppend.substr(0,uPortLength);
        }
        
        req._srcId.append(strAppend);
        pSmsInfo->m_strShowPhone= req._srcId;
        req._msgSrc = m_ChannelInfo._clientID;
        req._phone = pSmsInfo->m_strPhone;
        req._serviceId = m_ChannelInfo.m_strServerId;
        req.setContent((char*)sendContent.data(),len);
        pack::Writer writer(m_pSocket->Out());
        req.Pack(writer);

        LogNotice("==cmpp submit== [%s:%s:%d],sequence:%d",
                  pSmsInfo->m_strSmsId.data(),
                  pSmsInfo->m_strPhone.data(),
                  m_ChannelInfo.channelID,
                  req._sequenceId);

        saveSmsContent(pSmsInfo, (*it), uTotal, i, req._sequenceId , uMsgTotal );

    }
}


void CMPP3Channel::sendCmppLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent, UInt32 uTotal,UInt32 uMsgTotal )
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
        ////LogDebug("c long change len[%d], content[%s].", len,(*it).data());
        int MAXLENGTH = 134;
        //int size = ((len - len%MAXLENGTH)/MAXLENGTH) +1;
        int size = (len + MAXLENGTH -1)/MAXLENGTH;
        int index = 1;
        char* chtmp = new char[1024];

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

            cmpp3::CMPPSubmitReq req;
            req._sequenceId = m_SnMng.getDirectSequenceId();
            req._tpUdhi = 0x01;
            ////req._srcId = m_ChannelInfo._accessID;
            //////extro no
            string strAppend = "";
          
            if(CHAN_GEGULAR_SIGN_NO_EXTEND_PORT_TYPE == m_ChannelInfo.m_uChannelType)
            {
                strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort;
            }
            else if(CHAN_GEGULAR_SIGN_EXTEND_PORT_TYPE == m_ChannelInfo.m_uChannelType)
            {
                strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strDisplayNum;
            }
            else
            {
                strAppend = m_ChannelInfo._accessID + pSmsInfo->m_strUcpaasPort + pSmsInfo->m_strSignPort + pSmsInfo->m_strDisplayNum;
            }
            UInt32 uPortLength = m_ChannelInfo._accessID.length() + m_ChannelInfo.m_uExtendSize;
            if (uPortLength > 21)
            {
                uPortLength = 21;
            }
            if (strAppend.length() > uPortLength)
            {
                strAppend = strAppend.substr(0,uPortLength);
            }
            
            req._srcId.append(strAppend);
            pSmsInfo->m_strShowPhone = req._srcId;
            req._msgSrc = m_ChannelInfo._clientID;
            req._phone = pSmsInfo->m_strPhone; 
            req._pkTotal = size;
            req._pkNumber = index;
            ////Service_Id
            req._serviceId = m_ChannelInfo.m_strServerId;
            ////LogDebug("###serviceId[%s],strAppend[%s].",req._serviceId.data(),strAppend.data());
            req.setContent(chtmp, strlen+6);
            pack::Writer writer(m_pSocket->Out());
            req.Pack(writer);

            LogNotice("==cmpp submit==[%s:%s:%d]uRand[%u],sequence:%u",
                pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strPhone.data(), m_ChannelInfo.channelID, 
                uCand, req._sequenceId);

            string strContent = "";
            utfHelper.u162u(strtmp,strContent);
            saveSmsContent(pSmsInfo, strContent, uTotal, index, req._sequenceId, uMsgTotal);
            index++;
        }

        delete[] chtmp;
        chtmp = NULL;
    }
}

void CMPP3Channel::bindHandler()
{
    _hander[CMPP_CONNECT_RESP] = &CMPP3Channel::onConnResp;
    _hander[CMPP_SUBMIT_RESP] = &CMPP3Channel::onSubmitResp;
    _hander[CMPP_DELIVER] = &CMPP3Channel::onDeliver;
    _hander[CMPP_ACTIVE_TEST_RESP] = &CMPP3Channel::onHeartbeat;
    _hander[CMPP_ACTIVE_TEST] = &CMPP3Channel::onHeartbeatReq;
}

void CMPP3Channel::onHeartbeatReq(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    /////LogDebug("enter cmpp onHeartbeatReq");
    m_uHeartbeatTick = 0;

    if(packetLen > 12)      //12 = head.length
    {   
        LogWarn("warn, HeartbeatReq.length[%d]>12", packetLen);
        string strtmp;
        reader((UInt32)(packetLen-12), strtmp);
    }
    
    cmpp3::CMPPHeartbeatResp respHeartbeat;
    respHeartbeat._sequenceId = sequenceId;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    respHeartbeat.Pack(writer);
}

void CMPP3Channel::onSubmitResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    cmpp3::CMPPSubmitResp respSubmit;
    respSubmit._totalLength = packetLen; 
    if (respSubmit.Unpack(reader))
    {
        std::string retCode = Comm::int2str(respSubmit._result);
        LogDebug("result = %s",retCode.data());

        channelSessionMap::iterator it = _packetDB.find(sequenceId);
        if (it != _packetDB.end())
        {
            directSendSesssion *pSession = it->second;
            smsp::SmsContext   *pSms  = pSession->m_pSmsContext;
        
            LogNotice("==cmpp submitResp== [%s:%s:%d]sequence:%d,channelsmsid:%lu,result:%s",
                pSms->m_strSmsId.data(), 
                pSms->m_strPhone.data(),
                m_ChannelInfo.channelID,
                sequenceId,
                respSubmit._msgID,
                retCode.data());

            pSms->m_strSubret = retCode;
            pSms->m_strChannelSmsId = Comm::int2str(respSubmit._msgID);
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
            channelStatusReport( sequenceId );
            
        }
        else
        {
            LogWarn("channelid:%d,hasn't find Submit Packet,seq[%u], channelsmsid[%lu]!", 
                    m_ChannelInfo.channelID, sequenceId, respSubmit._msgID);
        }

    }
}


void CMPP3Channel::onDeliver(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    cmpp3::CMPPDeliverReq req;
    req._totalLength = packetLen;
    if (req.Unpack(reader))
    {   
        if (0 == req._registeredDelivery && 1 == req._tpUdhi && req._pkNumber > CMPPMO_MAX_NUM)
        {
            respDeliver(sequenceId, req._msgId, CMPP_ERROR_OVER_MAX_MSGLEN);
            LogError("===cmpp== channelid[%d] get long mo over 10,destphone[%s] srcPhone[%s] randcode[%u],pknum[%u],total[%u]",
                m_ChannelInfo.channelID,req._destId.data(),req._srcTerminalId.data(),req._randCode,req._pkNumber,req._pkTotal);
            return;
        }
        else
        {
            respDeliver(sequenceId, req._msgId, CMPP_SUCCESS);
        }
        
        if (1 == req._registeredDelivery)
        {
            LogElk("==cmpp3.0 report== channelid:%d,channelsmsid[%lu],status[%d],destphone[%s],srcPhone[%s].",
                      m_ChannelInfo.channelID,req._msgId,
                      req._status,req._destId.data(),
                      req._srcTerminalId.data());

            TDirectToDeliveryReportReqMsg* pMsg = new TDirectToDeliveryReportReqMsg();
            if (NULL == pMsg)
            {
                LogError("pMsg is NULL.");
                return;
            }

            UInt32 status = req._status;
            if( status != 0 )       //report failed
            {
                status = SMS_STATUS_CONFIRM_FAIL;
            }
            else    //report suc
            {
                status = SMS_STATUS_CONFIRM_SUCCESS;
            }
            pMsg->m_strDesc = req._remark;
            pMsg->m_strErrorCode = req._remark;
            pMsg->m_uChannelId = m_ChannelInfo.channelID;
            pMsg->m_uChannelIdentify = m_ChannelInfo.m_uChannelIdentify;
            pMsg->m_iStatus = status;
            pMsg->m_strChannelSmsId = Comm::int2str( req._msgId );
            pMsg->m_lReportTime = time(NULL);
            pMsg->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (m_ChannelInfo.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
            pMsg->m_iMsgType = MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ;
            pMsg->m_strPhone = req._srcTerminalId.data();
            pMsg->m_iHttpMode = YD_CMPP3;
            g_CChannelThreadPool->smsTxPostMsg(pMsg);
            
        }
        else if (0 == req._registeredDelivery)
        {
            if(NULL == g_pDirectMoThread)
            {
                LogWarn("g_pDirectMoThread is not ready!!");
                return;
            }
            string strContent = "";
            ContentCoding(req._msgContent,req._msgFmt,strContent);
            TToDirectMoReqMsg* pMo = new TToDirectMoReqMsg();
            if(NULL == pMo)
            {
                LogError("new fail!!!");
                return;
            }
            if(1 == req._tpUdhi)
            {//has head
                pMo->m_bHead = true;
                pMo->m_uRandCode = req._randCode;
                pMo->m_uPkNumber = req._pkNumber;
                pMo->m_uPkTotal = req._pkTotal;
                LogNotice("===cmpp3.0== channelid[%d] get long mo destphone[%s] srcPhone[%s] randcode[%u],pknum[%u],total[%u]",
                    m_ChannelInfo.channelID,req._destId.data(),req._srcTerminalId.data(),req._randCode,req._pkNumber,req._pkTotal);
            }
            LogElk("==cmpp== mo destphone[%s],srcPhone[%s],content[%s],ChannelType[%u].", 
                     req._destId.data(),req._srcTerminalId.data(),strContent.data(), m_ChannelInfo.m_uChannelType);
            if (pMo->m_uPkTotal > CMPPMO_MAX_NUM)
            {
                pMo->m_uPkTotal = CMPPMO_MAX_NUM;
            }
            pMo->m_iMsgType = MSGTYPE_TO_DIRECTMO_REQ;
            pMo->m_strContent = strContent;
            pMo->m_strPhone = req._srcTerminalId.data();
            pMo->m_strDstSp = req._destId.data();
            pMo->m_strSp = m_ChannelInfo._accessID.data();
            pMo->m_uChannelId = m_ChannelInfo.channelID;
            pMo->m_uMoType = YD_CMPP3;
            pMo->m_uChannelType = m_ChannelInfo.m_uChannelType;
            g_pDirectMoThread->PostMsg(pMo);
        }
        else
        {
            LogError("this is invalid flag[%d].",req._registeredDelivery);
        }
    }
    else
    {
        LogError("cmpp onDeliver is failed.");
    }
}
void CMPP3Channel::respDeliver(UInt32 seq, UInt64 msgId, int result)
{
    cmpp3::CMPPDeliverResp resp;
    resp._sequenceId = seq;
    resp._msgId = msgId;
    resp._result = result;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    resp.Pack(writer);

}
void CMPP3Channel::onHeartbeat(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    ////LogDebug("***cmpp_active_test_resp sequenceid[%u]***",sequenceId);
    m_uHeartbeatTick = 0;
    cmpp3::CMPPHeartbeatResp respHeartbeat;
    respHeartbeat._totalLength = packetLen; 
    if (respHeartbeat.Unpack(reader))
    {

    }
}

void CMPP3Channel::sendHeartbeat()
{
    if (!isAlive())
    {
        return;
    }

    cmpp3::CMPPHeartbeatReq req;
    req._sequenceId = m_SnMng.getDirectSequenceId();
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);
}

void CMPP3Channel::onConnResp( pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen )
{
    cmpp3::CMPPConnectResp resp;
    resp._totalLength = packetLen; 
    if (resp.Unpack(reader))
    {
        //if (resp.isLogin() && 0x30 == resp._version)
        if ( resp.isLogin() )
        {
            LogNotice("CMPP channelid[%d] Login success.ver:0x%x", m_ChannelInfo.channelID,resp._version);
            m_iState = CHAN_DATATRAN;
            m_bHasConn = true;  
            InitFlowCtr();
            updateLoginState(m_uChannelLoginState, 1, Comm::int2str(resp._status));
            m_uChannelLoginState = 1;
            
            m_uContinueLoginFailedNum = 0;
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState   =  CHAN_DATATRAN;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS; 
            m_pProcessThread->PostMsg(pmsg);
        }
        else
        {
            LogError("CMPP3.0 channelid[%d],rev ver[0x%x],status:%d, Login failed.", m_ChannelInfo.channelID,resp._version,resp._status);
            m_iState = CHAN_UNLOGIN;
            
            updateLoginState(m_uChannelLoginState, 2, Comm::int2str(resp._status)); 
            m_uChannelLoginState = 2;
        }   
    }
}

void CMPP3Channel::init(models::Channel& chan)
{   
    IChannel::init(chan);
    
    bindHandler();
    m_ChannelInfo = chan;       //has sliderWindow source int. will not change

    /* 设置通道滑窗大小*/
    //m_flowCtr.iWindowSize = chan.m_uSliderWindow;
    //m_flowCtr.lLastRspTime = time( NULL );
    
    if (m_wheelTime != NULL)
    {
        m_wheelTime->Destroy();
        m_wheelTime = NULL;
    }
    m_iState = CHAN_INIT;
}

void CMPP3Channel::conn()
{
    if (NULL != m_pSocket)
    {
        m_pSocket->Close();
        delete m_pSocket;
        m_pSocket = NULL;
    }
    
    m_pSocket = new InternalSocket();
    if(NULL == m_pSocket){
        LogError("new socket fail!!");
        return;
    }

    CDirectSendThread *drectThread =  static_cast< CDirectSendThread *>( m_pProcessThread );

    m_pSocket->Init(drectThread->m_pInternalService, drectThread->m_pLinkedBlockPool);

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
    LogNotice("CMPP3Channel::connect server [%s:%d] accout[%s], channelid[%d]",
        ip.data(), port,m_ChannelInfo._clientID.data(), m_ChannelInfo.channelID);
    Address addr(ip, port);
    m_pSocket->Connect(addr, 10000);
    cmpp3::CMPPConnectReq reqLogin;
    reqLogin._sequenceId = m_SnMng.getDirectSequenceId();
    m_iState = CHAN_INIT;
    reqLogin.setAccout(m_ChannelInfo._clientID, m_ChannelInfo._password);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);
    m_uHeartbeatTick = 0;

    updateLoginState(m_uChannelLoginState, 0, "channel connectting");   
    m_uChannelLoginState = 0;
}

void CMPP3Channel::reConn()
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

void CMPP3Channel::destroy()
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
        session->m_pSmsContext->m_strSubmit = CMPP_INTERNAL_ERROR;
        session->m_pSmsContext->m_strYZXErrCode = CMPP_INTERNAL_ERROR   ;        
        channelStatusReport( iter->first, false );
        SAFE_DELETE( iter->second );
        _packetDB.erase(iter);
    }

    delete this;

}

bool CMPP3Channel::onTransferData()
{
    cmpp3::CMPPBase resp;
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
        if(reader.GetSize() < resp._totalLength - 12)
        {
            reader.SetGood(false);
            return reader;
        }
        
        /////LogDebug("receive totallength[%d] commandid:0x[%x],sequenceid[%u].",resp._totalLength,resp._commandId,resp._sequenceId);

        //check length from head. 
        if(resp._totalLength > 5000) //add by fangjinxiong 20161117
        {
            //协议头中的 length过长。 断连接重连。
            LogError("resp.packetLength_[%d]>5000, reconn. channelid[%d], channelName[%s]", 
                    resp._totalLength, m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
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
            //cmd we need
            m_uHeartbeatTick = 0;   //add by fangjinxiong 20161111 noneed
            (this->*(*it).second)(reader, resp._sequenceId, resp._totalLength);
        }
        else if( resp._commandId == CMPP_CONNECT 
                || resp._commandId == CMPP_TERMINATE || resp._commandId == CMPP_TERMINATE_RESP
                || resp._commandId == CMPP_SUBMIT 
                || resp._commandId == CMPP_QUERY || resp._commandId == CMPP_QUERY_RESP
                || resp._commandId == CMPP_DELIVER_RESP
                || resp._commandId == CMPP_CANCEL || resp._commandId == CMPP_CANCEL_RESP
                || resp._commandId == CMPP_FWD || resp._commandId == CMPP_FWD_RESP
                || resp._commandId == CMPP_MT_ROUTE || resp._commandId == CMPP_MT_ROUTE_RESP
                || resp._commandId == CMPP_MO_ROUTE || resp._commandId == CMPP_MO_ROUTE_RESP
                || resp._commandId == CMPP_GET_ROUTE || resp._commandId == CMPP_GET_ROUTE_RESP
                || resp._commandId == CMPP_MT_ROUTE_UPDATE || resp._commandId == CMPP_MT_ROUTE_UPDATE_RESP
                || resp._commandId == CMPP_MO_ROUTE_UPDATE || resp._commandId == CMPP_MO_ROUTE_UPDATE_RESP
                || resp._commandId == CMPP_PUSH_MT_ROUTE_UPDATE || resp._commandId == CMPP_PUSH_MT_ROUTE_UPDATE_RESP
                || resp._commandId == CMPP_PUSH_MO_ROUTE_UPDATE || resp._commandId == CMPP_PUSH_MO_ROUTE_UPDATE_RESP
                || resp._commandId == CMPP_PUSH_MO_ROUTE_UPDATE_RESP || resp._commandId == CMPP_GET_MO_ROUTE_RESP
                )
        {
            //cmd we do not need
            char* buffer = new char[resp._totalLength - 12];
            reader(resp._totalLength - 12, buffer);
            delete[] buffer;
        }
        else
        {
            //cmd not belong to cmpp protocl
            LogError("resp.requestId_[%d] not belong to cmpp, reconn. channelid[%d], channelName[%s]",
                     resp._commandId, m_ChannelInfo.channelID, 
                     m_ChannelInfo.channelname.data());
            
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

void CMPP3Channel::OnEvent(int type, InternalSocket* socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        LogDebug("CMPP3Channel::Connected");
        m_iState = CHAN_INIT;
    }
    else if (type == EventType::Read)
    {
        m_uHeartbeatTick = 0;
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("CMPP3Channel::Close.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
        m_iState = CHAN_CLOSE;
        
        updateLoginState(m_uChannelLoginState, 2, "link is close");
        m_uChannelLoginState = 2;
    }
    else if (type == EventType::Error)
    {
        LogError("CMPP3Channel::Error.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
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

void CMPP3Channel::OnTimer()
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
            LogWarn("CMPP3Channel heartbeat timeout,channelid[%d], channelName[%s]",
                     m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
            reConn();
        }
        else
        {           
            FlowSliderResetCheck();
        }       
    }

    Int64 iNowTime = time(NULL);
    channelSessionMap::iterator iter = _packetDB.begin();
    for (;iter != _packetDB.end();)
    {
        directSendSesssion *session = iter->second ;
        if (( iNowTime - session->m_pSmsContext->m_lSubmitDate) > ICHANNEL_SESSION_TIMEOUT_SECS)
        {   
            LogWarn("==cmpp submitResp timeout== channelid:%d,smsid:%s,phone:%s,_packetDB size:%d.",
                    m_ChannelInfo.channelID,session->m_pSmsContext->m_strSmsId.data(),
                    session->m_pSmsContext->m_strPhone.data(), _packetDB.size());
            
            session->m_pSmsContext->m_strSubmit = CMPP_REPLY_TIME_OUT;
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

void CMPP3Channel::onData()
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



