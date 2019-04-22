#include "CHostIpThread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <time.h>
#include <errno.h>
#include "main.h"


CHostIpThread::CHostIpThread(const char *name):
             CThread(name)
{
}

CHostIpThread::~CHostIpThread()
{
}

bool CHostIpThread::Init()
{
	bool ret = CThread::Init();
	if(false == ret)
	{
		return false;
	}
	m_pTimer = SetTimer(CLEAN_MAP_TIMER_MSGID, "sessionid-clean-map", CLEAN_MAP_TIMEOUT);
	if(NULL == m_pTimer)
	{
		return false;
	}

	ret = ares_library_init(ARES_LIB_INIT_ALL);
  	if (ret != ARES_SUCCESS)
    {
    	LogError("ares_library_init: %s\n", ares_strerror(ret));
      	return false;
    }

	ret = ares_init(&m_ares);
    if(ARES_SUCCESS != ret)
    {
    	LogError("ares_init failed: %d", ret);
        return false;
    }

	return true;
}

bool SendRspMsg(THostIpReq* pHostIpReq, RESULTSTATUS result, std::string outIp)
{
	if(pHostIpReq->m_pSender == NULL)
		return false;

	THostIpResp* pHostIpResp =  new THostIpResp();
	pHostIpResp->m_iMsgType= MSGTYPE_HOSTIP_RESP;
	pHostIpResp->m_iSeq = pHostIpReq->m_iSeq;
	pHostIpResp->m_strSessionID = pHostIpReq->m_strSessionID;
	pHostIpResp->m_uIp= inet_addr(outIp.data());
	pHostIpResp->m_iResult = (int)result;
	pHostIpReq->m_pSender->PostMsg(pHostIpResp);

	return true;
}

void aresCallback (void* arg, int status, int timeouts, struct hostent* host)
{
	AresCbParam* pAresCbParam = (AresCbParam*)arg;
	THostIpReq* pHostIpReq = pAresCbParam->m_pHostIpReq;
	CHostIpThread* pHostIpThread = pAresCbParam->m_pHostIpThread;
	std::string strIP;

	if( pAresCbParam == NULL
		|| pHostIpReq == NULL )
	{
		LogWarn(" aresCallBack Para Null ");
		return;
	}

    if(ARES_SUCCESS == status &&  host != NULL && host->h_addr != NULL)
    {
		strIP = inet_ntoa(*((struct in_addr*)host->h_addr));
    	LogDebug("sucess url[%s] ip[%s], costtime[%d].",
				pHostIpReq->m_DomainUrl.data(), strIP.data(), timeouts);


		if(pHostIpReq->m_pSender != NULL)
		{
			//gethostip from user.
			SendRspMsg(pHostIpReq, SUCCEED, strIP);
			HostIP* hostip = new HostIP();
			hostip->m_domain  = pHostIpReq->m_DomainUrl;
			hostip->m_ip	  = strIP;
			hostip->m_timeadd = time(NULL);
			pHostIpThread->AddMap(hostip);
		}
		else
		{
			//request from thread timer
			HostIP* hostip = pHostIpThread->GetMap(pHostIpReq->m_DomainUrl);
			if(hostip != NULL && !(hostip->m_ip).empty())
			{
				//modify map ip
				LogDebug("replace hostip. url[%s] ip[%s], costtime[%d].",
						pHostIpReq->m_DomainUrl.data(), strIP.data(), timeouts);
				hostip->m_ip = strIP;
			}
			else
			{
				HostIP* hostip = new HostIP();
				hostip->m_domain  = pHostIpReq->m_DomainUrl;
				hostip->m_ip	  = strIP;
				hostip->m_timeadd = time(NULL);
				pHostIpThread->AddMap(hostip);
			}

		}
    }
    else
    {
    	LogError("failed url[%s] errCode[%d:%s].", pHostIpReq->m_DomainUrl.data(), status, ares_strerror(status));
		SendRspMsg(pHostIpReq, FAILURE, strIP);
	}

	delete pAresCbParam->m_pHostIpReq;
	delete pAresCbParam;
}

void CHostIpThread::CaresProcess()
{
    int nfds, count;
    fd_set readers, writers;
    timeval tvp;

    FD_ZERO(&readers);
    FD_ZERO(&writers);
    nfds = ares_fds(m_ares, &readers, &writers);
    if (0 == nfds)
		return;

    tvp.tv_sec  = 0;
	tvp.tv_usec = 0;
    count = select(nfds, &readers, &writers, NULL, &tvp);
    if(0 > count){
		LogError("select fail: %d", errno);

		return;
	}
	else if(0 == count){
		LogWarn("select timeout!");

		return;
	}

    ares_process(m_ares, &readers, &writers);
}

void CHostIpThread::MainLoop(void)
{
    WAIT_MAIN_INIT_OVER

    while(1)
    {
		m_pTimerMng->Click();
		CaresProcess();

        pthread_mutex_lock(&m_mutex);
        TMsg* pMsg = m_msgQueue.GetMsg();
        pthread_mutex_unlock(&m_mutex);

        if(pMsg == NULL)
        {
            usleep(1000*10);
        }
        else
        {
            HandleMsg(pMsg);
            delete pMsg;
        }
    }
}

void CHostIpThread::HandleMsg(TMsg* pMsg)
{
    if (NULL == pMsg)
    {
        LogError("pMsg is NULL.");
        return;
    }

    pthread_mutex_lock(&m_mutex);
	m_iCount++;
	pthread_mutex_unlock(&m_mutex);

	switch (pMsg->m_iMsgType)
	{
		case  MSGTYPE_HOSTIP_REQ:
		{
			HostIpReqProc(pMsg);
			break;
		}
		case  MSGTYPE_TIMEOUT:
		{
			CleanMap();
			if(NULL != m_pTimer)
			{
				delete m_pTimer;
				m_pTimer = NULL;
			}
			m_pTimer = NULL;
			m_pTimer = SetTimer(CLEAN_MAP_TIMER_MSGID, "", CLEAN_MAP_TIMEOUT);

			break;
		}
		default:
		{
			break;
		}
	}

}

bool CHostIpThread::HostIpReqProc(TMsg* pMsg)
{
	std::string url;
	std::string urlIP;
	std::string strIP;
	std::string::size_type posBegin;
	std::string::size_type pos;
	RESULTSTATUS res = FAILURE;
	THostIpReq* pHostIpReq = (THostIpReq*)pMsg;

	url = pHostIpReq->m_DomainUrl;
	pos = url.find_first_of("//");
	if(pos == std::string::npos)
	{
		LogError("bad uri, // not find. url[%s]", url.data());
		SendRspMsg(pHostIpReq, res, strIP);
		return false;
	}
	posBegin = pos + 2;
	if (url.length() < posBegin)
	{
		LogError("[GetHost::ReplaceUrl] url[%s] is invalid.", url.data());
		SendRspMsg(pHostIpReq, res, strIP);
		return false;
	}

	urlIP   = url.substr(posBegin);

	if (((pos = urlIP.find_first_of(":")) != std::string::npos) || ((pos = urlIP.find_first_of("/")) != std::string::npos))
	{
		urlIP = urlIP.substr(0, pos);
	}

	HostIP* hostip = NULL;
	hostip = GetMap(urlIP);
	if(hostip != NULL && !(hostip->m_ip).empty())
	{
		SendRspMsg(pHostIpReq, SUCCEED, hostip->m_ip);
		return true;
	}

	THostIpReq* pHostIpReq2Ares = new THostIpReq();
	pHostIpReq2Ares->m_DomainUrl = urlIP;
	pHostIpReq2Ares->m_iMsgType  = MSGTYPE_HOSTIP_RESP;
	pHostIpReq2Ares->m_iSeq		 = pHostIpReq->m_iSeq;
	pHostIpReq2Ares->m_pSender	 = pHostIpReq->m_pSender;
	pHostIpReq2Ares->m_strSessionID = pHostIpReq->m_strSessionID;

	AresCbParam *aresArgs = new AresCbParam();
	aresArgs->m_pHostIpReq    = pHostIpReq2Ares;
	aresArgs->m_pHostIpThread = this;
	ares_gethostbyname(m_ares, urlIP.data(), AF_INET, aresCallback, aresArgs);

	return true;
}

void CHostIpThread::AddMap(HostIP* hostip)
{
	std::map<std::string, HostIP*>::iterator it = m_mHostParsered.find(hostip->m_domain);
	if(it == m_mHostParsered.end())
	{
		m_mHostParsered.insert(std::pair<std::string, HostIP*>(hostip->m_domain,hostip));
	}
	else
	{
		HostIP* oldhostip = NULL;
		oldhostip = it->second;
		oldhostip->release();
		m_mHostParsered.erase(it);
		m_mHostParsered[hostip->m_domain] = hostip;
	}
}

HostIP* CHostIpThread::GetMap(std::string url)
{
	std::map<std::string, HostIP*>::iterator it = m_mHostParsered.find(url);
	if (it == m_mHostParsered.end())
	{
		return NULL;
	}

	return it->second;
}

void CHostIpThread::CleanMap()
{
	//HostIP* hostip = NULL;
	long nowtime   = time(NULL);
	std::map<std::string, HostIP*>::iterator it;

	LogDebug("hostipmap.size[%d]", m_mHostParsered.size());

	for(it = m_mHostParsered.begin(); it != m_mHostParsered.end(); )
	{
		long interval = nowtime - it->second->m_timeadd;
		if(interval > 3600) //stay more than one hour clean it.
		{
			LogDebug("remove url[%s], ip[%s].\n", it->second->m_domain.data(), it->second->m_ip.data());
			delete it->second;
			m_mHostParsered.erase(it++);

			continue;
		}
		LogDebug("retain url[%s], ip[%s].\n", it->second->m_domain.data(), it->second->m_ip.data());
		it++;
	}
}



