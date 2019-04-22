#ifndef THREAD_MQTHREAD_MQTHREAD_MQTHREAD_H
#define THREAD_MQTHREAD_MQTHREAD_MQTHREAD_H

#include "platform.h"
#include "Thread.h"

#include <sys/time.h>
#include <time.h>
#include <unistd.h>

extern "C"
{
    #include "amqp_tcp_socket.h"
    #include "amqp.h"
    #include "amqp_framing.h"
}

#define CHK_AMQP_RPC_REPLY(func) \
    {\
        amqp_rpc_reply_t reply = func; \
        if (AMQP_RESPONSE_NORMAL != reply.reply_type) \
        { \
            LogErrorP("Call %s failed. reply_type:%d, library_error:%d, msg:%s.\n", \
                VNAME(func), \
                reply.reply_type, \
                reply.library_error, \
                amqp_error_string2(reply.library_error)); \
            return false; \
        } \
    }

const Int64 ONE_SECOND_FOR_MICROSECOND = 1000 * 1000;
const UInt32 SLEEP_TIME = 3 * 1000;
const UInt32 RECONNECT_SLEEP_TIME = 1000*10;


inline void select_sleep(UInt64 microsecond)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = microsecond;

    select(0, NULL, NULL, NULL, &timeout);
}

inline void microsleep(int usec)
{
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 1000 * usec;
    nanosleep(&req, NULL);
}

inline UInt64 getInterval(struct timeval& tv_end, struct timeval& tv_start)
{
    Int64 diff = ONE_SECOND_FOR_MICROSECOND * (tv_end.tv_sec - tv_start.tv_sec) + (tv_end.tv_usec - tv_start.tv_usec);

    if (diff < 0)
    {
        return 0;
    }

    return diff;
}

inline UInt64 now_microseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (UInt64)tv.tv_sec * 1000000 + (UInt64)tv.tv_usec;
}


class MqThreadPool;

class MqThread : public CThread
{
public:
    MqThread();
    virtual ~MqThread();

    virtual bool init();

    virtual bool getLinkStatus();

    bool PostMngMsg(TMsg* pMsg);

    TMsg* GetMngMsg();

public:
    MqThreadPool* m_pMqThreadPool;

    UInt32 m_uiMqThreadType;

    string m_strIp;

    UInt16 m_usPort;

    string m_strUserName;

    string m_strPassword;

    bool m_bStop;

    TMsgQueue m_msgMngQueue;

protected:

    boost::mutex m_mutexMq;

private:

};

#endif // THREAD_MQTHREAD_MQTHREAD_MQTHREAD_H
