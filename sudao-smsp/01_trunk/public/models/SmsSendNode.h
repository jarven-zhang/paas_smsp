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
		UInt32 m_uPort;	//0£ºOK£¬1£ºLOCKED£¬2£ºForbidden

    };

}

#endif  /*__SMS_SEND_NODE_H__*/

