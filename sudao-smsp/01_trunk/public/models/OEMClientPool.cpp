#include "OEMClientPool.h"

OEMClientPool::OEMClientPool()
{
}

OEMClientPool::~OEMClientPool()
{
}
OEMClientPool& OEMClientPool::operator =(const OEMClientPool& other)
{
    this->m_strClientID = other.m_strClientID;
	this->m_uProductType = other.m_uProductType;
	this->m_uRemainCount = other.m_uRemainCount;
	this->m_fRemainAmount = other.m_fRemainAmount;
	this->m_iRemainTestNumber = other.m_iRemainTestNumber;
	this->m_strDeadTime = other.m_strDeadTime;
	this->m_uOperator = other.m_uOperator;
	this->m_uClientPoolId = other.m_uClientPoolId;
	this->m_fUnitPrice = other.m_fUnitPrice;
	
    return *this;
}

