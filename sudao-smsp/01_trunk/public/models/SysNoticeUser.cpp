#include "SysNoticeUser.h"

namespace models
{

    SysNoticeUser::SysNoticeUser()
    {
        m_iNoticeId = 0;
        m_iUserId   = 0;
        m_iAlarmType= 0;
    }

    SysNoticeUser::~SysNoticeUser()
    {

    }

    SysNoticeUser::SysNoticeUser(const SysNoticeUser& other)
    {
        this->m_iNoticeId = other.m_iNoticeId;
        this->m_iUserId   = other.m_iUserId;
        this->m_iAlarmType = other.m_iAlarmType;
    }

    SysNoticeUser& SysNoticeUser::operator =(const SysNoticeUser& other)
    {
        this->m_iNoticeId = other.m_iNoticeId;
        this->m_iUserId   = other.m_iUserId;
        this->m_iAlarmType = other.m_iAlarmType;

        return *this;
    }

} /* namespace models */

