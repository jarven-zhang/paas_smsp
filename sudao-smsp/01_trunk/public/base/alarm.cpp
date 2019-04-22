#include "alarm.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "UTFString.h"
#include <math.h>
#include <cassert>
#include "UrlCode.h"
#include "ssl/md5.h"
#include <wctype.h>
#include "CChineseConvert.h"
#include "Channel.h"
#include "Comm.h"


string DAlarm::getAlarmStr(const char* str)
{
	char chtmp[128] = {0};
	string str1 = "smsp_access%e5%8f%91%e9%80%81%e5%88%b0smsp_send%e6%a8%a1%e5%9d%973%e6%ac%a1%e5%a4%b1%e8%b4%a5";
	
	str1 = http::UrlCode::UrlDecode(str1);
	
	snprintf(chtmp, 128, "%s_%s.", str, str1.c_str());
	return string(chtmp);
}

string DAlarm::getAlarmStr(const char* str, int iQueueSize, int iQueueThreshold)
{
    // 队列
    string str1 = http::UrlCode::UrlDecode("%e9%98%9f%e5%88%97");

    // >
    string str2 = http::UrlCode::UrlDecode("%3e");

    // 去掉QueueSize
    string strQueue(str);
    size_t len = strlen(str);

    if (9 < len)
    {
        strQueue.assign(str, len - 9);
    }

    char chtmp[512] = {0};

    // HostIpQueueSize队列5000>4000
    snprintf(chtmp, 512, "%s%s%d%s%d", strQueue.data(), str1.data(), iQueueSize, str2.data(), iQueueThreshold);

    return string(chtmp);
}

string DAlarm::getAlarmStr(int iChannelId, UInt32 industrytype, int iNum, int iSubmitThreshold)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 连续
    string str2 = ChannelType2String(industrytype) + http::UrlCode::UrlDecode("%E8%BF%9E%E7%BB%AD");

    // 次提交失败或无响应
    string str3 = http::UrlCode::UrlDecode("%e6%ac%a1%e6%8f%90%e4%ba%a4%e5%a4%b1%e8%b4%a5%e6%88%96%e6%97%a0%e5%93%8d%e5%ba%94");

    char chtmp[512] = {0};

    // 通道7625连续10次提交失败或无响应
    snprintf(chtmp, 512, "%s%d%s%d%s", str1.data(), iChannelId, str2.data(), iNum, str3.data());
	
    return string(chtmp);
}

string DAlarm::getAlarmStr1(int iChannelId, UInt32 industrytype, int iNum, int iSubmitThreshold)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 连续
    string str2 = ChannelType2String(industrytype) + http::UrlCode::UrlDecode("%E8%BF%9E%E7%BB%AD");

    // 次提交，返回失败响应
    string str3 = http::UrlCode::UrlDecode("%e6%ac%a1%e6%8f%90%e4%ba%a4%ef%bc%8c%e8%bf%94%e5%9b%9e%e5%a4%b1%e8%b4%a5%e5%93%8d%e5%ba%94");

    char chtmp[512] = {0};

    // 通道5700连续10次提交，返回失败响应
    snprintf(chtmp, 512, "%s%d%s%d%s", str1.data(), iChannelId, str2.data(), iNum, str3.data());

    return string(chtmp);
}

string DAlarm::getAlarmStr2(int iChannelId, UInt32 industrytype, int iNum, int iSubmitThreshold, int minute)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 在
    string str2 = ChannelType2String(industrytype) + http::UrlCode::UrlDecode("%e5%9c%a8");

    // 分钟内返回
    string str3 = http::UrlCode::UrlDecode("%e5%88%86%e9%92%9f%e5%86%85%e8%bf%94%e5%9b%9e");

    // 次失败状态报告
    string str4 = http::UrlCode::UrlDecode("%e6%ac%a1%e5%a4%b1%e8%b4%a5%e7%8a%b6%e6%80%81%e6%8a%a5%e5%91%8a");

    char chtmp[512] = {0};

    // 通道7625在2分钟内返回100次失败状态报告
    snprintf(chtmp, 512, "%s%d%s%d%s%d%s", str1.data(), iChannelId, str2.data(), minute, str3.data(), iNum, str4.data());

    return string(chtmp);
}

string DAlarm::getLoginFailedAlarmStr(int iChannelId, UInt32 industrytype, UInt32 uSum)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 连续
    string str2 = ChannelType2String(industrytype) + http::UrlCode::UrlDecode("%E8%BF%9E%E7%BB%AD");

    // 次登录失败
    string str3 = http::UrlCode::UrlDecode("%e6%ac%a1%e7%99%bb%e5%bd%95%e5%a4%b1%e8%b4%a5");

    char chtmp[512] = {0};

    // 通道7625连续3次登录失败
    snprintf(chtmp, 512, "%s%d%s%d%s", str1.data(), iChannelId, str2.data(), uSum, str3.data());

    return string(chtmp);
}
#if 0
string DAlarm::getNoSubmitRespAlarmStr(int iChannelId)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 在滑窗为0时,连续180秒无应答告警
    string str2 = http::UrlCode::UrlDecode("%e5%9c%a8%e6%bb%91%e7%aa%97%e4%b8%ba0%e6%97%b6%2c%e8%bf%9e%e7%bb%ad180%e7%a7%92%e6%97%a0%e5%ba%94%e7%ad%94%e5%91%8a%e8%ad%a6");

    char chtmp[512] = {0};

    // 通道7625在滑窗为0时,连续180秒无应答告警
    snprintf(chtmp, 512, "%s%d%s", str1.data(), iChannelId, str2.data());

    return string(chtmp);
}
#endif

string DAlarm::getNoSubmitRespAlarmStr( int iChannelId, UInt32 industrytype, int unixTimes )
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 连续3次自动重置滑窗
    string str2 = ChannelType2String(industrytype) + http::UrlCode::UrlDecode("%e8%bf%9e%e7%bb%ad3%e6%ac%a1%e8%87%aa%e5%8a%a8%e9%87%8d%e7%bd%ae%e6%bb%91%e7%aa%97");

    char chtmp[512] = {0};

    // 通道7625连续3次自动重置滑窗
    snprintf(chtmp, 512, "%s%d%s", str1.data(), iChannelId, str2.data());

    return string(chtmp);
}

string DAlarm::getDBContinueFailedAlarmStr(string& dbName, int iCount, UInt64 uComponentId, int type)
{
    // 连续
    string str1 = http::UrlCode::UrlDecode("%E8%BF%9E%E7%BB%AD");

    // 次连接 or 次操作
    string str2 = http::UrlCode::UrlDecode((0 == type) ? "%e6%ac%a1%e8%bf%9e%e6%8e%a5" : "%e6%ac%a1%e6%93%8d%e4%bd%9c");

    // 失败
    string str3 = http::UrlCode::UrlDecode("%e5%a4%b1%e8%b4%a5");

    char chtmp[512] = {0};

    // 连续3次连接smsp_message_5.5失败
    // 连续3次操作smsp_message_5.5失败
    snprintf(chtmp, 512, "%s%d%s%s%s", str1.data(), iCount, str2.data(), dbName.data(), str3.data());

    return string(chtmp);
}

string DAlarm::getRedisFailedAlarmStr(int iCount, UInt64 uComponentId, int type)
{
    // 连续
    string str1 = http::UrlCode::UrlDecode("%E8%BF%9E%E7%BB%AD");

    // 次连接redis失败 or 次操作redis失败
    string str2 = http::UrlCode::UrlDecode((0 == type) ? "%e6%ac%a1%e8%bf%9e%e6%8e%a5redis%e5%a4%b1%e8%b4%a5" : "%e6%ac%a1%e6%93%8d%e4%bd%9credis%e5%a4%b1%e8%b4%a5");

    char chtmp[512] = {0};

    // 连续3次连接redis失败
    // 连续3次操作redis失败
    snprintf(chtmp, 512, "%s%d%s", str1.data(), iCount, str2.data());

    return string(chtmp);
}

string DAlarm::getAlarmStrChannelMsgQueueTooLarge(int iChannelId, int curQueueSize, int alarmQueueSize)
{
    // 通道
    string str1 = http::UrlCode::UrlDecode("%E9%80%9A%E9%81%93");

    // 队列
    string str2 = http::UrlCode::UrlDecode("%e9%98%9f%e5%88%97");

    // >
    string str3 = http::UrlCode::UrlDecode("%3e");

    char chtmp[512] = {0};

    // 通道7625队列5000，超过4000
    snprintf(chtmp, 512, "%s%d%s%d%s%d", str1.data(), iChannelId, str2.data(), curQueueSize, str3.data(), alarmQueueSize);

    return string(chtmp);
}


string DAlarm::ChannelType2String(UInt32 industrytype)
{
	string ret;
	switch (industrytype)
	{
		case 0:
			ret = http::UrlCode::UrlDecode("%ef%bc%88%e8%a1%8c%e4%b8%9a%ef%bc%89");//（行业）
			break;
		case 1:
			ret = http::UrlCode::UrlDecode("%ef%bc%88%e8%90%a5%e9%94%80%ef%bc%89");//（营销）
			break;
		default:
			ret = http::UrlCode::UrlDecode("%ef%bc%88%e6%9c%aa%e7%9f%a5%e7%b1%bb%e5%9e%8b%ef%bc%89");//（未知类型）
	}
	return ret;
}
