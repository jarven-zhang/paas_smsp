#include "CustomerOrder.h"

CustomerOrder::CustomerOrder()
{
}

CustomerOrder::~CustomerOrder()
{
    // TODO Auto-generated destructor stub
}
CustomerOrder& CustomerOrder::operator =(const CustomerOrder& other)
{
    this->m_strClientID = other.m_strClientID;
	this->m_uProductType = other.m_uProductType;
	this->m_uSubID = other.m_uSubID;
	this->m_uOperator = other.m_uOperator;
	this->m_fUnitPrice = other.m_fUnitPrice;
	this->m_iRemainCount = other.m_iRemainCount;
	this->m_fRemainAmount = other.m_fRemainAmount;
	this->m_strDeadTime = other.m_strDeadTime;
	this->m_fSalePrice = other.m_fSalePrice;
	this->m_fProductCost = other.m_fProductCost;
	this->m_fQuantity = other.m_fQuantity;
    return *this;
}

