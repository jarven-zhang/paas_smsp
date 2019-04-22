#include "SmsTemplate.h"

namespace models
{
    SmsTemplate::SmsTemplate()
    {
		m_strTemplateId = "";
		m_strType = "";
		m_strSmsType = "";
		m_strContent = "";
		m_strSign = "";
		m_strClientId = "";
		m_iAgentId = 0;

    }

	SmsTemplate::~SmsTemplate()
	{
	}

    SmsTemplate::SmsTemplate(const SmsTemplate& other)
    {
        m_strTemplateId = other.m_strTemplateId;
		m_strType = other.m_strType;
		m_strSmsType = other.m_strSmsType;
		m_strContent = other.m_strContent;
		m_strSign = other.m_strSign;
		m_strClientId = other.m_strClientId;
		m_iAgentId = other.m_iAgentId;
    }

    SmsTemplate& SmsTemplate::operator =(const SmsTemplate& other)
    {
        m_strTemplateId = other.m_strTemplateId;
		m_strType = other.m_strType;
		m_strSmsType = other.m_strSmsType;
		m_strContent = other.m_strContent;
		m_strSign = other.m_strSign;
		m_strClientId = other.m_strClientId;
		m_iAgentId = other.m_iAgentId;
        return *this;
    }

} /* namespace smsp */

