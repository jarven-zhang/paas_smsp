#ifndef NET_ADDRESS_H_
#define NET_ADDRESS_H_
#include "platform.h"

#include <arpa/inet.h>
#include <netdb.h>

#ifndef API
#include <string>
#endif /* API */



class Address 
{
public:
    Address();
    Address(bool isV6);
    Address(const std::string &ip, UInt16 port);
    Address(const std::string &ip, UInt16 port, bool isV6);
    Address(UInt32 ip, UInt16 port);
    Address(const Address& other);
    virtual ~Address();

public:
    virtual bool operator==(const Address &other) const;
    virtual bool operator!=(const Address &other) const;
    Address& operator= (const Address& other);
    virtual bool operator>(const Address &other) const;
    virtual bool operator<(const Address &other) const;

    virtual bool operator>=(const Address &other) const;
    virtual bool operator<=(const Address &other) const;

public:
    virtual bool IsIpV6() const;
    virtual UInt32 GetIp() const;
    virtual void SetIp(UInt32 ip);
    virtual std::string GetIpV6() const;

    virtual UInt16 GetPort() const;
    virtual void SetPort(UInt16 port);

    virtual std::string GetInetAddress() const;
    virtual void SetInetAddress(const std::string &ip);

    std::string ToInetAddress(UInt32 ip);
    UInt32 FromInetAddress(const std::string &ip);

	hostent * GetHostByNameEx(const std::string host);

private:
    UInt32 ip_;
    UInt16 port_;

    std::string ipV6_;
    bool isV6_;
};



#endif /* NET_ADDRESS_H_ */

