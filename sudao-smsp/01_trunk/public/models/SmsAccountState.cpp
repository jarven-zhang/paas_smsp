#include "SmsAccountState.h"

namespace models
{

    SmsAccountState::SmsAccountState()
    {
		m_strAccount = "";
		m_uCode = 0;
		m_strRemarks = "";
		m_strLockTime = "";
		m_uStatus = 0;
		m_strUpdateTime = "";
    }

    SmsAccountState::~SmsAccountState()
    {
		
    }

    SmsAccountState::SmsAccountState(const SmsAccountState& other)
    {
		this->m_strAccount = other.m_strAccount;
		this->m_uCode = other.m_uCode;
		this->m_strRemarks = other.m_strRemarks;
		this->m_strLockTime = other.m_strLockTime;
		this->m_uStatus = other.m_uStatus;
		this->m_strUpdateTime = other.m_strUpdateTime;
    }

    SmsAccountState& SmsAccountState::operator =(const SmsAccountState& other)
    {
		this->m_strAccount = other.m_strAccount;
		this->m_uCode = other.m_uCode;
		this->m_strRemarks = other.m_strRemarks;
		this->m_strLockTime = other.m_strLockTime;
		this->m_uStatus = other.m_uStatus;
		this->m_strUpdateTime = other.m_strUpdateTime;

        return *this;
    }

} /* namespace models */

