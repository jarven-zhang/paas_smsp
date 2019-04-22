#ifndef __CHANNELERRORDESC__
#define __CHANNELERRORDESC__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

class channelErrorDesc
{
public:
	channelErrorDesc();
	~channelErrorDesc();
	channelErrorDesc(const channelErrorDesc& other);
	channelErrorDesc& operator=(const channelErrorDesc& other);

public:
	string m_strChannelId;
	string m_strType;////1 subret,2 report
	string m_strErrorCode;
	string m_strErrorDesc;
	string m_strSysCode;
	string m_strInnnerCode;
};



#endif ////__CHANNELERRORDESC__

