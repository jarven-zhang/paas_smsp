#ifndef __SMSP_UTILS_H__
#define __SMSP_UTILS_H__

#include "platform.h"

class SmspUtils
{
public:
	SmspUtils();
	~SmspUtils();
	bool CheckIPIsValid(const std::string strIP);
	bool CheckIPFromUrl(const std::string strUrl, std::string& strIP);
	bool CheckIPFromIPPort(const std::string strIPPort, std::string& strIP);
};

#endif