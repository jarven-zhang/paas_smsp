#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Comm.h"
#include "CUDPSendThread.h"
#include "LogMacro.h"
#include <errno.h>

UInt32 m_uThreadCnt = 0; 
std::vector< CThread * > m_vecThreads;
UInt32 m_uPoolIdx = 0;

bool CUDPSendThread::postSendMsg( TMsg* pMsg )
{
	if( m_uThreadCnt == 0 )
	{
		LogWarn("UDPSendThread Pool Not Int Count:0");
		return false;
	}

	UInt32 idx = m_uPoolIdx++ % m_uThreadCnt;
	m_vecThreads.at(idx)->PostMsg(pMsg);	
	return true;
}

CUDPSendThread::CUDPSendThread(const char *name): CThread(name)
{
}

CUDPSendThread::~CUDPSendThread()
{
}

bool CUDPSendThread::gInit( string strAddrIp, UInt32 uAddrPort, UInt32 uThreadCnt )
{
	m_vecThreads.clear();	
	m_uThreadCnt = 0;		
	m_uPoolIdx = 0;

	for( UInt32 i = 0; i< uThreadCnt ; i++ )
    {
		string threadName = "UDPSendThread";
		threadName.append("-");
		threadName.append(Comm::int2str(i));
		CUDPSendThread *pThread = new CUDPSendThread(threadName.data());
		if( pThread->Init( strAddrIp, uAddrPort ) && pThread->CreateThread())
		{
			LogNotice("Init UDPSendThread[%d:%u] SUCC", i, uThreadCnt );
			m_vecThreads.push_back(pThread);
			m_uThreadCnt++;
		}	
    }
	if( m_uThreadCnt == 0 ) {
		return false;
	}

	return true;	
}


/*ÑÓ³Ù¹Ø±Õ*/
bool CUDPSendThread::gdeInit()
{
	for( UInt32 i = 0; i < m_vecThreads.size(); i++ )
	{
		CUDPSendThread *pThread = (CUDPSendThread *)m_vecThreads.at(i);
		LogNotice("Close Thread [%s]", pThread->GetThreadName().data());
		pThread->ThreadStop();
		close(pThread->m_isockfd );
		SAFE_DELETE(pThread);
	}

	m_vecThreads.clear();	
	m_uThreadCnt = 0;		
	m_uPoolIdx = 0;
	return true;
}

bool CUDPSendThread::Init( string strIp, UInt32 uport )
{
	if( false == CThread::Init())
        return false;

	m_strIp = strIp;
	m_uPort = uport;
	return openSocket();	
}

void CUDPSendThread::HandleMsg(TMsg* pMsg)
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
		case MSGTYPE_MONITOR_UDP_SEND:
			HandleUDPSend( pMsg );
			break;
        default:
			LogWarn( "Unkown MsgType:%d", pMsg->m_iMsgType );
    }
	
}

bool CUDPSendThread::HandleUDPSend( TMsg* pMsg )
{	
	TUDPSendMsg *pSendMsg = ( TUDPSendMsg * )pMsg;
	if(pSendMsg == NULL ){
		return false;
	}	

	if( m_isockfd == -1 ){
		LogWarn("Sockete Error reset Now ");
		if(!openSocket()){
			return false;
		}
	}

	LogDebug("UDP Send [ %s ]", pSendMsg->m_strSendData.data() );
	return udpSend( pSendMsg->m_strSendData );
}

bool CUDPSendThread::openSocket()
{	
	m_isockfd = socket( AF_INET, SOCK_DGRAM, 0 );	
	if(m_isockfd == -1 )
	{
		 LogError("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		 return false;
	}
	return true;
}

#if 0
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
					 const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
					   struct sockaddr *src_addr, socklen_t *addrlen);
#endif

bool CUDPSendThread::udpSend( string &strSendData )
{
	struct sockaddr_in destAddr;
	socklen_t addrlen = sizeof(destAddr);
	
	if( m_isockfd == -1 ){
		LogError("Send [%s] Error, socket not init", strSendData.data());
		return false;
	}	
	
	bzero(&destAddr, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(m_uPort);
	destAddr.sin_addr.s_addr = inet_addr( m_strIp.data() ) ;
	if( -1 == sendto( m_isockfd, strSendData.data(), strSendData.length(),0,
		  (struct sockaddr *)&destAddr, addrlen ))
	{
		LogError("Send Data[%s] destAddr[%s:%d] Err[%s(%d)]",
				 strSendData.data(), m_strIp.data(), m_uPort, strerror(errno),errno );
		return false;
	}

	return  true;
} 
