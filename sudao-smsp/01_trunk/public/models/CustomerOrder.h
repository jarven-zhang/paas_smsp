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
	UInt32 	m_uProductType;	//0����ҵ��1��Ӫ����2�����ʣ�3����֤�룬4��֪ͨ��7��USSD��8�����ţ�9���һ����ţ�����0��1��3��4Ϊ��ͨ���ţ�2Ϊ���ʶ���
	UInt64 	m_uSubID;		//key
	UInt32	m_uOperator;
	double 	m_fUnitPrice;//��ͨ���ŵ���, �ͻ�����
	int		m_iRemainCount;//��ͨ����ʣ������
	double 	m_fRemainAmount;//���ʶ���ʣ����
	string 	m_strDeadTime;
	double  m_fSalePrice;
	double  m_fProductCost;
	double 	m_fQuantity;
};

#endif /* CUSTOMERORDERS_H */
