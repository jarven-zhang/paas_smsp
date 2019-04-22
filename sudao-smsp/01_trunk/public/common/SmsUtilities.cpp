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
	*  ����:��֤�˺��Ƿ�Ϸ�
	*  @return true:�Ϸ�;false:�Ƿ�
	************************************************************************/
	bool Utilities::validateUserId(const string& userid)
	{
		// 1~16λ
		return userid.length() > 0 && userid.length() <= 16;
	}

	/************************************************************************
	*  ����: ��ȡ���Ͷ�������
	*  @return SEND_TYPE_HY:��ҵ;SEND_TYPE_YX:Ӫ��
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
	*  ����: ��ȡ�������ͱ��
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