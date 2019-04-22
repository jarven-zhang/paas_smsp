#include "ReportGetThread.h"
#include "main.h"
#include "SmsChlInterval.h"
#include "Comm.h"
#include "CHttpsPushThread.h"

ReportGetThread::ReportGetThread(const char *name) : CThread(name)
{

}

ReportGetThread::~ReportGetThread()
{

}

bool ReportGetThread::Init()
{
    if (false == CThread::Init())
    {
        LogError("CThread::Init is failed.");
        return false;
    }

    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.")
        return false;
    }
    m_pInternalService->Init();

    m_pLibMng = new ChannelMng();
    if (NULL == m_pLibMng)
    {
        LogError("m_pLibMng is NULL.");
        return false;
    }

    m_pLinkedBlockPool = new LinkedBlockPool();



    ChannelMap mapChannel;
    g_pRuleLoadThread->getChannlelMap(mapChannel);

    for (ChannelMap::iterator itr = mapChannel.begin(); itr != mapChannel.end();++itr)
    {
        if ((itr->second.httpmode != 1) && (itr->second.httpmode != 2))
        {
            continue;
        }

        if (itr->second.querystateurl.length() < 11)
        {
            continue;
        }

        TReportGetChannels* pReportChannel = new TReportGetChannels();
        pReportChannel->m_iChannelID = itr->second.channelID;
        pReportChannel->m_iHttpmode = itr->second.httpmode;
        pReportChannel->m_strChannelname = itr->second.channelname;
        pReportChannel->m_strChannelLibName = itr->second.m_strChannelLibName;
        pReportChannel->m_strRtUrl = itr->second.querystateurl;
        pReportChannel->m_strRtData = itr->second.querystatepostdata;
        pReportChannel->m_strMoUrl = itr->second.m_strQueryUpUrl;
        pReportChannel->m_strMoData = itr->second.m_strQueryUpPostData;
        pReportChannel->m_uChannelType = itr->second.m_uChannelType;
        pReportChannel->m_uChannelIdentify = itr->second.m_uChannelIdentify;
        pReportChannel->m_uClusterType  = itr->second.m_uClusterType;
        pReportChannel->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (itr->second.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
        pReportChannel->m_pTimer = SetTimer(itr->second.channelID,"TIMER_GET_RT",20*1000);
        m_mapChannelInfo[itr->second.channelID] = pReportChannel;
    }
    return true;
}

void ReportGetThread::MainLoop()
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

void ReportGetThread::HandleMsg(TMsg* pMsg)
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
        case MSGTYPE_HOSTIP_RESP:
        {
            THostIpResp* pHostIpResp = (THostIpResp*)pMsg;
            HandleHostIpRespMsg(pHostIpResp->m_iSeq, pHostIpResp->m_iResult, pHostIpResp->m_uIp, pHostIpResp->m_strSessionID);
            break;
        }
        case MSGTYPE_RULELOAD_CHANNEL_UPDATE_REQ:
        {
            HandleChannelInfoUpdateMsg(pMsg);
            break;
        }
        case MSGTYPE_HTTPS_ASYNC_RESPONSE:
        {
            HandleHttpResponseMsg(pMsg, true);

            break;
        }
        case MSGTYPE_HTTPRESPONSE:
        {
            HandleHttpResponseMsg(pMsg);
            break;
        }
        case MSGTYPE_TIMEOUT:
        {
            HandleTimeOutMsg(pMsg);
            break;
        }
        default:
        {
            LogWarn("msgType[%d] is invalid.",pMsg->m_iMsgType);
            break;
        }
    }
}

void ReportGetThread::HandleHostIpRespMsg(UInt64 iSeq, UInt32 uResult, UInt32 uIp, string strSessionID)
{
    TReportGetSessionMap::iterator itr = m_mapSession.find(iSeq);
    if (itr == m_mapSession.end())
    {
        LogWarn("iSeq[%lu] is not find m_mapSession.", iSeq);
        return;
    }

    if (0 != uResult)
    {
        LogError("iSeq[%lu] hostIp is failed.",iSeq);
        if (NULL != itr->second->m_timer)
        {
            delete itr->second->m_timer;
        }
        if (NULL != itr->second->m_pHttpSender)
        {
            itr->second->m_pHttpSender->Destroy();
            delete itr->second->m_pHttpSender;
        }

        delete itr->second;
        m_mapSession.erase(itr);
        return;
    }

    if (1 == itr->second->m_iHttpmode){
        /*
            *   Get
            */
        if(0 == itr->second->m_strUrl.compare(0, 5, "https")){
            THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
            pHttpsReq->strBody = itr->second->m_strData;
            pHttpsReq->strUrl  = itr->second->m_strUrl;
            pHttpsReq->uIP     = uIp;
            pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_GET;
            pHttpsReq->m_iSeq     = itr->first;
            pHttpsReq->m_pSender  = this;

            CHttpsSendThread::sendHttpsMsg ( pHttpsReq );
            //itr->second->m_timer = SetTimer(itr->first, "", 5000);
        }
        else{
            http::HttpSender* pSender = new http::HttpSender();
            if (false == pSender->Init(m_pInternalService, iSeq, this))
            {
                LogError("pSender->Init is failed.");
                delete pSender;
                if (NULL != itr->second->m_timer)
                {
                    delete itr->second->m_timer;
                }
                if (NULL != itr->second->m_pHttpSender)
                {
                    itr->second->m_pHttpSender->Destroy();
                    delete itr->second->m_pHttpSender;
                }

                delete itr->second;
                m_mapSession.erase(itr);
                return;
            }
            pSender->setIPCache(uIp);
            itr->second->m_pHttpSender = pSender;
            pSender->Get(itr->second->m_strUrl);
        }
    }
    else{
        std::vector<std::string> vs;
        string postData;

        /*
            *   POST
            */
        if (0 == strncmp(itr->second->m_strChannelname.data(),"XJCZ",strlen("XJCZ"))){
            vs.push_back("Content-Type:text/xml; charset=utf-8");
            vs.push_back("Action:\"getreportsms2\"");
            postData = itr->second->m_strData;
        }
        else if (0 == strncmp(itr->second->m_strChannelname.data(),"DCG",strlen("DCG"))){
            //app_key=4fb265850d938cc904ba26e050501d30&secret=759b56&timestamp=%timestamp%&check=%check%
            vs.push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");

            string data = itr->second->m_strData;
            std::map<string, string> mapData;
            Comm::splitExMap(data, "&", mapData);

            time_t t = time(0);
            char ctime[32] = {0};
            snprintf(ctime, sizeof(ctime), "%lu", (uint64_t)t);
            string strtimestamp = ctime;
            string key = mapData["secret"] + mapData["app_key"] + strtimestamp;

            string check;
            unsigned char md5[16] = {0};
            MD5((const unsigned char*)key.data(), key.length(), md5);

            std::string HEX_CHARS = "0123456789abcdef";
            for (int i = 0; i < 16; i++)
            {
                check.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
                check.append(1, HEX_CHARS.at(md5[i] & 0x0F));
            }

            Comm::string_replace(data, "&secret=" + mapData["secret"], "");
            Comm::string_replace(data, "%timestamp%", strtimestamp);
            Comm::string_replace(data, "%check%", check);

            postData = data;
        }
        else if (0 == strncmp(itr->second->m_strChannelname.data(),"CJXX",strlen("CJXX"))){
            //Account=tedingL&Password=123&Time=
            vs.push_back("Content-Type: application/x-www-form-urlencoded; charset=gb2312");

            string data = itr->second->m_strData;
            if (strSessionID == "RT")
            {
                time_t t = time(0); //20171206
                char strDate[64];
                strftime( strDate, sizeof(strDate), "%Y%m%d",localtime(&t) );

                data += string(strDate);
            }
            postData = data;

            LogDebug("url[%s] data[%s]", itr->second->m_strUrl.data(), data.data());
        }
        else if (0 == strncmp(itr->second->m_strChannelname.data(),"TESTIN",strlen("TESTIN"))){
            /////12545,sffg35//first apikey second secretkey
            struct timeval end;
            gettimeofday(&end, NULL);
            long msec = end.tv_sec * 1000 + end.tv_usec/1000;
            std::string strTime = "";
            char buf[32] = {0};
            sprintf(buf,"%ld",msec);
            strTime.assign(buf);

            int pos = 0;
            int length = itr->second->m_strData.length();
            pos = itr->second->m_strData.find(',');

            std::string apiKey = itr->second->m_strData.substr(0,pos);
            std::string secretKey = itr->second->m_strData.substr(pos + 1,length - pos -1);
            std::string op = "";

            if (strSessionID == "RT")
            {
                op = "Sms.status";
            }
            else if (strSessionID == "MO")
            {
                op = "Sms.mo";
            }
            else
            {
                LogError("strSessionID[%s] is invalid.",strSessionID.data());
            }

            std::string ts = strTime;

            std::string temp = "apiKey";
            temp.append("=");
            temp.append(apiKey);
            temp.append("op");
            temp.append("=");
            temp.append(op);
            temp.append("ts");
            temp.append("=");
            temp.append(ts);
            temp.append(secretKey);

            unsigned char md5[16] = {0};
            MD5((const unsigned char*) temp.data(), temp.length(), md5);
            std::string strMD5 = "";
            std::string HEX_CHARS = "0123456789abcdef";
            for (int i = 0; i < 16; i++)
            {
                strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
                strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
            }
            std::string sig = strMD5;

            std::string strJson = "";
            strJson.append("{");
            strJson.append("\"op\"");
            strJson.append(":");
            strJson.append("\"");
            strJson.append(op);
            strJson.append("\"");
            strJson.append(",");

            strJson.append("\"apiKey\"");
            strJson.append(":");
            strJson.append("\"");
            strJson.append(apiKey);
            strJson.append("\"");
            strJson.append(",");

            strJson.append("\"ts\"");
            strJson.append(":");
            strJson.append(ts);
            strJson.append(",");

            strJson.append("\"sig\"");
            strJson.append(":");
            strJson.append("\"");
            strJson.append(sig);
            strJson.append("\"");
            strJson.append("}");

            postData = strJson;
        }
        else if (0 == strncmp(itr->second->m_strChannelname.data(),"XXD",strlen("XXD"))){
            LogDebug("******get report xxd.");
            vs.push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
            postData = itr->second->m_strData;
        }
		else if (0 == strncmp(itr->second->m_strChannelname.data(),"XHR",strlen("XHR"))){
            LogDebug("******get report xhr.");
            vs.push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
            postData = itr->second->m_strData;
        }
        else{
            vs.clear();
            postData = itr->second->m_strData;
        }

        if(0 == itr->second->m_strUrl.compare(0, 5, "https")){
            THttpsRequestMsg *pHttpsReq = new THttpsRequestMsg();
            pHttpsReq->strBody = postData;
            pHttpsReq->strUrl  = itr->second->m_strUrl;
            pHttpsReq->uIP     = uIp;
            pHttpsReq->m_iMsgType = MSGTYPE_HTTPS_ASYNC_POST;
            pHttpsReq->m_iSeq     = itr->first;
            pHttpsReq->m_pSender  = this;

            CHttpsSendThread::sendHttpsMsg ( pHttpsReq );
        }
        else{
            http::HttpSender* pSender = new http::HttpSender();
            if (false == pSender->Init(m_pInternalService, iSeq, this))
            {
                LogError("pSender->Init is failed.");
                delete pSender;
                if (NULL != itr->second->m_timer)
                {
                    delete itr->second->m_timer;
                }
                if (NULL != itr->second->m_pHttpSender)
                {
                    itr->second->m_pHttpSender->Destroy();
                    delete itr->second->m_pHttpSender;
                }

                delete itr->second;
                m_mapSession.erase(itr);
                return;
            }
            pSender->setIPCache(uIp);
            itr->second->m_pHttpSender = pSender;

            pSender->Post(itr->second->m_strUrl, itr->second->m_strData, &vs);
        }
    }
}


void ReportGetThread::HandleChannelInfoUpdateMsg(TMsg* pMsg)
{
    TUpdateChannelReq* pUpdateChannelReq = (TUpdateChannelReq*)pMsg;

    //// remove is delete channel
    for (TReportGetChannelsMap::iterator itr = m_mapChannelInfo.begin();itr != m_mapChannelInfo.end();)
    {
        ChannelMap::iterator iter = pUpdateChannelReq->m_ChannelMap.find(itr->first);
        if (iter == pUpdateChannelReq->m_ChannelMap.end())
        {
            LogWarn("delete channelId[%d],channelName[%s].",
                    itr->second->m_iChannelID, itr->second->m_strChannelname.data());

            /* http 模式下删除SO handler*/
            if( HTTP_GET == itr->second->m_iHttpmode
                || HTTP_POST == itr->second->m_iHttpmode ){
                m_pLibMng->delChannelLib( itr->second->m_strChannelLibName );
                m_pLibMng->delChannelLib( itr->second->m_strChannelname );
            }

            delete itr->second->m_pTimer;
            delete itr->second;
            m_mapChannelInfo.erase(itr++);
        }
        else
        {
            itr++;
        }
    }

    for(ChannelMap::iterator itor = pUpdateChannelReq->m_ChannelMap.begin();itor != pUpdateChannelReq->m_ChannelMap.end();++itor)
    {
        if ((itor->second.httpmode != 1) && (itor->second.httpmode != 2))
        {
            continue;
        }

        TReportGetChannelsMap::iterator iter = m_mapChannelInfo.find(itor->first);
        if (iter == m_mapChannelInfo.end())
        {
            if (itor->second.querystateurl.length() < 11)
            {
                continue;
            }
            /// add new
            TReportGetChannels* pReportChannel = new TReportGetChannels();
            pReportChannel->m_iChannelID = itor->second.channelID;
            pReportChannel->m_iHttpmode = itor->second.httpmode;
            pReportChannel->m_strChannelname = itor->second.channelname;
            pReportChannel->m_strChannelLibName = itor->second.m_strChannelLibName;
            pReportChannel->m_strRtUrl = itor->second.querystateurl;
            pReportChannel->m_strRtData = itor->second.querystatepostdata;
            pReportChannel->m_strMoUrl = itor->second.m_strQueryUpUrl;
            pReportChannel->m_strMoData = itor->second.m_strQueryUpPostData;
            pReportChannel->m_uChannelType = itor->second.m_uChannelType;
            pReportChannel->m_uChannelIdentify = itor->second.m_uChannelIdentify;
            pReportChannel->m_uClusterType     = itor->second.m_uClusterType;
            pReportChannel->m_strMoPrefix      = itor->second.m_strMoPrefix;

            pReportChannel->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (itor->second.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
            pReportChannel->m_pTimer = SetTimer(itor->second.channelID,"TIMER_GET_RT",10*1000);
            m_mapChannelInfo[itor->second.channelID] = pReportChannel;
        }
        else
        {
            if (itor->second.querystateurl.length() < 11)
            {
                delete iter->second->m_pTimer;
                delete iter->second;
                m_mapChannelInfo.erase(iter);
                continue;
            }
            /// modie old
            iter->second->m_iHttpmode = itor->second.httpmode;
            iter->second->m_strRtUrl = itor->second.querystateurl;
            iter->second->m_strRtData = itor->second.querystatepostdata;
            iter->second->m_strMoUrl = itor->second.m_strQueryUpUrl;
            iter->second->m_strMoData = itor->second.m_strQueryUpPostData;
            iter->second->m_strChannelname = itor->second.channelname;
            iter->second->m_strChannelLibName = itor->second.m_strChannelLibName;
            iter->second->m_uChannelType = itor->second.m_uChannelType;
            iter->second->m_uChannelIdentify = itor->second.m_uChannelIdentify;
            iter->second->m_uClusterType  = itor->second.m_uClusterType;
            iter->second->m_strMoPrefix   = itor->second.m_strMoPrefix;
            iter->second->m_bSaveReport = (CHANNEL_REPORT_CACHE_FALG == (itor->second.m_uExValue & CHANNEL_REPORT_CACHE_FALG));
        }
    }
}

void ReportGetThread::HandleHttpResponseMsg(TMsg* pMsg, bool is_https){
    LogDebug("=== iSeq[%lu].",pMsg->m_iSeq);

    TReportGetSessionMap::iterator iter = m_mapSession.find(pMsg->m_iSeq);
    if (iter == m_mapSession.end())
    {
        LogError("iSeq[%lu] is not find in m_mapSession.", pMsg->m_iSeq);
        return;
    }

    UInt32 uReportSize = 0;

    //请求应答异常
    if(is_https == false ? (((THttpResponseMsg *)pMsg)->m_uStatus != THttpResponseMsg::HTTP_RESPONSE)
                            : (((THttpsResponseMsg *)pMsg)->m_uStatus != CHTTPS_SEND_STATUS_SUCC)){
        LogWarn("iSeq[%lu] return[%d] is unexpected", pMsg->m_iSeq,
                (is_https == false ? ((THttpResponseMsg *)pMsg)->m_uStatus : ((THttpsResponseMsg *)pMsg)->m_uStatus));
    }
    else
    {
        string content = (is_https == false ? ((THttpResponseMsg *)pMsg)->m_tResponse.GetContent()
                                             : ((THttpsResponseMsg *)pMsg)->m_strResponse);
        LogNotice("report content[%s]", content.data());
        StateReportVector report;
        UpstreamSmsVector upsmsRes;
        string strResponse;
        smsp::Channellib *mylib = m_pLibMng->loadChannelLibByNames(iter->second->m_strChannelLibName, iter->second->m_strChannelname);

        if(mylib != NULL && mylib->parseReport(content, report, upsmsRes,strResponse))
        {
            LogInfo("parseReport suc, report size: %d, upsmsRes size: %d", report.size(), upsmsRes.size());
            uReportSize = report.size();
            ////parse Rt
            for(StateReportVector::iterator it = report.begin(); it != report.end(); it++)
            {
                TReportToDispatchReqMsg* req = new TReportToDispatchReqMsg();
                req->m_iStatus = it->_status;
                req->m_lReportTime = it->_reportTime;
                if (req->m_iStatus != Int32(SMS_STATUS_CONFIRM_SUCCESS))
                {
                    req->m_strDesc = it->_desc;
                }
                else
                {
                    req->m_strDesc = it->_desc;
                }

                req->m_strChannelSmsId = it->_smsId;
                req->m_uChannelId = iter->second->m_uChannelId;
                req->m_strChannelLibName = iter->second->m_strChannelLibName;
                req->m_uChannelIdentify = iter->second->m_uChannelIdentify;
                req->m_uClusterType     = iter->second->m_uClusterType;
                req->m_strPhone         = Comm::trim(it->_phone);
                req->m_bSaveReport = iter->second->m_bSaveReport;
                req->m_iMsgType = MSGTYPE_REPORT_TO_DISPATCH_REQ;
                g_pDispatchThread->PostMsg(req);
            }
            ////parse Mo
            for (UpstreamSmsVector::iterator itr = upsmsRes.begin(); itr != upsmsRes.end(); ++itr)
            {
                TUpStreamToDispatchReqMsg* pUpStream = new TUpStreamToDispatchReqMsg();
                pUpStream->m_iChannelId = iter->second->m_uChannelId;
                pUpStream->m_strChannelLibName = iter->second->m_strChannelLibName;
                pUpStream->m_lUpTime = itr->_upsmsTimeInt;
                pUpStream->m_strAppendId = itr->_appendid;
                pUpStream->m_strContent = itr->_content;
                pUpStream->m_strPhone = Comm::trim(itr->_phone);
                pUpStream->m_strUpTime = itr->_upsmsTimeStr;
                pUpStream->m_uChannelType = iter->second->m_uChannelType;
                pUpStream->m_strMoPrefix  = iter->second->m_strMoPrefix;
                pUpStream->m_iMsgType = MSGTYPE_UPSTREAM_TO_DISPATCH_REQ;
                g_pDispatchThread->PostMsg(pUpStream);
            }
        }
        else
        {
            LogWarn("report parse failed,channelName[%s],channelLibName[%s],response[%s],query[%s]",
            iter->second->m_strChannelname.data(), iter->second->m_strChannelLibName.data(), content.data(), iter->second->m_strUrl.data());
        }
    }

    SetTimerSend(uReportSize,iter->second->m_uChannelId,iter->second->m_strChannelLibName, iter->second->m_strChannelname);

    if (NULL != iter->second->m_timer)
    {
        delete iter->second->m_timer;
    }
    if (NULL != iter->second->m_pHttpSender)
    {
        iter->second->m_pHttpSender->Destroy();
        delete iter->second->m_pHttpSender;
    }

    delete iter->second;
    m_mapSession.erase(iter);

}

void ReportGetThread::HandleTimeOutMsg(TMsg* pMsg)
{
    if (0 == pMsg->m_strSessionID.compare("TIMER_GET_RT"))
    {
        ////timer get status
        TReportGetChannelsMap::iterator itr = m_mapChannelInfo.find(pMsg->m_iSeq);
        if (itr == m_mapChannelInfo.end())
        {
            LogWarn("timer get status channelId[%lu] is not find m_mapChannelInfo.",pMsg->m_iSeq);
            return;
        }

        ////Rt
        UInt64 uRtSeq = m_SnMng.getSn();
        TReportGetSession* pRtSession = new TReportGetSession();
        pRtSession->m_uChannelId = itr->first;
        pRtSession->m_iHttpmode = itr->second->m_iHttpmode;
        pRtSession->m_strChannelname = itr->second->m_strChannelname;
        pRtSession->m_strChannelLibName = itr->second->m_strChannelLibName;
        pRtSession->m_strUrl = itr->second->m_strRtUrl;
        pRtSession->m_strData = itr->second->m_strRtData;
        pRtSession->m_uChannelType = itr->second->m_uChannelType;
        pRtSession->m_uChannelIdentify = itr->second->m_uChannelIdentify;
        pRtSession->m_uClusterType   = itr->second->m_uClusterType;
        pRtSession->m_bSaveReport    = itr->second->m_bSaveReport;
        pRtSession->m_strMoPrefix    = itr->second->m_strMoPrefix;

        pRtSession->m_timer = SetTimer(uRtSeq,"Session TimeOut",5*1000);
        LogDebug("=== uRtSeq[%ld].",uRtSeq);
        m_mapSession[uRtSeq] = pRtSession;
        string strRtIp = "";
        if (true == m_SmspUtils.CheckIPFromUrl(itr->second->m_strRtUrl,strRtIp))
        {
            HandleHostIpRespMsg(uRtSeq,0,inet_addr(strRtIp.data()),"RT");
        }
        else
        {
            THostIpReq* pHost = new THostIpReq();
            pHost->m_DomainUrl = itr->second->m_strRtUrl;
            pHost->m_iMsgType = MSGTYPE_HOSTIP_REQ;
            pHost->m_iSeq = uRtSeq;
            pHost->m_strSessionID = "RT";
            pHost->m_pSender = this;
            g_pHostIpThread->PostMsg(pHost);
        }

        if (itr->second->m_strMoUrl.length() > 11)
        {
            UInt64 uMoSeq = m_SnMng.getSn();
            TReportGetSession* pMoSession = new TReportGetSession();
            pMoSession->m_uChannelId = itr->first;
            pMoSession->m_iHttpmode = itr->second->m_iHttpmode;
            pMoSession->m_strChannelname = itr->second->m_strChannelname;
            pMoSession->m_strChannelLibName = itr->second->m_strChannelLibName;
            pMoSession->m_strUrl = itr->second->m_strMoUrl;
            pMoSession->m_strData = itr->second->m_strMoData;
            pMoSession->m_uChannelType = itr->second->m_uChannelType;
            pMoSession->m_timer = SetTimer(uMoSeq,"Session TimeOut",5*1000);
            LogDebug("=== uMoSeq[%ld].",uMoSeq);
            m_mapSession[uMoSeq] = pMoSession;
            string strMoIp = "";
            if (true == m_SmspUtils.CheckIPFromUrl(itr->second->m_strMoUrl,strMoIp))
            {
                HandleHostIpRespMsg(uMoSeq,0,inet_addr(strMoIp.data()),"MO");
            }
            else
            {
                THostIpReq* pHost = new THostIpReq();
                pHost->m_DomainUrl = itr->second->m_strMoUrl;
                pHost->m_iMsgType = MSGTYPE_HOSTIP_REQ;
                pHost->m_iSeq = uMoSeq;
                pHost->m_strSessionID = "MO";
                pHost->m_pSender = this;
                g_pHostIpThread->PostMsg(pHost);
            }
        }
        delete itr->second->m_pTimer;
        itr->second->m_pTimer = NULL;
        itr->second->m_pTimer = SetTimer(itr->first, "TIMER_GET_RT",20*1000);
    }
    else if (0 == pMsg->m_strSessionID.compare("Session TimeOut"))
    {
        ////session timeout
        TReportGetSessionMap::iterator iter = m_mapSession.find(pMsg->m_iSeq);
        if (iter == m_mapSession.end())
        {
            LogWarn("Session TimeOut iSeq[%lu] is not find m_mapSession.",pMsg->m_iSeq);
            return;
        }

        if (NULL != iter->second->m_timer)
        {
            delete iter->second->m_timer;
        }
        if (NULL != iter->second->m_pHttpSender)
        {
            iter->second->m_pHttpSender->Destroy();
            delete iter->second->m_pHttpSender;
        }

        delete iter->second;
        m_mapSession.erase(iter);

    }
    else
    {
        LogError("m_strSessionID[%s] is invalid.",pMsg->m_strSessionID.data());
    }
}

void ReportGetThread::SetTimerSend(UInt32 uReportsize,UInt32 uChannelId, string& strChannelLibName, string& strChannelName)
{
    UInt32 intvelTime = 35;
    if (0 == uReportsize)
    {
        TReportGetChannelsMap::iterator itr = m_mapChannelInfo.find(uChannelId);
        if (itr != m_mapChannelInfo.end())
        {
            delete itr->second->m_pTimer;
            itr->second->m_pTimer = NULL;
            itr->second->m_pTimer = SetTimer(itr->first, "TIMER_GET_RT",intvelTime*1000);
        }
        return;
    }

    smsp::Channellib *mylib = m_pLibMng->loadChannelLibByNames(strChannelLibName, strChannelName);
    if(NULL == mylib)
    {
        LogError("loadChannelLibByNames  mylib is NULL.")
        return;
    }

    SmsChlInterval chIntl;
    mylib->getChannelInterval(chIntl);

    LogDebug("get chintl, _itlTimeLev2[%d], _itlValLev1[%d], _itlTimeLev1[%d]",  chIntl._itlTimeLev2, chIntl._itlValLev1, chIntl._itlTimeLev1);

    if(uReportsize >= chIntl._itlValLev1)//  > _itlValLev1
    {
        intvelTime = chIntl._itlTimeLev2;
    }
    else if(uReportsize >=0 && uReportsize < chIntl._itlValLev1)
    {
        intvelTime = chIntl._itlTimeLev1;
    }
    else    //<0
    {
        LogWarn("smsNum error, smsNum[%d]", uReportsize);
    }

    TReportGetChannelsMap::iterator itr = m_mapChannelInfo.find(uChannelId);
    if (itr != m_mapChannelInfo.end())
    {
        delete itr->second->m_pTimer;
        itr->second->m_pTimer = NULL;
        itr->second->m_pTimer = SetTimer(itr->first, "TIMER_GET_RT",intvelTime*1000);
    }
}

UInt32 ReportGetThread::GetSessionMapSize()
{
    return m_mapSession.size();
}


