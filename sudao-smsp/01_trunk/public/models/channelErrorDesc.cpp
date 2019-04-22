#include "channelErrorDesc.h"

channelErrorDesc::channelErrorDesc()
{
}

channelErrorDesc::~channelErrorDesc()
{

}

channelErrorDesc::channelErrorDesc(const channelErrorDesc& other)
{
	this->m_strChannelId = other.m_strChannelId;
	this->m_strType = other.m_strType;
	this->m_strErrorCode = other.m_strErrorCode;
	this->m_strErrorDesc = other.m_strErrorDesc;
	this->m_strSysCode = other.m_strSysCode;
	this->m_strInnnerCode = other.m_strInnnerCode;
	
}

channelErrorDesc& channelErrorDesc::operator=(const channelErrorDesc& other)
{
	if (this == &other)
	{
		return *this;
	}

	this->m_strChannelId = other.m_strChannelId;
	this->m_strType = other.m_strType;
	this->m_strErrorCode = other.m_strErrorCode;
	this->m_strErrorDesc = other.m_strErrorDesc;
	this->m_strSysCode = other.m_strSysCode;
	this->m_strInnnerCode = other.m_strInnnerCode;

	return *this;
}

