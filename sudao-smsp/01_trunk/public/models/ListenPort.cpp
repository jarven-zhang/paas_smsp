#include "ListenPort.h"


ListenPort::ListenPort()
{
	m_uPortValue = 0;
}

ListenPort::~ListenPort()
{

}

ListenPort::ListenPort(const ListenPort& other)
{
	this->m_strPortKey = other.m_strPortKey;
	this->m_uPortValue = other.m_uPortValue;
	this->m_strComponentType = other.m_strComponentType;
}

ListenPort& ListenPort::operator=(const ListenPort& other)
{
	if (this == &other)
	{
		return *this;
	}

	this->m_strPortKey = other.m_strPortKey;
	this->m_uPortValue = other.m_uPortValue;
	this->m_strComponentType = other.m_strComponentType;

	return *this;
}

