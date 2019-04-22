#include "PhoneDao.h"
#include <stdio.h>
#include <algorithm>
#include "main.h"


PhoneDao::PhoneDao()
{
    // TODO Auto-generated constructor stub
}

PhoneDao::~PhoneDao()
{
    // TODO Auto-generated destructor stub
}

void PhoneDao::Init()
{
    m_pVerify = new VerifySMS();

    #if defined(smsp_c2s) || defined(smsp_http) || defined(smsp_access) || defined(smsp_reback)
    g_pRuleLoadThread->getOperaterSegmentMap(m_OperaterSegmentMap);
    #endif
}

UInt32 PhoneDao::getPhoneType(cs_t phone)
{
    ///VerifySMS verify;

    if (m_pVerify->isForeign(phone))
    {
        return BusType::FOREIGN;
    }
    std::string strPhoneHead3 = phone.substr(0, 3);
    std::string strPhoneHead4 = phone.substr(0, 4);

    map<string, UInt32>::iterator it;
    do{
        it = m_OperaterSegmentMap.find(strPhoneHead3);
        if(it != m_OperaterSegmentMap.end())
        {
            break;
        }

        it = m_OperaterSegmentMap.find(strPhoneHead4);
        if(it != m_OperaterSegmentMap.end())
        {
            break;
        }

        //can not find record, then return OTHER
        LogNotice("phone belong to no one. phone[%s]", phone.data());
        return BusType::OTHER;

    }while(0);

    if(it->second >= 0 && it->second <=  7)
    {
        return it->second;
    }
    else
    {
        LogWarn("operator failled. operator[%d]", it->second);
        return 1;   //返回一个移动
    }

}

