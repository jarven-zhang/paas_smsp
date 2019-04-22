#include "CHttpsPushThread.h"
#include <boost/functional/hash.hpp>
#include "Comm.h"
#include "LogMacro.h"
#include "main.h"
#include "initHelper.h"

UInt32  CHttpsSendThread::m_uThreadCount = 0;
std::vector< CThread *> CHttpsSendThread::m_uThreads;

CHttpsSendThread::CHttpsSendThread( const char *strName )
	: CThread( strName )
{
}

CHttpsSendThread::~CHttpsSendThread()
{
}

bool CHttpsSendThread::InitHttpsSendPool( UInt32 uThreadCount )
{
	LogInfo("uThreadCount: %u", uThreadCount);
	CHttpsSendThread::m_uThreads.clear();
	for( UInt32 i =0; i< uThreadCount; i++ )
	{
		string threadName = "CHttpsSendThread" + Comm::int2str(i);
		CHttpsSendThread* pThread = new CHttpsSendThread( threadName.data() );
		if( pThread->Init() && pThread->CreateThread())
		{
			m_uThreadCount++;
			m_uThreads.push_back( pThread );
			LogNotice(" Add %s Succ ", threadName.data());
		}
	}
	return true;
}

bool CHttpsSendThread::Init()
{
	if ( false == CThread::Init())
	{
		LogError("CThread::Init is failed.");
		return false;
	}
    m_pInternalService = new InternalService();
    if(m_pInternalService == NULL)
    {
        LogError("m_pInternalService is NULL.");        
        return false;
    }
    m_pInternalService->Init();	
    m_pLinkedBlockPool = new LinkedBlockPool();

	/*定时检查会话*/
	m_wheelTimer = SetTimer( CONN_CHECK_TIMER_ID, "checkHttpsHeart", CONN_CHECK_TIMER_INTERVAL );
	return true;
}

bool CHttpsSendThread::sendHttpsMsg ( TMsg* pMsg )
{
	boost::hash<std::string> hash_fn;
	THttpsRequestMsg* pHttpsReq = ( THttpsRequestMsg* ) pMsg;
	string stringHash = pHttpsReq->strUrl;

	/*未创建https发送线程，同步处理模式*/
	if( m_uThreadCount == 0 )
	{
		string strResponse;
		UInt32 ret = 0;
		http::HttpsSender httpsSender;
		ret = httpsSender.Send(pHttpsReq->uIP, pHttpsReq->strUrl, pHttpsReq->strBody, strResponse,
								pHttpsReq->m_iMsgType == MSGTYPE_HTTPS_ASYNC_GET ? 0 : 1, &pHttpsReq->vecHeader);
		LogWarn("Https Sync Request Url[%s], mode: %s, PostData[%s], Result[%d], Response[%s]",
				pHttpsReq->strUrl.data(),
				pHttpsReq->m_iMsgType == MSGTYPE_HTTPS_ASYNC_GET ? "HTTPS_MODE_GET" : "HTTPS_MODE_POST",
				pHttpsReq->strBody.data(), ret, strResponse.data());
		SAFE_DELETE(pMsg);
		return CURLE_OK == ret ;
	}

	UInt32 id = hash_fn(stringHash)% m_uThreadCount ;
	m_uThreads.at(id)->PostMsg( pMsg );

	return true;

}

void CHttpsSendThread::sendHttpMsg ( TMsg* pMsg)
{		
	boost::hash<std::string> hash_fn;
	THttpsRequestMsg* pHttpReq = ( THttpsRequestMsg* ) pMsg;
	string stringHash = pHttpReq->strUrl;
	
	if( m_uThreadCount == 0 )
	{
		LogError("not launch https push thread");
		return;
	}

	UInt32 id = hash_fn(stringHash)% m_uThreadCount ;	
	m_uThreads.at(id)->PostMsg( pMsg );
	
}

void CHttpsSendThread::HandleHttpSendReq( TMsg* pMsg)
{
	CHttpConn *conn = NULL;
	THttpsRequestMsg *pHttpReq = ( THttpsRequestMsg* )pMsg;

	conn = new CHttpConn();
	if (conn)
	{
		conn->pHttpSender = new http::HttpSender();
	    if (false == conn->pHttpSender->Init(m_pInternalService, pHttpReq->m_iSeq, this))
	    {
	        LogError("[%s:%s] ******pHttpSender->Init is failed.");
			SAFE_DELETE(conn->pHttpSender);
			SAFE_DELETE(conn);
			return;
	    }
		m_mapHttpConns[ pHttpReq->m_iSeq ] = conn;
		conn->pHttpSender->setIPCache(pHttpReq->uIP);
		conn->pHttpSender->Post(pHttpReq->strUrl, pHttpReq->strBody, &pHttpReq->vecHeader);
		conn->m_httpTimer = SetTimer(pHttpReq->m_iSeq, "checkHttpResp", 6000);
	}
	else
	{
		LogError("alloc memory fail");
	}


}

void CHttpsSendThread::HandleHttpResponseMsg(TMsg* pMsg)
{
	THttpResponseMsg* pHttpResponse = (THttpResponseMsg*)pMsg;
	if(NULL == pHttpResponse)
    {
        LogError("pHttpResponse is NULL!");
        return;
    }

	LogDebug("Log4:response, iSeq[%lu] ,status[%d], status code[%d], response[%s]", pHttpResponse->m_iSeq, pHttpResponse->m_uStatus, 
		pHttpResponse->m_tResponse.GetStatusCode(), pHttpResponse->m_tResponse.GetContent().data());
#ifdef SMSP_HTTP	
    THttpResponseMsg*  pReportPush = new THttpResponseMsg();
  
  	pReportPush->m_uStatus = pHttpResponse->m_uStatus;
    pReportPush->m_iSeq = pHttpResponse->m_iSeq;
    pReportPush->m_tResponse = pHttpResponse->m_tResponse;
	pReportPush->m_pSender = this;
	pReportPush->m_iMsgType = MSGTYPE_HTTPRESPONSE;
    g_pReportPushThread->PostMsg(pReportPush);
#endif	            			
	
	httpConnMap::iterator itr = m_mapHttpConns.find( pHttpResponse->m_iSeq );
	if( itr != m_mapHttpConns.end())
	{
		CHttpConn *conn  = itr->second ;
		conn->pHttpSender->Destroy();
		SAFE_DELETE(conn->pHttpSender);
		SAFE_DELETE(conn->m_httpTimer);
		SAFE_DELETE(conn);
		m_mapHttpConns.erase(itr);
		
	}
	
}

void CHttpsSendThread::MainLoop()
{	
    WAIT_MAIN_INIT_OVER
	LogWarn("enter in CReportPushThread MainLoop");
	TMsg* pMsg = NULL;
	while(true)
	{	
		UInt32 iSelect = m_pInternalService->GetSelector()->Select();
        m_pTimerMng->Click();	

		pthread_mutex_lock(&m_mutex);
		pMsg = m_msgQueue.GetMsg();
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
    return;
}

void CHttpsSendThread::HandleMsg( TMsg* pMsg )
{
	switch( pMsg->m_iMsgType )
	{
		case MSGTYPE_HTTPS_ASYNC_GET:
			HandleHttpsSendReq(pMsg, HTTPS_MODE_GET);
			break;

		case MSGTYPE_HTTPS_ASYNC_POST:
			HandleHttpsSendReq(pMsg, HTTPS_MODE_POST);
			break;
		case MSGTYPE_HTTP_ASYNC_POST:
			HandleHttpSendReq(pMsg);
			break;
		case MSGTYPE_HTTPRESPONSE:
			HandleHttpResponseMsg(pMsg);
			break;
		case MSGTYPE_TIMEOUT:
			HandleTimeOut(pMsg);
			break;

		default:
			LogWarn("unKnown MsgType :0x%x ", pMsg->m_iMsgType );
	}
}

void CHttpsSendThread::HandleTimeOut(TMsg* pMsg)
{
	if (pMsg->m_strSessionID == "checkHttpsHeart")
	{
		time_t now = time( NULL );
		httpsConnMap::iterator itr = m_mapHttpsConns.begin();
		for( ;itr != m_mapHttpsConns.end();)
		{
			CHttpsConn *conn = itr->second;
			if( !conn->httpsSender.isAvailable() /* 会话不可用删除*/
				|| ( now - conn->connLastSendTime ) > CONN_MAX_IDLE_TIME )/* 超过 最大空闲时间*/
			{
				LogWarn("HTTPS Session Delete Url:%s, IdleTime:( cur:%lu, max:%lu )",
						itr->first.data(), now - conn->connLastSendTime, CONN_MAX_IDLE_TIME);
				SAFE_DELETE(conn);
				m_mapHttpsConns.erase(itr++);
				continue;
			}
			itr++;
		}

		SAFE_DELETE( m_wheelTimer );
		m_wheelTimer = SetTimer( CONN_CHECK_TIMER_ID, "checkHttpsHeart", CONN_CHECK_TIMER_INTERVAL );
	}
	else if (pMsg->m_strSessionID == "checkHttpResp")
	{
		httpConnMap::iterator itr = m_mapHttpConns.find( pMsg->m_iSeq );
#ifdef SMSP_HTTP		
		THttpResponseMsg*  pReportPush = new THttpResponseMsg();
	  	pReportPush->m_uStatus = THttpResponseMsg::HTTP_TIMEOUT;
	    pReportPush->m_iSeq = pMsg->m_iSeq;
		pReportPush->m_pSender = this;
		pReportPush->m_iMsgType = MSGTYPE_HTTPRESPONSE;
	    g_pReportPushThread->PostMsg(pReportPush);
#endif		
		if( itr != m_mapHttpConns.end())
		{
			CHttpConn *conn  = itr->second ;
			conn->pHttpSender->Destroy();
			SAFE_DELETE(conn->pHttpSender);
			SAFE_DELETE(conn->m_httpTimer);
			SAFE_DELETE(conn);
			m_mapHttpConns.erase(itr);
			
		}

		LogWarn("http report response timer out,seq=%lu", pMsg->m_iSeq);
	}
	else
	{
		LogError("m_strSessionID[%s] is error", pMsg->m_strSessionID.c_str());
	}

}

UInt32 CHttpsSendThread::HandleHttpsSendReq( TMsg* pMsg, int mode)
{
	UInt32 ret = CHTTPS_SEND_STATUS_NONE;
	Int32 curlStatus = 0;
	CHttpsConn *conn = NULL;

	std::string strResponse = "";

	THttpsRequestMsg *pHttpsReq = ( THttpsRequestMsg* )pMsg;

	httpsConnMap::iterator itr = m_mapHttpsConns.find( pHttpsReq->strUrl );
	if( itr != m_mapHttpsConns.end())
	{
		conn = itr->second ;
		if( !conn->httpsSender.isAvailable())
		{
			LogNotice("HTTPS Session Cache Failed, URL:%s ", pHttpsReq->strUrl.data());
			SAFE_DELETE(conn);
			m_mapHttpsConns.erase(itr);
			conn = NULL;
		}
	}

	if( !conn ){
		conn = new CHttpsConn();
		m_mapHttpsConns[ pHttpsReq->strUrl ] = conn;
	}

	/*更新最后发送时间*/
	conn->connLastSendTime = time( NULL );

	curlStatus = conn->httpsSender.Send(pHttpsReq->uIP, pHttpsReq->strUrl, pHttpsReq->strBody,
																strResponse, mode, &pHttpsReq->vecHeader);

	if( curlStatus != CURLE_OK)
	{
		ret = CHTTPS_SEND_STATUS_CONN_ERR;
		LogError("[%s]https send err! { Url:%s, mode: %s, PostData:%s, result:%d, responseData:%s } ",
				    GetThreadName().data(), pHttpsReq->strUrl.data(),
				    HTTPS_MODE_GET == mode ? "HTTPS_MODE_GET" : "HTTPS_MODE_POST",
				    pHttpsReq->strBody.data(), curlStatus, strResponse.data());
	}
	else
	{
		ret = CHTTPS_SEND_STATUS_SUCC;
		LogNotice("[%s]https send,{ Url:%s, mode: %s, PostData:%s, result:%d, responseData:%s } ",
					  GetThreadName().data(), pHttpsReq->strUrl.data(),
					  HTTPS_MODE_GET == mode ? "HTTPS_MODE_GET" : "HTTPS_MODE_POST",
					  pHttpsReq->strBody.data(), curlStatus, strResponse.data());
	}

	/*结果推送到发送线程*/
	if( pMsg->m_pSender != NULL )
	{
		THttpsResponseMsg *pResponseMsg = new THttpsResponseMsg();
		pResponseMsg->m_iMsgType = MSGTYPE_HTTPS_ASYNC_RESPONSE;
		pResponseMsg->m_uStatus = (curlStatus == CURLE_OK ? CHTTPS_SEND_STATUS_SUCC : CHTTPS_SEND_STATUS_FAIL);
		pResponseMsg->m_strResponse = strResponse;
		pResponseMsg->m_iSeq = pMsg->m_iSeq;
		pMsg->m_pSender->PostMsg( pResponseMsg );
	}

	return ret ;

}
