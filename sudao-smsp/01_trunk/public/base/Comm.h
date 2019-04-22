/******************************************************************************

  Copyright (C), 2001-2011, DCN Co., Ltd.

 ******************************************************************************
  File Name     : Comm.h
  Version       : Initial Draft
  Author        : gonghuojin
  Created       : 2015/5/22
  Last Modified :
  Description   : common file head
  Function List :
  History       :
  1.Date        : 2015/5/22
    Author      : gonghuojin
    Modification: Created file

******************************************************************************/
#ifndef SMS_COMM_H
#define SMS_COMM_H
#include <ctype.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>
#include <list>
#include "platform.h"
#include "UrlCode.h"
#include <sys/time.h>

using namespace std;

enum
{
	OPERATOR_ALL_NETCOM = 0,
	OPERATOR_CMCC = 1,
	OPERATOR_UNICOM = 2,
	OPERATOR_TELECOM = 3,
	OPERATOR_INTERNATIONAL = 4,
};

#define SIGN_BRACKET_LEFT(bIsChinese)  (bIsChinese==true ? http::UrlCode::UrlDecode("%e3%80%90") : "[" )
#define SIGN_BRACKET_RIGHT(bIsChinese) (bIsChinese==true ? http::UrlCode::UrlDecode("%e3%80%91") : "]" )

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p){delete(p);  (p)=NULL;} }
#endif

#define ONE_WAN 10000

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p){delete[] (p);  (p)=NULL;} }
#endif
class Comm
{
public:
    Comm();
    virtual ~Comm();

    int RandKey(const string& param, string& outKey);

    int RandKey(const int param, string& outKey);

    int RandKey(const char* param, string& outKey);
	static void splitExVector(const std::string& strData, std::string strDime, std::vector<std::string>& vecSet);
	static void splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet);
	static void splitExMap(const std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet);
	static void splitMap(const std::string& strData, std::string strDime, std::string strFirstDime, std::map<std::string,std::string>& mapSet);
	static long strToTime(char *str);
	static void split(std::string& strData, std::string strDime, set<string>& vecSet);
	static void split(std::string& strData, std::string strDime, set<UInt32>& vecSet);
	static void split(std::string& strData, std::string strDime, vector<string>& vecSet);
	static void split(std::string& strData, std::string strDime, vector<UInt32>& vecSet);
	static void split(char* str, const char* delim, std::vector<char*>& list);
	static bool includeSign(string content, bool includeChinese);
	static void StrHandle(std::string& s);
	static bool PatternMatch(std::string strContent,std::string strSign,std::string strTemplate);
	static bool IncludeChinese(const char *str);
	static int hex_char_value_Ex(char c);
	static int hex_to_decimal_Ex(const char* szHex, int len);
	static string int2str(UInt8 idata);
	static string int2str(int idata);
	static string int2str(long idata);
	static string int2str(UInt32 idata);
	static string int2str(UInt64 idata);
	static string float2str(float idata);
	static string double2str(double idata);
	static string getCurrentTime();		//eg:  2016-08-16 16:14:31
	static string getCurrentTime_z(Int64 t );
	static time_t getTimeStampFormDate(const string& strDate,const string& strFormat);
	static string getCurrentDay();		//eg: 20160919
	static string getCurrentDayMMDD();		//eg: 0919
	static string getCurrentTime_2();		//eg:  20160816161431
	static string getCurrHourMins();	//eg: 08:05
	static int64_t getCurrentTimeMill();	// 获取当前时间精确到毫秒	
	
	/**
		return:(today.date - inum)
	*/
	static string getDayOffset(int inum);
	static void Toupper(std::string & str);
	static void ToLower(std::string & str);
	static UInt32 getHashIndex(string str, UInt32 uSum);		//str hash in uSum ge Bucket
	static string& trim(std::string& s);
	static UInt64 strToUint64( std::string s );
	static UInt32 strToUint32( std::string s );
	static UInt32 getSubStr(const std::string& strData,UInt32 subsize);
    static void SignFormat(const string &strSign, bool bChina, string &strSignData);   // 格式化无括号签名
	static void delSignSign(std::string& StrSign);  // 删除签名括号
	static void string_replace(string&s1, const string&s2, const string&s3);//把s1里面包含的s2全部替换成s3, 结果保存在s1
	static void ConvertPunctuations(string & str);//把绝大部分常见的标点符号替换成英文符??
	static bool isNumber(const std::string& strData);
	static string protocolToString(UInt32 ptlType);
	static void Sbc2Dbc(std::string& strData);
	static UInt32 utf8_bytes_len(const char *src);
	static string GetStrMd5(const string& str);
	static bool isAscii(string str);
	static size_t escape_string_for_mysql(char *to, size_t to_length, const char *from, size_t length);
	static bool checkTimeFormat(const std::string& strTime);
	static time_t asctime2seconds(const std::string& strTime, const std::string& strFormat);
    static std::string getMondayDate();
    static std::string getLastMondayByDate(const std::string& strTime, const std::string& strFormat = "%Y-%m-%d %H:%M:%S");
    static string trimSign(string & s);//如果签名在前，去掉左中括号左边的空格；如果签名在后，去掉右中括号右边的空格
    static int getHash(string &strData);
	static void split_Ex_List(std::string& strData, std::string strDime, std::list<std::string>& listSet);
private:

    int RandTime();
};

#endif



