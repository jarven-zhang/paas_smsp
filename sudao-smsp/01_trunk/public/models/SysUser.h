#ifndef __SYS_USER_H__
#define __SYS_USER_H__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class SysUser
    {
    public:
        SysUser();
        virtual ~SysUser();

        SysUser(const SysUser& other);

        SysUser& operator =(const SysUser& other);

    public:
        UInt32 m_iUserId;
        std::string m_Mobile;
        std::string m_Email;
    };

}

#endif  /*__SYS_USER_H__*/

