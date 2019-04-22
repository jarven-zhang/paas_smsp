#include "Channellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "ssl/md5.h"
#include "json/json.h"

namespace smsp
{
    const char Base64Chars[] =
    {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '/'
    };

    const std::string Base64Characters(Base64Chars);

    inline bool IsBase64(unsigned char c)
    {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

    UInt32 Channellib::_sn =0;
    Channellib::Channellib()
    {
    }
    Channellib::~Channellib()
    {

    }

    /* BEGIN: Added by fuxunhao, 2015/11/18   PN:Batch Send Sms */
    void Channellib::split_Ex_Map(std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet)
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

	std::string Channellib::CalMd5(const std::string str)
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

	void Channellib::string_replace(string&s1, const string&s2, const string&s3)
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

	void Channellib::splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
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
	
    void Channellib::split_Ex_Vec(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
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
    /* END:   Added by fuxunhao, 2015/11/18   PN:Batch Send Sms */

    void Channellib::split(char* str, const char* delim, std::vector<char*>& list)
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

    long Channellib::strToTime(char *str)
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
        int t1 = mktime(&t);
        return t1;
    }

    std::string Channellib::getTmpSmsId()
    {
        UInt64 curTime=time(NULL);
        char smsId[128]= {0};
        _sn++;
        snprintf(smsId,128,"%ld%d", curTime, _sn);
        if(_sn>100000)
        {
            _sn=0;
        }
        return smsId;
    }

    void Channellib::getMD5ForZY(string& strAppKey, string& strPhone, string& strUid, string& strContent, Int64 iSendtime, string& strSign)
    {
        ///appKey + mobile + uid + content  + timestamp
        char target[2048] = {0};
        char* pos = target;
        unsigned char md5[16] = {0};

        memcpy(pos, strAppKey.data(), strAppKey.length());
        UInt32 targetLen = strAppKey.length();
        pos += strAppKey.length();

        memcpy(pos, strPhone.data(), strPhone.length());
        targetLen += strPhone.length();
        pos += strPhone.length();

        memcpy(pos, strUid.data(), strUid.length());
        targetLen += strUid.length();
        pos += strUid.length();


        std::string strContent_min = strContent;
        std::string strMin = "";
        strMin = urlDecode(strContent_min);


        memcpy(pos, strMin.data(), strMin.length());
        targetLen += strMin.length();
        pos += strMin.length();

        char tmp[80] = {0};
        snprintf(tmp, 80,"%ld", iSendtime);
        std::string strTime = "";
        strTime.assign(tmp);
        memcpy(pos, strTime.data(), strTime.length());
        targetLen += strTime.length();

        MD5((const unsigned char*) target, targetLen, md5);

        std::string HEX_CHARS = "0123456789abcdef";

        for (int i = 0; i < 16; i++)
        {
            strSign.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            strSign.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }
        return;
    }

    unsigned char Channellib::toHex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }

    unsigned char Channellib::fromHex(unsigned char x)
    {
        unsigned char y=0;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else
        {
            //LogCrit("FromHex Err");
        }
        return y;
    }

    std::string Channellib::urlEncode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
                strTemp += str[i];
            else if (str[i] == ' ')
                strTemp += "+";
            else
            {
                strTemp += '%';
                strTemp += toHex((unsigned char)str[i] >> 4);
                strTemp += toHex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }

    std::string Channellib::urlDecode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+') strTemp += ' ';
            else if (str[i] == '%')
            {
                unsigned char high = fromHex((unsigned char)str[++i]);
                unsigned char low = fromHex((unsigned char)str[++i]);
                strTemp += high*16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }

    std::string Channellib::encodeBase64(const char *data, UInt32 length)
    {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        while (length--)
        {
            char_array_3[i++] = *(data++);
            if (i == 3)
            {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for(i = 0; (i <4) ; i++)
                {
                    ret += Base64Characters[char_array_4[i]];
                }
                i = 0;
            }
        }
        if (i)
        {
            for(j = i; j < 3; j++)
            {
                char_array_3[j] = '\0';
            }
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (j = 0; (j < i + 1); j++)
            {
                ret += Base64Characters[char_array_4[j]];
            }
            while((i++ < 3))
            {
                ret += '=';
            }
        }
        return ret;
    }

    std::string Channellib::encodeBase64(const std::string &data)
    {
        return encodeBase64(data.data(), data.size());
    }

    std::string Channellib::decodeBase64(const char *data, UInt32 length)
    {
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;
        while (length-- && ( data[in_] != '=') && IsBase64(data[in_]))
        {
            char_array_4[i++] = data[in_];
            in_++;
            if (i ==4)
            {
                for (i = 0; i <4; i++)
                {
                    char_array_4[i] = Base64Characters.find(char_array_4[i]);
                }
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; (i < 3); i++)
                {
                    ret += char_array_3[i];
                }
                i = 0;
            }
        }
        if (i)
        {
            for (j = i; j <4; j++)
            {
                char_array_4[j] = 0;
            }
            for (j = 0; j <4; j++)
            {
                char_array_4[j] = Base64Characters.find(char_array_4[j]);
            }
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; (j < i - 1); j++)
            {
                ret += char_array_3[j];
            }
        }
        return ret;
    }

    std::string Channellib::decodeBase64(const std::string &data)
    {
        return decodeBase64(data.data(), data.size());
    }

	int Channellib::getChannelInterval(SmsChlInterval& smsChIntvl)
	{
		smsChIntvl._itlValLev1 = 30;	//val for lev2, num of msg
		//smsChIntvl->_itlValLev2 = 20;	//val for lev2

		smsChIntvl._itlTimeLev1 = 20;		//time for lev1, time of interval
		smsChIntvl._itlTimeLev2 = 1;		//time for lev 2
		//smsChIntvl->_itlTimeLev3 = 1;		//time for lev 3
		return 0;
	}	
} /* namespace smsp */
