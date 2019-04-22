#ifndef __SYS_AUTHENTICATION_H__
#define __SYS_AUTHENTICATION_H__

#include "platform.h"

#include <string>
#include <vector>
#include <map>

namespace models
{

class SysAuthentication
{
public:
    SysAuthentication();
    ~SysAuthentication();

    bool checkIp(const std::string& strIp);

    void parseIp();

public:
    std::string m_id;
    std::string m_serial_num;
    std::string m_component_md5;
    std::string m_component_type;
    std::string m_bind_ip;
    std::string m_first_timestamp;
    std::string m_expire_timestamp;

    bool m_bAllIpAllow;
    std::vector<std::string> m_vecIpWhite;

};

}

typedef std::map<std::string, models::SysAuthentication> SysAuthenticationMap;

#endif

