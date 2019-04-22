#include "MonitorDetailOption.h"

namespace models
{
    MonitorDetailOption::MonitorDetailOption()
    {
    	m_uState = 0;
		m_uFieldType = 0;
    }

    MonitorDetailOption::~MonitorDetailOption()
    {
    }

    MonitorDetailOption::MonitorDetailOption( const MonitorDetailOption& other )
    {
    	m_uState = other.m_uState;
		m_strFieldName = other.m_strFieldName;
		m_uFieldType   = other.m_uFieldType;
		m_uMeasurement  = other.m_uMeasurement;
    }
	
    MonitorDetailOption& MonitorDetailOption::operator =(const MonitorDetailOption& other)
    {
		this->m_uState = other.m_uState;
		this->m_strFieldName = other.m_strFieldName;
		this->m_uFieldType   = other.m_uFieldType;
		this->m_uMeasurement  = other.m_uMeasurement;
        return *this;
    }
	
} /* namespace models */

