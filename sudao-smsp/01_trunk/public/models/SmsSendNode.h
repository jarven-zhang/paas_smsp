#ifndef __SMS_SEND_NODE_H__
#define __SMS_SEND_NODE_H__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class SmsSendNode
    {
    public:
        SmsSendNode();
        virtual ~SmsSendNode();

        SmsSendNode(const SmsSendNode& other);

        SmsSendNode& operator =(const SmsSendNode& other);

    public:
		string m_strIp;
		UInt32 m_uPort;	//0��OK��1��LOCKED��2��Forbidden

    };

}

#endif  /*__SMS_SEND_NODE_H__*/

