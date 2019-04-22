#ifndef AGENTACCOUNT_H_
#define AGENTACCOUNT_H_
#include <string>
#include <iostream>
#include "platform.h"

using namespace std;

namespace models
{
    class AgentAccount
    {
	    public:
	        AgentAccount();
	        virtual ~AgentAccount();
	        AgentAccount(const AgentAccount& other);
	        AgentAccount& operator =(const AgentAccount& other);

	    public:
			float   m_fbalance;       /*账号余额*/
			float   m_fcurrent_credit; /*授信余额*/
			float   m_fcommission_income;/*佣金收入*/
			float   m_frebate_income; /*返点收入*/
			float   m_fdeposit;       /*押金*/
    };

} /* namespace models */
#endif /* CHANNEL_H_ */
