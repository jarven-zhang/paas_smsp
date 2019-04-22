/******************************************************************************

  Copyright (C), 2018-2058, DCN Co., Ltd.

 ******************************************************************************
  File Name     : alarm.h
  Version       : Initial Draft
  Author        : sjh
  Created       : 2015/5/22
  Last Modified :
  Description   : alarm file head
  Function List :
  History       :
  1.Date        : 2018/6/5
    Author      : sjh
    Modification: Created file

******************************************************************************/
#ifndef __SMS_ALARM_H__
#define __SMS_ALARM_H__
#include <ctype.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>
#include "platform.h"
#include "UrlCode.h"

using namespace std;

class DAlarm
{

public:
	//SEND_TO_SMSPSEND_ALARM
    static string getAlarmStr(const char* str);   
	//QUENUE_QUANTITY_ALARM
	static string getAlarmStr(const char* str, int iQueueSize, int iQueueThreshold);	
	// CHANNEL_SUCCESS_ALARM
	static string getAlarmStr(int uChannelId, UInt32 industrytype, int uNum, int m_uSubmitFailedValue);	
	// CHANNEL_SUCCESS_ALARM 
	static string getAlarmStr1(int uChannelId, UInt32 industrytype, int uNum, int m_uSubmitFailedValue);
	// CHANNEL_REPORT_ALARM
	static string getAlarmStr2(int uChannelId, UInt32 industrytype, int uNum, int m_uSubmitFailedValue, int minute); 
    static string getDBContinueFailedAlarmStr(string& dbName, int iCount, UInt64 uComponentId, int type);
	static string getRedisFailedAlarmStr(int iCount, UInt64 uComponentId, int type);
	static string getLoginFailedAlarmStr(int iChannelId,UInt32 industrytype,UInt32 uSum);
	static string getNoSubmitRespAlarmStr(int iChannelId, UInt32 industrytype, int unixTimes);
	static string getAlarmStrChannelMsgQueueTooLarge(int iChannelId, int curQueueSize, int alarmQueueSize);

private:
	static string ChannelType2String(UInt32 industrytype);
	
};

#endif



