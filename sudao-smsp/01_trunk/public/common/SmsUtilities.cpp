#include <stdio.h>
#include <stdlib.h>
#include <sstream> 
#include <algorithm>
#include <time.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "SmsUtilities.h"

namespace smsutil {

	/************************************************************************
	*  功能:验证账号是否合法
	*  @return true:合法;false:非法
	************************************************************************/
	bool Utilities::validateUserId(const string& userid)
	{
		// 1~16位
		return userid.length() > 0 && userid.length() <= 16;
	}

	/************************************************************************
	*  功能: 获取发送短信类型
	*  @return SEND_TYPE_HY:行业;SEND_TYPE_YX:营销
	************************************************************************/
	UInt32 Utilities::GetSendSmsType(const string& strSmsType)
	{
		int iSmsType = atoi(strSmsType.c_str());
		UInt32 sendtype = SEND_TYPE_YX;
		if (SMS_TYPE_NOTICE == iSmsType ||
			SMS_TYPE_VERIFICATION_CODE == iSmsType ||
			SMS_TYPE_ALARM == iSmsType)
		{
			sendtype = SEND_TYPE_HY;
		}
		else
		{
			sendtype = SEND_TYPE_YX;
		}
		return sendtype;
	}

	/************************************************************************
	*  功能: 获取短信类型标记
	*  @return SMS_TYPE_NOTICE_FLAG;SMS_TYPE_VERIFICATION_CODE_FLAG
	*          SMS_TYPE_MARKETING_FLAG;SMS_TYPE_ALARM_FLAG
	*          SMS_TYPE_ALARM_FLAG;SMS_TYPE_USSD_FLAG
	*          SMS_TYPE_FLUSH_SMS_FLAG
	************************************************************************/
	UInt32 Utilities::GetSmsTypeFlag(const string& strSmsType)
	{
		int iSmsType = atoi(strSmsType.c_str());
		switch (iSmsType)
		{
		case SMS_TYPE_NOTICE:
			return SMS_TYPE_NOTICE_FLAG;
		case SMS_TYPE_VERIFICATION_CODE:
			return SMS_TYPE_VERIFICATION_CODE_FLAG;
		case SMS_TYPE_MARKETING:
			return SMS_TYPE_MARKETING_FLAG;
		case SMS_TYPE_ALARM:
			return SMS_TYPE_ALARM_FLAG;
		case SMS_TYPE_USSD:
			return SMS_TYPE_USSD_FLAG;
		case SMS_TYPE_FLUSH_SMS:
			return SMS_TYPE_FLUSH_SMS_FLAG;
		default:
			break;
		}
		return 0;
	}
}