#include "internalserversocket.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined(OS_LINUX) || defined(OS_MACX)
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif defined(OS_WIN32)
#include <winsock2.h>
#endif

#include "address.h"
#include "eventtype.h"

#include "internalsocket.h"
#include "LogMacro.h"




InternalServerSocket::InternalServerSocket(InternalService * pInternalServer, CThread* pThread)
{
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    server_ = pInternalServer;
    m_pThread = pThread;
}

InternalServerSocket::~InternalServerSocket()
{

}

bool InternalServerSocket::Listen(const Address &address)
{

    int flags = fcntl(socket_, F_GETFL);
    if (flags == -1)
    {
        return false;
    }

    flags = fcntl(socket_, F_SETFL, flags |= O_NONBLOCK);
    if (flags == -1)
    {
        //server_->GetTaskManager()->Schedule(this, TASK_FAILED);
        return false;
    }

    int optval = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0)
    {
        //server_->GetTaskManager()->Schedule(this, TASK_FAILED);
        return false;
    }


    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = address.GetIp();
    addr.sin_port = htons((u_short)address.GetPort());

    if (::bind(socket_, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        //server_->GetTaskManager()->Schedule(this, TASK_FAILED);

        LogErrorP("Call bind failed. ip[%s], port[%u].",
            address.GetInetAddress().data(), address.GetPort());
        return false;
    }

    if (listen(socket_, 128) == -1)
    {
        //server_->GetTaskManager()->Schedule(this, TASK_FAILED);

        LogErrorP("Call listen failed. ip[%s], port[%u].",
            address.GetInetAddress().data(), address.GetPort());
        return false;
    }

    server_->GetSelector()->Add(this, EPollSelector::SELECT_READ);
    return true;
}

void InternalServerSocket::Close()
{
    close(socket_);
    server_->GetSelector()->Remove(this);
}

int InternalServerSocket::GetDescriptor()
{
    return socket_;
}

void InternalServerSocket::OnRead()
{
    while (true)
    {
        sockaddr_in addr;

        socklen_t len = sizeof(addr);

        int socket = accept(socket_, (struct sockaddr*) &addr, &len);

        if (socket == -1)
        {
            if(errno == EAGAIN)
            {
                //LogError("accept error,EAGAIN, error[%s]", strerror(errno));
                break;
            }
            LogError("###$$$### accept error, error[%s]", strerror(errno));
            continue;
        }
        else
        {
            string strIp = "";
            char* pTemp = inet_ntoa(addr.sin_addr);
            strIp = pTemp;

            LogDebug("accept socket ip[%s].",strIp.data());
            Address address(addr.sin_addr.s_addr, ntohs(addr.sin_port));
            TAcceptSocketMsg * pTAcceptSocketMsg = new TAcceptSocketMsg();
            pTAcceptSocketMsg->m_iMsgType = MSGTYPE_ACCEPTSOCKET_REQ;
            pTAcceptSocketMsg->m_iSocket = socket;
            pTAcceptSocketMsg->m_address = address;
            m_pThread->PostMsg(pTAcceptSocketMsg);
        }
    }
}

void InternalServerSocket::OnWrite()
{
    delete this;
}

