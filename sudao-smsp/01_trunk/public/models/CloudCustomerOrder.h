#ifndef CLOUDCUSTOMERORDERS_H
#define CLOUDCUSTOMERORDERS_H
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

class CloudCustomerOrder
{
public:
	CloudCustomerOrder()
	{
		//m_uOrderId = 0;
		//m_uPurchaseId = 0;
		m_fUnitPrice = 0;
		m_uOrderNumber = 0;
		m_fOrderAmount = 0;
		m_uOrderState = 0;
		m_uUseState = 0;
		m_fOffSet = 0;

	}
	
	CloudCustomerOrder(const CloudCustomerOrder& other)
	{
		this->m_uOrderId = other.m_uOrderId;
		this->m_uPurchaseId = other.m_uPurchaseId;
		this->m_uOrderNumber = other.m_uOrderNumber;
		this->m_fOrderAmount = other.m_fOrderAmount;
		this->m_fUnitPrice = other.m_fUnitPrice;
		this->m_uOrderState = other.m_uOrderState;
		this->m_uUseState = other.m_uUseState;
		this->m_fOffSet = other.m_fOffSet;
		this->m_strCreateTime = other.m_strCreateTime;		
	}

	virtual ~CloudCustomerOrder(){}
	
	CloudCustomerOrder& operator = (const CloudCustomerOrder& other)
	{
		this->m_uOrderId = other.m_uOrderId;
		this->m_uPurchaseId = other.m_uPurchaseId;
		this->m_uOrderNumber = other.m_uOrderNumber;
		this->m_fOrderAmount = other.m_fOrderAmount;
		this->m_fUnitPrice = other.m_fUnitPrice;
		this->m_uOrderState = other.m_uOrderState;
		this->m_uUseState = other.m_uUseState;
		this->m_fOffSet = other.m_fOffSet;
		this->m_strCreateTime = other.m_strCreateTime;
		return *this;
	}

	bool operator <(const CloudCustomerOrder& other)
	{
      	if(m_strCreateTime < other.m_strCreateTime)
      	{
			return true;
		}
		else if(m_strCreateTime == other.m_strCreateTime)
		{
			return m_uOrderId < other.m_uOrderId;
		}
		else
		{
			return false;
		}
	}
public:
	string m_uOrderId;			////	ding dan hao
	string m_uPurchaseId;		////	cai gou dan hao
	float m_fUnitPrice; 		////	duan xin dan jia
	UInt32 m_uOrderNumber;		////	ding dan daun xin tiao shu
	float m_fOrderAmount;		////	ding dan jin e
	UInt32 m_uOrderState;		////	ding dan zhuang tai 
	UInt32 m_uUseState;			/////	shi yong zhuang tai ,1 = ok
	float m_fOffSet;			////	tong shang
	string m_strCreateTime;		////	chuang jian shi jian	
};


#endif /* CUSTOMERORDERS_H */
