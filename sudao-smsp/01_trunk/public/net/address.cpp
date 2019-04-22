#include "address.h"

#ifdef SetPort
#undef SetPort
#endif



Address::Address()
{
    ip_ = 0;
    port_ = 0;
    isV6_ = false;
}

Address::Address(bool isV6)
{
    ip_ = 0;
    port_ = 0;
    isV6_ = isV6;
}

Address::Address(const std::string &ip, UInt16 port)
{
	//std::cout << "Address::Address[" << "ip=" << ip << " port=" << port << "]" << std::endl;
	
    isV6_ = false;
    hostent *host = GetHostByNameEx(ip);
    if (host == NULL)
    {
        ip_ = 0;
    }
    else
    {
        ip_ = *reinterpret_cast<UInt32 *>(host->h_addr);
    }
    port_ = port;
}

Address::Address(const std::string &ip, UInt16 port, bool isV6)
{
    isV6_ = isV6;
    if (isV6)
    {
        ipV6_ = ip;
    }
    else
    {
        hostent *host = GetHostByNameEx(ip);
        if (host == NULL)
        {
            ip_ = 0;
        }
        else
        {
            ip_ = *reinterpret_cast<UInt32 *>(host->h_addr);
        }
    }
    port_ = port;
}

Address::Address(UInt32 ip, UInt16 port)
{
    isV6_ = false;
    ip_ = ip;
    port_ = port;
}

Address::~Address()
{
}

Address::Address(const Address& other)
{
    ip_ = other.ip_;
    port_ = other.port_;
    isV6_ = other.isV6_;
}

Address& Address::operator= (const Address& other)
{
    ip_ = other.ip_;
    port_ = other.port_;
    isV6_ = other.isV6_;
    return *this;
}
bool Address::operator==(const Address &other) const
{
    if (!isV6_)
    {
        return ip_ == other.ip_ && port_ == other.port_;
    }
    return ipV6_ == other.ipV6_ && port_ == other.port_;
}

bool Address::operator!=(const Address &other) const
{
    if (!isV6_)
    {
        return ip_ != other.ip_ || port_ != other.port_;
    }
    return ipV6_ != other.ipV6_ || port_ != other.port_;
}

bool Address::operator>(const Address &other) const
{
    if (!isV6_)
    {
        if (ip_ != other.ip_)
        {
            return ip_ > other.ip_;
        }
        else
        {
            return port_ > other.port_;
        }
    }
    else
    {
        if (ipV6_ != other.ipV6_)
        {
            return ipV6_ > other.ipV6_;
        }
        else
        {
            return port_ > other.port_;
        }
    }
    return false;
}

bool Address::IsIpV6() const
{
    return isV6_;
}

bool Address::operator<(const Address &other) const
{
    if (ip_ != other.ip_)
    {
        return ip_ < other.ip_;
    }
    else
    {
        return port_ < other.port_;
    }
}

bool Address::operator>=(const Address &other) const
{
    if (ip_ != other.ip_)
    {
        return ip_ >= other.ip_;
    }
    else
    {
        return port_ >= other.port_;
    }
}

bool Address::operator<=(const Address &other) const
{
    if (ip_ != other.ip_)
    {
        return ip_ <= other.ip_;
    }
    else
    {
        return port_ <= other.port_;
    }
}

UInt32 Address::GetIp() const
{
    return ip_;
}

void Address::SetIp(UInt32 ip)
{
    ip_ = ip;
}

std::string Address::GetIpV6() const
{
    return ipV6_;
}

UInt16 Address::GetPort() const
{
    return port_;
}

void Address::SetPort(UInt16 port)
{
    port_ = port;
}

std::string Address::GetInetAddress() const
{
    in_addr addr;
    addr.s_addr = ip_;
    std::string strip = inet_ntoa(addr);
    return strip;
}

void Address::SetInetAddress(const std::string &ip)
{
    hostent *host = GetHostByNameEx(ip);
    if (host == NULL)
    {
        ip_ = 0;
    }
    else
    {
        ip_ = *reinterpret_cast<UInt32 *>(host->h_addr);
    }
}

std::string Address::ToInetAddress(UInt32 ip)
{
    in_addr addr;
    addr.s_addr = ip;
    return inet_ntoa(addr);
}

UInt32 Address::FromInetAddress(const std::string &ip)
{
    hostent *host = GetHostByNameEx(ip.data());
    if (host == NULL)
    {
        return 0;
    }
    else
    {
        return *reinterpret_cast<UInt32 *>(host->h_addr);
    }
}

hostent * Address::GetHostByNameEx(const std::string host)
{
	hostent hostbuf, *hp;
    size_t hstbuflen = 8192;
    char tmphstbuf[8192] = {0};
    int res;
    int herr;

    res = gethostbyname_r(host.data(), &hostbuf, tmphstbuf, hstbuflen, &hp, &herr);

    if (res || hp == NULL)
    {    
    	return NULL;
    }
	
    return hp;
}

