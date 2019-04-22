#ifndef NET_SELECTOR_EPOLL_EPOLLSELECTOR_H_
#define NET_SELECTOR_EPOLL_EPOLLSELECTOR_H_
#include "platform.h"
#include <sys/epoll.h>

	class IHandler;
    class EPollSelector
    {
    public:
		static const int MAX_EVENTS = 3000;
		enum { Priority = 1 };
		enum SelectFlags
        {
            SELECT_READ = 0x01,
            SELECT_WRITE = 0x02,

            SELECT_ALL = 0x0f
        };

    public:
        EPollSelector();
        virtual ~EPollSelector();

        bool Init();
        bool Destroy();
        int Select();
        bool Add(IHandler *handler, int flag);
        bool Remove(IHandler *handler);
        bool Modify(IHandler *handler, int flag);
    private:
        int epollfd_;
        struct epoll_event events_[MAX_EVENTS];
    };


#endif /* NET_SELECTOR_EPOLL_EPOLLSELECTOR_H_ */

