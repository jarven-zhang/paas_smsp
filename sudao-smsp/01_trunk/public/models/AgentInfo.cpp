#include "AgentInfo.h"

namespace models
{

    AgentInfo::AgentInfo()
    {
        // TODO Auto-generated constructor stub
        m_iAgent_type = 0;
        m_iAgentPaytype = 0;
    }

    AgentInfo::~AgentInfo()
    {
        // TODO Auto-generated destructor stub
    }

	AgentInfo::AgentInfo(const AgentInfo& other)
    {
        this->m_iAgent_type=other.m_iAgent_type;
        this->m_iAgentPaytype=other.m_iAgentPaytype;

    }

    AgentInfo& AgentInfo::operator =(const AgentInfo& other)
    {
        this->m_iAgent_type=other.m_iAgent_type;
        this->m_iAgentPaytype=other.m_iAgentPaytype;
        return *this;
    }
} /* namespace models */
