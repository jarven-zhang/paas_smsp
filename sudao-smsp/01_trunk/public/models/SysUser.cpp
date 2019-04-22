#include "SysUser.h"

namespace models
{

    SysUser::SysUser()
    {
        m_iUserId  = 0;
        m_Mobile   = "";
        m_Email    = "";
    }

    SysUser::~SysUser()
    {

    }

    SysUser::SysUser(const SysUser& other)
    {
        this->m_iUserId = other.m_iUserId;
        this->m_Mobile  = other.m_Mobile;
        this->m_Email   = other.m_Email;
    }

    SysUser& SysUser::operator =(const SysUser& other)
    {
        this->m_iUserId = other.m_iUserId;
        this->m_Mobile = other.m_Mobile;
        this->m_Email = other.m_Email;

        return *this;
    }

} /* namespace models */

