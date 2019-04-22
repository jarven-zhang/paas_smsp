#include "main.h"
#include "UrlCode.h"
#include "Comm.h"
#include "base64.h"
#include "json/json.h"
#include "SmsAccount.h"
#include "CRuleLoadThread.h"

#include "CChannelTxThreadPool.h"
#include "CHttpsPushThread.h"
#include "global.h"

extern CChannelThreadPool *g_CChannelThreadPool;

#define HTTP_SESSION_FREE( Session ) SAFE_DELETE( Session )

CHTTPSendThread::CHTTPSendThread(const char *name) : CThread(name)
{
    m_pLibMng = NULL;
    m_pSnMng = NULL;
    m_pInternalService = NULL;
}

CHTTPSendThread::~CHTTPSendThread()
{

}

bool CHTTPSendThread::Init( UInt32 ThreadId )
{
    m_pLibMng = new ChannelMng();
    if (NULL == m_pLibMng)
    {
        LogError("m_pLibMng is NULL.");
        return false;
    }

    m_pSnMng = new SnManager();
    if (NULL == m_pSnMng)
    {
        LogError("m_pSnMng is NULL.");
        return false;
    }

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");
        return false;
    }
    m_pInternalService->Init();

    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    m_mapAccessToken.clear();
    g_pRuleLoadThread->getChannelAccessToken(m_mapAccessToken);
    m_pLinkedBlockPool = new LinkedBlockPool();

    m_ThreadId  = ThreadId ;

    return true;
}

void CHTTPSendThread::MainLoop()
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
        else if (NULL != pMsg)
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }

    m_pInternalService->GetSelector()->Destroy();
}

void CHTTPSendThread::HandleMsg( TMsg* pMsg )
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
    m_iCount++;
    pthread_mutex_unlock(&m_mutex);

    switch(pMsg->m_iMsgType)
    {
        case MSGTYPE_DISPATCH_TO_HTTPSEND_REQ:
        {
            TDispatchToHttpSendReqMsg* pSendReq = (TDispatchToHttpSendReqMsg*)( pMsg );
            if( true != yunMasGetToken( pSendReq ))
            {
                LogError("yunMasGetToken Error");
                break;
            }
            HandleDispatchToHttpSendReqMsg( pMsg );
            break;
        }
        case MSGTYPE_HOSTIP_RESP:
        {
            HandleHostIpResp(pMsg);
            break;
        }
        case MSGTYPE_HTTPRESPONSE:
        {
            HandleHttpResponse(pMsg);
            break;
        }
        case MSGTYPE_HTTPS_ASYNC_RESPONSE:
        {
            HandleHttpsResponse(pMsg);
            break;
        }
        case MSGTYPE_RULELOAD_ACCESSTOKEN_UPDATE_REQ:
        {
            LogNotice("RuleUpdate channel access_token update!");
            TUpdateAccessTokenReq* pReq = (TUpdateAccessTokenReq*)pMsg;
            m_mapAccessToken.clear();
            m_mapAccessToken = pReq->m_mapAccessToken;
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            LogNotice("RuleUpdate channel update!");
            HandleChannelUpdate( pMsg );
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOut(pMsg);
            break;
        }
        default:
        {
            break;
        }
    }
}


void CHTTPSendThread::HandleChannelUpdate ( TMsg* pMsg )
{
    TUpdateChannelReq *updateChanMsg = (TUpdateChannelReq *)pMsg;
    LogNotice("update Channel Del Size :%d.", updateChanMsg->m_ChannelMap.size());
    for( std::map<int, models::Channel>::iterator iter = updateChanMsg->m_ChannelMap.begin();
        iter != updateChanMsg->m_ChannelMap.end(); iter++)
    {
        LogNotice("ChanneMng Del Channel %d, name: %s, try unload lib [lib%s.so]",
                  iter->first, iter->second.channelname.data(), iter->second.m_strChannelLibName.data());
        m_pLibMng->delChannelLib( iter->second.m_strChannelLibName );
        m_pLibMng->delChannelLib( iter->second.channelname );
    }
}

bool CHTTPSendThread::yunMasGetToken( TDispatchToHttpSendReqMsg* pSendReq )
{
    string strMasUserId;
    string strAccessToken;

    if (0 != pSendReq->m_smsParam.m_strChannelName.compare(0, 6, "YUNMAS"))
    {
        return true;
    }

    map<UInt32, yunMasInfo>::iterator iter =  m_mapYunMasInfo.find( pSendReq->m_smsParam.m_iChannelId );
    if ( m_mapYunMasInfo.end() != iter )
    {
        UInt64 uTime = time(NULL);
        /* 如果当前token还有效从本地获取*/
        if ( uTime < iter->second.m_uAccessTokenExpireSeconds )
        {
            pSendReq->m_smsParam.m_strMasUserId = iter->second.m_strMasUserId;
            pSendReq->m_smsParam.m_strMasAccessToken = iter->second.m_strMasAccessToken;
            return true;
        }
        m_mapYunMasInfo.erase(iter);
    }

    std::string strResponse;
    std::vector<std::string> mv;
    http::HttpsSender httpsSender;

    int iRet = httpsSender.CurlPerform(pSendReq->m_smsParam.m_strOauthUrl,
                pSendReq->m_smsParam.m_strOauthData,strResponse, HTTPS_MODE_POST, &mv );
    if ( 0 != iRet )
    {
        LogError("url:%s,data:%s,channelid:%d,get access token is error.",
                 pSendReq->m_smsParam.m_strOauthUrl.c_str(),
                 pSendReq->m_smsParam.m_strOauthData.c_str(),
                 pSendReq->m_smsParam.m_iChannelId );
        return false;
    }

    LogDebug("strResponse:%s.",strResponse.data());

    try
    {
        std::string::size_type pos = strResponse.find_first_of("{");
        strResponse = strResponse.substr(pos);
        strResponse.erase(strResponse.find_last_of("}") + 1);
    }
    catch(...)
    {
        LogError("strResponse:%s,is not json format.",strResponse.data());
        return false;
    }

    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    std::string js;

    if (json_format_string(strResponse, js) < 0 )
    {
        LogError("1yunmas,strResponse:%s,parse is failed.",strResponse.data());
        return false;
    }
    if(!reader.parse(js,root))
    {
        LogError("2yunmas,strResponse:%s,parse is failed.",strResponse.data());
        return false;
    }

    yunMasInfo getInfo;
    getInfo.m_strMasAccessToken = root["access_token"].asString();
    getInfo.m_strMasUserId = root["mas_user_id"].asString();
    string strSeconds = root["access_token_expire_seconds"].asString();
    getInfo.m_uAccessTokenExpireSeconds = atoi(strSeconds.c_str()) + time(NULL) - 120;

    LogNotice("channelid:%d,get token success,masUserId:%s,accessToken:%s.",
              pSendReq->m_smsParam.m_iChannelId,
              getInfo.m_strMasUserId.data(),
              getInfo.m_strMasAccessToken.data());

    m_mapYunMasInfo.insert(make_pair(pSendReq->m_smsParam.m_iChannelId, getInfo));

    pSendReq->m_smsParam.m_strMasUserId.assign(getInfo.m_strMasUserId);
    pSendReq->m_smsParam.m_strMasAccessToken.assign(getInfo.m_strMasAccessToken);

    return true;
}

/*处理http请求保存会话，
   获取hostIP地址*/
void CHTTPSendThread::HandleDispatchToHttpSendReqMsg( TMsg* pMsg )
{
    TDispatchToHttpSendReqMsg* pHttpSendReqMsg = (TDispatchToHttpSendReqMsg*)(pMsg);

    HttpSendSession* pSession = new HttpSendSession();
    smsp::SmsContext* pSms = pSession->m_pSmsContext;

    /*保存会话ID*/
    pSession->m_strSessionId = pMsg->m_strSessionID;
    pSession->m_SmsHttpInfo = pHttpSendReqMsg->m_smsParam;
    SmsHttpInfo *smsInfo = &pSession->m_SmsHttpInfo;

    string strShowPhone = smsInfo->m_strUcpaasPort + smsInfo->m_strSignPort + smsInfo->m_strDisplayNum;
    if (smsInfo->m_uExtendSize > 21)
    {
        smsInfo->m_uExtendSize = 21;
    }

    if ( strShowPhone.length() > smsInfo->m_uExtendSize )
    {
        strShowPhone = strShowPhone.substr(0,smsInfo->m_uExtendSize);
    }

    LogDebug("ChannelName: %s", smsInfo->m_strChannelLibName.c_str());
    pSms->m_iChannelId     = smsInfo->m_iChannelId;
    pSms->m_strChannelname = smsInfo->m_strChannelName;
    pSms->m_strChannelLibName = smsInfo->m_strChannelLibName;
    pSms->m_iOperatorstype = smsInfo->m_uPhoneType;
    pSms->m_strSmsId       = smsInfo->m_strSmsId;
    pSms->m_strPhone       = smsInfo->m_strPhone;
    pSms->m_iSmsfrom       = smsInfo->m_uSmsFrom;
    pSms->m_fCostFee       = smsInfo->m_fCostFee;
    pSms->m_strClientId    = smsInfo->m_strClientId;
    pSms->m_uPayType       = smsInfo->m_uPayType;
    pSms->m_lSendDate      = smsInfo->m_uSendDate;
    pSms->m_strUserName    = smsInfo->m_strUserName;
    pSms->m_strShowPhone   = strShowPhone;
    pSms->m_strC2sTime     = smsInfo->m_strC2sTime;
    pSms->m_lSubmitDate    = time(NULL);

    pSms->m_uChannelOperatorsType = smsInfo->m_uChannelOperatorsType;
    pSms->m_strChannelRemark      = smsInfo->m_strChannelRemark;
    pSms->m_uChannelIdentify      = smsInfo->m_uChannelIdentify;
    pSms->m_uClientCnt            = smsInfo->m_uClientCnt;
    pSms->m_iArea                 = smsInfo->m_uArea;
    pSms->m_uC2sId                = smsInfo->m_uC2sId;
    pSms->m_strTemplateId         = smsInfo->m_strTemplateId;
    pSms->m_strTemplateParam      = smsInfo->m_strTemplateParam;
    pSms->m_strChannelTemplateId  = smsInfo->m_strChannelTemplateId;
    pSms->m_strMasUserId          = smsInfo->m_strMasUserId;
    pSms->m_strMasAccessToken     = smsInfo->m_strMasAccessToken;
    pSms->m_strDisplaynum         = smsInfo->m_strDisplayNum;
    pSms->m_strSign               = smsInfo->m_strSign;
    pSms->m_uSmsType              = smsInfo->m_uSmsType;
    pSms->m_uShowSignType         = smsInfo->m_uShowSignType;
    pSms->m_Channel               = smsInfo->m_Channel;

    LogDebug("Support LongMsg:%d", pSms->m_ulongsms);

    pSession->m_strSendContent = smsInfo->m_strContent;
    pSession->m_uSendType      = smsInfo->m_uSendType;
    pSession->m_strHttpData.assign(smsInfo->m_strSendData);
    pSession->m_strHttpUrl.assign(smsInfo->m_strSendUrl);
    pSession->m_strOauthData.assign(smsInfo->m_strOauthData);
    pSession->m_strOauthUrl.assign(smsInfo->m_strOauthUrl);


    /* 保存消息到内存*/
    UInt64 uSeq = m_pSnMng->getSn();
    m_mapSession[ uSeq ] = pSession;

    /* 获取IP 地址一个请求只请求一次*/
    THostIpReq* pHost = new THostIpReq();
    pHost->m_DomainUrl.assign(smsInfo->m_strSendUrl);
    pHost->m_iMsgType = MSGTYPE_HOSTIP_REQ;
    pHost->m_pSender = this;
    pHost->m_iSeq = uSeq;
    g_pHostIpThread->PostMsg(pHost);

    return;
}

bool CHTTPSendThread::yumMasGetTokenAccess( HttpSendSession *httpSession, smsp::Channellib *mylib,
                                  UInt32 uIpAddr, std::vector<std::string> &mv )
{
    bool isNeedGetAccessToken = false;

    std::string strOauthUrl = httpSession->m_strOauthUrl;
    std::string strOauthData = httpSession->m_strOauthData;
    UInt32 HttpMethod = httpSession->m_uSendType;

    smsp::SmsContext* pSms = httpSession->m_pSmsContext;

    if( strOauthUrl.empty() || 0 == pSms->m_strChannelname.compare(0,6,"YUNMAS")){
        return true;
    }

    AccessTokenMap::iterator itor = m_mapAccessToken.find(pSms->m_iChannelId);

    if (itor == m_mapAccessToken.end())
    {
        LogDebug("[%s:%s] channelid[%u] has not find in m_mapAccessToken, need to get.",
                pSms->m_strSmsId.data(), pSms->m_strPhone.data(), pSms->m_iChannelId);
        isNeedGetAccessToken = true;
    }
    else
    {
        long now = (long)(time(NULL));
        if ( now >= itor->second.m_iExpiresIn )
        {
            LogNotice("[%s:%s:%u] access_token has expired (ExpiresIn[%ld], now[%ld]), need to get again.",
                    pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                    pSms->m_iChannelId, itor->second.m_iExpiresIn,
                    now);
            isNeedGetAccessToken = true;
        }
        else
        {
            pSms->m_strAccessToken = itor->second.m_strAccessToken;
        }
    }

    if ( 0 != mylib->adaptEachChannelforOauth( pSms, &strOauthUrl, &strOauthData, &mv ))
    {
        LogError("[%s:%s] Get appSecret failed.", pSms->m_strSmsId.data(), pSms->m_strPhone.data());
        return false;
    }

    if ( isNeedGetAccessToken )
    {
        //post
        if ( HttpMethod == HTTP_POST )
        {
            std::string strResponse;
            http::HttpsSender httpsSender;
            httpsSender.Send(uIpAddr, strOauthUrl, strOauthData, strResponse, HTTPS_MODE_POST, &mv );
            if (strResponse.empty())
            {
                LogError("[%s:%s] Get access token failure.",
                           pSms->m_strSmsId.c_str(), pSms->m_strPhone.c_str());
                return false;
            }
            models::AccessToken accessToken;
            std::string strReason = "";
            if ( mylib->parseOauthResponse(strResponse, accessToken, strReason ))
            {
                pSms->m_strAccessToken = accessToken.m_strAccessToken;
                m_mapAccessToken[pSms->m_iChannelId] = accessToken;

                char sql[1024] = {0};
                snprintf(sql, sizeof(sql), "insert into t_sms_channel_token_log( "
                                           " channelid, access_token, expires_in, refresh_token, create_time) "
                                            " values(%u, '%s', %ld, '%s', now());",
                                            pSms->m_iChannelId, accessToken.m_strAccessToken.c_str(),
                                            accessToken.m_iExpiresIn, accessToken.m_strRefreshToken.c_str());
                LogDebug("[%s:%s] Exec Sql Cmd[%s]", pSms->m_strSmsId.c_str(), pSms->m_strPhone.c_str(), sql);
                TDBQueryReq* pMsg = new TDBQueryReq();
                pMsg->m_iMsgType = MSGTYPE_DB_NOTQUERY_REQ;
                pMsg->m_SQL.assign(sql);
                g_pDisPathDBThreadPool->PostMsg(pMsg);
            }
            else
            {
                LogError("[%s:%s] parse Oauth Response failure, for the reason[%s].",
                        pSms->m_strSmsId.c_str(), pSms->m_strPhone.c_str(), strReason.c_str());
                return false;
            }
        }
        else
        {
            if( HttpMethod != HTTP_GET )
            {
                LogError("[%s:%s] SendType[%d] is invalid.",
                         pSms->m_strSmsId.data(), pSms->m_strPhone.data(), HttpMethod);
                return false;
            }
        }

    }

    return true;
}


/*
 * 将短信发送到http通道侧
 * ret  SmsHttpSendStat
 */
void CHTTPSendThread:: smsSendMsgToHttpChan ( HttpSendSession *httpSession, std::string encoded_msg, UInt64 uSeq, UInt64 uIpAddr )
{
    std::string postData = httpSession->m_strHttpData;
    std::string urltmp = httpSession->m_strHttpUrl;

    smsp::SmsContext* pSms = httpSession->m_pSmsContext;
    std::string strChannelTemplateId = pSms->m_strChannelTemplateId;
    std::string strTemplateParam = pSms->m_strTemplateParam;

    std::vector<std::string> mv;

    smsp::Channellib *mylib = m_pLibMng->loadChannelLibByNames( pSms->m_strChannelLibName, pSms->m_strChannelname );
    if ( !mylib )
    {
        LogError(" Get lib Handler Error [%s:%s] mylib is NULL.",
                pSms->m_strSmsId.data(), pSms->m_strPhone.data());
        pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        pSms->m_strSubmit = LIB_IS_NULL ;
        pSms->m_strYZXErrCode = LIB_IS_NULL;
        return;
    }

    /*yumMas通道token获取*/
    if( true != yumMasGetTokenAccess( httpSession, mylib, uIpAddr, mv ))
    {
        LogError("YUMMAS Get Token AccessError Error [%s:%s].",
                  pSms->m_strSmsId.data(), pSms->m_strPhone.data());
        pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        pSms->m_strSubmit = LIB_IS_NULL ;
        pSms->m_strYZXErrCode = LIB_IS_NULL;
        return;
    }

    pSms->m_strContent = encoded_msg;
    mylib->adaptEachChannel(pSms, &urltmp, &postData, &mv );

    if ( httpSession->m_uSendType == HTTP_GET )
    {
        std::string::size_type pos = urltmp.find("%phone%");
        if (pos != std::string::npos)
        {
            urltmp.replace(pos, strlen("%phone%"), pSms->m_strPhone);
        }

        pos = urltmp.find("%content%");
        if (pos != std::string::npos)
        {
            urltmp.replace(pos, strlen("%content%"), pSms->m_strContent);
        }
        pSms->m_strContent = httpSession->m_strSendContent;     //recover to raw message data

        pos = urltmp.find("%msgid%");
        if(pos!=std::string::npos)
        {
            urltmp.replace(pos, strlen("%msgid%"), pSms->m_strChannelSmsId);
        }

        pos = urltmp.find("%displaynum%");
        if(pos!=std::string::npos)
        {
            urltmp.replace(pos, strlen("%displaynum%"), pSms->m_strShowPhone);
        }

        if(httpSession->m_strHttpUrl.substr(0, 5) == "https"){
            //std::string strResponse;
            //http::HttpsSender httpsSender;
            THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
            //pHttpsReq->strBody = postData;
            pHttpsReq->strUrl  = urltmp;
            pHttpsReq->uIP     = uIpAddr;
            pHttpsReq->vecHeader = mv;
            pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_GET;
            pHttpsReq->m_iSeq     = uSeq;
            pHttpsReq->m_pSender  = this;

            CHttpsSendThread::sendHttpsMsg (pHttpsReq);

            HttpSendSessionMap::iterator it = m_mapSession.find(uSeq);
            if(m_mapSession.end() != it){
                it->second->m_pWheelTime = SetTimer(it->first, "", HTTPS_SEND_SESSION_TIMEOUT);
            }
        }
        else{
            http::HttpSender* pHttpSender = new http::HttpSender();
            if ( false == pHttpSender->Init( m_pInternalService, uSeq, this ))
            {
                LogError("[%s:%s] pHttpSender->Init is failed.",
                          pSms->m_strSmsId.data(), pSms->m_strPhone.data());
                delete pHttpSender;

                pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
                pSms->m_strSubmit = HTTPSEND_INIT_FAILED ;
                pSms->m_strYZXErrCode = HTTPSEND_INIT_FAILED;
                return;
            }

            httpSession->m_pHttpSender = pHttpSender;

            LogNotice("==http send to channel== [%s:%s:%d]uSeq:%lu,url[%s].",
                      pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                      pSms->m_iChannelId, uSeq,urltmp.data());

            pHttpSender->setIPCache( uIpAddr );
            pHttpSender->Get(urltmp);
            httpSession->m_pWheelTime = SetTimer(uSeq, "", HTTP_SEND_SESSION_TIMEOUT );
        }
    }
    else ////post
    {
        std::string::size_type pos = postData.find("%phone%");
        if (pos != std::string::npos)
        {
            postData.replace(pos, strlen("%phone%"), pSms->m_strPhone);
        }

        pos = postData.find("%content%");
        if (pos != std::string::npos)
        {
            postData.replace(pos, strlen("%content%"), pSms->m_strContent);
        }
        pSms->m_strContent = httpSession->m_strSendContent;     //recover to raw message data

        pos = postData.find("%msgid%");
        if(pos!=std::string::npos)
        {
            postData.replace(pos, strlen("%msgid%"), pSms->m_strChannelSmsId);
        }

        pos = postData.find("%displaynum%");
        if(pos!=std::string::npos)
        {
            postData.replace(pos, strlen("%displaynum%"), pSms->m_strShowPhone);
        }

        /*https同步请求*/
        if( httpSession->m_strHttpUrl.substr(0, 5) == "https" )
        {
            //std::string strResponse;
            //http::HttpsSender httpsSender;
            THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
            pHttpsReq->strBody = postData;
            pHttpsReq->strUrl  = urltmp;
            pHttpsReq->uIP     = uIpAddr;
            pHttpsReq->vecHeader = mv;
            pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_POST;
            pHttpsReq->m_iSeq     = uSeq;
            pHttpsReq->m_pSender  = this;

            CHttpsSendThread::sendHttpsMsg(pHttpsReq);

            HttpSendSessionMap::iterator it = m_mapSession.find(uSeq);
            if(m_mapSession.end() != it){
                it->second->m_pWheelTime = SetTimer(it->first, "", HTTPS_SEND_SESSION_TIMEOUT);
            }
        }
        else
        {
            http::HttpSender* pHttpSender = new http::HttpSender();
            if ( false == pHttpSender->Init(m_pInternalService, uSeq, this ))
            {
                LogError("[%s:%s] pHttpSender->Init is failed.",
                         pSms->m_strSmsId.data(), pSms->m_strPhone.data());
                delete pHttpSender;
                pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
                pSms->m_strSubmit = HTTPSEND_INIT_FAILED ;
                pSms->m_strYZXErrCode = HTTPSEND_INIT_FAILED;
                return;
            }

            LogNotice("==http send to channel== [%s:%s:%d]uSeq:%lu,url[%s],data[%s]",
                      pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                      pSms->m_iChannelId,uSeq, urltmp.data(), postData.data());

            httpSession->m_pHttpSender = pHttpSender;
            pHttpSender->setIPCache(uIpAddr);
            pHttpSender->Post(urltmp, postData,&mv);
            httpSession->m_pWheelTime = SetTimer(uSeq, "", HTTP_SEND_SESSION_TIMEOUT );

        }
    }

}

void CHTTPSendThread::smsSendShortMsg(HttpSendSession  *httpSession, string& szSign, smsp::SmsContext *pSmsContext
    , THostIpResp* pResp , vector<string>& contents)
{
    int iIndexNum = 1;
    UInt64 tmpSeqs = 0;
    utils::UTFString utfHelper;
    HttpSendSession*  pHttpSession = NULL;
    smsp::SmsContext* pSms = NULL;

    for ( std::vector<std::string>::iterator it = contents.begin(); it != contents.end(); it++ )
    {
        pHttpSession = httpSession;
        pSms         = pSmsContext;
        tmpSeqs      = pResp->m_iSeq;

        /* 生成msgid */
        char msgid[512]={0};
        string strPhones = "";
        if( pSms->m_strPhone.length() > 23 )
        {
            unsigned char md5[32] = {0};
            MD5((const unsigned char*) pSms->m_strPhone.data(), pSms->m_strPhone.length(), md5);
            std::string HEX_CHARS = "0123456789abcdef";
            for (int j = 0; j < 16; j++)
            {
               strPhones.append(1, HEX_CHARS.at(md5[j] >> 4 & 0x0F));
               strPhones.append(1, HEX_CHARS.at(md5[j] & 0x0F));
            }
        }
        else
        {
            strPhones = pSms->m_strPhone;
        }

        if (pSms->m_strChannelLibName != "HLJW")
        {
            snprintf( msgid, 201, "%u%ld%s%lu", m_ThreadId, (long)time(NULL), strPhones.data(), tmpSeqs );
        }
        else /* 鸿联九五通道只支持20位以下的channelsmsid */
        {
            snprintf( msgid, 201, "%u%s%lu", m_ThreadId, strPhones.data(), tmpSeqs );
        }
        pSms->m_strChannelSmsId = std::string( msgid );

        std::string sendContent;
        if ( pSmsContext->m_uShowSignType == 2 )
        {
            (*it) = (*it) + szSign;
        }
        else if ((pSmsContext->m_uShowSignType == 1) || (pSmsContext->m_uShowSignType == 0))
        {
            (*it) = szSign + (*it);
        }

        pSms->m_strContent = (*it);
        pHttpSession->m_strSendContent = pSms->m_strContent;
        sendContent = pSms->m_strContent;

        /* 添加编码格式处理gb2312*/
        if (( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("gbk") == 0)
             || ( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("GBK") == 0))
        {
            utfHelper.u2g((*it), sendContent);
        }

        sendContent = http::UrlCode::UrlEncode( sendContent );
        pSms->m_iIndexNum = iIndexNum++;


        /* 插入新增的会话*/
        if( tmpSeqs != pResp->m_iSeq ){
            m_mapSession[tmpSeqs] = pHttpSession;
        }

        if( pResp->m_iResult != 0 )
        {
            LogError("[%s:%s]httpUrl:%s,hostip analyze is failed.",
                     pSms->m_strSmsId.data(),
                     pSms->m_strPhone.data(),
                     pHttpSession->m_strHttpUrl.data());

            pSms->m_strSubmit = HOSTIP_ANALYZE_FAILED;
            pSms->m_strYZXErrCode = HOSTIP_ANALYZE_FAILED;
            pSms->m_lSubretDate = time(NULL);
            pSms->m_iStatus = SMS_STATUS_CONFIRM_FAIL;
        }
        else
        {
            smsSendMsgToHttpChan(pHttpSession, sendContent, tmpSeqs, pResp->m_uIp );
        }

        if( pHttpSession->m_pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
        {
            HttpSendStatusReport( pHttpSession );
            if( tmpSeqs != pResp->m_iSeq )
            {
                HTTP_SESSION_FREE( pHttpSession );
                m_mapSession.erase(tmpSeqs);
            }
        }
    }

    if( pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
    {
        HTTP_SESSION_FREE( httpSession );
        m_mapSession.erase( pResp->m_iSeq );
    }
}


void CHTTPSendThread::smsSendLongMsg(HttpSendSession    * httpSession, string& szSign, smsp::SmsContext *pSmsContext
    , THostIpResp* pResp , vector<string>& contents, bool isIncludeChinese)
{
    int iIndexNum = 1;
    UInt64 tmpSeqs = 0;
    utils::UTFString utfHelper;
    HttpSendSession*  pHttpSession = NULL;
    smsp::SmsContext* pSms = NULL;
    int totalLenWithSisn = 0;

    for ( std::vector<std::string>::iterator it = contents.begin(); it != contents.end(); it++ )
    {
        std::string sendContent;
        if ( pSmsContext->m_uShowSignType == 2 )
        {
            (*it) = (*it) + szSign;
        }
        else if ((pSmsContext->m_uShowSignType == 0) || (pSmsContext->m_uShowSignType == 1))
        {
            (*it) = szSign + (*it);
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
        int lenSign = utfHelper.u2u16(szSign, out);
        int MAXLENGTH = httpSession->m_SmsHttpInfo.m_Channel.m_uCnSplitLen * 2;
        if (!isIncludeChinese)
        {
             MAXLENGTH = httpSession->m_SmsHttpInfo.m_Channel.m_uEnSplitLen * 2;
        }

        int size = (len + MAXLENGTH -1)/MAXLENGTH;
        int sizeWithSign = size;
        totalLenWithSisn = size;
        if (pSmsContext->m_uShowSignType == 3)
        {
            if (httpSession->m_SmsHttpInfo.m_Channel.m_uSplitRule)
            {
                sizeWithSign = (len + lenSign + MAXLENGTH -1)/MAXLENGTH;
                LogDebug("size=%d,sizeWithSign=%d", size, sizeWithSign);
            }
            totalLenWithSisn = (len + lenSign + MAXLENGTH -1)/MAXLENGTH;
        }
        LogDebug("totalLenWithSisn=%d", size, totalLenWithSisn);
        int index = 1;
        bool ifLast = false;

        while(len > 0)
        {
            std::string strtmp;
            int strlen = 0;

            if(len > MAXLENGTH)
            {
                strtmp = sendContent.substr(index*MAXLENGTH - MAXLENGTH, MAXLENGTH);
                strlen = MAXLENGTH;
                len = len - MAXLENGTH;

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

            string strContent = "";
            utfHelper.u162u(strtmp, strContent);

            if ( iIndexNum > 1 )
            {
                pHttpSession = new HttpSendSession();
                pSms = pHttpSession->m_pSmsContext;
                string sendUUid = pSms->m_strSmsUuid;
                pSmsContext->m_iSmscnt = 1;
                *pSms = *pSmsContext;
                pSms->m_strSmsUuid = sendUUid;

                pHttpSession->m_strSessionId = httpSession->m_strSessionId;
                pHttpSession->m_SmsHttpInfo = httpSession->m_SmsHttpInfo;
                pHttpSession->m_uSendType      = httpSession->m_uSendType;
                pHttpSession->m_strHttpData.assign(httpSession->m_strHttpData);
                pHttpSession->m_strHttpUrl.assign(httpSession->m_strHttpUrl);
                pHttpSession->m_strOauthData.assign(httpSession->m_strOauthData);
                pHttpSession->m_strOauthUrl.assign(httpSession->m_strOauthUrl);
                tmpSeqs = m_pSnMng->getSn();
            }
            else if (iIndexNum == 1)
            {
                if (pSmsContext->m_uShowSignType == 3 && index == 1)
                {
                    if (totalLenWithSisn > size && !httpSession->m_SmsHttpInfo.m_Channel.m_uSplitRule)
                    {
                        pSmsContext->m_iSmscnt = 2;

                    }
                    LogDebug("noSigncnt = %d, total=%d",size, totalLenWithSisn);
                }

                pHttpSession = httpSession;
                pSms         = pSmsContext;
                tmpSeqs      = pResp->m_iSeq;
            }

            /* 生成msgid */
            char msgid[512]={0};
            string strPhones = "";
            if( pSms->m_strPhone.length() > 23 )
            {
                unsigned char md5[32] = {0};
                MD5((const unsigned char*) pSms->m_strPhone.data(), pSms->m_strPhone.length(), md5);
                std::string HEX_CHARS = "0123456789abcdef";
                for (int j = 0; j < 16; j++)
                {
                   strPhones.append(1, HEX_CHARS.at(md5[j] >> 4 & 0x0F));
                   strPhones.append(1, HEX_CHARS.at(md5[j] & 0x0F));
                }
            }
            else
            {
                strPhones = pSms->m_strPhone;
            }

            if (pSms->m_strChannelLibName != "HLJW")
            {
                snprintf( msgid, 201, "%u%ld%s%lu", m_ThreadId, (long)time(NULL), strPhones.data(), tmpSeqs );
            }
            else /* 鸿联九五通道只支持20位以下的channelsmsid */
            {
                snprintf( msgid, 201, "%u%s%lu", m_ThreadId, strPhones.data(), tmpSeqs );
            }
            pSms->m_strChannelSmsId = std::string( msgid );
            char beforeIndex[1024] = {0};
            sprintf(beforeIndex, "%d/%d", index, sizeWithSign);
            strContent.insert(0, beforeIndex);
            pSms->m_strContent = strContent;

            pHttpSession->m_strSendContent = pSms->m_strContent;
            string inFactContent = pSms->m_strContent;



            /* 添加编码格式处理gb2312*/
            if (( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("gbk") == 0)
                 || ( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("GBK") == 0))
            {
                utfHelper.u2g((*it), inFactContent);
            }

            inFactContent = http::UrlCode::UrlEncode( inFactContent );
            pSms->m_uChannelCnt= size;
            pSms->m_iIndexNum = iIndexNum++;


            /* 插入新增的会话*/
            if( tmpSeqs != pResp->m_iSeq ){
                m_mapSession[tmpSeqs] = pHttpSession;
            }

            if( pResp->m_iResult != 0 )
            {
                LogError("[%s:%s]httpUrl:%s,hostip analyze is failed.",
                         pSms->m_strSmsId.data(),
                         pSms->m_strPhone.data(),
                         pHttpSession->m_strHttpUrl.data());

                pSms->m_strSubmit = HOSTIP_ANALYZE_FAILED;
                pSms->m_strYZXErrCode = HOSTIP_ANALYZE_FAILED;
                pSms->m_lSubretDate = time(NULL);
                pSms->m_iStatus = SMS_STATUS_CONFIRM_FAIL;
            }
            else
            {
                smsSendMsgToHttpChan(pHttpSession, inFactContent, tmpSeqs, pResp->m_uIp );
            }

            if( pHttpSession->m_pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
            {
                HttpSendStatusReport( pHttpSession );
                if( tmpSeqs != pResp->m_iSeq )
                {
                    HTTP_SESSION_FREE( pHttpSession );
                    m_mapSession.erase(tmpSeqs);
                }
            }

            index++;
        }

    }
    if( pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
    {
        HTTP_SESSION_FREE( httpSession );
        m_mapSession.erase( pResp->m_iSeq );
    }
}

void CHTTPSendThread::smsSendSplitContent(HttpSendSession  *httpSession, string& szSign, smsp::SmsContext *pSmsContext
    , THostIpResp* pResp, bool isIncludeChinese)
{
    std::vector<string> contents;
    UInt32 cnt = SplitMsgContent(httpSession, contents, szSign, isIncludeChinese, pSmsContext);
    UInt32 uLongSms = httpSession->m_SmsHttpInfo.m_uLongSms;

    /*  获取计费条数*/
    if (!uLongSms)
    {
        pSmsContext->m_iSmscnt = 1;
    }
    else
    {
        pSmsContext->m_iSmscnt = cnt;
    }

    /* 设置实际是否为长短信*/
    if ( cnt > 1 )
    {
        pSmsContext->m_ulongsms = 1;
    }
    else
    {
        pSmsContext->m_ulongsms = 0;
    }

    /**拆分条数**/
    if (uLongSms)
    {
        pSmsContext->m_uDivideNum = 1;
    }
    else
    {
        pSmsContext->m_uDivideNum = cnt;
    }
    pSmsContext->m_uChannelCnt = cnt;

    /*http短信发送*/
    if (pSmsContext->m_uDivideNum == 1)
    {
        smsSendShortMsg(httpSession, szSign, pSmsContext, pResp, contents);
    }
    else
    {
        smsSendLongMsg(httpSession, szSign, pSmsContext, pResp, contents, isIncludeChinese);
    }

}

void CHTTPSendThread::HandleHostIpResp( TMsg* pMsg )
{
    THostIpResp* pResp = (THostIpResp*)pMsg;

    if( pResp == NULL ){
        return;
    }

    HttpSendSessionMap::iterator itr = m_mapSession.find( pResp->m_iSeq );
    if ( itr == m_mapSession.end())
    {
        LogError("HostIp response uSeq:%lu is not find in m_mapSession", pResp->m_iSeq );
        return;
    }

    HttpSendSession  *httpSession  = ( HttpSendSession *)itr->second;
    smsp::SmsContext *pSmsContext = httpSession->m_pSmsContext;

    /*拆分短信*/
    string szSign = "";
    string strLeft = "";
    string strRight = "";
    bool isIncludeChinese = false;
    if ( true == Comm::IncludeChinese((char*)( httpSession->m_strSendContent + pSmsContext->m_strSign).data()))
    {
        strLeft = "%e3%80%90";
        strRight = "%e3%80%91";
        strLeft = http::UrlCode::UrlDecode(strLeft);
        strRight = http::UrlCode::UrlDecode(strRight);
        isIncludeChinese = true;
    }
    else
    {
        strLeft = "[";
        strRight = "]";
    }

    if ( false == pSmsContext->m_strSign.empty())
    {
        szSign = strLeft + pSmsContext->m_strSign + strRight;
    }


    if (pSmsContext->m_Channel.m_uExValue & CHANNEL_OWN_SPLIT_FLAG )
    {
        LogDebug("start the CHANNEL_OWN_SPLIT_FLAG, m_uExValue=%d,channelid=%d", pSmsContext->m_Channel.m_uExValue, pSmsContext->m_Channel.channelID);
        smsSendSplitContent(httpSession, szSign, pSmsContext, pResp, isIncludeChinese);
    }
    else
    {
        std::vector<string> contents;
        UInt32 cnt = ParseMsgContent(httpSession->m_strSendContent, contents,
                                     szSign, httpSession->m_SmsHttpInfo.m_uLongSms, 70 );
        int msgTotal  = contents.size();
        int iIndexNum = 1;

        /*  获取计费条数*/
        if( msgTotal > 1 )
        {
            pSmsContext->m_iSmscnt = 1;
        }
        else
        {
            pSmsContext->m_iSmscnt = cnt;
        }

        /* 设置实际是否为长短信*/
        if ( cnt > 1 )
        {
            pSmsContext->m_ulongsms = 1;
        }
        else
        {
            pSmsContext->m_ulongsms = 0;
        }

        /**拆分条数**/
        pSmsContext->m_uDivideNum = msgTotal;
        pSmsContext->m_uChannelCnt = cnt;

        UInt64 tmpSeqs = 0;
        utils::UTFString utfHelper;
        HttpSendSession*  pHttpSession = NULL;
        smsp::SmsContext* pSms = NULL;

        /*http短信发送*/
        for ( std::vector<std::string>::iterator it = contents.begin(); it != contents.end(); it++ )
        {
            /*添加注释*/
            if( it != contents.begin())
            {
                pHttpSession = new HttpSendSession();
                pSms = pHttpSession->m_pSmsContext;
                string sendUUid = pSms->m_strSmsUuid;
                *pSms = *pSmsContext;
                pSms->m_strSmsUuid = sendUUid;

                pHttpSession->m_strSessionId = httpSession->m_strSessionId;
                pHttpSession->m_SmsHttpInfo = httpSession->m_SmsHttpInfo;
                pHttpSession->m_uSendType      = httpSession->m_uSendType;
                pHttpSession->m_strHttpData.assign(httpSession->m_strHttpData);
                pHttpSession->m_strHttpUrl.assign(httpSession->m_strHttpUrl);
                pHttpSession->m_strOauthData.assign(httpSession->m_strOauthData);
                pHttpSession->m_strOauthUrl.assign(httpSession->m_strOauthUrl);
                tmpSeqs = m_pSnMng->getSn();
            }
            else
            {
                pHttpSession = httpSession;
                pSms         = pSmsContext;
                tmpSeqs      = pResp->m_iSeq;
            }

            /* 生成msgid */
            char msgid[512]={0};
            string strPhones = "";
            if( pSms->m_strPhone.length() > 23 )
            {
                unsigned char md5[32] = {0};
                MD5((const unsigned char*) pSms->m_strPhone.data(), pSms->m_strPhone.length(), md5);
                std::string HEX_CHARS = "0123456789abcdef";
                for (int j = 0; j < 16; j++)
                {
                   strPhones.append(1, HEX_CHARS.at(md5[j] >> 4 & 0x0F));
                   strPhones.append(1, HEX_CHARS.at(md5[j] & 0x0F));
                }
            }
            else
            {
                strPhones = pSms->m_strPhone;
            }

            if (pSms->m_strChannelLibName != "HLJW")
            {
                snprintf( msgid, 201, "%u%ld%s%lu", m_ThreadId, (long)time(NULL), strPhones.data(), tmpSeqs );
            }
            else /* 鸿联九五通道只支持20位以下的channelsmsid */
            {
                snprintf( msgid, 201, "%u%s%lu", m_ThreadId, strPhones.data(), tmpSeqs );
            }
            pSms->m_strChannelSmsId = std::string( msgid );

            std::string sendContent;
            if ( pSmsContext->m_uShowSignType == 2 )
            {
                (*it) = (*it) + szSign;
            }
            else if ((pSmsContext->m_uShowSignType == 1) || (pSmsContext->m_uShowSignType == 0))
            {
                (*it) = szSign + (*it);
            }

            pSms->m_strContent = (*it);
            pHttpSession->m_strSendContent = pSms->m_strContent;
            sendContent = pSms->m_strContent;

            /* 添加编码格式处理gb2312*/
            if (( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("gbk") == 0)
                 || ( pHttpSession->m_SmsHttpInfo.m_strCodeType.compare("GBK") == 0))
            {
                utfHelper.u2g((*it), sendContent);
            }

            sendContent = http::UrlCode::UrlEncode( sendContent );
            pSms->m_uChannelCnt= cnt;
            pSms->m_iIndexNum = iIndexNum++;

            /*发送的内容*/
            //pSms->m_strContent = sendContent;

            /* 插入新增的会话*/
            if( tmpSeqs != pResp->m_iSeq ){
                m_mapSession[tmpSeqs] = pHttpSession;
            }

            if( pResp->m_iResult != 0 )
            {
                LogError("[%s:%s]httpUrl:%s,hostip analyze is failed.",
                         pSms->m_strSmsId.data(),
                         pSms->m_strPhone.data(),
                         pHttpSession->m_strHttpUrl.data());

                pSms->m_strSubmit = HOSTIP_ANALYZE_FAILED;
                pSms->m_strYZXErrCode = HOSTIP_ANALYZE_FAILED;
                pSms->m_lSubretDate = time(NULL);
                pSms->m_iStatus = SMS_STATUS_CONFIRM_FAIL;
            }
            else
            {
                smsSendMsgToHttpChan(pHttpSession, sendContent, tmpSeqs, pResp->m_uIp );
            }

            if( pHttpSession->m_pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
            {
                HttpSendStatusReport( pHttpSession );
                if( tmpSeqs != pResp->m_iSeq )
                {
                    HTTP_SESSION_FREE( pHttpSession );
                    m_mapSession.erase(tmpSeqs);
                }
            }
        }

        if( pSmsContext->m_iStatus != Int32(SMS_STATUS_INITIAL) )
        {
            HTTP_SESSION_FREE( httpSession );
            m_mapSession.erase( pResp->m_iSeq );
        }
    }

}

void CHTTPSendThread::HandleHttpsResponse( TMsg* pMsg ){
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }
    THttpsResponseMsg *msg = (THttpsResponseMsg *)pMsg;

    /* get session */
    HttpSendSessionMap::iterator iter = m_mapSession.find(msg->m_iSeq);
    if (iter == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", msg->m_iSeq);
        return;
    }
    LogNotice("==http sendResp== [%s:%s]RspContent[%s], iSeq:%lu, m_uStatus[%d]",
                  iter->second->m_pSmsContext->m_strSmsId.c_str(),
                  iter->second->m_pSmsContext->m_strPhone.c_str(),
                  msg->m_strResponse.c_str(),
                  msg->m_iSeq,
                  msg->m_uStatus);

    /* parse it */
    DisposeHttpsResponse(msg->m_strResponse, iter->second);

    HttpSendStatusReport(iter->second);
    delete iter->second;
    m_mapSession.erase(iter);
}

/* http异步请求响应*/
void CHTTPSendThread::HandleHttpResponse( TMsg* pMsg )
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    THttpResponseMsg* pHttpResponse = dynamic_cast<THttpResponseMsg*>(pMsg);

    HttpSendSessionMap::iterator itr = m_mapSession.find( pHttpResponse->m_iSeq );
    if ( itr == m_mapSession.end() )
    {
        LogWarn("iSeq[%lu] not find in m_mapSession.",pHttpResponse->m_iSeq);
        return;
    }

    HttpSendSession  *pHttpSession = itr->second;
    smsp::SmsContext *pSmsContext  = pHttpSession->m_pSmsContext;

    std::string content = "";
    int statusCode = 0;

    bool bLinkedStatus = true;  ////true response 200,false response not 200

    if( THttpResponseMsg::HTTP_ERR == pHttpResponse->m_uStatus )    //err,
    {
        LogError("==httpResp== [%s:%s] submit is failed.",
                 pSmsContext->m_strSmsId.data(),
                 pSmsContext->m_strPhone.data());
        pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL; //submit failed, write socket failed or connect error
        pSmsContext->m_strSubmit = SOCKET_IS_ERROR;
        pSmsContext->m_strYZXErrCode = SOCKET_IS_ERROR;
        bLinkedStatus = false;
    }
    else if (THttpResponseMsg::HTTP_RESPONSE != pHttpResponse->m_uStatus)
    {
        LogError("==httpResp== [%s:%s] submit response is failed.",
                  pSmsContext->m_strSmsId.data(),
                  pSmsContext->m_strPhone.data());
        pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL; //submitResp failed.
        pSmsContext->m_strSubmit = SOCKET_IS_CLOSE;
        pSmsContext->m_strYZXErrCode = SOCKET_IS_CLOSE;
        bLinkedStatus = false;
    }
    else
    {
        statusCode = pHttpResponse->m_tResponse.GetStatusCode();
        if ( statusCode == 200 )
        {
            content = pHttpResponse->m_tResponse.GetContent();
            BindParser(content, pHttpSession );
        }
        else
        {
            LogError("==httpResp== [%s:%s] submit response is not 200,stautsCode[%d]",
                     pSmsContext->m_strSmsId.data(),
                     pSmsContext->m_strPhone.data(),
                     statusCode);
            pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL; //submitResp failed.
            pSmsContext->m_strSubmit = RESPONSE_IS_NOT_200;
            pSmsContext->m_strYZXErrCode = RESPONSE_IS_NOT_200;
            bLinkedStatus = false;
        }
    }

    if ( true == bLinkedStatus )
    {
        LogNotice("==http sendResp== [%s:%s]iSeq:%lu,m_uStatus[%d],statusCode[%d],RspContent[%s]",
                  pSmsContext->m_strSmsId.c_str(),pSmsContext->m_strPhone.c_str(),
                  pHttpResponse->m_iSeq, pHttpResponse->m_uStatus, statusCode, content.c_str());

        if ( 0 == pSmsContext->m_strChannelname.compare(0,9,"JXTEMPNEW"))
        {
            if ( pSmsContext->m_iStatus == Int32(SMS_STATUS_SUBMIT_SUCCESS) )
            {
                 pSmsContext->m_iStatus = Int32(SMS_STATUS_CONFIRM_SUCCESS);
            }
        }

        pSmsContext->m_lSubretDate = time(NULL);

        if(( pSmsContext->m_iStatus != Int32(SMS_STATUS_SUBMIT_SUCCESS) )
             && (pSmsContext->m_iStatus != Int32(SMS_STATUS_CONFIRM_SUCCESS) ))
        {
             pSmsContext->m_iStatus = Int32(SMS_STATUS_CONFIRM_FAIL);
        }

    }
    else
    {
        LogError("==http sendResp== [%s:%s]iSeq:%lu,status[%u],statusCode[%d],content[%s],is failed reback.",
                pSmsContext->m_strSmsId.data(),
                pSmsContext->m_strPhone.data(),
                pHttpResponse->m_iSeq,
                pHttpResponse->m_uStatus,
                statusCode,
                content.data());
        pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        pSmsContext->m_strSubmit = SOCKET_IS_CLOSE;
        pSmsContext->m_strYZXErrCode = SOCKET_IS_CLOSE;
    }

    HttpSendStatusReport( pHttpSession );
    HTTP_SESSION_FREE( pHttpSession );
    m_mapSession.erase(itr);

}


/*处理会话请求超时*/
void CHTTPSendThread::HandleTimeOut( TMsg* pMsg )
{
    HttpSendSessionMap::iterator itr = m_mapSession.find( pMsg->m_iSeq );
    if (itr == m_mapSession.end())
    {
        LogWarn("uSeq[%lu] not find in m_mapSession.", pMsg->m_iSeq);
        return;
    }

    LogNotice("==http== timeout [%s:%s] TimeOutLog:iSeq[%lu], m_mapSession.size[%d]",
                itr->second->m_pSmsContext->m_strSmsId.data(),
                itr->second->m_pSmsContext->m_strPhone.data(),
                pMsg->m_iSeq, m_mapSession.size());

    itr->second->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_SUCCESS; ///submit to dest ok,but no response.
    itr->second->m_pSmsContext->m_strSubmit = TIME_OUT;
    itr->second->m_pSmsContext->m_lSubretDate = time(NULL);
    HttpSendStatusReport( itr->second );
    HTTP_SESSION_FREE(itr->second);
    m_mapSession.erase(itr);

}


/* 解析多值返回时候错误号码不返回问题*/
void CHTTPSendThread::ConvertMutiResponse( HttpSendSession* pSession, smsp::mapStateResponse &reponses )
{
    smsp::SmsContext* pSms = pSession->m_pSmsContext;
    if( 0 == pSms->m_strChannelname.compare( 0, 4, "LJLZ" ))
    {
        std::vector<std::string> vecPhones;
        Comm::split( pSms->m_strPhone, ",",  vecPhones);
        for( UInt32 i =0; i< vecPhones.size(); i++ )
        {
            smsp::mapStateResponse::iterator itr = reponses.find( vecPhones[i] );
            if( itr == reponses.end())
            {
                smsp::StateResponse stateResp;
                stateResp._status = SMS_STATUS_SUBMIT_FAIL;
                stateResp._phone  = vecPhones[i];
                reponses[vecPhones[i]] = stateResp;
            }
        }
    }

}


void CHTTPSendThread::BindParser(string& strData, HttpSendSession* pSession )
{
    if ( NULL == pSession ||  NULL == pSession->m_pSmsContext )
    {
        LogError("******pSms is NULL.******");
        return;
    }

    smsp::SmsContext* pSms = pSession->m_pSmsContext;

    smsp::Channellib *mylib = m_pLibMng->loadChannelLibByNames(pSms->m_strChannelLibName, pSms->m_strChannelname );

    string strReason = "";
    if( mylib )
    {
        pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL;

        if( true != mylib->parseSendMutiResponse( strData, pSession->m_mapResponses, strReason ))
        {
            string state = "";
            if( true != mylib->parseSend( strData, pSms->m_strChannelSmsId, state, strReason ))
            {
                LogError("[%s:%s] mylib[%s] Parse Error.******",
                        pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                        pSms->m_strChannelname.data());
                pSms->m_strSubret.assign(strReason);
                return;
            }

            LogDebug("parseSend, smsid: %s, status: %s, desc: %s",
                        pSms->m_strSmsId.data(), state.c_str(), strReason.c_str());
            if( state == "0" ){
                pSms->m_iStatus = SMS_STATUS_SUBMIT_SUCCESS;
            }
            else{
                LogError("[%s:%s] mylib[%s] submit fail, state: %s.******",
                        pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                        pSms->m_strChannelname.data(), state.c_str());
            }

            pSms->m_strSubret.assign( strReason );
            pSession->m_mapResponses.clear();
        }
        else
        {
            pSms->m_strSubret.assign( strReason);
            smsp::mapStateResponse::iterator itr = pSession->m_mapResponses.begin();
            for( ;itr != pSession->m_mapResponses.end(); itr ++ )
            {
                /* 存在成功返回成功*/
                if( itr->second._status == 0 )
                {
                    if( !itr->second._smsId.empty()){
                        pSms->m_strChannelSmsId = itr->second._smsId;
                    }
                    pSms->m_iStatus = SMS_STATUS_SUBMIT_SUCCESS;
                    break;
                }
            }
            ConvertMutiResponse( pSession, pSession->m_mapResponses );
        }

    }
    else
    {
        pSms->m_strRemark = LIB_IS_NULL;
        pSms->m_iStatus = SMS_STATUS_SUBMIT_FAIL; ///submitResp failed.
        pSms->m_strSubmit = LIB_IS_NULL;
        pSms->m_strYZXErrCode = LIB_IS_NULL;
        LogError("[%s:%s] mylib[%s] is NULL",
                pSms->m_strSmsId.data(), pSms->m_strPhone.data(),
                pSms->m_strChannelname.data());
    }
}

UInt32 CHTTPSendThread::ParseMsgContent( string& content, vector<string>& contents,
                        string& sign, UInt32 uLongSms, UInt32 uSmslength)
{
    utils::UTFString utfHelper;

    UInt32 msgLength = utfHelper.getUtf8Length(content);
    UInt32 signLen = utfHelper.getUtf8Length(sign);

    if ((uLongSms == 0) && (msgLength > (70 - signLen)))    //not support long msg   //70       ||160
    {
        int lastIndex = 0;
        while (msgLength > 0)
        {
            int count = std::min((70 - signLen), msgLength);
            std::string substr;
            utfHelper.subUtfString(content, substr, lastIndex, lastIndex + count);
            lastIndex += count;
            msgLength -= count;
            contents.push_back(substr);
        }
        return contents.size();
    }
    else        //support long msg
    {
        contents.push_back(content);

        if(msgLength + signLen <= 70)   //70    ||160
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
}


UInt32 CHTTPSendThread::SplitMsgContent(HttpSendSession  * httpSession, vector<string>& contents, string& sign
    , bool isIncludeChinese, smsp::SmsContext *pSmsContext)
{
    utils::UTFString utfHelper;
    string& content = httpSession->m_strSendContent;
    UInt32 msgLength = utfHelper.getUtf8Length(content);
    UInt32 signLen = utfHelper.getUtf8Length(sign);

    UInt32 maxWord = httpSession->m_SmsHttpInfo.m_Channel.m_uCnLen;
    UInt32 splitLen = httpSession->m_SmsHttpInfo.m_Channel.m_uCnSplitLen;
    if (!isIncludeChinese)
    {
        maxWord = httpSession->m_SmsHttpInfo.m_Channel.m_uEnLen;
        splitLen = httpSession->m_SmsHttpInfo.m_Channel.m_uEnSplitLen;
    }
    LogDebug("maxWord=%d, splitLen=%d, phone=%s", maxWord, splitLen, httpSession->m_SmsHttpInfo.m_strPhone.data());
    contents.push_back(content);

    if(msgLength + signLen <= maxWord)  //70    ||160
    {
        return 1;
    }

    UInt32 cnt = (msgLength+signLen)/splitLen;  //67
    if((msgLength+signLen)*1.0/splitLen == cnt) //67
    {
        return cnt;
    }
    else
    {
        return cnt + 1;
    }

}


/*同步处理Https响应*/
void CHTTPSendThread::DisposeHttpsResponse( std::string strResponse, HttpSendSession* pSession )
{
    bool bLinkedStatus = true;

    if ( strResponse.empty() )
    {
        LogError("[%s:%s] send to channel failure.",
                pSession->m_pSmsContext->m_strSmsId.c_str(),
                pSession->m_pSmsContext->m_strPhone.c_str());
        pSession->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL; //submitResp failed.
        pSession->m_pSmsContext->m_strSubmit = RESPONSE_IS_NOT_200;
        pSession->m_pSmsContext->m_strYZXErrCode = RESPONSE_IS_NOT_200;
        bLinkedStatus = false;
    }
    else
    {
        BindParser( strResponse, pSession );
    }

    if ( true == bLinkedStatus )
    {
        LogNotice("[%s:%s] ==http sendResp== content[%s]",
                  pSession->m_pSmsContext->m_strSmsId.data(),
                  pSession->m_pSmsContext->m_strPhone.data(),
                  strResponse.data());

        pSession->m_pSmsContext->m_lSubretDate = time( NULL );

        if( pSession->m_pSmsContext->m_iStatus != Int32(SMS_STATUS_SUBMIT_SUCCESS) )
        {
            pSession->m_pSmsContext->m_iStatus = Int32(SMS_STATUS_CONFIRM_FAIL);
        }
    }
    else
    {
        LogError("[%s:%s] ==http sendResp== content[%s],is failed reback.",
                pSession->m_pSmsContext->m_strSmsId.data(),
                pSession->m_pSmsContext->m_strPhone.data(),
                strResponse.data());

        pSession->m_pSmsContext->m_iStatus = SMS_STATUS_SUBMIT_FAIL;
        pSession->m_pSmsContext->m_strSubmit = SOCKET_IS_CLOSE;
        pSession->m_pSmsContext->m_strYZXErrCode = SOCKET_IS_CLOSE;
    }
}

UInt32 CHTTPSendThread::GetSessionMapSize()
{
    return m_mapSession.size();
}

/*http发送状态上报*/
void CHTTPSendThread::HttpSendStatusReport( HttpSendSession* pSession )
{
    CSMSTxStatusReportMsg *txReport = new CSMSTxStatusReportMsg();

    smsp::SmsContext* pSms = pSession->m_pSmsContext ;

    LogDebug("SmSid:%s, channelSmsid:%s, status:%d",
              pSms->m_strSmsId.c_str(),
              pSms->m_strChannelSmsId.c_str(),
              pSms->m_iStatus);

    txReport->report.m_status        = pSms->m_iStatus;
    txReport->report.m_strSubmit     = pSms->m_strSubmit;
    txReport->report.m_strYZXErrCode = pSms->m_strYZXErrCode;
    txReport->report.m_strSubRet     = pSms->m_strSubret;
    txReport->report.m_strChannelLibName  = pSms->m_strChannelLibName;
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

    txReport->report.m_mapResponses = pSession->m_mapResponses;/*拷贝响应状态*/
    txReport->report.m_strErrorCode = pSms->m_strErrorCode;

    txReport->m_strSessionID = pSession->m_strSessionId;
    txReport->m_iMsgType     = MSGTYPE_CHANNEL_SEND_STATUS_REPORT;

    g_CChannelThreadPool->PostMsg(txReport);

}
