/*****************************************************************************************************
copyright (C),2018-2030, ucpaas .Co.,Ltd.

FileName     : SmsUtilities.h
Author       : Shijh      Version : 1.0    Date: 2018/06/20
Description  : ����ҵ�񹫹�ģ��
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

// ����������
class Utilities
{
public:

	//////////////////////////////////////////////////////////////////////////
	//ҵ�����
	/************************************************************************
	*  ����:��֤�˺��Ƿ�Ϸ�
	*  @return true:�Ϸ�;false:�Ƿ�
	************************************************************************/
	static bool validateUserId(const string& userid);

	/************************************************************************
	*  ����: ��ȡ���Ͷ�������
	*  @return SEND_TYPE_HY:��ҵ;SEND_TYPE_YX:Ӫ��
	************************************************************************/
	static UInt32 GetSendSmsType(const string& strSmsType);

	/************************************************************************
	*  ����: ��ȡ�������ͱ��
	*  @return SMS_TYPE_NOTICE_FLAG;SMS_TYPE_VERIFICATION_CODE_FLAG
	*          SMS_TYPE_MARKETING_FLAG;SMS_TYPE_ALARM_FLAG
	*          SMS_TYPE_ALARM_FLAG;SMS_TYPE_USSD_FLAG
	*          SMS_TYPE_FLUSH_SMS_FLAG
	************************************************************************/
	static UInt32 GetSmsTypeFlag(const string& strSmsType);


};

}

#endif // __SMSUTILITIES_H__