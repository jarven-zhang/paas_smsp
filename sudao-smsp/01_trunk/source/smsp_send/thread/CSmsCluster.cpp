#include "Comm.h"
#include "base64.h"
#include <boost/functional/hash.hpp>
#include "CChannelTxThreadPool.h"
#include "main.h"
#include "CSmsCluster.h"
#include "json/json.h"
#include "global.h"


#define SESSION_FREE( Session ) SAFE_DELETE(Session)

bool CSMSCluster::Init()
{
    m_pSnMng = new SnManager();

    if (NULL == m_pSnMng )
    {
        LogError("m_pSnMng is NULL.");
        return false;
    }

    if ( false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    /*加载redis数据*/
    smsClusterLoadDataIdx();

    return true;

}

void CSMSCluster::MainLoop()
{
    WAIT_MAIN_INIT_OVER

    while(true)
    {
        m_pTimerMng->Click();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if( pMsg == NULL )
        {
            usleep( 10*1000 );
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
            pMsg = NULL;
        }
    }
}

void CSMSCluster::HandleMsg( TMsg* pMsg )
{
    UInt32 ret = CC_TX_RET_SUCCESS;

    switch(pMsg->m_iMsgType ){

        case MSGTYPE_DISPATCH_TO_HTTPSEND_REQ:
            ret = smsClusterRequst(pMsg);
            break;

        case MSGTYPE_CHANNEL_SEND_STATUS_REPORT:
            ret = smsClusterStatusComm( pMsg);
            break;

        case MSGTYPE_TIMEOUT :
            ret = smsClusterTimeOut( pMsg );
            break;

        case MSGTYPE_REDIS_RESP:
            ret = smsClusterLoadRedisRsp( pMsg );
            break;

        default:
            ;;
    }

}


/*发送组包消息到发送线程*/
UInt32 CSMSCluster::smsSendToHttp( smsCtxInfo_t* smsCtxInfo, UInt64 ClusterId,
                                      string strSendType, UInt32 sessionId, string strPhones, UInt32 uSize )
{
    TDispatchToHttpSendReqMsg* msg = new TDispatchToHttpSendReqMsg();
    msg->m_smsParam = smsCtxInfo->http_msg;

    if(!strPhones.empty())
    {
        msg->m_smsParam.m_strPhone = strPhones;
    }

    msg->m_iMsgType = MSGTYPE_DISPATCH_TO_HTTPSEND_REQ;
    msg->m_iSeq = ClusterId;
    msg->m_strSessionID = strSendType;
    msg->m_uPhonesCount = uSize;
    msg->m_uSessionId  = sessionId;
    msg->m_pSender = this;

    if( true != g_CChannelThreadPool->PostMsg( msg ))
    {
        LogError("Cluster[%lu] Send Error Smsid:%s,Phone:%s",
                 ClusterId, msg->m_smsParam.m_strSmsId.c_str(),
                 msg->m_smsParam.m_strPhone.c_str());
        return CC_TX_RET_SEND_MSG_ERROR;
    }

    LogNotice("Cluster[%lu : %u] Send Success size:%u, Smsid:%s, Phone:%s",
              ClusterId, sessionId, uSize, msg->m_smsParam.m_strSmsId.c_str(),
              msg->m_smsParam.m_strPhone.c_str());
    return CC_TX_RET_SUCCESS;

}


/*写reback mQ*/
UInt32 CSMSCluster::smsSendToReback( smsCtxInfo_t* smsCtxInfo, CSMSClusterStatusReq *statusReq )
{
    SmsHttpInfo* HttpMsg = &smsCtxInfo->http_msg;
    ////data
    string strData = "";
    ////clientId
    strData.append("clientId=");
    strData.append(HttpMsg->m_strClientId);
    ////userName
    strData.append("&userName=");
    strData.append(Base64::Encode(HttpMsg->m_strUserName));
    ////smsId
    strData.append("&smsId=");
    strData.append(HttpMsg->m_strSmsId);
    ////phone
    strData.append("&phone=");
    strData.append(HttpMsg->m_strPhone);
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(HttpMsg->m_strSign));
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(HttpMsg->m_strContent));
    ////smsType
    strData.append("&smsType=");
    strData.append(Comm::int2str(HttpMsg->m_uSmsType));
    ////smsfrom
    strData.append("&smsfrom=");
    strData.append(Comm::int2str(HttpMsg->m_uSmsFrom));
    ////paytype
    strData.append("&paytype=");
    strData.append(Comm::int2str(HttpMsg->m_uPayType));
    ////showsigntype
    strData.append("&showsigntype=");
    strData.append(HttpMsg->m_strShowSignType_c2s);
    ////userextpendport
    strData.append("&userextpendport=");
    strData.append(HttpMsg->m_strUserExtendPort_c2s);

    ////signextendport
    strData.append("&signextendport=");
    strData.append(HttpMsg->m_strSignExtendPort_c2s);

    ////operater
    strData.append("&operater=");
    strData.append(Comm::int2str(HttpMsg->m_uPhoneType));
    ////ids
    strData.append("&ids=");
    strData.append(HttpMsg->m_strAccessids);
    ////clientcnt
    strData.append("&clientcnt=");
    strData.append(Comm::int2str(HttpMsg->m_uClientCnt));
    ////process_times
    strData.append("&process_times=");
    strData.append(Comm::int2str(HttpMsg->m_uProcessTimes));
    ////csid
    strData.append("&csid=");
    strData.append(Comm::int2str(HttpMsg->m_uC2sId));
    ////csdate
    strData.append("&csdate=");
    strData.append(HttpMsg->m_strC2sTime);
    ////errordate
    strData.append("&errordate=");
    if (1 == HttpMsg->m_uProcessTimes)
    {
        UInt64 uNowTime = time(NULL);
        char pNow[25] = {0};
        snprintf(pNow,25,"%lu",uNowTime);
        strData.append(pNow);
    }
    else
    {
        strData.append(HttpMsg->m_strErrorDate);
    }

    strData.append("&templateid=").append(HttpMsg->m_strTemplateId);
    strData.append("&temp_params=").append(HttpMsg->m_strTemplateParam);
    if(HttpMsg->m_uResendTimes > 0 && !HttpMsg->m_strFailedChannel.empty())
    {
        strData.append("&failed_resend_times=").append(Comm::int2str(HttpMsg->m_uResendTimes));
        strData.append("&failed_resend_channel=").append(HttpMsg->m_strFailedChannel);
    }

    if (!HttpMsg->m_strChannelOverrate_access.empty())
    {
        strData.append("&channelOverrateDate=").append(HttpMsg->m_strChannelOverrate_access);
        strData.append("&oriChannelid=");
        strData.append(HttpMsg->m_strOriChannelId);
    }

    LogDebug("==http push except== exchange:%s,routingkey:%s,data:%s.",
               statusReq->strExchange.c_str(),
               statusReq->strRoutingKey.c_str(),
               strData.c_str());

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_strData = strData;
    pMQ->m_strExchange = statusReq->strExchange;
    pMQ->m_strRoutingKey = statusReq->strRoutingKey;
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    g_pMqAbIoProductThread->PostMsg(pMQ);

    return 0;
}


/*失败上报*/
UInt32 CSMSCluster::smsSendToReport( SmsHttpInfo* HttpMsg, CSMSClusterStatusReq *req )
{
    if ( NULL == HttpMsg )
    {
        return CC_TX_RET_PARAM_ERROR;
    }

    if( req->txReport.m_uTotalCount > 1
        && req->txReport.m_iIndexNum == 1 )
    {
        smsClusterSetLongFlagRedis( HttpMsg->m_strSmsId, HttpMsg->m_strPhone);
    }

    TSendToDeliveryReportReqMsg* pMsg = new TSendToDeliveryReportReqMsg();
    pMsg->m_iMsgType = MSGTYPE_SEND_TO_DELIVERYREPORT_REQ;
    pMsg->m_strType = "1";

    pMsg->m_strClientId.assign(HttpMsg->m_strClientId);
    pMsg->m_uChannelId = HttpMsg->m_iChannelId;
    pMsg->m_strSmsId.assign(HttpMsg->m_strSmsId);
    pMsg->m_strPhone.assign(HttpMsg->m_strPhone);
    pMsg->m_iStatus = req->txReport.m_status;
    pMsg->m_strDesc = req->txReport.m_strSubRet;
    pMsg->m_strReportDesc = req->txReport.m_strYZXErrCode;
    pMsg->m_uChannelCnt = req->txReport.m_uChanCnt;
    pMsg->m_uReportTime = time(NULL);
    pMsg->m_uSmsFrom = HttpMsg->m_uSmsFrom;
    pMsg->m_uLongSms = req ->txReport.m_uTotalCount;
    pMsg->m_uC2sId = HttpMsg->m_uC2sId;
    g_pDeliveryReportThread->PostMsg(pMsg);

    return 0;
}


/*设置长短信标识*/
UInt32 CSMSCluster::smsClusterSetLongFlagRedis( string strSmsId, string strPhone )
{
    TRedisReq* pDividedReq = new TRedisReq();
    string strKey = "";
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_strKey = "sendsms:" + strSmsId + "_" + strPhone;
    strKey = pDividedReq->m_strKey;
    pDividedReq->m_RedisCmd.assign("set ");
    pDividedReq->m_RedisCmd.append(pDividedReq->m_strKey);
    pDividedReq->m_RedisCmd.append(" 1");
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDividedReq);

    //set redis timeout
    TRedisReq* pExpireDividedReq = new TRedisReq();
    pExpireDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pExpireDividedReq->m_strKey = strKey;       //share same index
    pExpireDividedReq->m_RedisCmd.assign("EXPIRE ");
    pExpireDividedReq->m_RedisCmd.append(pExpireDividedReq->m_strKey);
    pExpireDividedReq->m_RedisCmd.append(" 259200");
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pExpireDividedReq );

    LogDebug("[%s:%s] Set LongMsg Flag [%s].",
              strSmsId.data(), strPhone.data(), strKey.data());

    return CC_TX_RET_SUCCESS;

}

UInt32 CSMSCluster::smsClusterSetMoRedis( SmsHttpInfo* HttpMsg )
{
    const int bufsize = 2048;
    char cmd[bufsize] = {0};
    char cmdkey[1024] = {0};

    string strSign = "null";
    string strSignPort = "null";
    string strDisplayNum = "null";
    TRedisReq* req = NULL;

    if (HttpMsg->m_uChannelType >= 0 && HttpMsg->m_uChannelType <= 3)
    {
        req = new TRedisReq();
        if (false == HttpMsg->m_strSign.empty())
        {
            strSign.assign(HttpMsg->m_strSign);
        }
        if (false == HttpMsg->m_strSignPort.empty())
        {
            strSignPort.assign(HttpMsg->m_strSignPort);
        }
        if (false == HttpMsg->m_strDisplayNum.empty())
        {
            strDisplayNum.assign(HttpMsg->m_strDisplayNum);
        }
    }

    if ((1 == HttpMsg->m_uChannelType ) || ( 2 == HttpMsg->m_uChannelType ))
    {
        /////HMSET sign_mo_port:channelid_showphone:phone clientid * sign * signport * userport * mourl * smsfrom*

        string strShowPhone;
        if (1 == HttpMsg->m_uChannelType)
        {
            strShowPhone = HttpMsg->m_strUcpaasPort;
        }
        else
        {
            strShowPhone = HttpMsg->m_strUcpaasPort + HttpMsg->m_strDisplayNum;
        }
        if (HttpMsg->m_uExtendSize > 21)
        {
            HttpMsg->m_uExtendSize = 21;
        }

        if ( strShowPhone.length() > HttpMsg->m_uExtendSize )
        {
            strShowPhone = strShowPhone.substr( 0, HttpMsg->m_uExtendSize );
        }

        snprintf(cmdkey, 1024, "sign_mo_port:%d_%s:%s", HttpMsg->m_iChannelId, strShowPhone.data(),HttpMsg->m_strPhone.data());

        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
                cmdkey,
                HttpMsg->m_strClientId.data(),
                strSign.data(),
                strSignPort.data(),
                strDisplayNum.data(),
                HttpMsg->m_uC2sId,
                HttpMsg->m_uSmsFrom);

        req->m_RedisCmd = cmd;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        req->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, req );

        LogDebug("==http sign_mo_port==smsid:%s,phone:%s,redisCmd:%s.", HttpMsg->m_strSmsId.data(), HttpMsg->m_strPhone.data(), cmd );

        TRedisReq* pDel = new TRedisReq();
        memset(cmd, 0x00, sizeof(cmd));
        snprintf(cmd, bufsize, "EXPIRE %s %u", cmdkey, HttpMsg->m_uSignMoPortExpire);
        pDel->m_RedisCmd = cmd;
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDel );
    }
    else if ((0 == HttpMsg->m_uChannelType ) || ( 3 == HttpMsg->m_uChannelType ))
    {
        /////HMSET moport:channelid_showphone client* sign* signport* userport* mourl* smsfrom*

        string strShowPhone = HttpMsg->m_strUcpaasPort + HttpMsg->m_strSignPort + HttpMsg->m_strDisplayNum;
        if (HttpMsg->m_uExtendSize > 21)
        {
            HttpMsg->m_uExtendSize = 21;
        }

        if ( strShowPhone.length() > HttpMsg->m_uExtendSize )
        {
            strShowPhone = strShowPhone.substr( 0, HttpMsg->m_uExtendSize );
        }

        snprintf(cmdkey, 1024, "moport:%d_%s", HttpMsg->m_iChannelId, strShowPhone.data());

        snprintf(cmd, bufsize,"HMSET %s clientid %s sign %s signport %s userport %s mourl %lu  smsfrom %d",
                cmdkey,
                HttpMsg->m_strClientId.data(),
                strSign.data(),
                strSignPort.data(),
                strDisplayNum.data(),
                HttpMsg->m_uC2sId,
                HttpMsg->m_uSmsFrom);

        req->m_RedisCmd = cmd;
        req->m_iMsgType = MSGTYPE_REDIS_REQ;
        req->m_strKey = cmdkey;
        SelectRedisThreadPoolIndex( g_pRedisThreadPool, req );

        TRedisReq* pDel = new TRedisReq();
        pDel->m_RedisCmd = "EXPIRE ";
        pDel->m_RedisCmd.append(cmdkey);
        pDel->m_RedisCmd.append("  259200");
        pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDel->m_strKey = cmdkey;

        SelectRedisThreadPoolIndex( g_pRedisThreadPool, pDel );

        LogDebug("==http moport==smsid:%s,phone:%s,redisCmd:%s.",
                 HttpMsg->m_strSmsId.data(), HttpMsg->m_strPhone.data(), cmd );

    }

    return CC_TX_RET_SUCCESS;

}


/*设置流水到redis*/
UInt32 CSMSCluster::smsRecordToRedis( smsCtxInfo_t *smsCtx, CSMSClusterStatusReq *req )
{

    SmsHttpInfo* HttpMsg = &smsCtx->http_msg;

    TRedisReq* pMsg = new TRedisReq();

    string strkey = "";
    strkey = "cluster_sms:" + Comm::int2str( HttpMsg->m_iChannelId) + "_" + req->txReport.m_strChannelSmsid+ "_" + HttpMsg->m_strPhone;

    string strCmd = "";
    strCmd.assign("HMSET  ");
    strCmd.append(strkey);
    ////sendTime
    strCmd.append("  sendTime  ");
    strCmd.append(http::UrlCode::UrlEncode(HttpMsg->m_strC2sTime));         //http::UrlCode::UrlEncode(pSms->m_strC2sTime)
    ////smsid
    strCmd.append("  smsid ");
    strCmd.append(HttpMsg->m_strSmsId);
    ////clientid
    strCmd.append("  clientid ");
    strCmd.append(HttpMsg->m_strClientId);
    ////smsfrom
    strCmd.append("  smsfrom ");
    char buf3[10];
    snprintf(buf3, 10,"%d", HttpMsg->m_uSmsFrom);
    strCmd.append(buf3);
    ////senduid
    strCmd.append("  senduid ");
    strCmd.append( req->txReport.m_strSmsUuid );
    ////longsms
    strCmd.append("  longsms ");
    char buf_divide[10];
    snprintf(buf_divide, 10,"%u", req->txReport.m_uTotalCount);
    strCmd.append(buf_divide);
    ////phone
    strCmd.append("  phone ");
    strCmd.append( HttpMsg->m_strPhone );
    ////c2sid
    char buf_c2sId[50] = {0};
    snprintf(buf_c2sId,50,"%lu",HttpMsg->m_uC2sId);
    strCmd.append("  c2sid ");
    strCmd.append(buf_c2sId);

    ////channelcnt
    char buf_channelCnt[50] = {0};
    snprintf(buf_channelCnt,50,"%u", req->txReport.m_uChanCnt);
    strCmd.append("  channelcnt ");
    strCmd.append(buf_channelCnt);

    ////submitdate
    char buf_submitDate[50] = {0};
    snprintf(buf_submitDate,50,"%ld",req->txReport.m_lSubmitDate);
    strCmd.append("  submitdate ");
    strCmd.append(buf_submitDate);

    if(HttpMsg->m_bResendCfgFlag && (req->txReport.m_uTotalCount < 2))
    {
        smsClusterSetRedisResendInfo(HttpMsg,strCmd);
    }
    pMsg->m_iMsgType = MSGTYPE_REDIS_REQ;
    pMsg->m_strKey= strkey;
    pMsg->m_RedisCmd = strCmd;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pMsg);

    LogDebug("[%s:%s] send msg to RedisThread strCmd[%s].",
              HttpMsg->m_strSmsId.data(), HttpMsg->m_strPhone.data(), strCmd.data());

    TRedisReq* pExpire = new TRedisReq();
    pExpire->m_iMsgType = MSGTYPE_REDIS_REQ;
    pExpire->m_strKey = strkey;
    pExpire->m_RedisCmd.assign("EXPIRE ");
    pExpire->m_RedisCmd.append(strkey);
    pExpire->m_RedisCmd.append("  259200");

    SelectRedisThreadPoolIndex( g_pRedisThreadPool, pExpire );

    /*设置长度信标志
       第一条设置*/
    if( req->txReport.m_uTotalCount > 0
        && req->txReport.m_iIndexNum == 1 )
    {
        if(req->txReport.m_uTotalCount > 1)
        {
            smsClusterSetLongFlagRedis(HttpMsg->m_strSmsId, HttpMsg->m_strPhone );
        }
        smsClusterSetMoRedis( HttpMsg );
    }

    return CC_TX_RET_SUCCESS;

}


/*插入到数据库*/
UInt32 CSMSCluster::smsRecordToDB( smsCtxInfo_t *smsCtx, CSMSClusterStatusReq *req )
{
    char sql[10240]  = {0};
    struct tm timenow = {0};
    time_t now = 0;

    char dateTime[64] = {0};
    char submitTime[64] = {0};
    char subretTime[64] = {0};
    UInt32 uDbFailedResendFlag = 0;
    SmsHttpInfo* HttpMsg = &smsCtx->http_msg;
    if((HttpMsg->m_uResendTimes > 0) && !HttpMsg->m_strFailedChannel.empty())
    {
        uDbFailedResendFlag = 1;
    }
    if ( req->txReport.m_lSendDate != 0 )
    {
        now = req->txReport.m_lSendDate;
    }
    else
    {
        now = time(NULL);
    }

    localtime_r(&now,&timenow);
    strftime(dateTime,sizeof(dateTime),"%Y%m%d%H%M%S",&timenow);

    string strRecordDate = HttpMsg->m_strC2sTime.substr( 0, 8 );

    if ( req->txReport.m_lSubmitDate != 0 )
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&req->txReport.m_lSubmitDate,&pTime);
        strftime(submitTime,sizeof(submitTime),"%Y%m%d%H%M%S",&pTime);
    }
    else
    {
        strftime(submitTime,sizeof(submitTime),"%Y%m%d%H%M%S",&timenow);
    }

    if ( req->txReport.m_lSubretDate != 0 )
    {
        struct tm pTime = {0};
        localtime_r((time_t*)&req->txReport.m_lSubretDate,&pTime);
        strftime(subretTime,sizeof(subretTime),"%Y%m%d%H%M%S",&pTime);
    }
    else
    {
        strftime(subretTime,sizeof(subretTime),"%Y%m%d%H%M%S",&timenow);
    }

    char contentSQL[2500] = { 0 };
    char strSubret[1024] = {0};
    char strSign[512] = {0};
    char strTempParam[2048] = {0};
    MYSQL* MysqlConn = g_pDisPathDBThreadPool->CDBGetConn();

    if( MysqlConn != NULL )
    {
        mysql_real_escape_string(MysqlConn, contentSQL, req->txReport.m_strContent.data(), req->txReport.m_strContent.length());
        mysql_real_escape_string(MysqlConn, strSubret, req->txReport.m_strSubRet.data(), req->txReport.m_strSubRet.length());
        mysql_real_escape_string(MysqlConn, strSign, req->txReport.m_strSign.data(), req->txReport.m_strSign.length());
        if (!HttpMsg->m_strTemplateParam.empty())
        {
            mysql_real_escape_string(MysqlConn, strTempParam, HttpMsg->m_strTemplateParam.data(),
                             HttpMsg->m_strTemplateParam.length());
        }
    }

    string strShowPhone = HttpMsg->m_strUcpaasPort + HttpMsg->m_strSignPort + HttpMsg->m_strDisplayNum;

    if ( strShowPhone.length() > HttpMsg->m_uExtendSize )
    {
        strShowPhone = strShowPhone.substr(0, HttpMsg->m_uExtendSize > 21 ? 21 : HttpMsg->m_uExtendSize );
    }

    char strBelongBusiness[32] = {0};
    snprintf( strBelongBusiness, sizeof(strBelongBusiness)-1, "%lu", HttpMsg->m_uBelong_business );

    char cBelongSale[32] = {0};
    snprintf(cBelongSale,32,"%lu", HttpMsg->m_uBelong_salse);
    string strBelongSale = cBelongSale;

    snprintf(sql,10240,"insert into t_sms_record_%d_%s(smsuuid,channelid,channeloperatorstype,channelremark,operatorstype,"
    "area,smsid,username,content,smscnt,state,phone,smsfrom,smsindex,costFee,channelsmsid,paytype,clientid,"
    "date,submit,submitdate,subret,subretdate,showphone,send_id,longsms,clientcnt,channelcnt,senddate,template_id,channel_tempid,"
    "temp_params,belong_sale, belong_business, sign, smstype, failed_resend_flag)  "
    " values('%s','%d','%d','%s','%d','%d','%s','%s','%s','%d','%d','%s','%u','%d','%f','%s','%d',"
    "'%s','%s','%s','%s','%s','%s','%s','%lu','%d','%d','%d','%s', %s, '%s', '%s', %s,%s,'%s', '%u','%u');",
    HttpMsg->m_uChannelIdentify, strRecordDate.data(),
    req->txReport.m_strSmsUuid.data(),
    HttpMsg->m_iChannelId,
    HttpMsg->m_uChannelOperatorsType,
    HttpMsg->m_strChannelRemark.data(),
    HttpMsg->m_uPhoneType,
    HttpMsg->m_uArea,
    HttpMsg->m_strSmsId.data(),
    HttpMsg->m_strUserName.data(),
    contentSQL,
    req->txReport.m_iSmsCnt,
    req->txReport.m_status,
    HttpMsg->m_strPhone.substr(0, 20).data(),
    HttpMsg->m_uSmsFrom,
    req->txReport.m_iIndexNum,
    (HttpMsg->m_fCostFee)*1000,
    req->txReport.m_strChannelSmsid.data(),
    HttpMsg->m_uPayType,
    HttpMsg->m_strClientId.data(),
    HttpMsg->m_strC2sTime.data(),
    req->txReport.m_strSubmit.data(),
    submitTime,
    strSubret,
    subretTime,
    strShowPhone.substr(0, 20).data(),
    g_uComponentId,
    req->txReport.m_uChanCnt > 1 ?  1 : 0,
    HttpMsg->m_uClientCnt,
    req->txReport.m_uChanCnt,
    dateTime,
    (true == HttpMsg->m_strTemplateId.empty()) ? "NULL" : HttpMsg->m_strTemplateId.data(),
    HttpMsg->m_strChannelTemplateId.data(),
    strTempParam,
    (!strBelongSale.compare("0")) ? "NULL" : strBelongSale.data(),
    ( 0 == HttpMsg->m_uBelong_business ) ? "NULL" : strBelongBusiness,
    ( true == HttpMsg->m_strSign.empty()) ? "NULL" : strSign,
    HttpMsg->m_uSmsType,
    uDbFailedResendFlag );

    TMQPublishReqMsg* pMQ = new TMQPublishReqMsg();
    pMQ->m_iMsgType = MSGTYPE_MQ_PUBLISH_REQ;
    pMQ->m_strExchange.assign(req->strExchange);
    pMQ->m_strRoutingKey.assign(req->strRoutingKey);
    pMQ->m_strData.assign(sql);
    pMQ->m_strData.append("RabbitMQFlag=");
    pMQ->m_strData.append( req->txReport.m_strSmsUuid);
    g_pRecordMQProducerThread->PostMsg(pMQ);

    return 0;
}

UInt32 CSMSCluster::smsRecordMutiDB(  CSMSClusterSession *session, CSMSClusterStatusReq *req )
{
    return 0;
}

/* 写流水redis */
UInt32 CSMSCluster::smsClusterSuccess(  CSMSClusterSession *session, CSMSClusterStatusReq *req )
{
    Int32 i = 1;
    smsCtxInfo_t *smsCtx = NULL;
    clusterContextMap::iterator msgItr = session->m_smsContextsMap.begin();
    string uuidUnique = req->txReport.m_strSmsUuid ;

    for(; msgItr != session->m_smsContextsMap.end(); msgItr++, i++ )
    {

        UInt32 saveStatue = req->txReport.m_status;
        smsCtx = msgItr->second;

        /* 转换smsid */
        smsp::mapStateResponse::iterator phoneItr = req->txReport.m_mapResponses.find( smsCtx->http_msg.m_strPhone );
        if( phoneItr != req->txReport.m_mapResponses.end())
        {
            req->txReport.m_strChannelSmsid = phoneItr->second._smsId;
            if( phoneItr->second._status !=  0 )
            {
                string strDesc = "";
                string strReportDesc = "";
                g_CChannelThreadPool->smsTxGetErrorCode(Comm::int2str(smsCtx->http_msg.m_iChannelId ),
                                phoneItr->second._desc, strDesc, strReportDesc );
                req->txReport.m_strYZXErrCode = strReportDesc;
                req->txReport.m_strSubRet     = strDesc;
                req->txReport.m_status        = SMS_STATUS_SUBMIT_FAIL;
            }
        }

        req->txReport.m_strSmsUuid = uuidUnique + "-" + Comm::int2str(i);
        smsRecordToDB( smsCtx, req );

        /*处理号码提交失败处理*/
        if( req->txReport.m_status == (UInt32)SMS_STATUS_SUBMIT_FAIL
            || req->txReport.m_status == (UInt32)SMS_STATUS_CONFIRM_SUCCESS)/*for jxtempnew*/
        {
            smsSendToReport(&smsCtx->http_msg, req );
            req->txReport.m_strYZXErrCode = "";
            req->txReport.m_strSubRet     = "";
            req->txReport.m_strSubmit     = "";
            req->txReport.m_status = saveStatue;
        }
        else
        {
            smsRecordToRedis( smsCtx, req );
        }
    }

    return 0;
}

/* 错误状态上报处理*/
UInt32 CSMSCluster::smsClusterFailReport( CSMSClusterSession *session, CSMSClusterStatusReq *req )
{
    Int32 i = 1;
    string uuidUnique = req->txReport.m_strSmsUuid ;
    clusterContextMap::iterator msgItr = session->m_smsContextsMap.begin();
    smsCtxInfo_t *psmsCtx = NULL;
    for(; msgItr !=  session->m_smsContextsMap.end(); msgItr ++, i++ )
    {
        req->txReport.m_strSmsUuid  = uuidUnique + "-" + Comm::int2str(i);
        psmsCtx = msgItr->second;

        smsp::mapStateResponse::iterator phoneItr = req->txReport.m_mapResponses.find(psmsCtx->http_msg.m_strPhone);
        if( phoneItr != req->txReport.m_mapResponses.end())
        {
            req->txReport.m_strChannelSmsid = phoneItr->second._smsId;
        }

        smsRecordToDB( psmsCtx, req );
        smsSendToReport(&psmsCtx->http_msg, req );
    }
    session->m_status= SMS_CLUSTER_STATUS_FAIL;
    return CC_TX_RET_SUCCESS;
}

/*reabck处理*/
UInt32 CSMSCluster::smsClusterReback(  CSMSClusterSession *session, CSMSClusterStatusReq *req )
{
    Int32 i = 1;
    string uuidUnique = req->txReport.m_strSmsUuid ;
    clusterContextMap::iterator msgItr = session->m_smsContextsMap.begin();
    for(; msgItr != session->m_smsContextsMap.end(); msgItr ++, i++ )
    {
        req->txReport.m_strSmsUuid = uuidUnique + "-" + Comm::int2str(i);
        smsSendToReback( msgItr->second, req );
    }
    session->m_status= SMS_CLUSTER_STATUS_REBACK;
    session->m_uRecvCount = session->m_uTotalCount;

    return CC_TX_RET_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////
UInt32 CSMSCluster::smsClusterTimeOut( TMsg * pMsg )
{
    UInt64 clusterId =  pMsg->m_iSeq;
    string SessionId =  pMsg->m_strSessionID;
    UInt32 ret = CC_TX_RET_OTHER;

    if( SessionId == SMS_CLUSTER_RECOVER_TIMEER )
    {
        LogWarn(" Recover TimeOut reset m_bReCover = false ");
        m_iRecoverCnt = 0;
        m_bReCover = false;
    }
    else
    {
        UInt64 uSessionId = Comm::strToUint64( pMsg->m_strSessionID );
        CSMSClusterSession *ss = smsClusterSessionGet( clusterId, uSessionId );
        if( NULL != ss )
        {
            if( ss->m_status <= SMS_CLUSTER_STATUS_WAIT ){
                ret = smsClusterSend( clusterId, ss );
            }else{
                ret = CC_TX_RET_SESSION_TIMEOUT;
            }

            if( ret != CC_TX_RET_SUCCESS )
            {
                LogWarn("TimeOut ClusterId:%lu, SessionId:%s State:%d ret:%u, TotalCnt:%u, RecvCnt:%u, Now Wait to Delete...",
                         clusterId,SessionId.c_str(), ss->m_status,
                         ret, ss->m_uTotalCount, ss->m_uRecvCount );
                smsClusterSessionDelete( clusterId , ss->m_uSessionId );
            }
        }else{
            LogWarn("Timeout ClusterId[%lu], SessionId[%s] Can't find Map",
                    clusterId, SessionId.data());
        }


    }

    return ret;

}

/***
    检查包是否需要组包处理
    如果在当前模块处理，返回true,
    否则其他模块处理
****/
UInt32 CSMSCluster::smsClusterCheck( SmsHttpInfo* httpMsg, UInt32 ClusterId )
{
    UInt64  uSessionId = 0;
    CSMSClusterSession *session = smsClusterSessionCreate( ClusterId, uSessionId,
                httpMsg->m_uclusterMaxNumber, httpMsg->m_uClusterMaxTime, 0, false );

    if( session == NULL ){
        LogError("ClusterId[%lu] Create Session Error");
        return CC_TX_RET_SESSION_INIT_ERR;
    }

    return smsClusterSaveContent( ClusterId, session, httpMsg );
}


/*处理请求消息*/
UInt32 CSMSCluster::smsClusterRequst( TMsg* pMsg )
{
    UInt32 ret = CC_TX_RET_OTHER ;
    TDispatchToHttpSendReqMsg *httpReq = ( TDispatchToHttpSendReqMsg * )pMsg;

    if( NULL == httpReq ){
        LogError("Get Msg Error NULL");
        return ret;
    }

    SmsHttpInfo* httpMsg = &httpReq->m_smsParam;

    LogDebug("ClusterType=%s, m_bReCove %s",
             httpMsg->m_uClusterType == 1 ? "ON" : "OFF",
             m_bReCover == true ? "true" : "false" );

    /*计算hash值*/
    if((httpMsg->m_uClusterType == 1)
        && ( m_bReCover == false ))
    {
        char   srcString[1024]={0};
        UInt64 ClusterId = 0;

        string strShowPhone = httpMsg->m_strUcpaasPort + httpMsg->m_strSignPort + httpMsg->m_strDisplayNum;
        if ( strShowPhone.length() > httpMsg->m_uExtendSize )
        {
            strShowPhone = strShowPhone.substr(0, httpMsg->m_uExtendSize > 21 ? 21 : httpMsg->m_uExtendSize );
        }

        snprintf( srcString, sizeof(srcString), "%d|%s|%s|%s|%s|%s",
                    httpMsg->m_iChannelId,
                    httpMsg->m_strClientId.c_str(),
                    httpMsg->m_strContent.c_str(),
                    httpMsg->m_strSign.c_str(),
                    strShowPhone.c_str(),
                    httpMsg->m_strTemplateId.c_str());

        string stringHash( srcString );
        boost::hash<std::string> hash_fn;
        ClusterId = hash_fn(stringHash);

        LogDebug("ClusterId:[%lu], ClusterInfo [%s]", ClusterId, srcString );
        ret = smsClusterCheck( httpMsg, ClusterId );

    }

    /*组包逻辑处理失败
       后直接发送*/
    if( ret != CC_TX_RET_SUCCESS )
    {
        smsCtxInfo_t smsCxt;
        smsCxt.http_msg = *httpMsg;
        ret = smsSendToHttp( &smsCxt, 0, SMS_CLUSTER_TX_REBACK, 0, "" );
    }

    return ret;
}

/*redis 保存数据封装*/
UInt32 CSMSCluster::smsClusterDataDetailEncode( SmsHttpInfo* httpMsg, string &outString )
{
    Json::FastWriter fast_writer;
    Json::Value Jsondata;
    char uStr[128] = {0};

    Jsondata["clientId"] = Json::Value(httpMsg->m_strClientId);
    Jsondata["userName"] = Json::Value(httpMsg->m_strUserName);
    Jsondata["smsId"] = Json::Value(httpMsg->m_strSmsId);
    Jsondata["phone"] = Json::Value(httpMsg->m_strPhone);

    Jsondata["sign"] = Json::Value(httpMsg->m_strSign);
    Jsondata["content"] = Json::Value(httpMsg->m_strContent);
    Jsondata["channelRemark"] = Json::Value(httpMsg->m_strChannelRemark);
    Jsondata["sendData"] = Json::Value(httpMsg->m_strSendData);
    Jsondata["sendUrl"] = Json::Value(httpMsg->m_strSendUrl);
    Jsondata["oauthUrl"] = Json::Value(httpMsg->m_strOauthUrl);
    Jsondata["oauthData"] = Json::Value(httpMsg->m_strOauthData);
    Jsondata["smsType"] = Json::Value(httpMsg->m_uSmsType);
    Jsondata["smsFrom"] = Json::Value(httpMsg->m_uSmsFrom);
    Jsondata["payType"] = Json::Value(httpMsg->m_uPayType);
    Jsondata["showSignType"] = Json::Value(httpMsg->m_uShowSignType);
    Jsondata["ucpaasPort"] = Json::Value(httpMsg->m_strUcpaasPort);
    Jsondata["signPort"] = Json::Value(httpMsg->m_strSignPort);
    Jsondata["displayNum"] = Json::Value(httpMsg->m_strDisplayNum);

    snprintf(uStr, sizeof(uStr), "%lu", httpMsg->m_uAccessId );
    Jsondata["accessId"] = Json::Value(uStr);

    Jsondata["area"] = Json::Value(httpMsg->m_uArea);
    Jsondata["channelId"] = Json::Value(httpMsg->m_iChannelId);

    snprintf(uStr, sizeof(uStr), "%lf", httpMsg->m_lSaleFee );
    Jsondata["saleFee"] = Json::Value( uStr );

    Jsondata["costFee"] = Json::Value(httpMsg->m_fCostFee);
    Jsondata["phoneType"] = Json::Value(httpMsg->m_uPhoneType);
    Jsondata["accessids"] = Json::Value(httpMsg->m_strAccessids);
    Jsondata["clientCnt"] = Json::Value(httpMsg->m_uClientCnt);
    Jsondata["processTimes"] = Json::Value(httpMsg->m_uProcessTimes);
    Jsondata["longSms"] = Json::Value(httpMsg->m_uLongSms);

    snprintf(uStr, sizeof(uStr), "%lu", httpMsg->m_uC2sId );
    Jsondata["c2sId"] = Json::Value(uStr);

    snprintf(uStr, sizeof(uStr), "%lu", httpMsg->m_uSendDate );
    Jsondata["sendDate"] = Json::Value(uStr);

    Jsondata["c2sTime"] = Json::Value(httpMsg->m_strC2sTime);
    Jsondata["errorDate"] = Json::Value(httpMsg->m_strErrorDate);
    Jsondata["codeType"] = Json::Value(httpMsg->m_strCodeType);
    Jsondata["channelName"] = Json::Value(httpMsg->m_strChannelName);

    Jsondata["sendType"] = Json::Value(httpMsg->m_uSendType);
    Jsondata["extendSize"] = Json::Value(httpMsg->m_uExtendSize);
    Jsondata["channelType"] = Json::Value(httpMsg->m_uChannelType);
    Jsondata["channelOperatorsType"] = Json::Value(httpMsg->m_uChannelOperatorsType);

    Jsondata["channelIdentify"] = Json::Value(httpMsg->m_uChannelIdentify);
    Jsondata["templateId"] = Json::Value(httpMsg->m_strTemplateId);
    Jsondata["templateParam"] = Json::Value(httpMsg->m_strTemplateParam);
    Jsondata["channelTemplateId"] = Json::Value(httpMsg->m_strChannelTemplateId);
    Jsondata["masUserId"] = Json::Value(httpMsg->m_strMasUserId);
    Jsondata["masAccessToken"] = Json::Value(httpMsg->m_strMasAccessToken);

    snprintf(uStr, sizeof(uStr), "%lu", httpMsg->m_uBelong_salse );
    Jsondata["belong_salse"] = Json::Value(uStr);

    snprintf(uStr, sizeof(uStr), "%lu", httpMsg->m_uBelong_business );
    Jsondata["belong_business"] = Json::Value(uStr);

    Jsondata["clusterType"] = Json::Value(httpMsg->m_uClusterType);
    Jsondata["clusterMaxNumber"] = Json::Value(httpMsg->m_uclusterMaxNumber);
    Jsondata["clusterMaxTime"] = Json::Value(httpMsg->m_uClusterMaxTime );

    Jsondata["channelOverrateDate"] = Json::Value(httpMsg->m_strChannelOverrate_access);
    Jsondata["oriChannelid"] = Json::Value(httpMsg->m_strOriChannelId );

    string tmpString = fast_writer.write( Jsondata );

    outString = Base64::Encode(tmpString);

    return  CC_TX_RET_SUCCESS;
}


UInt32 CSMSCluster::smsClusterDataDetailDecode( SmsHttpInfo* httpMsg, string &inString )
{
    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    std::string js;

    string outString = Base64::Decode( inString );

    if ( json_format_string(outString, js) < 0 )
    {
        LogError("==err== json message error, req[%s]", outString.data());
        return CC_TX_RET_PARAM_ERROR;
    }

    if(!reader.parse(js,root))
    {
        LogError("==err== json parse is failed, req[%s]", outString.data());
        return CC_TX_RET_PARAM_ERROR;
    }

    httpMsg->m_strClientId = root["clientId"].asString();
    httpMsg->m_strUserName = root["userName"].asString();
    httpMsg->m_strSmsId =    root["smsId"].asString();
    httpMsg->m_strPhone =    root["phone"].asString();
    httpMsg->m_strSign = root["sign"].asString();
    httpMsg->m_strContent = root["content"].asString();
    httpMsg->m_strChannelRemark = root["channelRemark"].asString();
    httpMsg->m_strSendData = root["sendData"].asString();
    httpMsg->m_strSendUrl = root["sendUrl"].asString();
    httpMsg->m_strOauthUrl = root["oauthUrl"].asString();
    httpMsg->m_strOauthData = root["oauthData"].asString();

    httpMsg->m_uSmsType  = root["smsType"].asUInt();
    httpMsg->m_uSmsFrom  = root["smsFrom"].asUInt();
    httpMsg->m_uPayType  = root["payType"].asUInt();
    httpMsg->m_uShowSignType =  root["showSignType"].asUInt();
    httpMsg->m_strUcpaasPort = root["ucpaasPort"].asString();
    httpMsg->m_strSignPort =   root["signPort"].asString();
    httpMsg->m_strDisplayNum = root["displayNum"].asString();
    httpMsg->m_uAccessId = Comm::strToUint64(root["accessId"].asCString());

    httpMsg->m_uArea = root["area"].asUInt();
    httpMsg->m_iChannelId = root["channelId"].asInt();
    httpMsg->m_lSaleFee  = atof(root["saleFee"].asCString());

    httpMsg->m_fCostFee = root["costFee"].asDouble();
    httpMsg->m_uPhoneType = root["phoneType"].asUInt();
    httpMsg->m_strAccessids = root["accessids"].asString();
    httpMsg->m_uClientCnt = root["clientCnt"].asUInt();
    httpMsg->m_uProcessTimes = root["processTimes"].asUInt();
    httpMsg->m_uC2sId  = Comm::strToUint64(root["c2sId"].asCString());
    httpMsg->m_uSendDate = Comm::strToUint64(root["sendDate"].asCString());

    httpMsg->m_strC2sTime = root["c2sTime"].asString();
    httpMsg->m_strErrorDate = root["errorDate"].asString();
    httpMsg->m_strCodeType = root["codeType"].asString();
    httpMsg->m_strChannelName = root["channelName"].asString();

    httpMsg->m_uLongSms      = root["longSms"].asUInt();

    httpMsg->m_uSendType = root["sendType"].asUInt();

    httpMsg->m_uExtendSize = root["extendSize"].asUInt();

    httpMsg->m_uChannelType = root["channelType"].asUInt();

    httpMsg->m_uChannelOperatorsType = root["channelOperatorsType"].asUInt();

    httpMsg->m_uChannelIdentify = root["channelIdentify"].asUInt();
    httpMsg->m_strTemplateId = root["templateId"].asString();
    httpMsg->m_strTemplateParam = root["templateParam"].asString();
    httpMsg->m_strChannelTemplateId = root["channelTemplateId"].asString();

    httpMsg->m_strMasUserId = root["masUserId"].asString();
    httpMsg->m_strMasAccessToken = root["masAccessToken"].asString();

    httpMsg->m_uBelong_salse = Comm::strToUint64( root["belong_salse"].asCString());
    httpMsg->m_uBelong_business  = Comm::strToUint64(root["belong_business"].asCString());

    httpMsg->m_uClusterType = root["clusterType"].asUInt();
    httpMsg->m_uclusterMaxNumber = root["clusterMaxNumber"].asUInt();
    httpMsg->m_uClusterMaxTime = root["clusterMaxTime"].asUInt();

    httpMsg->m_strChannelOverrate_access = root["channelOverrateDate"].asString();
    httpMsg->m_strOriChannelId = root["oriChannelid"].asString();

    return CC_TX_RET_SUCCESS;

}


/*redis索引数据*/
UInt32 CSMSCluster::smsClusterDataIndexEncode( UInt32 uMaxTime, UInt32 uMaxNum, string &outString )
{
    Json::FastWriter fast_writer;
    Json::Value Jsondata;
    char uStr[128] = {0};

    UInt64 endTime = time( NULL ) +  uMaxTime / 1000 ;

    snprintf( uStr, sizeof(uStr), "%lu", endTime );
    Jsondata["endTime"] = Json::Value(uStr);
    Jsondata["maxNum"] = Json::Value(uMaxNum);

    string tmpString = fast_writer.write( Jsondata );
    outString = Base64::Encode( tmpString );

    return CC_TX_RET_SUCCESS;
}


/*解析索引数据*/
UInt32 CSMSCluster::smsClusterDataIndexDecode( string inString, UInt64 &uEndTime, UInt32 &uMaxNum )
{
    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    std::string js;

    string intString = Base64::Decode(inString);
    if ( json_format_string( intString, js ) < 0 )
    {
        LogError("==err== json message error, req[%s]", intString.data());
        return CC_TX_RET_PARAM_ERROR;
    }

    if( true != reader.parse( js,root ))
    {
        LogError("==err== json parse is failed, req[%s]", js.data());
        return CC_TX_RET_PARAM_ERROR;
    }

    string endTimeStr = string(root["endTime"].asCString());

    uEndTime = Comm::strToUint64(endTimeStr);
    uMaxNum = root["maxNum"].asUInt();
    LogDebug("endTime:%lu, uMaxNum:%u", uEndTime, uMaxNum);

    return CC_TX_RET_SUCCESS;

}


/*保存数据到redis*/
UInt32 CSMSCluster::smsClusterSaveToRedis( UInt64 ClusterId, UInt64 uSessionId,
                    UInt64 uMsgSeq, SmsHttpInfo* httpMsg, bool bSetIdx )
{
    string HttpDataString = "";
    char strKey[ 1024 ] = { 0 };
    char strCmd[ 2048 ] = { 0 };

    /*增加组包数据*/
    smsClusterDataDetailEncode( httpMsg,  HttpDataString );
    snprintf( strKey, sizeof(strKey), "cluster_dataDetail:%lu_%lu_%lu ", g_uComponentId, ClusterId, uSessionId );
    snprintf( strCmd, sizeof(strCmd), "HSET %s %lu ", strKey, uMsgSeq  );

    TRedisReq* pDividedReq = new TRedisReq();
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_strKey = string(strKey);
    pDividedReq->m_RedisCmd = string( strCmd ) + " ";
    pDividedReq->m_RedisCmd.append( HttpDataString );
    SelectRedisThreadPoolIndex( g_pRedisThreadPool, pDividedReq );

    /*设置过期时间*/
    TRedisReq* pDel = new TRedisReq();
    pDel->m_RedisCmd = "EXPIRE ";
    pDel->m_RedisCmd.append(strKey);
    pDel->m_RedisCmd.append("  259200");
    pDel->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDel->m_strKey = strKey;
    SelectRedisThreadPoolIndex(g_pRedisThreadPool,pDel);

    /*设置索引*/
    if( bSetIdx )
    {
        string HttpDataIdxString = "";
        /*设置组包索引*/
        smsClusterDataIndexEncode( httpMsg->m_uClusterMaxTime,
                      httpMsg->m_uclusterMaxNumber, HttpDataIdxString );
        snprintf( strKey, sizeof(strKey), "cluster_dataIndex:%lu ", g_uComponentId );
        snprintf( strCmd, sizeof(strCmd), "HSETNX %s %lu_%lu %s",
                 strKey, ClusterId, uSessionId, HttpDataIdxString.c_str());

        TRedisReq* pDividedReq2 = new TRedisReq();
        pDividedReq2->m_iMsgType = MSGTYPE_REDIS_REQ;
        pDividedReq2->m_strKey = string(strKey);
        pDividedReq2->m_RedisCmd.assign(strCmd);
        SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDividedReq2 );
    }

    return CC_TX_RET_SUCCESS;

}

/*删除redis中数据*/
UInt32 CSMSCluster::smsClusterDeleteRedisData( UInt64 ClusterId, CSMSClusterSession *session )
{
    string HttpDataString = "";
    char strKey[ 1024 ] = { 0 };
    char strCmd[ 2048 ] = { 0 };

    ClusterSessionMap::iterator clusterItr = m_mapClusterSession.find( ClusterId );
    if( clusterItr == m_mapClusterSession.end())
    {
        LogError("Session %lu Delete ", ClusterId );
        return CC_TX_RET_SESSION_NOT_FIND;
    }

    /*删除组包数据*/
    snprintf( strKey, sizeof(strKey), "cluster_dataDetail:%lu_%lu_%lu  ",
            g_uComponentId, ClusterId, session->m_uSessionId );
    TRedisReq* pDividedReq = new TRedisReq();
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_strKey = string(strKey);
    pDividedReq->m_RedisCmd.assign(" HDEL ");
    pDividedReq->m_RedisCmd.append(strKey);
    pDividedReq->m_RedisCmd.append(session->m_strRedisKey );
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDividedReq );

    /*删除组包索引*/
    snprintf( strKey, sizeof(strKey), "cluster_dataIndex:%lu ", g_uComponentId );
    snprintf( strCmd, sizeof(strCmd), "HDEL %s %lu_%lu",
            strKey, ClusterId, session->m_uSessionId );
    TRedisReq* pDividedReq2 = new TRedisReq();
    pDividedReq2->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq2->m_strKey = string(strKey);
    pDividedReq2->m_RedisCmd.assign(strCmd);
    SelectRedisThreadPoolIndex(g_pRedisThreadPool, pDividedReq2 );

    return  CC_TX_RET_SUCCESS;

}


/*加载数据索引*/
UInt32 CSMSCluster::smsClusterLoadDataIdx()
{
    char strKey[ 1024 ] = { 0 };

    /*设置同步标志*/
    m_bReCover = true;

    /*设置组包索引*/
    snprintf( strKey, sizeof(strKey), "cluster_dataIndex:%lu ",  g_uComponentId );
    TRedisReq* pDividedReq = new TRedisReq();
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_strSessionID = "cluster_loadDataIdx";
    pDividedReq->m_pSender = this;
    pDividedReq->m_strKey = string(strKey);
    pDividedReq->m_RedisCmd.assign("HGETALL ");
    pDividedReq->m_RedisCmd.append( strKey );
    SelectRedisThreadPoolIndex( g_pRedisThreadPool, pDividedReq );
    m_Timer = SetTimer(0, SMS_CLUSTER_RECOVER_TIMEER, CLUSTER_SESSION_RECOVER_TIME );

    return CC_TX_RET_SUCCESS;
}


/*加载数据*/
UInt32 CSMSCluster::smsClusterLoadDataDetail( UInt64 ClusterId, UInt64 uSessionId )
{
    char strKey[ 1024 ] = { 0 };

    m_iRecoverCnt++;

    snprintf( strKey, sizeof(strKey), "cluster_dataDetail:%lu_%lu_%lu ",
            g_uComponentId, ClusterId, uSessionId );
    TRedisReq* pDividedReq = new TRedisReq();
    pDividedReq->m_iMsgType = MSGTYPE_REDIS_REQ;
    pDividedReq->m_iSeq     = ClusterId;
    pDividedReq->m_strSessionID = "cluster_loadDataDetail";
    pDividedReq->m_pSender = this;
    pDividedReq->m_strKey = string(strKey);
    pDividedReq->m_RedisCmd.assign("HGETALL ");
    pDividedReq->m_RedisCmd.append( strKey );
    SelectRedisThreadPoolIndex( g_pRedisThreadPool, pDividedReq );

    return CC_TX_RET_SUCCESS;

}

/*redis返回数据加载内容*/
UInt32 CSMSCluster::smsClusterLoadDataRedisRsp( UInt64 ClusterId, UInt64 uSessionId, string redisKey, string redisData )
{
    UInt32 ret = CC_TX_RET_SUCCESS;
    SmsHttpInfo httpMsg;

    CSMSClusterSession *session = smsClusterSessionGet( ClusterId, uSessionId, false );
    if( session == NULL )
    {
        LogError("Session %lu not Find", ClusterId );
        return CC_TX_RET_SESSION_NOT_FIND;
    }

    session->m_strRedisKey.append(redisKey);
    session->m_strRedisKey.append(" ");

    if( CC_TX_RET_SUCCESS != smsClusterDataDetailDecode( &httpMsg, redisData ))
    {
        LogWarn("Decode DataDetail Error, ClusterId:%lu, Data:%s",
                ClusterId, redisData.c_str());
        return CC_TX_RET_PARAM_ERROR;
    }

    ret = smsClusterSaveContent( ClusterId, session, &httpMsg );

    return ret;
}

/*redis返回数据加载索引*/
UInt32 CSMSCluster::smsClusterLoadDataIndexRedisRsp( UInt64 ClusterId, UInt64 sessionId, string redisData )
{
    UInt32 ret = CC_TX_RET_SUCCESS;
    UInt32 uMaxNum = 0;
    UInt64 uEndTime = 0;

    ret = smsClusterDataIndexDecode( redisData, uEndTime, uMaxNum );
    if(ret != CC_TX_RET_SUCCESS ){
        LogError("Decode DataIndex Error CluserId:%lu, Data:%s",
                 ClusterId, redisData.c_str());
        return ret;
    }

    smsClusterSessionCreate( ClusterId, sessionId, uMaxNum, CLUSTER_MAX_TIME, uEndTime, true );
    LogDebug("XXXX ClusterId:%lu, SessionId:%lu", ClusterId, sessionId );

    ret = smsClusterLoadDataDetail( ClusterId, sessionId );

    return ret ;

}


/* 解析响应数据*/
UInt32 CSMSCluster::smsClusterLoadRedisRsp( TMsg *pMsg )
{
    TRedisResp* pResp = (TRedisResp*)pMsg;
    if ((NULL == pResp->m_RedisReply )
        || (pResp->m_RedisReply->type == REDIS_REPLY_ERROR)
        || (pResp->m_RedisReply->type == REDIS_REPLY_NIL)
        ||((pResp->m_RedisReply->type == REDIS_REPLY_ARRAY)
        && (pResp->m_RedisReply->elements == 0 )))
    {
        LogWarn("smsCluster Reload %s Error m_iRecoverCnt:%d",
                 pMsg->m_strSessionID.c_str(), m_iRecoverCnt );
        if( --m_iRecoverCnt <= 0 )
        {
            SAFE_DELETE(m_Timer);
            m_bReCover = false;
        }

        if( pResp->m_RedisReply != NULL ){
            freeReply(pResp->m_RedisReply);
        }

        return CC_TX_RET_REDIS_ERROR;
    }

    if( pMsg->m_strSessionID == "cluster_loadDataDetail" )
    {
        UInt64 uSessionId = 0;
        std::size_t found = pResp->m_RedisCmd.rfind( "_" );
        if ( found != std::string::npos ) {
            uSessionId = Comm::strToUint64( pResp->m_RedisCmd.substr( found + 1 ));
        }

        if( pResp->m_RedisReply->type == REDIS_REPLY_ARRAY )
        {
            for( UInt32 i = 0; i < pResp->m_RedisReply->elements; i = i+ 2 )
            {
                smsClusterLoadDataRedisRsp( pMsg->m_iSeq, uSessionId, pResp->m_RedisReply->element[i]->str,
                            pResp->m_RedisReply->element[i+1]->str );
            }

            if( --m_iRecoverCnt <= 0 ){
                SAFE_DELETE(m_Timer);
                m_bReCover = false;
            }
        }

    }
    else if ( pMsg->m_strSessionID == "cluster_loadDataIdx" )
    {
        if( pResp->m_RedisReply->type == REDIS_REPLY_ARRAY )
        {
            for( UInt32 i = 0; i< pResp->m_RedisReply->elements; i = i+ 2 )
            {
                /* 解析clusterId  clusterid_sessionid */
                vector<string> strVec;
                string strClusterId( pResp->m_RedisReply->element[i]->str );
                Comm::split( strClusterId, "_", strVec );
                if( strVec.size() >= 2 )
                {
                    UInt64 uClusterId = Comm::strToUint64( strVec.at(0) );
                    UInt64 uSessionId = Comm::strToUint64( strVec.at(1) );
                    smsClusterLoadDataIndexRedisRsp( uClusterId, uSessionId, pResp->m_RedisReply->element[i+1]->str );
                }
            }
        }
    }

    LogDebug("redis rsp Size :%d, m_iRecoverCnt:%u", pResp->m_RedisReply->elements, m_iRecoverCnt );
    freeReply(pResp->m_RedisReply);

    return CC_TX_RET_SUCCESS;;
}


/*保存发送对象*/
UInt32 CSMSCluster::smsClusterSaveContent( UInt32 ClusterId, CSMSClusterSession *session, SmsHttpInfo* httpMsg  )
{
    if( session == NULL )
    {
        LogWarn("Cluster[%lu] Create Session Error, smsid:%s, phone:%s",
                httpMsg->m_strSmsId.data(),
                httpMsg->m_strPhone.data());
        return CC_TX_RET_SESSION_INIT_ERR;
    }

    /*检查号码是否重复*/
    std::pair<std::set<string>::iterator, bool> ret;
    ret = session->m_strPhones.insert( httpMsg->m_strPhone );
    if( ret.second != true ){
        LogWarn("CluserId:%lu, SmsId:%s, Phone %s Exsit",
                ClusterId, httpMsg->m_strSmsId.c_str(),
                httpMsg->m_strPhone.c_str());
        return CC_TX_RET_PHONE_EXSIT;
    }

    UInt64 uSeq = m_pSnMng->getSn();
    smsCtxInfo_t *pSmsCtx = new smsCtxInfo_t();
    pSmsCtx->http_msg = *httpMsg;
    pSmsCtx->smsId    = httpMsg->m_strSmsId;
    session->m_smsContextsMap[ uSeq ] =  pSmsCtx;
    session->m_uClusterRecvNumber++;

    /*非同步模式下才去存储数据到redis*/
    if( session->m_bRecover == false )
    {
        char keyStr[128] = {0};
        snprintf( keyStr, sizeof(keyStr), "%lu", uSeq );
        session->m_strRedisKey.append(keyStr);
        session->m_strRedisKey.append(" ");
        smsClusterSaveToRedis( ClusterId, session->m_uSessionId, uSeq, httpMsg, session->m_status == SMS_CLUSTER_STATUS_INIT );
    }
    else
    {
        /*重启时取会话时间和组包超时时间中的最大值*/
        if( session->m_uClusterMaxTime == CLUSTER_MAX_TIME )
        {
            if( httpMsg->m_uClusterMaxTime > CLUSTER_SESSION_MAX_TIME_MS ){
                session->m_uClusterMaxTime= httpMsg->m_uClusterMaxTime;
            }else{
                session->m_uClusterMaxTime = CLUSTER_SESSION_MAX_TIME_MS;
            }
            SAFE_DELETE( session->m_pWheelTime );
            session->m_pWheelTime = SetTimer( ClusterId, Comm::int2str(session->m_uSessionId), session->m_uClusterMaxTime );
        }
    }

    LogDebug("Cluster Save ClusterId:%lu, SessionId:%lu, smsid:%s, Phone:%s, Size:%d, MaxSize:%d, MaxTime:%d, EndTime:%lu",
          ClusterId, session->m_uSessionId, pSmsCtx->smsId.c_str(), httpMsg->m_strPhone.c_str(),
          session->m_uClusterRecvNumber,session->m_uclusterMaxNumber,
          session->m_uClusterMaxTime, session->m_uEndTime );

    /*更新会话状态*/
    session->m_status = SMS_CLUSTER_STATUS_WAIT;

    if( session->m_uClusterRecvNumber>= session->m_uclusterMaxNumber )
    {
        return smsClusterSend( ClusterId, session );
    }

    return CC_TX_RET_SUCCESS;

}


/*组包发送短信到send模块*/
UInt32 CSMSCluster::smsClusterSend(UInt64 uClusterId, CSMSClusterSession *session )
{
    string strPhones;
    smsCtxInfo_t *smsCtx = NULL;

    /* 获取一个发送*/
    clusterContextMap::iterator ctxItr = session->m_smsContextsMap.begin();
    if( ctxItr == session->m_smsContextsMap.end()) {
        LogError("ClusterId:%lu, Context NULL", uClusterId );
        return CC_TX_RET_SESSION_RECORD_NULL;
    }

    smsCtx = ctxItr->second;

    /*合并手机号码*/
    std::set<string>::iterator phonesItr = session->m_strPhones.begin();
    if( phonesItr != session->m_strPhones.end()){
        strPhones = *phonesItr;
        phonesItr++;
    }

    for( ;phonesItr != session->m_strPhones.end(); phonesItr ++ )
    {
        strPhones += "," + *phonesItr;
    }

    /*保存发送的消息ID */
    session->m_strSubmitSmsid = smsCtx->smsId;

    /* 发送处理*/
    smsSendToHttp( smsCtx, uClusterId, SMS_CLUSTER_TX_SEND, session->m_uSessionId,
                strPhones, session->m_strPhones.size());

    session->m_status = SMS_CLUSTER_STATUS_SEND ;

    /*设置超时时间*/
    SAFE_DELETE( session->m_pWheelTime );
    session->m_pWheelTime = SetTimer(uClusterId, Comm::int2str(session->m_uSessionId), CLUSTER_SESSION_MAX_TIME_MS );

    return CC_TX_RET_SUCCESS;

}

CSMSClusterSession* CSMSCluster::smsClusterSessionGet( UInt64 uClusterId, UInt64 uSessionId, bool bGetOk )
{
    CSMSClusterSession *session = NULL;
    ClusterSessionMap::iterator clusterItr = m_mapClusterSession.find( uClusterId );
    if( clusterItr == m_mapClusterSession.end())
    {
        return NULL;
    }

    SessionsList &ssList = clusterItr->second;
    for( SessionsList::iterator itr = ssList.begin(); itr != ssList.end(); itr++ )
    {
        LogDebug("ClusterId:%lu, ListSessid:%lu, checkSessionId:%lu, bGetOk=%s",
                uClusterId, (*itr)->m_uSessionId, uSessionId,
                bGetOk ? "true": "false" );

        if( true == bGetOk )
        {
            if((*itr)->m_status <= SMS_CLUSTER_STATUS_WAIT ){
                session = *itr;
                break;
            }
        }
        else
        {
            if((*itr)->m_uSessionId == uSessionId ) {
                session = *itr;
                break;
            }
        }
    }

    return session;

}


CSMSClusterSession* CSMSCluster::smsClusterSessionCreate( UInt64 uClusterId, UInt64 &uSessionId,
                                Int32 maxNum, Int32 maxTime, UInt32 uEndTime, bool bRecover )
{
    CSMSClusterSession *session =  NULL;

    ClusterSessionMap::iterator clusterItr = m_mapClusterSession.find( uClusterId );
    if( clusterItr == m_mapClusterSession.end())
    {
        SessionsList sessionsList;
        sessionsList.clear();
        session = new CSMSClusterSession();
        session->m_uclusterMaxNumber = maxNum ;
        session->m_uClusterMaxTime   = maxTime;
        session->m_uEndTime = uEndTime;
        session->m_bRecover = bRecover;

        if( session->m_uclusterMaxNumber > CLUSTER_MAX_NUMBER ){
            LogWarn("ClusterNumer %u Too Large Limit %u ", session->m_uclusterMaxNumber, CLUSTER_MAX_NUMBER );
            session->m_uclusterMaxNumber = CLUSTER_MAX_NUMBER;
        }

        if( session->m_uClusterMaxTime > CLUSTER_MAX_TIME ){
            LogWarn("ClusterTime %u Too Large Limit %u ", session->m_uClusterMaxTime, CLUSTER_MAX_TIME );
            session->m_uClusterMaxTime = CLUSTER_MAX_TIME;
        }

        session->m_uSessionId = (session->m_bRecover ? uSessionId : m_pSnMng->getSeq64());
        session->m_pWheelTime = SetTimer( uClusterId, Comm::int2str(session->m_uSessionId), session->m_uClusterMaxTime );
        session->m_status = SMS_CLUSTER_STATUS_INIT;

        sessionsList.push_back( session );
        m_mapClusterSession[ uClusterId ] = sessionsList;

    }
    else
    {
        UInt64 m_uSessionId = m_pSnMng->getSeq64();
        SessionsList &sessionsList = clusterItr->second;
        for( SessionsList::iterator itr = sessionsList.begin(); itr != sessionsList.end(); itr++ )
        {
            if( bRecover ) {
                if( uSessionId == (*itr)->m_uSessionId ){
                    session = *itr;
                    break;
                }
            }
            else
            {
                /*寻找状态可用的会有*/
                if(( *itr )->m_status <= SMS_CLUSTER_STATUS_WAIT )
                {
                    session = *itr;
                    break;
                }

                /*获取最大的序列号*/
                if( m_uSessionId < (*itr)->m_uSessionId )
                {
                    m_pSnMng->setSeq64( 1, (*itr)->m_uSessionId );
                    m_uSessionId = (*itr)->m_uSessionId + 1;
                }
            }
        }

        if( NULL == session )
        {
            if( sessionsList.size() >= CLUSTER_CLUSTER_SESSION_CNT ){
                LogWarn("Cluster Session reach MaxNum %lu, Direct Send Now",
                        sessionsList.size(), CLUSTER_CLUSTER_SESSION_CNT );
                return NULL;
            }

            LogNotice("Cluster New Session Size [%lu:%lu]",
                      sessionsList.size() + 1, CLUSTER_CLUSTER_SESSION_CNT );

            session = new CSMSClusterSession();
            session->m_uclusterMaxNumber = maxNum ;
            session->m_uClusterMaxTime   = maxTime;
            session->m_uEndTime          = uEndTime;
            session->m_bRecover          = bRecover;

            if( session->m_uclusterMaxNumber > CLUSTER_MAX_NUMBER ){
                LogWarn("ClusterNumer %u Too Large Limit %u ", session->m_uclusterMaxNumber, CLUSTER_MAX_NUMBER );
                session->m_uclusterMaxNumber = CLUSTER_MAX_NUMBER;
            }

            if( session->m_uClusterMaxTime > CLUSTER_MAX_TIME ){
                LogWarn("ClusterTime %u Too Large Limit %u ", session->m_uClusterMaxTime, CLUSTER_MAX_TIME);
                session->m_uClusterMaxTime = CLUSTER_MAX_TIME;
            }

            session->m_uSessionId = session->m_bRecover ? uSessionId : m_uSessionId;
            session->m_pWheelTime = SetTimer( uClusterId, Comm::int2str(session->m_uSessionId), session->m_uClusterMaxTime );
            session->m_status = SMS_CLUSTER_STATUS_INIT;
            sessionsList.push_back( session );
        }
    }

    return session;

}

UInt32 CSMSCluster::smsClusterSessionDelete( UInt64 uClusterId, UInt64 uSessionId )
{
    ClusterSessionMap::iterator clusterItr = m_mapClusterSession.find( uClusterId );
    if( clusterItr == m_mapClusterSession.end())
    {
        LogError("CSMSCluster ClusterId[%lu], SessionId[%lu] Not Find", uClusterId, uSessionId );
        return CC_TX_RET_SESSION_NOT_FIND;
    }

    SessionsList &ssList = clusterItr->second;
    SessionsList::iterator itr = ssList.begin();
    for( ;itr != ssList.end(); itr++ )
    {
        if((*itr)->m_uSessionId == uSessionId ) {
            break;
        }
    }

    if( itr != ssList.end())
    {
        LogNotice("Cluster[%lu] Delete sessionId[%lu]", uClusterId, uSessionId );
        smsClusterDeleteRedisData( uClusterId, (*itr));
        SESSION_FREE(*itr);
        ssList.erase( itr );
        if(ssList.size() == 0 )
        {
            LogNotice("Cluster[%lu] all Session Free ", uClusterId );
            m_mapClusterSession.erase( clusterItr );
        }
    }

    return CC_TX_RET_SUCCESS ;

}


/*组发送成功设置流水,更新DB状态*/
UInt32 CSMSCluster::smsClusterStatusComm( TMsg *pMsg )
{
    CSMSClusterSession *session = NULL;
    UInt32 ret = CC_TX_RET_SUCCESS;

    CSMSClusterStatusReqMsg *statusReqMsg = ( CSMSClusterStatusReqMsg* ) pMsg;

    CSMSClusterStatusReq *statusReq = &statusReqMsg->txClusterStatus;

    if( NULL == statusReq ){
        return CC_TX_RET_PARAM_ERROR;
    }

    UInt64 uSessionId = Comm::strToUint64( pMsg->m_strSessionID );

    session = smsClusterSessionGet( statusReqMsg->m_iSeq, uSessionId );
    if( session == NULL ){
        LogError("ClusterId:%lu, SessionId:%u Session Not Exsit",
                statusReqMsg->m_iSeq, uSessionId );
        return CC_TX_RET_SESSION_NOT_FIND;
    }

    LogDebug(" smsTxStaus iSeq:%lu, Status:%u, SubmitSmsid:%s, Smsid:%s phoneSize:%d",
              statusReqMsg->m_iSeq, statusReq->txReport.m_status,
              session->m_strSubmitSmsid.c_str(), statusReq->txReport.m_strSmsid.c_str(),
              session->m_strPhones.size());

    if( session->m_strSubmitSmsid != statusReq->txReport.m_strSmsid ){
        ret = CC_TX_RET_SMSID_NOT_MATCH;
        goto GO_EXIT;
    }

    /*保存分段个数*/
    if ( session->m_uTotalCount == 0 ){
        session->m_uTotalCount = statusReq->txReport.m_uTotalCount;
    }

    session->m_uRecvCount++;
    session->m_status = statusReq->txReport.m_status;

    switch( statusReq->uMsgType )
    {
        /*状态上报*/
        case SMS_CLUSTER_MSG_TYPE_SUCCESS:
            ret = smsClusterSuccess( session, statusReq );
            break;

        case SMS_CLUSTER_MSG_TYPE_FAIL:
            ret = smsClusterFailReport( session, statusReq );
            break;

        /*需要reback 处理 */
        case SMS_CLUSTER_MSG_TYPE_REBACK:
            ret = smsClusterReback( session, statusReq );
            break;

        default:
            ret = CC_TX_RET_PARAM_ERROR ;
            ;;
    }

GO_EXIT:

    /* 已经全部处理完,  释放会话*/
    if( session->m_uTotalCount != 0
        && session->m_uRecvCount == session->m_uTotalCount )
    {
        if( ret != CC_TX_RET_SESSION_NOT_FIND )
        {
            LogDebug("clusterId[%lu] sessionID:[%lu], SendSmsid:%s Delete ************ ",
                     statusReqMsg->m_iSeq, uSessionId, session->m_strSubmitSmsid.c_str());
            smsClusterSessionDelete( statusReqMsg->m_iSeq, uSessionId );
        }
    }

    if( ret !=  CC_TX_RET_SUCCESS )
    {
        LogWarn("Comm Process Smsid:%s Status[%d] ErrorRet[%u]",
                 statusReq->txReport.m_strSmsid.c_str(),
                 statusReq->txReport.m_status, ret);
    }

    return ret;
}


/*
    for set failed resend info to redis
    add by ljl 2018-01-20

*/
void CSMSCluster::smsClusterSetRedisResendInfo( SmsHttpInfo* pSms, string& strRedisData )
{
    if(!pSms)
    {
        LogWarn("pSms is null!!");
        return;
    }
    //smstype
    strRedisData.append("  smstype ");
    strRedisData.append(Comm::int2str(pSms->m_uSmsType));

    if(pSms->m_uResendTimes > 0 && !pSms->m_strFailedChannel.empty())
    {
        //failed_resend_times
        strRedisData.append("  failed_resend_times ");
        strRedisData.append(Comm::int2str(pSms->m_uResendTimes));
        //failed_resend_channel
        strRedisData.append("  failed_resend_channel ");
        strRedisData.append(pSms->m_strFailedChannel);
    }
    //failed_resend_msg
    string strData = "";
    ////userName
    strData.assign("userName=");
    strData.append(Base64::Encode(pSms->m_strUserName));
    ////sign
    strData.append("&sign=");
    strData.append(Base64::Encode(pSms->m_strSign));
    ////content
    strData.append("&content=");
    strData.append(Base64::Encode(pSms->m_strContent));
    ////smsType
    strData.append("&smsType=");
    strData.append(Comm::int2str( pSms->m_uSmsType));
    ////paytype
    strData.append("&paytype=");
    strData.append(Comm::int2str(pSms->m_uPayType));
    ////showsigntype
    strData.append("&showsigntype=");
    strData.append(pSms->m_strShowSignType_c2s);
    ////userextpendport
    strData.append("&userextpendport=");
    strData.append(pSms->m_strUserExtendPort_c2s);

    ////signextendport
    strData.append("&signextendport=");
    strData.append(pSms->m_strSignExtendPort_c2s);

    ////operater
    strData.append("&operater=");
    strData.append(Comm::int2str(pSms->m_uPhoneType));
    ////ids
    strData.append("&ids=");
    strData.append(pSms->m_strAccessids);
    ////clientcnt
    strData.append("&clientcnt=");
    strData.append(Comm::int2str(pSms->m_uClientCnt));
    ////process_times
    strData.append("&process_times=");
    strData.append(Comm::int2str(pSms->m_uProcessTimes));
    ////csid
    strData.append("&csid=");
    strData.append(Comm::int2str(pSms->m_uC2sId));
    ////csdate
    strData.append("&csdate=");
    strData.append(pSms->m_strC2sTime);
    ////errordate
    strData.append("&errordate=");
    if ( 1 == pSms->m_uProcessTimes )
    {
        UInt64 uNowTime = time(NULL);
        strData.append(Comm::int2str(uNowTime));
    }
    else
    {
        strData.append(pSms->m_strErrorDate);
    }
    strData.append("&templateid=");
    strData.append(pSms->m_strTemplateId);
    strData.append("&temp_params=");
    strData.append(pSms->m_strTemplateParam);

    strRedisData.append(" failed_resend_msg ");
    strRedisData.append(strData);
    return;
}

