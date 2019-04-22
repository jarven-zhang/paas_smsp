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
    string 		m_strClientID;		//用户账号
	UInt32 		m_uProductType;		//0：行业，1：营销，2：国际，3：验证码，4：通知，7：USSD，8：闪信，9：挂机短信，其中0、1、3、4为普通短信，2为国际短信
	int 		m_uRemainCount;		//剩余条数，国内
	double		m_fRemainAmount;	//剩余金额，国际
	int 		m_iRemainTestNumber;//要扣一起扣	
	string 		m_strDeadTime;		//到期时间
	UInt64		m_uClientPoolId;	//Key
	UInt32 		m_uOperator;
	double 		m_fUnitPrice;		//单价
};
#endif /* OEMCLIENTPOOL_H */
