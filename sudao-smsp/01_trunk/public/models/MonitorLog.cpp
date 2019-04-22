#include "MonitorLog.h"
namespace models
{

    MonitorLog::MonitorLog()
    {

        channelid = 0;
        operatorstype = 0;
        recvtotalcnt = 0;
        sendtotalcnt = 0;
        delaytotalcnt = 0;
        verifytotalcnt = 0;
        reachtotalcnt = 0;
        delayavg = 0;
        sendrate = 0.0;
        verifyrate = 0.0;
        reachrate = 0.0;
        createtime = 0;

    }

    MonitorLog::~MonitorLog()
    {

    }

    MonitorLog::MonitorLog(const MonitorLog& other)
    {
        this->channelid = other.channelid;
        this->operatorstype = other.operatorstype;
        this->recvtotalcnt = other.recvtotalcnt;
        this->sendtotalcnt = other.sendtotalcnt;
        this->delaytotalcnt = other.delaytotalcnt;
        this->verifytotalcnt = other.verifytotalcnt;
        this->reachtotalcnt = other.reachtotalcnt;
        this->delayavg = other.delayavg;
        this->sendrate = other.sendrate;
        this->verifyrate = other.verifyrate;
        this->reachrate = other.reachrate;
        this->createtime = other.createtime;

    }

    MonitorLog& MonitorLog::operator =(const MonitorLog& other)
    {
        this->channelid = other.channelid;
        this->operatorstype = other.operatorstype;
        this->recvtotalcnt = other.recvtotalcnt;
        this->sendtotalcnt = other.sendtotalcnt;
        this->delaytotalcnt = other.delaytotalcnt;
        this->verifytotalcnt = other.verifytotalcnt;
        this->reachtotalcnt = other.reachtotalcnt;
        this->delayavg = other.delayavg;
        this->sendrate = other.sendrate;
        this->verifyrate = other.verifyrate;
        this->reachrate = other.reachrate;
        this->createtime = other.createtime;

        return *this;
    }
} /* namespace models */

