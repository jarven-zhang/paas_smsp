#ifndef __SYS_NOTICE_USER_H__
#define __SYS_NOTICE_USER_H__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class SysNoticeUser
    {
    public:
        SysNoticeUser();
        virtual ~SysNoticeUser();

        SysNoticeUser(const SysNoticeUser& other);

        SysNoticeUser& operator =(const SysNoticeUser& other);

    public:
        UInt32 m_iNoticeId;
        UInt32 m_iUserId;
        UInt32 m_iAlarmType;
    };

}

#endif  /*__SYS_NOTICE_USER_H__*/

