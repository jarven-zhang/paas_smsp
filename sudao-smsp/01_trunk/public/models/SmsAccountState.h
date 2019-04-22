#ifndef __SMSACCOUNT_STATE_H__
#define __SMSACCOUNT_STATE_H__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class SmsAccountState
    {
    public:
        SmsAccountState();
        virtual ~SmsAccountState();

        SmsAccountState(const SmsAccountState& other);

        SmsAccountState& operator =(const SmsAccountState& other);

    public:
		string m_strAccount;
		UInt32 m_uCode;
		string m_strRemarks;
		string m_strLockTime;
		UInt32 m_uStatus;
		string m_strUpdateTime;
    };

}

#endif  /*__SMSACCOUNT_STATE_H__*/

