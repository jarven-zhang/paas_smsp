#ifndef MONITORLOG_H_
#define MONITORLOG_H_
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

namespace models
{

    class MonitorLog
    {
    public:
        MonitorLog();
        virtual ~MonitorLog();

        MonitorLog(const MonitorLog& other);

        MonitorLog& operator =(const MonitorLog& other);

    public:
        UInt32 channelid;
        UInt32 operatorstype;
        UInt32 recvtotalcnt;
        UInt32 sendtotalcnt;
        UInt32 delaytotalcnt;
        UInt32 verifytotalcnt;
        UInt32 reachtotalcnt;
        UInt32 delayavg;
        double sendrate;
        double verifyrate;
        double reachrate;
        UInt32 createtime;

    };

}

#endif
