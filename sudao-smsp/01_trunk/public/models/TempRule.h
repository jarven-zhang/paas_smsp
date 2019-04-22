#ifndef TEMPLERULES_H_
#define TEMPLERULES_H_
#include <string>
#include <iostream>
#include "platform.h"
using namespace std;

namespace models
{

    class TempRule
    {
    public:
        TempRule();
        virtual ~TempRule();

        TempRule(const TempRule& other);

        TempRule& operator =(const TempRule& other);

    public:
        string m_strSmsType;
        string m_strUserID;
		string m_strSign;
        UInt32 m_overRate_num_s;
        UInt32 m_overRate_time_s;
        UInt32 m_overRate_num_h;
        UInt32 m_overRate_time_h;
		UInt32 m_overRate_time_m;
		UInt32 m_overRate_num_m;
        UInt32 m_enable;    //orverrate_swi,系统级超频开关
        UInt32 m_overrate_sign;
		UInt32 m_state; //状态，0：关闭，1：开启
		string m_strCid;
    };

}

#endif

