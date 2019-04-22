#include "IChannel.h"
#include "UTFString.h"
#include "UrlCode.h"
#include <sstream>
#include "Comm.h"
#include "alarm.h"
#include "main.h"

IChannel::IChannel()
{
    m_iState = CHAN_INIT;
    m_bHasReconnTask=false;
    m_bHasConn = false;
    m_wheelTime = NULL;
    m_uContinueLoginFailedNum = 10;
    m_pProcessThread = NULL;
    m_uContinueLoginFailedValue = 10;
}

IChannel::~IChannel()
{
}

directSendSesssion::directSendSesssion()
{
    m_strSessionId = "";
    m_pSmsContext = new smsp::SmsContext();
}

directSendSesssion::~directSendSesssion()
{
    SAFE_DELETE( m_pSmsContext );
}

void IChannel::init(models::Channel& chan)
{
    m_uRandom = 0;
    srand((int)time(NULL));
    m_uRandom = rand()%255;
    m_uChannelLoginState = 0;
    m_bCloseChannel = false;
    m_bDelAll = false;
    m_SnMng.Init(m_uNodeId);
}

UInt32 IChannel::getRandom()
{
    m_uRandom++;
    if(m_uRandom >=255)
    {
        m_uRandom = 0;
    }

    return m_uRandom;
}

void IChannel::OnTimer()
{

}

void IChannel::InitFlowCtr()
{
    m_flowCtr.iWindowSize = m_ChannelInfo.m_uSliderWindow;
    m_flowCtr.lLastRspTime = time( NULL );
}

UInt32 IChannel::sendSms( smsDirectInfo &smsInfo  )
{
    return m_iState;
}

void IChannel::destroy()
{

}

void IChannel::reConn()
{
}

UInt32 IChannel::parseMsgContent(string& content, vector<string>& contents, string& sign, bool isIncludeChinese)
{
    int maxWord = 0;
    if(!isIncludeChinese)   //ȫӢ�Ķ���
    {
        maxWord = 140;
    }
    else        //�������ĵĶ���
    {
        maxWord = 70;
    }


    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(content);
    UInt32 signLen = utfHelper.getUtf8Length(sign);

    contents.push_back(content);

    if(msgLength + signLen <= (UInt32)maxWord)  //70    ||160
    {
        return 1;
    }

    UInt32 cnt = (msgLength+signLen)/67;    //67
    if((msgLength+signLen)*1.0/67 == cnt)   //67
    {
        return cnt;
    }
    else
    {
        return cnt + 1;
    }

}

UInt32 IChannel::parseSplitContent(string& content, vector<string>& contents, string& sign, bool isIncludeChinese)
{
    int maxWord = 0;
    int splitWord = 0;
    if(!isIncludeChinese)
    {
        maxWord = m_ChannelInfo.m_uEnLen;
        splitWord = m_ChannelInfo.m_uEnSplitLen;
    }
    else
    {
        maxWord = m_ChannelInfo.m_uCnLen;
        splitWord = m_ChannelInfo.m_uCnSplitLen;
    }


    utils::UTFString utfHelper;
    UInt32 msgLength = utfHelper.getUtf8Length(content);
    UInt32 signLen = utfHelper.getUtf8Length(sign);

    contents.push_back(content);

    if(msgLength + signLen <= (UInt32)maxWord)  //70    ||140
    {
        return 1;
    }

    UInt32 cnt = (msgLength+signLen)/splitWord; //67
    if((msgLength+signLen)*1.0/splitWord == cnt)    //67
    {
        return cnt;
    }
    else
    {
        return cnt + 1;
    }

}

UInt32 IChannel::saveSplitContent( smsDirectInfo *smsDirect, string& content, UInt32 total,
                                        UInt32 index, UInt32 seq, UInt32 msgTotal, bool isIncludeChinese )
{
    directSendSesssion *session = new directSendSesssion();
    smsp::SmsContext* sms = session->m_pSmsContext;
    sms->m_iChannelId = m_ChannelInfo.channelID;
    sms->m_strChannelname.assign(m_ChannelInfo.channelname);
    sms->m_iOperatorstype = smsDirect->m_uPhoneType;
    sms->m_strSmsId.assign(smsDirect->m_strSmsId);
    sms->m_strPhone.assign(smsDirect->m_strPhone);
    sms->m_strContent = content;
    sms->m_lSendDate = smsDirect->m_uSendDate;
    sms->m_iSmscnt = 1;
    int maxWord = m_ChannelInfo.m_uCnSplitLen;

    if (!isIncludeChinese)
    {
        maxWord = m_ChannelInfo.m_uEnSplitLen;
    }

    if (smsDirect->m_uShowSignType == 3 && index == 1)
    {
        sms->m_strContent.insert(0, smsDirect->m_strSign);
        utils::UTFString utfHelper;
        UInt32 msgLength = utfHelper.getUtf8Length(smsDirect->m_strContent);
        if(smsDirect->m_uLongSms == 1)
        {
            UInt32 noSigncnt = msgLength/maxWord;
            if((msgLength)*1.0/maxWord == noSigncnt)
            {
                //do nothing
            }
            else
            {
                noSigncnt =  noSigncnt + 1;
            }

            if (total > noSigncnt && !m_ChannelInfo.m_uSplitRule)
                sms->m_iSmscnt = 2;

            LogDebug("noSigncnt = %d, total=%d, m_strContent[%s]",noSigncnt, total, smsDirect->m_strContent.data());

        }
    }

    sms->m_iIndexNum = index;
    sms->m_iOperatorstype = smsDirect->m_uPhoneType;
    sms->m_iSmsfrom = smsDirect->m_uSmsFrom;
    sms->m_iArea = smsDirect->m_uArea;
    sms->m_fCostFee = smsDirect->m_fCostFee;
    sms->m_uDivideNum = total;
    sms->m_strClientId.assign(smsDirect->m_strClientId);
    sms->m_uPayType = smsDirect->m_uPayType;
    sms->m_strUserName.assign(smsDirect->m_strUserName);
    sms->m_lSubmitDate = time(NULL);
    sms->m_uChannelOperatorsType = m_ChannelInfo.operatorstyle;
    sms->m_strChannelRemark.assign(m_ChannelInfo.m_strRemark);
    sms->m_uChannelIdentify = smsDirect->m_uChannelIdentify;
    sms->m_uC2sId = smsDirect->m_uC2sId;
    sms->m_strC2sTime.assign(smsDirect->m_strC2sTime);
    sms->m_uClientCnt = smsDirect->m_uClientCnt;
    sms->m_ulongsms = smsDirect->m_uLongSms;
    sms->m_uChannelCnt = smsDirect->m_uChannelCnt;
    sms->m_strShowPhone= smsDirect->m_strShowPhone;
    sms->m_strTemplateId = smsDirect->m_strTemplateId;
    sms->m_strTemplateParam = smsDirect->m_strTemplateParam;
    sms->m_strChannelTemplateId = smsDirect->m_strChannelTemplateId;
    sms->m_uBelong_sale = smsDirect->m_uBelong_sale;
    sms->m_uBelong_business = smsDirect->m_uBelong_business;
    sms->m_strSign = smsDirect->m_strSign;
    sms->m_uSmsType= smsDirect->m_uSmsType;

    session->m_strSessionId = smsDirect->m_strSessionId;

    /*����Ϊ��ʧ��*/
    if( content.empty() )
    {
        sms->m_iStatus = SMS_STATUS_SOCKET_ERR_TO_REBACK;
        sms->m_strSubmit = SOCKET_IS_ERROR;
    }
    LogDebug("content=[%s]", content.data());
    FlowSendRequst();
    _packetDB[seq] = session;
    return CC_TX_RET_SUCCESS;
}

bool IChannel::ContentCoding(string& strSrc,UInt8 uCodeType,string& strDst)
{
    utils::UTFString utfHelper;

    if (0 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (3 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (4 == uCodeType)
    {
        LogWarn("codeType[%d] is not support.",uCodeType);
        strDst = strSrc;
    }
    else if (8 == uCodeType)
    {
        utfHelper.u162u(strSrc,strDst);
    }
    else if (15 == uCodeType)
    {
        utfHelper.g2u(strSrc,strDst);
    }
    else
    {
        strDst = strSrc;
        LogError("strSrc[%s],codeType[%d] is invalid code type.",strSrc.data(),uCodeType);
    }

    return true;
}


/* ������Ϣ*/
UInt32 IChannel::saveSmsContent( smsDirectInfo *smsDirect, string& content, UInt32 total,
                                        UInt32 index, UInt32 seq, UInt32 msgTotal )
{
    directSendSesssion *session = new directSendSesssion();
    smsp::SmsContext* sms = session->m_pSmsContext;
    sms->m_iChannelId = m_ChannelInfo.channelID;
    sms->m_strChannelname.assign(m_ChannelInfo.channelname);
    sms->m_iOperatorstype = smsDirect->m_uPhoneType;
    sms->m_strSmsId.assign(smsDirect->m_strSmsId);
    sms->m_strPhone.assign(smsDirect->m_strPhone);
    sms->m_strContent = content;
    sms->m_lSendDate = smsDirect->m_uSendDate;
    sms->m_iSmscnt = 1;

    if (smsDirect->m_uShowSignType == 3 && index == 1)
    {
        sms->m_strContent.insert(0, smsDirect->m_strSign);
        utils::UTFString utfHelper;
        UInt32 msgLength = utfHelper.getUtf8Length(smsDirect->m_strContent);
        if(smsDirect->m_uLongSms == 1)
        {
            UInt32 noSigncnt = msgLength/67;
            if((msgLength)*1.0/67 == noSigncnt)
            {
                //do nothing
            }
            else
            {
                noSigncnt =  noSigncnt + 1;
            }

            if (total > noSigncnt )
                sms->m_iSmscnt = 2;

            LogDebug("noSigncnt = %d, total=%d, m_strContent[%s]",noSigncnt, total, content.data());

        }
    }
    LogDebug("index=%d,total=%d, m_strContent[%s]",index, total, content.data());

    sms->m_iIndexNum = index;
    sms->m_iOperatorstype = smsDirect->m_uPhoneType;
    sms->m_iSmsfrom = smsDirect->m_uSmsFrom;
    sms->m_iArea = smsDirect->m_uArea;
    sms->m_fCostFee = smsDirect->m_fCostFee;
    sms->m_uDivideNum = total;
    sms->m_strClientId.assign(smsDirect->m_strClientId);
    sms->m_uPayType = smsDirect->m_uPayType;
    sms->m_strUserName.assign(smsDirect->m_strUserName);
    sms->m_lSubmitDate = time(NULL);
    sms->m_uChannelOperatorsType = m_ChannelInfo.operatorstyle;
    sms->m_strChannelRemark.assign(m_ChannelInfo.m_strRemark);
    sms->m_uChannelIdentify = smsDirect->m_uChannelIdentify;
    sms->m_uC2sId = smsDirect->m_uC2sId;
    sms->m_strC2sTime.assign(smsDirect->m_strC2sTime);
    sms->m_uClientCnt = smsDirect->m_uClientCnt;
    sms->m_ulongsms = smsDirect->m_uLongSms;
    sms->m_uChannelCnt = smsDirect->m_uChannelCnt;
    sms->m_strShowPhone= smsDirect->m_strShowPhone;
    sms->m_strTemplateId = smsDirect->m_strTemplateId;
    sms->m_strTemplateParam = smsDirect->m_strTemplateParam;
    sms->m_strChannelTemplateId = smsDirect->m_strChannelTemplateId;
    sms->m_uBelong_sale = smsDirect->m_uBelong_sale;
    sms->m_uBelong_business = smsDirect->m_uBelong_business;
    sms->m_strSign = smsDirect->m_strSign;
    sms->m_uSmsType= smsDirect->m_uSmsType;

    session->m_strSessionId = smsDirect->m_strSessionId;

    /*����Ϊ��ʧ��*/
    if( content.empty() )
    {
        sms->m_iStatus = SMS_STATUS_SOCKET_ERR_TO_REBACK;
        sms->m_strSubmit = SOCKET_IS_ERROR;
    }

    FlowSendRequst();
    _packetDB[seq] = session;
    return CC_TX_RET_SUCCESS;
}

void IChannel::UpdateContinueLoginFailedValue(UInt32 newValue)
{
    if(m_uContinueLoginFailedValue != newValue)
    {
        LogNotice("Channel[%d] update continue login failed value from %u to %u", m_ChannelInfo.channelID, m_uContinueLoginFailedValue, newValue);
        m_uContinueLoginFailedValue = newValue;
    }
}


/* 连续登录失败告警*/
void IChannel::continueLoginFailed( UInt32& uFailedNum  )
{
    if ( uFailedNum > m_uContinueLoginFailedValue )
    {
        char temp[15] = {0};
        snprintf( temp,15,"%d",m_ChannelInfo.channelID );

        string strChinese = "";
        strChinese = DAlarm::getLoginFailedAlarmStr(m_ChannelInfo.channelID, m_ChannelInfo.industrytype, m_uContinueLoginFailedValue );

        LogError("==continueLoginFailed==channelid:%d,continue login failed over %d.",
                 m_ChannelInfo.channelID, m_uContinueLoginFailedValue );
        Alarm(CHNN_LOGIN_FAILED_ALARM,temp,strChinese.data());
        uFailedNum = 0;
    }
}

void IChannel::updateLoginState(UInt32 oldState, UInt32 newState, string strDesc)
{
    if(oldState != newState)
    {
        LogDebug("Channel[%d] update LoginState %u -> %u. m_bDeleteAll[%d]", m_ChannelInfo.channelID, oldState, newState, m_bDelAll);
        TUpdateChannelLoginStateReq* pReq = new TUpdateChannelLoginStateReq();
        pReq->m_iChannelId = m_ChannelInfo.channelID;
        pReq->m_uLoginStatus = newState;
        pReq->m_uOldLoginStatus = oldState;
        pReq->m_strDesc = strDesc;
        pReq->m_NodeId = m_uNodeId;
        pReq->m_bDelAll = m_bDelAll;
        pReq->m_iMsgType = MSGTYPE_RULELOAD_CHANNEL_LOGIN_STATE_REQ;

        if(oldState != 1 && newState == 1)
        {
            pReq->m_iWnd = m_flowCtr.iWindowSize;
        }
        else if(oldState == 1 && newState != 1)
        {
            pReq->m_iWnd = 0 - m_flowCtr.iWindowSize;
        }

        pReq->m_iMaxWnd = m_ChannelInfo.m_uSliderWindow;

        g_CChannelThreadPool->PostMsg(pReq);

        //更新本地滑窗
        if(newState != 1)
        {
            m_flowCtr.iWindowSize = 0;
        }
        else
        {
            InitFlowCtr();
        }
    }
}

void IChannel::ResetChannelSlideWindowReq(int Channelid)
{
    LogNotice("Channel[%d] timeout Reset SlideWindow", m_ChannelInfo.channelID);

    TResetChannelSlideWindowMsg * pMsg = new TResetChannelSlideWindowMsg();
    pMsg->m_iMsgType = MSGTYPE_TIMEOUT_RESET_SLIDE_WINDOW;
    pMsg->m_iChannelId = Channelid;
    pMsg->m_iChannelWnd = m_ChannelInfo.m_uSliderWindow;
    g_CChannelThreadPool->PostMsg(pMsg);
}

/*通道发送状态上报*/
void IChannel::channelStatusReport( UInt64 uSeq, bool bDestory )
{
    std::map<UInt32, directSendSesssion*>::iterator itr =  _packetDB.find( uSeq );
    if( itr == _packetDB.end()){
        LogWarn( "Direct Report, SessionId %lu Not Find", uSeq );
        return;
    }

    directSendSesssion *pSession = itr->second;
    smsp::SmsContext* pSms = pSession->m_pSmsContext;

    CSMSTxStatusReportMsg *txReport = new CSMSTxStatusReportMsg();

    LogDebug("SmSid:%s, channelSmsid:%s, status:%d",
              pSms->m_strSmsId.c_str(),
              pSms->m_strChannelSmsId.c_str(),
              pSms->m_iStatus);

    txReport->report.m_status        = pSms->m_iStatus;
    txReport->report.m_strSubmit     = pSms->m_strSubmit;
    txReport->report.m_strYZXErrCode = pSms->m_strYZXErrCode;
    txReport->report.m_strSubRet     = pSms->m_strSubret;
    txReport->report.m_strSmsid      = pSms->m_strSmsId;
    txReport->report.m_strChannelSmsid = pSms->m_strChannelSmsId;
    txReport->report.m_strSmsUuid   = pSms->m_strSmsUuid;
    txReport->report.m_lSubmitDate  = pSms->m_lSubmitDate;
    txReport->report.m_lSubretDate  = pSms->m_lSubretDate;
    txReport->report.m_strContent   = pSms->m_strContent;
    txReport->report.m_strSign      = pSms->m_strSign;
    txReport->report.m_iSmsCnt      = pSms->m_iSmscnt;     /*计费条数*/
    txReport->report.m_iIndexNum    = pSms->m_iIndexNum;
    txReport->report.m_uTotalCount  = pSms->m_uDivideNum;  /*通道发送条数*/
    txReport->report.m_uChanCnt     = pSms->m_uChannelCnt; /*通道条数*/
    txReport->report.m_strErrorCode = pSms->m_strErrorCode;

    txReport->m_strSessionID        = pSession->m_strSessionId;
    txReport->m_iMsgType            = MSGTYPE_CHANNEL_SEND_STATUS_REPORT;

    g_CChannelThreadPool->smsTxPostMsg( txReport );

    /* 发送超时*/
    if( CMPP_REPLY_TIME_OUT != pSms->m_strSubmit
        && SGIP_REPLY_TIME_OUT != pSms->m_strSubmit
        && SMGP_REPLY_TIME_OUT != pSms->m_strSubmit
        && SMPP_REPLY_TIME_OUT != pSms->m_strSubmit)
    {
        FlowSendResponse();
    }

    /* 删除会话*/
    if( true == bDestory )
    {
        SAFE_DELETE( pSession );
        _packetDB.erase( itr );
    }
}


/* 通道发送流控请求*/
Int32 IChannel::FlowSendRequst()
{
    time_t now = time(NULL);
    m_flowCtr.iWindowSize --;

    if( m_flowCtr.uTotalReqCnt++ > 0xFFFFFFFFFFFFFFFE ){
        m_flowCtr.uTotalReqCnt = 0;
    }

    //如果滑窗为0了，记录为最后一次响应时间，后续定时重置滑窗的时候，才会有个初始值，比较准
    if(m_flowCtr.iWindowSize <= 0)
    {
        m_flowCtr.lLastRspTime = now;
    }

    LogNotice("FlowCtr(%u):[ uWindSize:[%d:%d], totalReqCnt:%u, totalRspCnt:%u, RetCnt:%d, LastRspTime:%d ReqTimeElapse:%ld]",
            m_ChannelInfo.channelID, m_flowCtr.iWindowSize, m_ChannelInfo.m_uSliderWindow,
            m_flowCtr.uTotalReqCnt,m_flowCtr.uTotalRspCnt,
            m_flowCtr.uResetCount, m_flowCtr.lLastRspTime, now - m_flowCtr.lLastReqTime);

    m_flowCtr.lLastReqTime = now;

    return CC_TX_RET_SUCCESS;
}

/*通道发送流控响应*/
Int32 IChannel::FlowSendResponse()
{
    time_t now = time(NULL);

    if( ++m_flowCtr.iWindowSize > m_ChannelInfo.m_uSliderWindow ){
        m_flowCtr.iWindowSize = m_ChannelInfo.m_uSliderWindow;
    }

    if( m_flowCtr.uTotalRspCnt++ > 0xFFFFFFFFFFFFFFFE ){
        m_flowCtr.uTotalRspCnt = 0;
    }

    LogNotice("FlowCtr(%u):[ uWindSize:[%d:%d], totalReqCnt:%u, totalRspCnt:%u, RetCnt:%d, RspTimeElapse:%ld]",
            m_ChannelInfo.channelID, m_flowCtr.iWindowSize, m_ChannelInfo.m_uSliderWindow,
            m_flowCtr.uTotalReqCnt,m_flowCtr.uTotalRspCnt,
            m_flowCtr.uResetCount, now - m_flowCtr.lLastRspTime);

    m_flowCtr.lLastRspTime = now;

    return CC_TX_RET_SUCCESS;

}

void IChannel::UpdateChannelInfo(models::Channel& chan)
{
    LogDebug("Channel[%d] OldWnd[%d] NewWnd[%d]", chan.channelID, m_ChannelInfo.m_uSliderWindow, chan.m_uSliderWindow);
    if(m_ChannelInfo.m_uSliderWindow != chan.m_uSliderWindow)
    {
        //更新滑窗
        m_flowCtr.iWindowSize = chan.m_uSliderWindow;
    }

    m_ChannelInfo = chan;

    LogNotice("Channel[%d] update, CurrWnd[%d], MaxWnd[%d]!", chan.channelID, m_flowCtr.iWindowSize, m_ChannelInfo.m_uSliderWindow);
}

/*通道滑窗检测*/   //这里要改，断开不能重置
void IChannel::FlowSliderResetCheck()
{
    if(m_uChannelLoginState != 1)
    {
        return;
    }

    time_t now = time( NULL );
    if( m_flowCtr.iWindowSize <= 0
        && now - m_flowCtr.lLastRspTime > ICHANNEL_SLIDER_RESET_TIME )
    {
        LogWarn("Channel[%d] now[%ld] lLastRspTime[%ld] Reset Window from %d to %d",
            m_ChannelInfo.channelID, now, m_flowCtr.lLastRspTime, m_flowCtr.iWindowSize, m_ChannelInfo.m_uSliderWindow);

        m_flowCtr.iWindowSize = m_ChannelInfo.m_uSliderWindow;
        m_flowCtr.lLastRspTime = now;

        ResetChannelSlideWindowReq(m_ChannelInfo.channelID);

        if( ++m_flowCtr.uResetCount >= ICHANNEL_SLIDER_RESET_COUNT )
        {
            string strChinese = DAlarm::getNoSubmitRespAlarmStr( m_ChannelInfo.channelID, m_ChannelInfo.industrytype, ICHANNEL_SLIDER_RESET_TIME * m_flowCtr.uResetCount );
            string strChanneId = Comm::int2str( m_ChannelInfo.channelID );
            Alarm(CHNN_SLIDE_WINDOW_ALARM, strChanneId.data(), strChinese.data());
            LogWarn("FlowCtr(%u): Reset sliderWindow:%d retCnt %d Alarm...",
                    m_ChannelInfo.channelID,m_ChannelInfo.m_uSliderWindow, m_flowCtr.uResetCount );
            m_flowCtr.uResetCount = 0;
        }
    }
}

/*获取当前线程处理流控*/
UInt32 IChannel::GetChannelFlowCtr( int& flowRemain )
{
    flowRemain = m_flowCtr.iWindowSize;

    if( m_iState != CHAN_DATATRAN || m_bCloseChannel == true )
    {
        return CHANNEL_FLOW_STATE_CLOSE;
    }

    if( m_flowCtr.iWindowSize <= 0 )
    {
        return CHANNEL_FLOW_STATE_BLOCK;
    }

    return CHANNEL_FLOW_STATE_NORMAL;
}
