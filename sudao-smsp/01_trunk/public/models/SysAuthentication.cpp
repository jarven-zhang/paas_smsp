#include "SysAuthentication.h"
#include "Comm.h"

using namespace std;
using namespace models;

SysAuthentication::SysAuthentication()
{
    m_bAllIpAllow = false;
}

SysAuthentication::~SysAuthentication()
{
}

void SysAuthentication::parseIp()
{
    string tmpIP;
    string::size_type pos;

    if (std::string::npos == m_bind_ip.find(","))
    {
        tmpIP = m_bind_ip;
        if (tmpIP == "*")
        {
            m_bAllIpAllow = true;
        }
        else
        {
            pos = tmpIP.find("*");
            if (std::string::npos != pos)
            {
                tmpIP = tmpIP.substr(0, pos);
            }

            m_vecIpWhite.push_back(tmpIP);
        }
    }
    else
    {
        std::vector<string> vecIp;
        Comm::splitExVector(m_bind_ip, ",", vecIp);

        for (vector<string>::iterator it = vecIp.begin(); it != vecIp.end(); it++)
        {
            if ((*it) == "*")
            {
                m_bAllIpAllow = true;
                break;
            }
            else
            {
                tmpIP = *it;
                pos = it->find("*");

                if (std::string::npos != pos)
                {
                    tmpIP.substr(0, pos);
                }

                m_vecIpWhite.push_back(tmpIP);
            }
        }
    }
}

bool SysAuthentication::checkIp(const string& strIp)
{
    if (false == m_bAllIpAllow)
    {
        bool bIsIpInWhiteList = false;
        for (vector<string>::iterator it = m_vecIpWhite.begin(); it != m_vecIpWhite.end(); it++)
        {
            const string& strIpTmp = *it;
            if (string::npos != strIp.find(strIpTmp, 0))
            {
                bIsIpInWhiteList = true;
                break;
            }
        }

        return bIsIpInWhiteList;
    }

    return true;
}



