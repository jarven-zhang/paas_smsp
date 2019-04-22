#include "SignExtnoGw.h"
namespace models
{

    SignExtnoGw::SignExtnoGw()
    {
    }

    SignExtnoGw::~SignExtnoGw()
    {
    }

    SignExtnoGw::SignExtnoGw(const SignExtnoGw& other)
    {
        this->channelid=other.channelid;
        this->appendid=other.appendid;
        this->sign=other.sign;
        this->username=other.username;
        this->m_strClient = other.m_strClient;
		this->m_strFee_Terminal_Id = other.m_strFee_Terminal_Id;
    }

    SignExtnoGw& SignExtnoGw::operator =(const SignExtnoGw& other)
    {
        this->channelid=other.channelid;
        this->appendid=other.appendid;
        this->sign=other.sign;
        this->username=other.username;
        this->m_strClient = other.m_strClient;
		this->m_strFee_Terminal_Id = other.m_strFee_Terminal_Id;
        return *this;
    }


} /* namespace models */

ClientIdSignPort::ClientIdSignPort()
{
    m_strClientId = "";
    m_strSign = "";
    m_strSignPort = "";
}

ClientIdSignPort::~ClientIdSignPort()
{
}

ClientIdSignPort::ClientIdSignPort(const ClientIdSignPort& other)
{
    this->m_strClientId =other.m_strClientId;
    this->m_strSign =other.m_strSign;
    this->m_strSignPort =other.m_strSignPort;
}

ClientIdSignPort& ClientIdSignPort::operator =(const ClientIdSignPort& other)
{
    this->m_strClientId =other.m_strClientId;
    this->m_strSign =other.m_strSign;
    this->m_strSignPort =other.m_strSignPort;

    return *this;
}


