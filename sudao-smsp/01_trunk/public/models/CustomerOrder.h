#ifndef CUSTOMERORDERS_H
#define CUSTOMERORDERS_H
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

class CustomerOrder
{
public:
    CustomerOrder();
    virtual ~CustomerOrder();
    CustomerOrder(const CustomerOrder& other)
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
    }
    CustomerOrder& operator =(const CustomerOrder& other);
	
public:
    string 	m_strClientID;
	UInt32 	m_uProductType;	//0：行业，1：营销，2：国际，3：验证码，4：通知，7：USSD，8：闪信，9：挂机短信，其中0、1、3、4为普通短信，2为国际短信
	UInt64 	m_uSubID;		//key
	UInt32	m_uOperator;
	double 	m_fUnitPrice;//普通短信单价, 客户级别
	int		m_iRemainCount;//普通短信剩余条数
	double 	m_fRemainAmount;//国际短信剩余金额
	string 	m_strDeadTime;
	double  m_fSalePrice;
	double  m_fProductCost;
	double 	m_fQuantity;
};

#endif /* CUSTOMERORDERS_H */
