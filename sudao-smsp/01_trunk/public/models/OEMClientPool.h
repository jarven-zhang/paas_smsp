#ifndef OEMCLIENTPOOL_H
#define OEMCLIENTPOOL_H
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

class OEMClientPool
{
public:
    OEMClientPool();
    virtual ~OEMClientPool();
    OEMClientPool(const OEMClientPool& other)
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
    }
    OEMClientPool& operator =(const OEMClientPool& other);
	
public:
    string 		m_strClientID;		//�û��˺�
	UInt32 		m_uProductType;		//0����ҵ��1��Ӫ����2�����ʣ�3����֤�룬4��֪ͨ��7��USSD��8�����ţ�9���һ����ţ�����0��1��3��4Ϊ��ͨ���ţ�2Ϊ���ʶ���
	int 		m_uRemainCount;		//ʣ������������
	double		m_fRemainAmount;	//ʣ�������
	int 		m_iRemainTestNumber;//Ҫ��һ���	
	string 		m_strDeadTime;		//����ʱ��
	UInt64		m_uClientPoolId;	//Key
	UInt32 		m_uOperator;
	double 		m_fUnitPrice;		//����
};
#endif /* OEMCLIENTPOOL_H */
