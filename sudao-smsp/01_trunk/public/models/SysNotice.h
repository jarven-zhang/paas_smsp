#ifndef __SYS_NOTICE_H__
#define __SYS_NOTICE_H__
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class SysNotice
    {
    public:
        SysNotice();
        virtual ~SysNotice();

        SysNotice(const SysNotice& other);

        SysNotice& operator =(const SysNotice& other);

    public:
        UInt32 		m_iNoticeId;
        UInt32 		m_iStatus;
        std::string m_StartTime;
        std::string m_EndTime;
		TINYINT 	m_ucNoticeType;	// 0:ÏµÍ³¸æ¾¯,1:ÔËÓª¸æ¾¯ by add sjh
    };

}

#endif  /*__SYS_NOTICE_H__*/

