#include "CMPPProvinceChannel.h"
#include "UTFString.h"
#include "CMPPConnectReq.h"
#include "CMPPConnectResp.h"
#include "CMPPProvinceSubmitReq.h"
#include "CMPPSubmitResp.h"
#include "CMPPDeliverResp.h"
#include "CMPPDeliverReq.h"
#include "CMPPHeartbeatReq.h"
#include "CMPPHeartbeatResp.h"
#include "CMPPQueryReq.h"
#include "CMPPQueryResp.h"

#include <sstream>
#include <cwctype>
#include <algorithm>
#include "ErrorCode.h"
#include "main.h"
#include "Comm.h"


CMPPProvinceChannel::CMPPProvinceChannel()
{
    m_pSocket = NULL;
    m_uHeartbeatTick = 0;
    m_bHasReconnTask = false;
    m_wheelTime = NULL;
    
}

CMPPProvinceChannel::~CMPPProvinceChannel()
{
}

UInt32 CMPPProvinceChannel::sendSms( smsDirectInfo &SmsInfo  )
{
    if (m_iState != CHAN_DATATRAN || NULL == m_pSocket )
    {
        UInt32 uSeq = m_SnMng.getDirectSequenceId();
        string strContent = "";
        saveSmsContent( &SmsInfo, strContent, 1, 1, uSeq, 1 );      
        LogError("==cmpp province error==[%s:%s:%d]connect is abnoramal.",
            SmsInfo.m_strSmsId.data(),SmsInfo.m_strPhone.data(),m_ChannelInfo.channelID);
        channelStatusReport( uSeq );
        return CHAN_CLOSE;
    }

    std::vector<string> contents;

    bool isIncludeChinese = Comm::IncludeChinese((char*)(SmsInfo.m_strSign + SmsInfo.m_strContent).data());

    string strSign = "";
    string strLeft = SIGN_BRACKET_LEFT( isIncludeChinese );
    string strRight = SIGN_BRACKET_RIGHT( isIncludeChinese );
    
    if (false == SmsInfo.m_strSign.empty())
    {
        strSign = strLeft + SmsInfo.m_strSign + strRight;
    }
    SmsInfo.m_strSign = strSign;
    

    UInt32 cnt =1;
    int msgTotal = 1;
    cnt = parseMsgContent( SmsInfo.m_strContent, contents, SmsInfo.m_strSign, isIncludeChinese);
    msgTotal = contents.size();

    ///long message
    if ( cnt > 1 && msgTotal == 1 )
    {
        SmsInfo.m_uLongSms = 1;
        SmsInfo.m_uChannelCnt = cnt;
        sendCmppLongMessage( &SmsInfo, contents, cnt, msgTotal );
    }
    else ///short message
    {
        SmsInfo.m_uLongSms = 0; 
        SmsInfo.m_uChannelCnt = 1;
        sendCmppShortMessage(&SmsInfo, contents, cnt, msgTotal, isIncludeChinese );
    }

    return CHAN_DATATRAN;
}

void CMPPProvinceChannel::sendCmppShortMessage( smsDirectInfo *pSmsInfo, vector<string>& vecContent,
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
        
        smsp::CMPPProvinceSubmitReq req;
        
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

        string strAppend;       
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
        /////temp use m_strQueryUpUrl param replace _serviceId param
        ////Service_Id
        req._serviceId = m_ChannelInfo.m_strServerId;
        req._feeTerminalId = pSmsInfo->m_strFee_Terminal_Id;
        req._feeUserType = 3;
        
        req.setContent((char*)sendContent.data(),len);
        
        pack::Writer writer(m_pSocket->Out());
        req.Pack(writer);

        LogNotice("==cmpp province submit==[%s:%s:%d]sequence:%d",
            pSmsInfo->m_strSmsId.data(),pSmsInfo->m_strPhone.data(),m_ChannelInfo.channelID,req._sequenceId);
        saveSmsContent(pSmsInfo, (*it), uTotal, i, req._sequenceId, uMsgTotal);
    }
}

void CMPPProvinceChannel::sendCmppLongMessage(smsDirectInfo *pSmsInfo, vector<string>& vecContent,UInt32 uTotal,UInt32 uMsgTotal)
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
        int MAXLENGTH = 134;
        int size = (len + MAXLENGTH -1)/MAXLENGTH;
        int index = 1;
        char* chtmp = new char[1024];
        int send_len = 0;
        int strlen = 0;
        UInt32 uCand = getRandom();

        while(len > 0)
        {//len 剩余长度，send_len 已经发送的长度, strlen 此次发送的长度
            memset(chtmp,0,1024);
            std::string strtmp;                     
            strlen = ( len > MAXLENGTH ) ? MAXLENGTH : len;             
            
            strtmp = sendContent.substr(send_len, strlen);
            len = len - strlen;
            send_len += strlen;
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
            a = size;
            memcpy(chtmp+4,(void*)&a,1);
            a = index;
            memcpy(chtmp+5,(void*)&a,1);

            //拼content内容部分
            memcpy(chtmp+6, strtmp.data(), strlen);

            smsp::CMPPProvinceSubmitReq req;
            req._sequenceId = m_SnMng.getDirectSequenceId();
            req._tpUdhi = 0x01;
            ////req._srcId = m_ChannelInfo._accessID;
            //////extro no
            string strAppend;
          
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
            req._feeTerminalId = pSmsInfo->m_strFee_Terminal_Id;
            req._feeUserType = 3;
            ////LogDebug("###serviceId[%s],strAppend[%s].",req._serviceId.data(),strAppend.data());
            req.setContent(chtmp, strlen+6);
            
            pack::Writer writer(m_pSocket->Out());
            req.Pack(writer);

            LogNotice("==cmpp province submit==[%s:%s:%d]sequence:%d,uRand[%d].",
                      pSmsInfo->m_strSmsId.data(),
                      pSmsInfo->m_strPhone.data(),
                      m_ChannelInfo.channelID,req._sequenceId,
                      uCand);

            string strContent = "";
            utfHelper.u162u(strtmp, strContent);
            saveSmsContent(pSmsInfo, strContent, uTotal, index, req._sequenceId, uMsgTotal);
            index++;
        }

        delete[] chtmp;
        chtmp = NULL;
    }
}

void CMPPProvinceChannel::bindHandler()
{
    _hander[CMPP_CONNECT_RESP] = &CMPPProvinceChannel::onConnResp;
    _hander[CMPP_SUBMIT_RESP] = &CMPPProvinceChannel::onSubmitResp;
    _hander[CMPP_DELIVER] = &CMPPProvinceChannel::onDeliver;
    _hander[CMPP_ACTIVE_TEST_RESP] = &CMPPProvinceChannel::onHeartbeat;
    _hander[CMPP_ACTIVE_TEST] = &CMPPProvinceChannel::onHeartbeatReq;
}

void CMPPProvinceChannel::onHeartbeatReq(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    /////LogDebug("enter cmpp onHeartbeatReq");
    m_uHeartbeatTick = 0;

    if(packetLen > 12)      //12 = head.length
    {   
        LogWarn("warn, HeartbeatReq.length[%d]>12", packetLen);
        string strtmp;
        reader((UInt32)(packetLen-12), strtmp);
    }
    
    smsp::CMPPHeartbeatResp respHeartbeat;
    respHeartbeat._sequenceId = sequenceId;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    respHeartbeat.Pack(writer);
}

void CMPPProvinceChannel::onSubmitResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    smsp::CMPPSubmitResp respSubmit;
    respSubmit._totalLength = packetLen; 
    
    if (respSubmit.Unpack(reader))
    {
        std::string retCode = Comm::int2str(respSubmit._result);
        LogDebug("result = %s",retCode.data());
        
        channelSessionMap::iterator it = _packetDB.find( sequenceId );
        if (it != _packetDB.end())
        {
            directSendSesssion *pSession = it->second;
            smsp::SmsContext   *pSms  = pSession->m_pSmsContext;
            
            LogNotice("==cmpp provice submitResp==[%s:%s:%d]sequence:%d,channelsmsid:%lu,result:%s",
                pSms->m_strSmsId.data(),
                pSms->m_strPhone.data(),
                m_ChannelInfo.channelID,sequenceId,
                respSubmit._msgID, retCode.data());

            pSms->m_strSubret = retCode;
            pSms->m_strChannelSmsId = Comm::int2str(respSubmit._msgID);;
            pSms->m_lSubretDate= time(NULL);
            if(retCode != "0")  //submitResp failed
            {
                pSms->m_iStatus = SMS_STATUS_SEND_FAIL;
            }
            else
            {   
                pSms->m_iStatus = SMS_STATUS_SEND_SUCCESS;
            }

            channelStatusReport( sequenceId );
        }
        else
        {
            LogWarn("channelid:%d,hasn't find Submit Packet,seq[%u], channelsmsid[%lu]!",m_ChannelInfo.channelID, sequenceId, respSubmit._msgID);
        }

    }
}

void CMPPProvinceChannel::onDeliver(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    smsp::CMPPDeliverResp resp;
    resp._totalLength = packetLen; 
    if (resp.Unpack(reader))
    {   
        /*鍝嶅簲澶勭悊*/
        if (0 == resp._registeredDelivery && 1 == resp._tpUdhi && resp._pkNumber > CMPPMO_MAX_NUM)
        {
            respDeliver(sequenceId, resp._msgId, CMPP_ERROR_OVER_MAX_MSGLEN);
            LogError("===cmpp== channelid[%d] get long mo over 10,destphone[%s] srcPhone[%s] randcode[%u],pknum[%u],total[%u]",
                m_ChannelInfo.channelID,resp._destId.data(),resp._srcTerminalId.data(),resp._randCode,resp._pkNumber,resp._pkTotal);
            return;
        }
        else
        {
            respDeliver(sequenceId, resp._msgId, CMPP_SUCCESS);
        }

        if (1 == resp._registeredDelivery)
        {
            LogElk("==cmpp report== channelid:%d,channelsmsid[%lu],status[%d],destphone[%s],srcPhone[%s].",
                m_ChannelInfo.channelID,resp._msgId,resp._status,resp._destId.data(),resp._srcTerminalId.data());

            TDirectToDeliveryReportReqMsg* pMsg = new TDirectToDeliveryReportReqMsg();
            if (NULL == pMsg)
            {
                LogError("pMsg is NULL.");
                return;
            }

            UInt32 status = resp._status;
            if(status != 0)     //report failed
            {
                status = SMS_STATUS_CONFIRM_FAIL;
            }
            else    //report suc
            {
                status = SMS_STATUS_CONFIRM_SUCCESS;
            }

            pMsg->m_strDesc = resp._remark;
            pMsg->m_strErrorCode = resp._remark;
            pMsg->m_uChannelId = m_ChannelInfo.channelID;
            pMsg->m_uChannelIdentify = m_ChannelInfo.m_uChannelIdentify;
            pMsg->m_iStatus = status;
            pMsg->m_strChannelSmsId = Comm::int2str( resp._msgId );
            pMsg->m_lReportTime = time(NULL);
            pMsg->m_iMsgType = MSGTYPE_DIRECT_TO_DELIVERYREPORT_REQ;
            pMsg->m_strPhone = resp._srcTerminalId.data();
            pMsg->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (m_ChannelInfo.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
            pMsg->m_iHttpMode = YD_CMPP;
            g_CChannelThreadPool->smsTxPostMsg( pMsg );
        }
        else if (0 == resp._registeredDelivery)
        {
            if(NULL == g_pDirectMoThread)
            {
                LogWarn("g_pDirectMoThread is not ready!!");
                return;
            }
            string strContent = "";
            ContentCoding(resp._msgContent,resp._msgFmt,strContent);
            
            TToDirectMoReqMsg* pMo = new TToDirectMoReqMsg();
            if(NULL == pMo)
            {
                LogError("new fail!!!");
                return;
            }

            if(1 == resp._tpUdhi)
            {//has head
                pMo->m_bHead = true;
                pMo->m_uRandCode = resp._randCode;
                pMo->m_uPkNumber = resp._pkNumber;
                pMo->m_uPkTotal = resp._pkTotal;
                LogNotice("===cmpp==channelid[%d] get long mo destphone[%s] srcPhone[%s] randcode[%u],pknum[%u],total[%u]",
                    m_ChannelInfo.channelID,resp._destId.data(),resp._srcTerminalId.data(),resp._randCode,resp._pkNumber,resp._pkTotal);
            }
            LogElk("==cmpp== mo destphone[%s],srcPhone[%s],content[%s].",resp._destId.data(),resp._srcTerminalId.data(),strContent.data());
            
            if (pMo->m_uPkTotal > CMPPMO_MAX_NUM)
            {
                pMo->m_uPkTotal = CMPPMO_MAX_NUM;
            }
            pMo->m_iMsgType = MSGTYPE_TO_DIRECTMO_REQ;
            pMo->m_strContent = strContent;
            pMo->m_strPhone = resp._srcTerminalId.data();
            pMo->m_strDstSp = resp._destId.data();
            pMo->m_strSp = m_ChannelInfo._accessID.data();
            pMo->m_uChannelId = m_ChannelInfo.channelID;
            pMo->m_uMoType = YD_CMPP;
            pMo->m_uChannelType = m_ChannelInfo.m_uChannelType;
            pMo->m_uChannelMoId = resp._msgId;
            g_pDirectMoThread->PostMsg(pMo);
        }
        else
        {
            LogError("this is invalid flag[%d].",resp._registeredDelivery);
        }
    }
    else
    {
        LogError("cmpp onDeliver is failed.");
    }
}
void CMPPProvinceChannel::respDeliver(UInt32 seq, UInt64 msgId, int result)
{
    smsp::CMPPDeliverReq req;
    req._sequenceId = seq;
    req._msgId = msgId;
    req._result = 0;
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);

}
void CMPPProvinceChannel::onHeartbeat(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    ////LogDebug("***cmpp_active_test_resp sequenceid[%u]***",sequenceId);
    m_uHeartbeatTick = 0;
    smsp::CMPPHeartbeatResp respHeartbeat;
    respHeartbeat._totalLength = packetLen; 
    if (respHeartbeat.Unpack(reader))
    {

    }
}

void CMPPProvinceChannel::sendHeartbeat()
{
    if (!isAlive())
    {
        return;
    }

    smsp::CMPPHeartbeatReq req;
    req._sequenceId = m_SnMng.getDirectSequenceId();
    ////LogDebug("***cmpp_active_test sequenceid[%u]***",req._sequenceId);
    if(NULL == m_pSocket){
        LogError("socket is null!!");
        return;
    }
    pack::Writer writer(m_pSocket->Out());
    req.Pack(writer);
}

void CMPPProvinceChannel::onConnResp(pack::Reader& reader, UInt32 sequenceId, UInt32 packetLen)
{
    smsp::CMPPConnectResp resp;
    resp._totalLength = packetLen; 
    if (resp.Unpack(reader))
    {
        if (resp.isLogin())
        {
            LogNotice("CMPP channelid[%d] Login success.ver:0x%x", m_ChannelInfo.channelID,resp._version);
            m_iState = CHAN_DATATRAN;
            m_bHasConn = true;  
            m_uContinueLoginFailedNum = 0;
            InitFlowCtr();
            updateLoginState(m_uChannelLoginState, 1, "Login Success" );
            m_uChannelLoginState = 1;
            
            TDirectChannelConnStatusMsg* pmsg = new  TDirectChannelConnStatusMsg();
            pmsg->m_uChannelId = m_ChannelInfo.channelID;
            pmsg->m_uState   =  CHAN_DATATRAN;
            pmsg->m_iMsgType = MSGTYPE_CHANNEL_CONN_STATUS;         
            m_pProcessThread->PostMsg(pmsg);
        }
        else
        {
            LogError("CMPP province channelid[%d],status:%d, Login failed.", m_ChannelInfo.channelID,resp._status);
            m_iState = CHAN_UNLOGIN;
            
            updateLoginState(m_uChannelLoginState, 2, Comm::int2str(resp._status));     
            m_uChannelLoginState = 2;
        }
    }
}

void CMPPProvinceChannel::init(models::Channel& chan)
{   
    IChannel::init(chan);
    
    bindHandler();
    m_ChannelInfo = chan;       //has sliderWindow source int. will not change

    //m_flowCtr.iWindowSize = chan.m_uSliderWindow;
    //m_flowCtr.lLastRspTime = time( NULL );
    
    if (m_wheelTime != NULL)
    {
        m_wheelTime->Destroy();
        m_wheelTime = NULL;
    }
    m_iState = CHAN_INIT;
    LogNotice("CMPPProvinceChannel channelid[%d] init OK", m_ChannelInfo.channelID);
}

void CMPPProvinceChannel::conn()
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
    LogNotice("CMPPChannel::connect server [%s:%d] accout[%s], channelid[%d]", ip.data()
        , port,m_ChannelInfo._clientID.data(), m_ChannelInfo.channelID);
    Address addr(ip, port);
    m_pSocket->Connect(addr, 10000);
    smsp::CMPPConnectReq reqLogin;
    reqLogin._sequenceId = m_SnMng.getDirectSequenceId();
    m_iState = CHAN_INIT;
    reqLogin.setAccout(m_ChannelInfo._clientID, m_ChannelInfo._password);
    pack::Writer writer(m_pSocket->Out());
    reqLogin.Pack(writer);
    m_uHeartbeatTick = 0;

    updateLoginState(m_uChannelLoginState, 0, "");  
    m_uChannelLoginState = 0;
}

void CMPPProvinceChannel::reConn()
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

void CMPPProvinceChannel::destroy()
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

    /* 释放当前所有会话*/
    channelSessionMap::iterator iter = _packetDB.begin();
    LogDebug("_packetDB.size()=%d", _packetDB.size());
    for (; iter != _packetDB.end(); ++iter )
    {
        directSendSesssion *session = iter->second;
        session->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        session->m_pSmsContext->m_strSubmit = CMPP_INTERNAL_ERROR;
        session->m_pSmsContext->m_strYZXErrCode = CMPP_INTERNAL_ERROR;
        channelStatusReport( iter->first, false );
        SAFE_DELETE( iter->second );
        _packetDB.erase(iter);
    }

    delete this;

}

bool CMPPProvinceChannel::onTransferData()
{
    smsp::CMPPBase resp;
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
        
        //check length from head. 
        if( resp._totalLength > 5000 )
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

void CMPPProvinceChannel::OnEvent(int type, InternalSocket* socket)
{
    if (socket != m_pSocket)
    {
        return;
    }
    if (type == EventType::Connected)
    {
        LogDebug("CMPPProvinceChannel::Connected");
        m_iState = CHAN_INIT;
    }
    else if (type == EventType::Read)
    {
        m_uHeartbeatTick = 0;
        onData();
    }
    else if (type == EventType::Closed)
    {
        LogError("CMPPProvinceChannel::Close.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());

        m_iState = CHAN_CLOSE;
        
        updateLoginState(m_uChannelLoginState, 2, "link is close");
        m_uChannelLoginState = 2;
    }
    else if (type == EventType::Error)
    {
        LogError("CMPPProvinceChannel::Error.channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());

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

void CMPPProvinceChannel::OnTimer()
{
    if (false == m_bHasConn)
    {
        conn();
        m_bHasReconnTask = false;
        /* BEGIN: Added by fxh, 2016/11/26   PN:RabbitMQ */
        continueLoginFailed(m_uContinueLoginFailedNum);
        m_uContinueLoginFailedNum++;
        /* END:   Added by fxh, 2016/11/26   PN:RabbitMQ */
    }
    else 
    {
        sendHeartbeat();
        if ( ++m_uHeartbeatTick > 10 )
        {
            LogWarn("CMPPProvinceChannel heartbeat timeout,channelid[%d], channelName[%s]", m_ChannelInfo.channelID, m_ChannelInfo.channelname.data());
            reConn();
        }
        else
        {
            FlowSliderResetCheck();
        }   
    }

    /* 检测会话是否超时*/
    Int64 iNowTime = time( NULL );
    channelSessionMap::iterator iter = _packetDB.begin();
    for (;iter != _packetDB.end();)
    {
        directSendSesssion *session = iter->second ;
        /* 直连协议超时*/
        if (( iNowTime - session->m_pSmsContext->m_lSubmitDate) > ICHANNEL_SESSION_TIMEOUT_SECS )
        {   
            LogWarn("==cmpp submitResp timeout==[%s:%s:%d]_packetDB size:%d.",
                session->m_pSmsContext->m_strSmsId.data(),
                session->m_pSmsContext->m_strPhone.data(),
                m_ChannelInfo.channelID,_packetDB.size());
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

void CMPPProvinceChannel::onData()
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



