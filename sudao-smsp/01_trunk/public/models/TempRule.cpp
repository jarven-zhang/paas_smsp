#include "TempRule.h"
namespace models
{

    TempRule::TempRule()
    {
        m_strSmsType = "";
		m_strSign = "";
        m_strUserID = "";
        m_overRate_num_s = 0;
        m_overRate_time_s = 0;
        m_overRate_num_h = 0;
        m_overRate_time_h = 0;
		m_overRate_num_m = 0;
		m_overRate_time_m = 0;
        m_enable= 0;
		m_overrate_sign = 0;
		m_state = 0;
		m_strCid = "";
    }

    TempRule::~TempRule()
    {

    }

    TempRule::TempRule(const TempRule& other)
    {

        this->m_strSmsType = other.m_strSmsType;
        this->m_strUserID = other.m_strUserID;
		this->m_strSign = other.m_strSign;
        this->m_overRate_num_s = other.m_overRate_num_s;
        this->m_overRate_time_s = other.m_overRate_time_s;
        this->m_overRate_num_h = other.m_overRate_num_h;
        this->m_overRate_time_h = other.m_overRate_time_h;
		this->m_overRate_num_m = other.m_overRate_num_m;
		this->m_overRate_time_m = other.m_overRate_time_m;
        this->m_enable = other.m_enable;
		this->m_overrate_sign = other.m_overrate_sign;
		this->m_state = other.m_state;
		this->m_strCid = other.m_strCid;
    }

    TempRule& TempRule::operator =(const TempRule& other)
    {
		if (this == &other)
			return *this;
		
		this->m_strSmsType = other.m_strSmsType;
        this->m_strUserID = other.m_strUserID;
		this->m_strSign = other.m_strSign;
        this->m_overRate_num_s = other.m_overRate_num_s;
        this->m_overRate_time_s = other.m_overRate_time_s;
        this->m_overRate_num_h = other.m_overRate_num_h;
        this->m_overRate_time_h = other.m_overRate_time_h;
		this->m_overRate_num_m = other.m_overRate_num_m;
		this->m_overRate_time_m = other.m_overRate_time_m;
        this->m_enable = other.m_enable;
		this->m_overrate_sign = other.m_overrate_sign;
		this->m_state = other.m_state;
		this->m_strCid = other.m_strCid;

        return *this;
    }
} /* namespace models */

