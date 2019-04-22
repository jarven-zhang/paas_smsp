#ifndef CLOUDCUSTOMERPURCHASE_H
#define CLOUDCUSTOMERPURCHASE_H
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

class CloudCustomerPurchase
{
public:
	CloudCustomerPurchase()
	{
		m_strPurchaseId = 0;
		m_strCloudId = 0;
		//m_strCloudState = 0;
	}
	virtual ~CloudCustomerPurchase(){}
public:
	UInt64 m_strPurchaseId;
	UInt64 m_strCloudId;
	//UInt32 m_strCloudState;	
};

#endif /* CUSTOMERORDERS_H */
