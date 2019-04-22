#ifndef SIGNEXTNO_GW_H_
#define SIGNEXTNO_GW_H_
#include "platform.h"

#include <string>
#include <iostream>
using namespace std;

namespace models
{

    class SignExtnoGw
    {
    public:
        SignExtnoGw();
        virtual ~SignExtnoGw();

        SignExtnoGw(const SignExtnoGw& other);

        SignExtnoGw& operator =(const SignExtnoGw& other);

    public:
        string channelid;
        string sign;
        string appendid;
        string username;
        string m_strClient;          // ccess is client,rest is sid
        string m_strFee_Terminal_Id; // 计费号码，适用cmpp省网通道
    };
}

class ClientIdSignPort
    {
    public:
        ClientIdSignPort();
        virtual ~ClientIdSignPort();

        ClientIdSignPort(const ClientIdSignPort& other);

        ClientIdSignPort& operator =(const ClientIdSignPort& other);

    public:
        string m_strClientId;  ///// access client, rest  sid
        string m_strSign;
        string m_strSignPort;
    };


#endif

