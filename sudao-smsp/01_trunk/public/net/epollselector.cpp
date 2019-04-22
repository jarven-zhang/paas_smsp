#ifdef OS_LINUX
#include "epollselector.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "ihandler.h"
#include "LogMacro.h"
	
    EPollSelector::EPollSelector()
    {
        epollfd_ = -1;

    }

    EPollSelector::~EPollSelector()
    {
    }

    bool EPollSelector::Init()
    {
        if (epollfd_ == -1)
        {
            signal(SIGPIPE, SIG_IGN);
            epollfd_ = epoll_create(65535);
        }
        return epollfd_ != -1;
    }

    bool EPollSelector::Destroy()
    {
        if (epollfd_ != -1)
        {
            close(epollfd_);
        }
        return true;
    }

    int EPollSelector::Select()
    {
    	memset(events_, 0, sizeof(events_));
    	
        int count = epoll_wait(epollfd_, events_, MAX_EVENTS, 0);
        for (int i = 0; i < count; ++i)
        {
            IHandler *handler = reinterpret_cast<IHandler *>(events_[i].data.ptr);
            UInt32 event = events_[i].events;
            if (event & EPOLLIN || event & EPOLLERR || event & EPOLLHUP)
            {
                handler->OnRead();
            }
            if (event &EPOLLOUT)
            {
                handler->OnWrite();
            }
        }
        return count;
    }

    bool EPollSelector::Add(IHandler *handler, int flag)
    {
        struct epoll_event event;
        event.data.ptr = handler;
        event.events = EPOLLET;
        if (flag & SELECT_READ)
        {
            event.events = event.events | EPOLLIN;
        }
        if (flag & SELECT_WRITE)
        {
            event.events = event.events | EPOLLOUT;
        }

		int iRet = epoll_ctl(epollfd_, EPOLL_CTL_ADD, handler->GetDescriptor(), &event);
		if (iRet != 0)
		{
            LogError("******Add epoll_ctl is failed.");
			return false;
		}
			
        return true;
    }

    bool EPollSelector::Remove(IHandler *handler)
    {
        struct epoll_event event;
        event.data.ptr = handler;
        event.events = EPOLLET | EPOLLIN;
        int iRet = epoll_ctl(epollfd_, EPOLL_CTL_DEL, handler->GetDescriptor(), &event);
        if (0 != iRet)
        {
            LogError("******Remove epoll_ctl is failed.");
            return false;
        }
        
        return true;
    }

    bool EPollSelector::Modify(IHandler *handler, int flag)
    {
        return true;
    }




#endif /* OS_LINUX */

