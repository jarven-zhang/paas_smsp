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
			float   m_fbalance;       /*�˺����*/
			float   m_fcurrent_credit; /*�������*/
			float   m_fcommission_income;/*Ӷ������*/
			float   m_frebate_income; /*��������*/
			float   m_fdeposit;       /*Ѻ��*/
    };

} /* namespace models */
#endif /* CHANNEL_H_ */
