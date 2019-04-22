#include "Comm.h"
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

Comm::Comm()
{

}

Comm::~Comm()
{

}

int Comm::RandTime()
{
    int rand_num = 0;
    srand((unsigned)time(NULL));
    rand_num = rand();
    return rand_num;
}

int Comm::RandKey(const string& param, string& outKey)
{
    static unsigned nsceq = 0;
    char tmpBuf[1024] = {0};

    if (nsceq++ > 9999)
        nsceq = 0;

    snprintf(tmpBuf, 1024,"%s%d%d", param.c_str(), RandTime(), nsceq);

    outKey.append(tmpBuf);

    return 0;
}

int Comm::RandKey(const int param, string& outKey)
{
    static unsigned nsceq = 0;
    char tmpBuf[1024] = {0};

    if (nsceq++ > 9999)
        nsceq = 0;

    snprintf(tmpBuf, 1024,"%d%d%d", param, RandTime(), nsceq);

    outKey.append(tmpBuf);

    return 0;
}

int Comm::RandKey(const char* param, string& outKey)
{
    static unsigned nsceq = 0;
    char tmpBuf[1024] = {0};

    if (nsceq++ > 9999)
        nsceq = 0;

    snprintf(tmpBuf, 1024,"%s%d%d", param, RandTime(), nsceq);

    outKey.append(tmpBuf);

    return 0;
}

void Comm::splitExVector(const std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
{
    int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
        vecSet.push_back(strData.substr(lastPosition, index - lastPosition));
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        vecSet.push_back(lastString);
    }
}

void Comm::splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
{
	int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
    	if(index > lastPosition)
    	{
        	vecSet.push_back(strData.substr(lastPosition, index - lastPosition));
    	}
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        vecSet.push_back(lastString);
    }
}

void Comm::splitExMap(const std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet)
{
    std::vector<std::string> pair;
    int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
        pair.push_back(strData.substr(lastPosition, index - lastPosition));
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        pair.push_back(lastString);
    }


    std::string::size_type pos = 0;
    for (std::vector<std::string>::iterator ite = pair.begin(); ite != pair.end(); ite++)
    {

        pos = (*ite).find('=');
        if (pos != std::string::npos)
        {
            mapSet[(*ite).substr(0, pos)] = (*ite).substr(pos + 1);
        }
        else
        {
            //LogCrit("parse split_Ex param err");
        }
    }
    return;
}

void Comm::splitMap(const std::string& strData, std::string strDime, std::string strFirstDime, std::map<std::string,std::string>& mapSet)
{
    std::vector<std::string> pair;
    int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
        pair.push_back(strData.substr(lastPosition, index - lastPosition));
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        pair.push_back(lastString);
    }

    std::string::size_type pos = 0;
    for (std::vector<std::string>::iterator ite = pair.begin(); ite != pair.end(); ite++)
    {
        pos = (*ite).find(strFirstDime);
        if (pos != std::string::npos)
        {
            mapSet[(*ite).substr(0, pos)] = (*ite).substr(pos + 1);
        }
        else
        {

        }
    }
    return;
}

void Comm::split_Ex_List(std::string& strData, std::string strDime, std::list<std::string>& listSet)
{
	int strDimeLen = strDime.size();
	int lastPosition = 0, index = -1;
	while (-1 != (index = strData.find(strDime, lastPosition)))
	{
		listSet.push_back(strData.substr(lastPosition, index - lastPosition));
		lastPosition = index + strDimeLen;
	}
	string lastString = strData.substr(lastPosition);
	if (!lastString.empty())
	{
		listSet.push_back(lastString);
	}

	return;
}

long Comm::strToTime(char *str)
{
    if (str == NULL)
    {
        return 0;
    }
    char *p = str;
    char q[64] = {0};
    int i=0;
    int count = 0;
    while((*p))
    {
        if((*p)=='-' ||(*p)==' '||(*p)==':'||(*p)=='/'||(*p)=='.')
        {
            if(count<2&&i>1)
            {
                q[i]=q[i-1];
                q[i-1]='0';
                i++;
            }
            count=0;
            p++;
            continue;
        }
        if ((*p) < '0' || (*p) > '9') return 0;
        q[i]=*p;
        count++;
        i++;
        p++;
    }
    struct tm t;
    t.tm_year = (q[0] - '0') * 1000 + (q[1] - '0') * 100 + (q[2] - '0') * 10 + (q[3] - '0') - 1900;
    t.tm_mon = (q[4] - '0') * 10 + (q[5] - '0') - 1;
    t.tm_mday = (q[6] - '0') * 10 + (q[7] - '0');
    t.tm_hour = (q[8] - '0') * 10 + (q[9] - '0');
    t.tm_min = (q[10] - '0') * 10 + (q[11] - '0');
    t.tm_sec = (q[12] - '0') * 10 + (q[13] - '0');
    t.tm_isdst = 0;
    long t1 = mktime(&t);
    return t1;
}

/**************************************************************************************
* Function      : SignFormat
* Description   : 格式化无括号签名
* Input         : [IN] string &strSign          - 签名内容
*               : [IN] bool bChina              - 是否为中文
* Output        : string&
* Return        : strSignData 格式化后带括号的签名
* Author        : Shijh         Date: 2018年08月10日
* Modify        : 无
* Others        : 无
**************************************************************************************/
void Comm::SignFormat(const string &strSign, bool bChina, string &strSignData)
{
    if (!strSign.empty())
    {
        // 若为中文直接采用中文签名符【】
        if (bChina)
        {
            static const string strLeft = http::UrlCode::UrlDecode("%e3%80%90");
            static const string strRight = http::UrlCode::UrlDecode("%e3%80%91");
            strSignData = strLeft + strSign + strRight;
        }
        // 若为英文直接采用英文签名符[]
        else
        {
            strSignData = "[" + strSign + "]";
        }
    }
}

/*
	删除签名的括号
*/
void Comm::delSignSign(std::string& StrSign)
{
	string strLeft = "";
    string strRight = "";
	string StrContent = StrSign;
	if (true == Comm::IncludeChinese((char*)StrContent.data()))
	{
		strLeft = "%e3%80%90";
        strRight = "%e3%80%91";

		strLeft = http::UrlCode::UrlDecode(strLeft);

		strRight = http::UrlCode::UrlDecode(strRight);
	}
	else
	{
		strLeft = "[";
        strRight = "]";
	}

	std::string::size_type chsPosBegin = StrContent.find(strLeft);
	std::string::size_type chsPosEnd = StrContent.find(strRight);
	if(std::string::npos != chsPosBegin &&
		std::string::npos != chsPosEnd &&
		chsPosEnd > chsPosBegin)
	{
		StrSign = StrContent.substr(chsPosBegin + strLeft.length(),chsPosEnd - chsPosBegin - strLeft.length());
	}
	else
	{
		//StrSign = StrContent;
	}
	return;
}
bool Comm::includeSign(string content, bool includeChinese)
{
	utils::UTFString utfHelper;

	string strLeftSign;
	string strRightSign;

	strLeftSign  = SIGN_BRACKET_LEFT( includeChinese );
	strRightSign = SIGN_BRACKET_RIGHT(includeChinese );

	std::string::size_type chsPosBegin = content.find(strLeftSign);
	std::string::size_type chsPosEnd = content.find(strRightSign);
	if((chsPosBegin == 0 && chsPosEnd != std::string::npos)
		|| (chsPosEnd == (content.length()- strRightSign.length()) && chsPosBegin != std::string::npos)
		)
	{
		string strtmp = content.substr(chsPosBegin, chsPosEnd - chsPosBegin);
		int length = utfHelper.getUtf8Length(strtmp);

		if(length <= 9 && length > 1)	//8+length(1) = 9
		{
			return true;
		}
		else
		{
			return false;
		}

	}
	return false;
}
/*
	1.????????
	2.upper to lower
	3.???? -> ????
	4.????????? -> ????????

*/
void Comm::StrHandle(std::string& s)
{
	if(s.empty())
		return;
	trim(s);
	ToLower(s);
	//string strTemp;
	//CChineseConvert::ChineseTraditional2Simple(s,strTemp);
	//s = strTemp;
	ConvertPunctuations(s);
	return;
}
/*
	??????
	strContent?????????
	strSign ??????????:{}
	strTemplate ???
*/
bool Comm::PatternMatch(std::string strContent,std::string strSign,std::string strTemplate)
{
	if(strContent.empty() || strSign.empty() || strTemplate.empty())
	{
		return false;
	}
	std::size_t pos = 0;
	std::size_t lastpos = 0;
	std::vector<std::string> vecset;
	splitExVector(strTemplate,strSign,vecset);
	for(UInt32 i  = 0 ; i < vecset.size(); i++)
	{
		if(std::string::npos == (pos = strContent.find(vecset[i],lastpos)))
		{
			return  false;
		}
		if((0 == i) &&
			(pos > lastpos) &&
			(0 != strTemplate.find(strSign)))
		{
			return  false;
		}

		lastpos = pos + vecset[i].length();

	}
	std::string strLeft = strContent.substr(lastpos);
	if(!strLeft.empty() &&
		(strSign != strTemplate.substr(strTemplate.length() - 2,2)))
	{
		return false;
	}
	return true;
}
bool Comm::IncludeChinese(const char *str)//锟斤拷锟斤拷false锟斤拷锟斤拷锟斤拷锟侥ｏ拷锟斤拷锟斤拷true锟斤拷锟斤拷锟斤拷锟斤拷
{
   char c;
   while(1)
   {
       c=*str++;
       if (c==0)
       {
			break;  //閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷峰潃閿熸枻鎷烽敓杞胯鎷烽敓鍓跨鎷烽敓鏂ゆ嫹閿熸枻鎷峰潃閿熸枻鎷烽敓鐭紮鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹鍧€??
	   }

       if (c&0x80)        //閿熸枻鎷烽敓鏂ゆ嫹鍧€閿熸枻鎷烽敓杞挎檭?閿熸枻鎷烽敓鏂ゆ嫹涓€閿熻鍑ゆ嫹閿熸枻鎷蜂綅涔熼敓鏂ゆ??閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熻鍑ゆ嫹
       {
			if (*str & 0x80)
			{
				return true;
			}
	   }
   }

   return false;
}

void Comm::split(std::string& strData, std::string strDime, std::set<std::string>& strSet)
{
	int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
        strSet.insert(strData.substr(lastPosition, index - lastPosition));
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        strSet.insert(lastString);
    }

    return;
}

void Comm::split(std::string& strData, std::string strDime, set<UInt32>& vecSet)
{
	int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
    	std::string strTemp = strData.substr(lastPosition, index - lastPosition);
		if(!strTemp.empty())
		{
			vecSet.insert(atoi(strTemp.data()));
		}
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        vecSet.insert(atoi(lastString.data()));
    }

    return;
}
void Comm::split(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
{
	int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
        vecSet.push_back(strData.substr(lastPosition, index - lastPosition));
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        vecSet.push_back(lastString);
    }

    return;
}

void Comm::split(std::string& strData, std::string strDime, vector<UInt32>& vecSet)
{
	int strDimeLen = strDime.size();
    int lastPosition = 0, index = -1;
    while (-1 != (index = strData.find(strDime, lastPosition)))
    {
    	std::string strTemp = strData.substr(lastPosition, index - lastPosition);
		if(!strTemp.empty())
		{
			vecSet.push_back(atoi(strTemp.data()));
		}
        lastPosition = index + strDimeLen;
    }
    string lastString = strData.substr(lastPosition);
    if (!lastString.empty())
    {
        vecSet.push_back(atoi(lastString.data()));
    }

    return;
}

void Comm::split(char* str, const char* delim, std::vector<char*>& list)
{
    if (str == NULL)
    {
        return;
    }
    if (delim == NULL)
    {
        list.push_back(str);
        return;
    }
    char *s;
    const char *spanp;

    s = str;
    while (*s)
    {
        spanp = delim;
        while (*spanp)
        {
            if (*s == *spanp)
            {
                list.push_back(str);
                *s = '\0';
                str = s + 1;
                break;
            }
            spanp++;
        }
        s++;
    }
    if (*str)
    {
        list.push_back(str);
    }
}

int Comm::hex_char_value_Ex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    else if(c >= 'A' && c <= 'F')
        return (c - 'A' + 10);
    assert(0);
    return 0;
}

int Comm::getHash(string &strData)
{
	unsigned int seed = 31;
    UInt64 hash = 0;
	char *str = const_cast<char*>(strData.c_str());
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
	return (hash%5 + 1);
}


int Comm::hex_to_decimal_Ex(const char* szHex, int len)
{
    int result = 0;
    for(int i = 0; i < len; i++)
    {
        result += (int)pow((float)16, (int)len-i-1) * hex_char_value_Ex(szHex[i]);
    }
    return result;
}

string Comm::int2str(UInt8 idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%u", idata);
	return string(chtmp);
}

string Comm::int2str(int idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%d", idata);
	return string(chtmp);
}

string Comm::int2str(long idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%ld", idata);
	return string(chtmp);
}

string Comm::int2str(UInt32 idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%u", idata);
	return string(chtmp);
}

string Comm::int2str(UInt64 idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%lu", idata);
	return string(chtmp);
}

string Comm::float2str(float idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50,"%f", idata);
	return string(chtmp);
}

string Comm::double2str(double idata)
{
	char chtmp[50] = {0};
	snprintf(chtmp, 50, "%lf", idata);
	return string(chtmp);
}

string Comm::getCurrentTime()
{
	time_t iCurtime = time(NULL);
	char szreachtime[64] = { 0 };
	struct tm pTime = {0};
	localtime_r((time_t*) &iCurtime,&pTime);

	strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
	return szreachtime;
}

string Comm::getCurrentDay()
{
	time_t iCurtime = time(NULL);
	char szreachtime[64] = { 0 };
	struct tm pTime = {0};
	localtime_r((time_t*) &iCurtime,&pTime);

	strftime(szreachtime, sizeof(szreachtime), "%Y%m%d", &pTime);
	return szreachtime;
}

string Comm::getCurrentDayMMDD()
{
	time_t iCurtime = time(NULL);
	char szreachtime[64] = { 0 };
	struct tm pTime = {0};
	localtime_r((time_t*) &iCurtime,&pTime);

	strftime(szreachtime, sizeof(szreachtime), "%m%d", &pTime);
	return szreachtime;
}

std::string Comm::getCurrentTime_2()
{
    struct tm timenow = {0};
    time_t now = time(NULL);
    localtime_r(&now, &timenow);
    char submitTime[64] = { 0 };
    strftime(submitTime, sizeof(submitTime), "%Y%m%d%H%M%S", &timenow);

    return submitTime;
}

string Comm::getCurrentTime_z( Int64 t )
{
	time_t iCurtime = t;
	char szreachtime[64] = { 0 };
	struct tm pTime = {0};

	if( iCurtime == 0 ){
		iCurtime = time(NULL);
	}
	localtime_r((time_t*) &iCurtime,&pTime);
	strftime(szreachtime, sizeof(szreachtime), "%Y%m%d%H%M%S", &pTime);
	return szreachtime;
}

//date: eg:20180122114810  YYYYmmddHHMMSS
time_t Comm::getTimeStampFormDate(const string& strDate,const string& strFormat)
{
	struct tm tm_time = {0};
	if (NULL == strptime(strDate.data(), strFormat.data(), &tm_time))
	{
		return -1;
	}
	return mktime(&tm_time); //将tm时间转换为秒时间
}
string Comm::getCurrHourMins()
{
	time_t iCurtime = time(NULL);
	char szreachtime[64] = { 0 };
	struct tm pTime = {0};
	localtime_r((time_t*) &iCurtime,&pTime);

	strftime(szreachtime, sizeof(szreachtime), "%H:%M:%S", &pTime);
	return szreachtime;
}

int64_t Comm::getCurrentTimeMill()       
{      
   struct timeval tv;      
   gettimeofday(&tv,NULL);    //该函数在sys/time.h头文件中  
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;      
}

/*
	return:(today.date - inum)
*/
string Comm::getDayOffset(int inum)	//eg:today 20160917,int =2. func will return 20160915
{
	char destDt[9];
    time_t now = time(NULL);
    struct tm ts =  {0};
	localtime_r(&now,&ts);
    ts.tm_mday -= inum;
    mktime(&ts);
    strftime(destDt, sizeof(destDt), "%Y%m%d", &ts);
	string strDate = destDt;
	return strDate;
}

void Comm::string_replace(string&s1, const string&s2, const string&s3)
{
	string::size_type pos=0;
	string::size_type a=s2.size();
	string::size_type b=s3.size();
	while((pos=s1.find(s2,pos))!=string::npos)
	{
		s1.replace(pos,a,s3);
		pos+=b;
	}
}

void Comm::ConvertPunctuations(string & str)
{
	string_replace(str, "。", ".");
	string_replace(str, "？", "?");
	string_replace(str, "！", "!");
	string_replace(str, "，", ",");
	string_replace(str, "、", ",");
	string_replace(str, "；", ";");
	string_replace(str, "：", ":");
	string_replace(str, "“", "\"");
	string_replace(str, "”", "\"");
	string_replace(str, "‘", "'");
	string_replace(str, "’", "'");
	string_replace(str, "「", "'");
	string_replace(str, "」", "'");
	string_replace(str, "『", "'");
	string_replace(str, "』", "'");
	string_replace(str, "（", "(");
	string_replace(str, "）", ")");
	string_replace(str, "【", "[");
	string_replace(str, "】", "]");
	string_replace(str, "——", "-");
	string_replace(str, "．", ".");
	//string_replace(str, "~", "-");
	string_replace(str, "《", "<");
	string_replace(str, "》", ">");
	string_replace(str, "～", "~");
	string_replace(str, "＠", "@");
	string_replace(str, "＃", "#");
	string_replace(str, "＄", "$");
	string_replace(str, "％", "%");
	string_replace(str, "＾", "^");
	string_replace(str, "＆", "&");
	string_replace(str, "＊", "*");
	string_replace(str, "－", "-");
	string_replace(str, "＋", "+");
	string_replace(str, "｛", "{");
	string_replace(str, "｝", "}");
	string_replace(str, "＿", "_");
	string_replace(str, "＝", "=");
	string_replace(str, "／", "/");
	string_replace(str, "＼", "\\");
	string_replace(str, "｜", "|");
}

void Comm::Toupper(std::string & str)
{
	for(size_t i=0;i<str.size();i++)
		str.at(i)=toupper(str.at(i));
}

void Comm::ToLower(std::string & str)
{
	for(size_t i=0;i<str.size();i++)
		str.at(i)=tolower(str.at(i));
}


UInt32 Comm::getHashIndex(string str, UInt32 uSum)
{
	//get md5
	string strMd5;
	unsigned char md5[16] = { 0 };
    MD5((const unsigned char*) str.data(), str.length(), md5);
	std::string HEX_CHARS = "0123456789abcdef";
    for (int i = 0; i < 16; i++)
    {
        strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
        strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
    }

	//get md5 head 4
	strMd5 = strMd5.substr(0, 4);
	UInt64 uKey = strtol(strMd5.data(), NULL, 16);

	//get threadIndex
	UInt32 index=0;
	index = ((uKey-1)*uSum / 0xffff);  	//((ukey-1)+((0xffff/m_uThreadNum)-1)) /(0xffff/m_uThreadNum) = nodeNumber;
	if(index >= uSum)
	{
		index = uSum-1;
	}

	return index;
}

/*
	鐏忎粙鍣洪幋顏勫絿subsize婢堆冪毈鐎涙顑?
*/
UInt32  Comm::getSubStr(const std::string& strData,UInt32  subsize)
{
	UInt32 strlen = strData.length();
	UInt32 i = 0;
	if(strlen <= subsize)
		return strlen;

	const char *src = strData.c_str();
	while(i < subsize){
		short cur = ((short)src[i]) > 0 ? ((short)src[i]) : ((short)src[i]) + 256;
		int j = 0;
		if(((cur>>7)^0x00) == 0){
			j = 1;
		}else if(((cur>>5)^0x06) == 0){
			j = 2;
		}else if(((cur >> 4) ^ 0x0E) == 0){
			j = 3;
		}else if(((cur >> 3) ^ 0x1E) == 0){
			j = 4;
		}else if (((cur >> 2) ^ 0x3E) == 0){
			j = 5;
		}else if(((cur >> 1) ^ 0x7E) == 0){
			j = 6;
		}else{
			j = 1;
		}
		if(i + j > subsize)
			return i;
		else
			i += j;
	}
	return i;
}

string Comm::trimSign(string & s)
{
	if(s.empty())
	{
		return s;
	}

	const string strChineseLeftBracket = http::UrlCode::UrlDecode("%e3%80%90");
	const string strEnglishLeftBracket = "[";

	std::string::iterator c;
	size_t pos = 0;
	for (c = s.begin(); c != s.end() && iswspace(*c);)
	{
		pos++;
		c++;
	}

	if(   (s.substr(pos).find(strChineseLeftBracket) == string::npos && s.substr(pos).find(strEnglishLeftBracket) == 0)
	   || (s.substr(pos).find(strChineseLeftBracket) == 0))
	{
		s.erase(s.begin(), c);
	}
	else
	{
		for (c = s.end(); c != s.begin() && iswspace(*--c);)
			;
		s.erase(++c, s.end());
	}

	return s;
}

string& Comm::trim(std::string& s)
{
	if (s.empty())
	{
		return s;
	}

	std::string::iterator c;
	// Erase whitespace before the string
	for (c = s.begin(); c != s.end() && iswspace(*c++);)
		;
	s.erase(s.begin(), --c);

	// Erase whitespace after the string
	for (c = s.end(); c != s.begin() && iswspace(*--c);)
		;
	s.erase(++c, s.end());

	return s;
}

UInt64 Comm::strToUint64( std::string  s )
{
	return strtoull(s.data(), NULL, 0 );
}

UInt32 Comm::strToUint32( std::string s )
{
	return strtoul(s.data(), NULL, 0 );
}


bool Comm::isNumber(const std::string& strData)
{
    int len = strData.size();
    for (int i = 0; i < len; ++i) {
        if (strData[i] < '0' || strData[i] > '9') {
            return false;
        }
    }

    return true;
}

string Comm::protocolToString(UInt32 ptlType)
{
	string str;

	switch(ptlType)
	{
		case HTTP_GET:
			str = "HTTP_GET";
			break;
		case HTTP_POST:
			str = "HTTP_POST";
			break;
		case YD_CMPP:
			str = "YD_CMPP";
			break;
		case DX_SMGP:
			str = "DX_SMGP";
			break;
		case LT_SGIP:
			str = "LT_SGIP";
			break;
		case GJ_SMPP:
			str = "GJ_SMPP";
			break;
		case YD_CMPP3:
			str = "YD_CMPP3";
			break;
		default:
			str = "UNKNOW";
	}

	return str;
}
UInt32 Comm::utf8_bytes_len(const char *src)
{
	if(!src)
		return 0;
	unsigned char first = *src;
	if(first < 0xC0)
	{
		return 1;
	}
	else if (first < 0xE0)
	{
		return 2;
	}
	else if (first < 0xF0)
	{
		return 3;
	}
	else
	{
		return 4;
	}
}

void Comm::Sbc2Dbc(std::string& strData)
{
	//string strSbcNum = "%ef%bc%90";//SBC 0
	//string strSbca = "%ef%bd%81";//SBC a
	//strSbcNum = http::UrlCode::UrlDecode(strSbcNum);
	//strSbca = http::UrlCode::UrlDecode(strSbca);
	UInt32 strlenth = strData.size();
	UInt32 pos = 0;
	UInt32 i = 0;
	UInt32 byteslen = 0;

	if(strlenth <= 0)
		return;
	char *strOut = new char[strlenth+1];
	if(!strOut)
		return;
	memset(strOut,0,strlenth+1);
	while(pos < strlenth)
	{
		byteslen = utf8_bytes_len(&strData[pos]);
		if(pos + byteslen > strlenth)
		{
			strncpy(strOut + i,&strData[pos],strlenth-pos);
			break;
		}

		if(3 == byteslen)
		{
			//printf("****staData[pos]=0x%x  staData[pos+1]=0x%x *******\n",strData[pos],strData[pos+1]);
			if(0xE3 == (strData[pos]&0xFF) && 0x80 == (strData[pos+1]&0xFF) && 0x80 == (strData[pos+2]&0xFF))
			{//SBC space
				strOut[i++] = 0x20;
			}
			else if ((0xEF == (strData[pos]&0xFF)) && (0xBD == (strData[pos+1]&0xFF)))
			{//SBC : a-z
				strOut[i++] = strData[pos+2] - 0x20;
			}
			else if ((0xEF == (strData[pos]&0xFF)) && (0xBC == (strData[pos+1]&0xFF)))
			{//SBC: 0-9 A-Z   ?!...
				strOut[i++] =  strData[pos+2] - 0x60;
			}
			else
			{
				strncpy(strOut + i,&strData[pos],3);
				i += 3;
			}
		}
		else
		{
			strncpy(strOut + i,&strData[pos],byteslen);
			i += byteslen;
		}
		pos += byteslen;
	}
	strOut[i] = '\0';
	strData = strOut;
	SAFE_DELETE_ARRAY(strOut);
	return;
}

bool Comm::isAscii(string str)
{
	unsigned int i = 0;
	for (i = 0; i<str.length(); i++)
	{
		if(0 != (str[i] & ~0x7f))
			return false;

	}
	return true;
}

string Comm::GetStrMd5(const string& str)
{
	string strMd5 = "";
	unsigned char md5[16] = { 0 };
	MD5((const unsigned char*) str.c_str(), str.length(), md5);
	std::string HEX_CHARS = "0123456789abcdef";
	for (int i = 0; i < 16; i++)
	{
	    strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
	    strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
	}
	return strMd5;
}

size_t Comm::escape_string_for_mysql(char *to, size_t to_length, const char *from, size_t length)
{
	const char *to_start = to;
	const char *end, *to_end = to_start + (to_length ? to_length - 1 : 2 * length);
	int overflow = 0;

	for (end = from + length; from < end; from++)
	{
		char escape = 0;

		switch (*from) {
		case 0:				/* Must be escaped for 'mysql' */
			escape = '0';
			break;
		case '\n':				/* Must be escaped for logs */
			escape = 'n';
			break;
		case '\r':
			escape = 'r';
			break;
		case '\\':
			escape = '\\';
			break;
		case '\'':
			escape = '\'';
			break;
		case '"':				/* Better safe than sorry */
			escape = '"';
			break;
		case '\032':			/* This gives problems on Win32 */
			escape = 'Z';
			break;
		case '\%':
			escape = '%';
			break;

		}
		if (escape)
		{
			if (to + 2 > to_end)
			{
				overflow = 1;
				break;
			}
			*to++ = '\\';
			*to++ = escape;
		}
		else
		{
			if (to + 1 > to_end)
			{
				overflow = 1;
				break;
			}
			*to++ = *from;
		}
	}
	*to = 0;
	return overflow ? (size_t)-1 : (size_t)(to - to_start);
}

bool Comm::checkTimeFormat(const std::string& strTime)
{
    // valid format: 2016-11-11 09:00:00
    tm tm_time;
    return (strptime(strTime.c_str(), "%Y-%m-%d %H:%M:%S", &tm_time) != NULL);
}

time_t Comm::asctime2seconds(const std::string& strTime, const std::string& strFormat)
{
    tm tm_time;
    strptime(strTime.c_str(), strFormat.c_str(), &tm_time);

    return mktime(&tm_time);
}

std::string Comm::getMondayDate()
{
#define SEC_PER_DAY 24*60*60
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    int iDaysToMonday = 0;
    // tm_wday starts with sunday[0]
    if (tm_now.tm_wday == 0)
    {
        iDaysToMonday = 6;
    }
    else
    {
        iDaysToMonday = tm_now.tm_wday - 1;
    }
    time_t secToMonday = iDaysToMonday * SEC_PER_DAY;
    time_t monday = now - secToMonday;
    struct tm tmMonday;
    localtime_r(&monday, &tmMonday);
    char szDate[64] = {0};
    strftime(szDate, sizeof(szDate), "%Y%m%d", &tmMonday);

    return szDate;
}

std::string Comm::getLastMondayByDate(const std::string& strTime, const std::string& strFormat)
{
#define SEC_PER_DAY 24*60*60
    time_t the_time = asctime2seconds(strTime, strFormat);
    struct tm tm_time;
    localtime_r(&the_time, &tm_time);
    int iDaysToMonday = 0;
    // tm_wday starts with sunday[0]
    if (tm_time.tm_wday == 0)
    {
        iDaysToMonday = 6;
    }
    else
    {
        iDaysToMonday = tm_time.tm_wday - 1;
    }
    time_t secToMonday = iDaysToMonday * SEC_PER_DAY;
    time_t monday = the_time - secToMonday;
    struct tm tmMonday;
    localtime_r(&monday, &tmMonday);
    char szDate[64] = {0};
    strftime(szDate, sizeof(szDate), "%Y%m%d", &tmMonday);

    return szDate;
}

