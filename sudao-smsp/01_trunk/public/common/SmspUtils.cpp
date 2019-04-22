#include "SmspUtils.h"
#include <arpa/inet.h>

SmspUtils::SmspUtils()
{

}

SmspUtils::~SmspUtils()
{

}

bool SmspUtils::CheckIPIsValid(const std::string strIP)
{
    if(strIP.empty())
    {
        return false;
    }

    UInt32 iIP= inet_addr(strIP.data());
    if((iIP == 0)||(iIP == 0xffffffff))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool SmspUtils::CheckIPFromUrl(const std::string strUrl, std::string& strIP)
{
	if(strUrl.empty())
    {
        return false;
    }
	//"http://221.179.180.158:9007/QxtSms/QxtFirewall";
	std::string::size_type pos;
    pos = strUrl.find_first_of("//");
    if(pos == std::string::npos)
    {
		return false;
	}
	std::string urlIP = strUrl.substr(pos+2);
	pos = urlIP.find_first_of(":");
	if(pos == std::string::npos)
	{
		return false;
	}
	std::string sIP = urlIP.substr(0, pos);
	if(true == CheckIPIsValid(sIP))
	{
		strIP = sIP;
		return true;
	}
	else
	{
		return false;
	}
}

bool SmspUtils::CheckIPFromIPPort(const std::string strIPPort, std::string& strIP)
{
	if(strIPPort.empty())
    {
        return false;
    }
	//221.179.180.158:9007;
	std::string::size_type pos;
	pos = strIPPort.find(":");
	if(pos == std::string::npos)
	{
		return false;
	}
	std::string sIP = strIPPort.substr(0, pos);
	if(true == CheckIPIsValid(sIP))
	{
		strIP = sIP;
		return true;
	}
	else
	{
		return false;
	}
}