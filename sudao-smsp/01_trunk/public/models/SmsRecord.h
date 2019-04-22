#ifndef SMSRECORD_H_
#define SMSRECORD_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class SmsRecord
    {
    public:
        SmsRecord();
        virtual ~SmsRecord();

        SmsRecord(const SmsRecord& other);

        SmsRecord& operator=(const SmsRecord& other);

    public:
        UInt32 channelid;
        UInt32 operatorstype;
        string content;
        UInt32 smscnt;
        UInt32 state;
        string phone;
        UInt32 sendtime;
        UInt32 reachtime;
        string remarks;


    };

}

#endif

