#include "SmsRecord.h"
namespace models
{

    SmsRecord::SmsRecord()
    {
        channelid=0;
        operatorstype=0;
        smscnt=0;
        state=0;

        sendtime=0;
        reachtime=0;
    }

    SmsRecord::~SmsRecord()
    {

    }

    SmsRecord::SmsRecord(const SmsRecord& other)
    {
        this->channelid=other.channelid;
        this->operatorstype=other.operatorstype;
        this->content=other.content;
        this->smscnt=other.smscnt;
        this->state=other.state;
        this->phone=other.phone;
        this->sendtime=other.sendtime;
        this->reachtime=other.reachtime;
        this->remarks=other.remarks;

    }

    SmsRecord& SmsRecord::operator =(const SmsRecord& other)
    {
        this->channelid=other.channelid;
        this->operatorstype=other.operatorstype;
        this->content=other.content;
        this->smscnt=other.smscnt;
        this->state=other.state;
        this->phone=other.phone;
        this->sendtime=other.sendtime;
        this->reachtime=other.reachtime;
        this->remarks=other.remarks;

        return *this;
    }
} /* namespace models */

