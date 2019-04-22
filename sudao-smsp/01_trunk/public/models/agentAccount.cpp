#include "agentAccount.h"

namespace models
{
    AgentAccount::AgentAccount()
    {
		m_fbalance = 0;      
		m_fcurrent_credit = 0;
		m_fcommission_income = 0;
	    m_frebate_income = 0;
		m_fdeposit = 0;
	}

    AgentAccount::~AgentAccount()
    {
    }

	AgentAccount::AgentAccount(const AgentAccount& other)
    {
    	this->m_fbalance =  other.m_fbalance;
		this->m_fcurrent_credit = other.m_fcurrent_credit;
		this->m_frebate_income = other.m_frebate_income;
		this->m_fdeposit   = other.m_fdeposit;
    }
	
    AgentAccount& AgentAccount::operator =(const AgentAccount& other)
    {
		this->m_fbalance =  other.m_fbalance;
		this->m_fcurrent_credit = other.m_fcurrent_credit;
		this->m_frebate_income = other.m_frebate_income;
		this->m_fdeposit   = other.m_fdeposit;
        return *this;
    }

} /* namespace models */
