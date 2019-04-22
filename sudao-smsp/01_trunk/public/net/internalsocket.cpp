#include "internalsocket.h"

#include <string.h>
#include <stdlib.h>

#if defined(OS_LINUX) || defined(OS_MACX)
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#elif defined(OS_WIN32)
#include <winsock2.h>
#define socklen_t int
#endif

#include "address.h"
#include "eventtype.h"
#include "sockethandler.h"
#include "ibuffermanager.h"
#include "ibufferin.h"
#include "ibufferout.h"
#include "iblockin.h"
#include "iblockout.h"
#include "LogMacro.h"


	InternalSocket::InternalSocket()
	{
		socket_ = -1;
        input_ = NULL;
        output_ = NULL;
	}

    bool InternalSocket::Init(InternalService * service,LinkedBlockPool* pLinkedBlockPool)
    {

        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == socket_)
        {
            LogError("socket is failed,err[%s].",strerror(errno));
            return false;
        }
        
        handler_ = NULL;
        state_ = 0;
        flag_ = false;
        service_ = service;

        input_ = service_->GetBufferManager()->CreateBufferIn(pLinkedBlockPool);
        output_ = service_->GetBufferManager()->CreateBufferOut(pLinkedBlockPool);
        output_->SetFlushHandler(this);

        int flags = fcntl(socket_, F_GETFL);
        if (flags == -1)
        {
            LogError("fcntl is failed,err[%s].",strerror(errno));
            return false;
        }

        flags = fcntl(socket_, F_SETFL, flags |= O_NONBLOCK);
        if (flags == -1)
        {
            LogError("fcntl is failed,err[%s].",strerror(errno));
            return false;
        }

        int32_t one = 1;
        if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char *) &one, sizeof(one)) != 0)
        {
            LogError("setsockopt is failed,err[%s].",strerror(errno));
        	return false;
        }
		
        if (setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, (char *) &one, sizeof(one)) != 0)
        {
            LogError("setsockopt is failed,err[%s].",strerror(errno));
        	return false;
        }

		return service_->GetSelector()->Add(this, EPollSelector::SELECT_ALL);
    }

    bool InternalSocket::Init(InternalService *service, int socket, const Address& address,LinkedBlockPool* pLinkedBlockPool)
    {
        socket_ = socket;
        handler_ = NULL;
        state_ = 0;
        flag_ = false;
        service_ = service;

        input_ = service_->GetBufferManager()->CreateBufferIn(pLinkedBlockPool);
        output_ = service_->GetBufferManager()->CreateBufferOut(pLinkedBlockPool);

        output_->SetFlushHandler(this);
        SetState(STATE_CONNECTING);

        int flags = fcntl(socket_, F_GETFL);
        if (flags == -1)
        {
            LogError("fcntl is failed,err[%s].",strerror(errno));
            return false;
        }

        flags = fcntl(socket_, F_SETFL, flags |= O_NONBLOCK);
        if (flags == -1)
        {
            LogError("fcntl is failed,err[%s].",strerror(errno));
            return false;
        }

        int32_t one = 1;
        if (setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char *) &one, sizeof(one)) != 0)
        {
            LogError("setsockopt is failed,err[%s].",strerror(errno));
        	return false;
        }

		return service_->GetSelector()->Add(this, EPollSelector::SELECT_ALL);
    }

    InternalSocket::~InternalSocket()
    {
    	if(input_ != NULL)
    	{
        	input_->Close();
    	}

		if (output_!= NULL)
		{
        	output_->Close();
		}
        if (-1 != socket_)
        {
            close(socket_);
        }
    }

    void InternalSocket::Connect(const Address &address, int timeout)
    {
        if (HasState(STATE_CONNECTING))
        {
            return;
        }

        Connect(address);
    }

    void InternalSocket::Connect(const Address &address)
    {	
        if (HasState(STATE_CONNECTING))
        {
            return;
        }

        SetState(STATE_CONNECTING);

        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr));

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = address.GetIp();
        addr.sin_port = htons(address.GetPort());

        connect(socket_, (struct sockaddr*) &addr, sizeof(addr));
    }
    
    void InternalSocket::Close()
    {
		service_->GetSelector()->Remove(this);
    }
    
    const char *InternalSocket::GetLastError() const
    {
        switch (errno)
        {
            case EAGAIN:
                return "Try again";
            case EPIPE:
                return "Broken pipe";
            case ENOTCONN:
                return "Transport endpoint is not connected";
            case ECONNREFUSED:
                return "Connection refused";
            case ECONNRESET:
                return "Connection reset by peer";
            default:
                return "Unknown error";
        }
    }

    Address InternalSocket::GetLocalAddress()
    {

        sockaddr addr;
        socklen_t addr_len = sizeof(addr);
        if (getsockname(socket_,&addr, &addr_len) == 0)
        {
            if (addr.sa_family == AF_INET)
            {
                sockaddr_in *addrIn = reinterpret_cast<sockaddr_in *>(&addr);
                return Address(addrIn->sin_addr.s_addr, ntohs(addrIn->sin_port));
            }
        }
        return Address(0, 0);
    }

    Address InternalSocket::GetRemoteAddress()
    {
        sockaddr addr;
        socklen_t addr_len = sizeof(addr);
        if (getpeername(socket_, &addr, &addr_len) == 0)
        {
            if (addr.sa_family == AF_INET)
            {
                sockaddr_in *addrIn = reinterpret_cast<sockaddr_in *>(&addr);
                return Address(addrIn->sin_addr.s_addr, ntohs(addrIn->sin_port));
            }
        }
        return Address(0, 0);
    }

    InputBuffer *InternalSocket::In()
    {
        return input_;
    }

    OutputBuffer *InternalSocket::Out()
    {
        return output_;
    }

    void InternalSocket::SetHandler(SocketHandler *handler)
    {
        handler_ = handler;
    }

    int InternalSocket::GetSocketDescriptor() const
    {
        return socket_;
    }

    int InternalSocket::GetDescriptor()
    {
        return socket_;
    }

    void InternalSocket::OnRead()
    {
        bool readed = false;
        bool closed = false;
        while (true)
        {
            IBlockIn *block = input_->GetBlock();
            int flags = 0;

            if (flag_)
            {
                flags |= MSG_PEEK;
            }

            int bytes = recv(socket_, block->Data(), block->Size(), flags);
			if (bytes > 0)
            {
                readed = true;
                block->Fill(bytes);
            }
            block->Release();

            if (flag_ && bytes > 0)
            {
                break;
            }

            if (bytes <= 0)
            {
                if (bytes == 0)
                {
                    closed = true;
                    break;
                }
                else if (bytes == -1)
                {
                    if (errno == EAGAIN)
                    {
                        break;
                    }
                    else if (errno == ENOTCONN || errno == ECONNREFUSED)
                    {
                        LogError("***OnRead is failed,err[%s].",strerror(errno));
                        NotifyEvent(EventType::Error);
                        break;
                    }
                    else
                    {
                        LogError("***OnRead is failed,err[%s].",strerror(errno));
                        NotifyEvent(EventType::Error);
                        break;
                    }
                }
				else
				{
					//??????
					LogError("==================OnRead is failed,recv = [%d], err[%s].", bytes, strerror(errno));
				}
            }
        }
		
        if (readed)
        {
            NotifyEvent(EventType::Read);
        }
		
        if (closed)
        {
        	NotifyEvent(EventType::Closed);
        }
    }

    void InternalSocket::OnWrite()
    {
        if (HasState(STATE_CLOSING))
        {
            return;
        }

        if (!HasState(STATE_CONNECTED))
        {
            OnConnected();
        }

        if (HasState(STATE_CONNECTED))
        {
            SetState(STATE_WRITABLE);
            if (!HasState(STATE_DELAYED) && output_->Size() != 0)
            {
                SetState(STATE_DELAYED, true);
                Send();
            }
        }
    }

    void InternalSocket::OnFlush(IBufferOut *buffer)
    {
        if (HasState(STATE_WRITABLE) && !HasState(STATE_DELAYED) && output_->Size() > 0 && !HasState(STATE_CLOSING))
        {
            SetState(STATE_DELAYED, true);
            Send();
        }
    }


    void InternalSocket::Send()
    {	
        SetState(STATE_DELAYED, false);
        //bool wroten = false;
        while (HasState(STATE_WRITABLE) && output_->Size() != 0)
        {
            IBlockOut *block = output_->GetBlock();
            int bytes = send(socket_, block->Data(), block->Size(), MSG_NOSIGNAL);
            if (bytes > 0)
            {
                //wroten = true;
                block->Consume(bytes);
            }
            block->Release();
            if (bytes == -1)
            {
                if (errno == EAGAIN)
                {	
                    SetState(STATE_WRITABLE, false);
                }
                else if (errno == EPIPE || errno == ECONNRESET)
                {	
                    LogError("***Send is failed,err[%s].",strerror(errno));
                    NotifyEvent(EventType::Error);
                    SetState(STATE_WRITABLE, false);
                }
				else
				{
					//?????
					LogError("==================Send is failed,return -1,err[%s].",strerror(errno));
				}
                break;
            }
			else
			{
				//LogError("==================Send is failed,return[%d], err[%s].", bytes, strerror(errno));
			}
        }
 
        if (output_->Size() == 0)
        {	
            NotifyEvent(EventType::Writable);
        }
    }

    void InternalSocket::OnConnected()
    {	
        int optval = 0;
        socklen_t optlen = sizeof(optval);
        getsockopt(socket_, SOL_SOCKET, SO_ERROR, &optval, &optlen);

        if (optval == 0)
        {	
            SetState(STATE_CONNECTED);
        }
        else
        {
            LogError("***OnConnected is failed,err[%s].",strerror(errno));
            NotifyEvent(EventType::Error);
        }
    }

    void InternalSocket::NotifyEvent(int type)
    {
        if (handler_ != NULL)
        {	
            handler_->OnEvent(type, this);
        }
    }

    void InternalSocket::SetPeek(bool flag)
    {
        flag_ = flag;
    }

    bool InternalSocket::HasState(UInt8 state)
    {
        return (state_ & state) != 0;
    }

    bool InternalSocket::SetState(UInt8 state, bool flag)
    {
        if (flag)
        {
            state_ |= state;
        }
        else
        {
            state_ &= ~state;
        }
        return true;
    }



