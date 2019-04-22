/*****************************************************************************************************
copyright (C),2018-2030, ucpaas .Co.,Ltd.

FileName     : SmsUtilities.h
Author       : Shijh      Version : 1.0    Date: 2018/06/20
Description  : 短信业务公共模块
Version      : 1.0
Function List:

History      :
<author>       <time>             <version>            <desc>
Shijh       2018/06/20          1.0          build this moudle
******************************************************************************************************/

#ifndef __SMSUTILITIES_H__
#define __SMSUTILITIES_H__

#include <string>
#include <list>
#include "platform.h"

using std::string;

namespace smsutil {

// 公共函数类
class Utilities
{
public:

	//////////////////////////////////////////////////////////////////////////
	//业务规则
	/************************************************************************
	*  功能:验证账号是否合法
	*  @return true:合法;false:非法
	************************************************************************/
	static bool validateUserId(const string& userid);

	/************************************************************************
	*  功能: 获取发送短信类型
	*  @return SEND_TYPE_HY:行业;SEND_TYPE_YX:营销
	************************************************************************/
	static UInt32 GetSendSmsType(const string& strSmsType);

	/************************************************************************
	*  功能: 获取短信类型标记
	*  @return SMS_TYPE_NOTICE_FLAG;SMS_TYPE_VERIFICATION_CODE_FLAG
	*          SMS_TYPE_MARKETING_FLAG;SMS_TYPE_ALARM_FLAG
	*          SMS_TYPE_ALARM_FLAG;SMS_TYPE_USSD_FLAG
	*          SMS_TYPE_FLUSH_SMS_FLAG
	************************************************************************/
	static UInt32 GetSmsTypeFlag(const string& strSmsType);


};

}

#endif // __SMSUTILITIES_H__