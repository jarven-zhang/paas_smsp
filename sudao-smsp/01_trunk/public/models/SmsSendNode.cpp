#include "SmsSendNode.h"

namespace models
{

    SmsSendNode::SmsSendNode()
    {
        m_uPort = 0;
        m_strIp = "";
    }

    SmsSendNode::~SmsSendNode()
    {

    }

    SmsSendNode::SmsSendNode(const SmsSendNode& other)
    {	
    	this->m_uPort = other.m_uPort;
        this->m_strIp = other.m_strIp;
    }

    SmsSendNode& SmsSendNode::operator =(const SmsSendNode& other)
    {
       	this->m_uPort = other.m_uPort;
        this->m_strIp = other.m_strIp;
        return *this;
    }

} /* namespace models */

