#include "SysNotice.h"

namespace models
{

    SysNotice::SysNotice()
    {
        m_iNoticeId = 0;
        m_iStatus   = 0;
        m_StartTime = "";
        m_EndTime   = "";
		m_ucNoticeType = 0;
    }

    SysNotice::~SysNotice()
    {

    }

    SysNotice::SysNotice(const SysNotice& other)
    {
        this->m_iNoticeId = other.m_iNoticeId;
        this->m_iStatus = other.m_iStatus;
        this->m_StartTime = other.m_StartTime;
        this->m_EndTime = other.m_EndTime;
		this->m_ucNoticeType = other.m_ucNoticeType;
    }

    SysNotice& SysNotice::operator =(const SysNotice& other)
    {
        this->m_iNoticeId = other.m_iNoticeId;
        this->m_iStatus = other.m_iStatus;
        this->m_StartTime = other.m_StartTime;
        this->m_EndTime = other.m_EndTime;
		this->m_ucNoticeType = other.m_ucNoticeType;

        return *this;
    }

} /* namespace models */

