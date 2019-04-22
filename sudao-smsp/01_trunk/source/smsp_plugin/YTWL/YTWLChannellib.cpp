#include "YTWLChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include <cstring> 
#include <math.h>

namespace smsp
{

	typedef unsigned long   UTF32;  /* at least 32 bits */
	typedef unsigned short  UTF16;  /* at least 16 bits */
	typedef unsigned char   UTF8;   /* typically 8 bits */
	typedef unsigned int    INT;

	#define UTF8_ONE_START      (0xOOO1)
	#define UTF8_ONE_END        (0x007F)
	#define UTF8_TWO_START      (0x0080)
	#define UTF8_TWO_END        (0x07FF)
	#define UTF8_THREE_START    (0x0800)
	#define UTF8_THREE_END      (0xFFFF)

    YTWLChannellib::YTWLChannellib()
    {
        m_mapErrorCode["ET:0118"] = "MB:1056";
        m_mapErrorCode["ET:0119"] = "MB:1056";
        m_mapErrorCode["ET:0120"] = "MB:1056";
        m_mapErrorCode["ET:0127"] = "UNDELIV";
        m_mapErrorCode["ET:0128"] = "UNDELIV";
        m_mapErrorCode["ET:0201"] = "ID:0013";
        m_mapErrorCode["ET:0202"] = "ID:0013";
        m_mapErrorCode["ET:0210"] = "ID:0141";
        m_mapErrorCode["ET:0220"] = "ID:0318";
        m_mapErrorCode["ET:0221"] = "ID:0318";
        m_mapErrorCode["ET:0226"] = "ID:0318";
    }
    
    YTWLChannellib::~YTWLChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new YTWLChannellib;
        }
    }

    string g_strSpid = "";

    bool YTWLChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        ///single -> command=MT_RESPONSE&spid=****&mtmsgid=118464012710100208&mtstat=ACCEPTD&mterrcode=000
        try
        {
            std::string::size_type pos = respone.find("MT_RESPONSE");
            if (pos == std::string::npos)
            {
                //LogError("[%s] this is error format.", respone.data());
                return false;
            }

            std::map<std::string,std::string> mapSet;
            split_Ex_Map(respone, "&", mapSet);
            std::map<std::string,std::string>::iterator itr = mapSet.find("mtstat");
            if (itr == mapSet.end())
            {
                //LogError("[%s] this is error format.", respone.data());
                return false;
            }

            if (0 == itr->second.compare("ACCEPTD"))
            {
                status = "0";
            }
            else
            {
                status = "4";
            }

            strReason = mapSet["mtstat"];
            smsid = mapSet["mtmsgid"];
            g_strSpid = mapSet["spid"];

            return true;
        }
        catch(...)
        {
            //LogError("[YTWLChannellib]:parseSend is failed.");
            return false;
        }
    }


    unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z') 
        {
        	y = x - 'A' + 10;
        }
        else if (x >= 'a' && x <= 'z') 
        {
        	y = x - 'a' + 10;
        }
        else if (x >= '0' && x <= '9') 
        {
        	y = x - '0';
        }
        else 
        {
        	//LogCrit("FromHex Err");
        }
        return y;
    }

    string Decode(const string& str)
    {
        string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        return strTemp;
    }

    string AppendPer(const string& str)
    {
        string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            strTemp += '%';
            strTemp += str[i++];
            strTemp += str[i];

        }
        return strTemp;
    }

	void UTF16ToUTF8(UTF16* pUTF16Start, UTF16* pUTF16End, UTF8* pUTF8Start, UTF8* pUTF8End)
	{
		UTF16* pTempUTF16 = pUTF16Start;
		UTF8* pTempUTF8 = pUTF8Start;

		while (pTempUTF16 < pUTF16End)
		{
			if (*pTempUTF16 <= UTF8_ONE_END
				&& pTempUTF8 + 1 < pUTF8End)
			{
				//0000 - 007F  0xxxxxxx
				*pTempUTF8++ = (UTF8)*pTempUTF16;
			}
			else if (*pTempUTF16 >= UTF8_TWO_START && *pTempUTF16 <= UTF8_TWO_END
				&& pTempUTF8 + 2 < pUTF8End)
			{
				//0080 - 07FF 110xxxxx 10xxxxxx
				*pTempUTF8++ = (*pTempUTF16 >> 6) | 0xC0;
				*pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
			}
			else if (*pTempUTF16 >= UTF8_THREE_START && *pTempUTF16 <= UTF8_THREE_END
				&& pTempUTF8 + 3 < pUTF8End)
			{
				//0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
				*pTempUTF8++ = (*pTempUTF16 >> 12) | 0xE0;
				*pTempUTF8++ = ((*pTempUTF16 >> 6) & 0x3F) | 0x80;
				*pTempUTF8++ = (*pTempUTF16 & 0x3F) | 0x80;
			}
			else
			{
				break;
			}
			pTempUTF16++;
		}
		*pTempUTF8 = 0;
	}

	int hex_char_value_Ex(char c)
	{
		if (c >= '0' && c <= '9')
			return c - '0';
		else if (c >= 'a' && c <= 'f')
			return (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			return (c - 'A' + 10);
		return 0;
	}

	int hex_to_decimal_Ex(const char* szHex, int len)
	{
		int result = 0;
		for (int i = 0; i < len; i++)
		{
			result += (int)pow((float)16, (int)len - i - 1) * hex_char_value_Ex(szHex[i]);
		}
		return result;
	}

    bool YTWLChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /*
        single
            rt?command=RT_REQUEST&spid=****&sppassword=****&mtmsgid=118464091310100268&mtstat=DELIVRD&mterrcode=000

        multi
            http://sphost/sms/rt?command=MULTI_RT_REQUEST&spid=1234
            rts参数值在HTTP请求的content里面提交，如下
            rts=118464091310100268/DELIVRD/000,118464091310100269/DELIVRD/000
        */
        try
        {
            ///single report
            std::string::size_type pos = respone.find("RT_REQUEST");
            if (pos != std::string::npos)
            {
                std::map<std::string,std::string> reportSet;
                split_Ex_Map(respone, "&", reportSet);

                std::string strSmsid = reportSet["mtmsgid"];
                std::string strStat = reportSet["mtstat"];
                std::string strErr = reportSet["mterrcode"];
                std::string strSpid = reportSet["spid"];
                int iStat = -1;
                if (0 == strStat.compare("DELIVRD"))
                {
                    ////iStat = 0;
                    iStat = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else
                {
                    ///iStat = 4;
                    iStat = SMS_STATUS_CONFIRM_FAIL;
                }

                string strDesc = "";
                std::map<string,string>::iterator itr = m_mapErrorCode.find(strStat);
                if (itr == m_mapErrorCode.end())
                {
                    strDesc.assign(strStat);
                }
                else
                {
                    strDesc.assign(itr->second);
                }

                smsp::StateReport report;
                report._desc = strDesc;
                report._smsId = strSmsid;
                report._status = iStat;
                report._reportTime = time(NULL);

                reportRes.push_back(report);

                /// rt response: command=RT_RESPONSE&spid=****&mtmsgid=mtmsgid&rtstat=ACCEPTD&rterrcode=000
                strResponse.assign("command=RT_RESPONSE&spid=");
                strResponse.append(strSpid);
                strResponse.append("&mtmsgid=");
                strResponse.append(strSmsid);
                strResponse.append("&rtstat=ACCEPTD&rterrcode=000");

                return true;
            }


            /// multi report
            if (respone.find("rts=") != std::string::npos)
            {
                /// multi response: command=MULTI_RT_RESPONSE&spid=1234&rtstat=ACCEPTD&rterrcode=000
                strResponse.assign("command=MULTI_RT_RESPONSE&spid=");
                strResponse.append(g_strSpid);
                strResponse.append("&rtstat=ACCEPTD&rterrcode=000");

                std::string strData = respone.substr(respone.find("=")+1);
                std::vector<std::string> reportList;
                std::vector<std::string> reportValue;
                split_Ex_Vec(strData, ",", reportList);

                if (0 == reportList.size())
                {
                    //LogError("[YTWLChannellib] [%s] is not reports.", respone.data());
                    return false;
                }

                for (int i = 0; i < reportList.size(); i++)
                {
                    std::string strRt = reportList.at(i);
                    reportValue.clear();
                    split_Ex_Vec(strRt, "/", reportValue);
                    if (4 != reportValue.size())
                    {
                        //LogError("[YTWLChannellib] last param[%s].", strRt.data());
                        return false;
                    }

                    std::string strSmsid = reportValue.at(0);
                    std::string strStat = reportValue.at(1);
                    std::string strErr = reportValue.at(2);
                    int iStat = -1;

                    if (0 == strStat.compare("DELIVRD"))
                    {
                        iStat = SMS_STATUS_CONFIRM_SUCCESS;
                    }
                    else
                    {
                        iStat = SMS_STATUS_CONFIRM_FAIL;
                    }

                    string strDesc = "";
                    std::map<string,string>::iterator itr = m_mapErrorCode.find(strStat);
                    if (itr == m_mapErrorCode.end())
                    {
                        strDesc.assign(strStat);
                    }
                    else
                    {
                        strDesc.assign(itr->second);
                    }
                    
                    smsp::StateReport report;
                    report._desc = strDesc;
                    report._smsId = strSmsid;
                    report._status = iStat;
                    report._reportTime = time(NULL);
                    reportRes.push_back(report);
                }

                return true;
            }


            /// single mo: command=MO_REQUEST&spid=****&sppassword=****&momsgid=1&da=8&sa=8&dc=15&sm=c
            if (respone.find("MO_REQUEST") != std::string::npos)
            {
                std::map<std::string,std::string> mapSet;
                split_Ex_Map(respone, "&", mapSet);

                std::string strSpid = mapSet["spid"];
                std::string strMsgid = mapSet["momsgid"];
                std::string strDa = mapSet["da"];
                std::string strSa = mapSet["sa"];
                std::string strDc = mapSet["dc"];
                std::string strSm = mapSet["sm"];

                //// single mo response:command=MO_RESPONSE&spid=****&momsgid=1184&mostat=ACCEPTD&moerrcode=000
                strResponse.assign("command=MO_RESPONSE&spid=");
                strResponse.append(strSpid);
                strResponse.append("&momsgid=");
                strResponse.append(strMsgid);
                strResponse.append("&mostat=ACCEPTD&moerrcode=000");

                if(strDa.empty())
                {
                    //LogError("this appendId is empty not support MO");
                    return false;
                }

                time_t t = time(NULL);
                struct tm pT = {0};
				localtime_r(&t,&pT);
                char strT[50] = {0};
                strftime(strT, 49, "%Y-%m-%d %X", &pT);

                string strPhone = "";
                if (0 == strSa.compare(0,2,"86"))
                {
                    strPhone = strSa.substr(2);
                }
                else
                {
                    strPhone = strSa;
                }

                smsp::UpstreamSms upsms;
                upsms._upsmsTimeStr = strT;
                upsms._upsmsTimeInt = t;
                upsms._phone = strPhone;
                upsms._appendid = strDa;

				cout<<"dc:"<<strDc.data()<<endl;
                if (0 == strDc.compare("0"))
                {
                    int iLen = strSm.length();
                    if (iLen >= 256)
                    {
                        iLen = 256;
                    }

                    char* pT = (char*)strSm.data();
                    char cTemp[127] = {0};
                    for (int i = 0; i < iLen/2; i++)
                    {
                        char temp[3] = {0};
                		memcpy(temp,pT,2);
                		pT = pT + 2;

                		int iT = hex_to_decimal_Ex(temp,2);
                		cTemp[i] = iT;
                    }

                    cout<<"mo content:"<<cTemp<<endl;
                    upsms._content.assign(cTemp);
                }
                else if (0 == strDc.compare("8"))
                {
					UTF16 utf16[256] = {0};
					int iHex = strSm.length() / 4;
					if (iHex >= 256)
					{
						iHex = 256;
					}
					
					for (int i = 0; i < iHex; i++)
					{
						string strValue = strSm.substr(i*4,4);
						utf16[i] = hex_to_decimal_Ex(strValue.data(),4);
					}
					UTF8 utf8[256] = {0};

					UTF16ToUTF8(utf16, utf16 + strSm.length() / 4, utf8, utf8 + 256);
					cout<<"content:"<<utf8<<endl;
					upsms._content.assign((char*)utf8);
                }
                else if (0 == strDc.compare("15"))
                {
                    upsms._content = AppendPer(strSm);
                    strSm = Decode(upsms._content);
                    utils::UTFString utfHelper = utils::UTFString();
                    utfHelper.g2u(strSm, upsms._content);
                    ///LogNotice("***MO***[%s].", upsms._content.data());
                }
                else
                {
                    //LogError("datacode is error [%s].", strDc.data());
                    return false;
                }

                upsmsRes.push_back(upsms);
                return true;
            }

            /// multi mo:
            if (respone.find("mos=") != std::string::npos)
            {
                /// multi mo response:command=MULTI_MO_RESPONSE&spid=1234&mostat=ACCEPTD&moerrcode=000
                strResponse.assign("command=MULTI_MO_RESPONSE&spid=");
                strResponse.append(g_strSpid);
                strResponse.append("&mostat=ACCEPTD&moerrcode=000");

                std::string strData = respone.substr(respone.find("=")+1);
                ///mos=1184640/8613012345678//0/0/15/c4e3bac332303038,0268/8//0/0/15/c4e3bac33230303832
                std::vector<std::string> upSet;
                std::vector<std::string> upData;
                split_Ex_Vec(strData, ",", upSet);
                if (0 == upSet.size())
                {
                    //LogError("this is up number is 0");
                    return false;
                }

                for (int i = 0; i < upSet.size(); i++)
                {
                    std::string strUp = upSet.at(i);
                    upData.clear();
                    split_Ex_Vec(strUp, "/", upData);
                    if (upData.size() < 7)
                    {
                        //LogError("up param is last[%d].", upData.size());
                        return false;
                    }


                    std::string strMsgid = upData.at(0);
                    std::string strSa = upData.at(1);
                    std::string strDa = upData.at(2);
                    std::string strDc = upData.at(5);
                    std::string strSm = upData.at(6);

                    if (strDa.empty())
                    {
                        //LogError("appendid is empty not support MO");
                        continue;
                    }


                    time_t t = time(NULL);
                    struct tm pT = {0};
					localtime_r(&t,&pT);
                    char strT[50] = {0};
                    strftime(strT, 49, "%Y-%m-%d %X", &pT);

                    string strPhone = "";
                    if (0 == strSa.compare(0,2,"86"))
                    {
                        strPhone = strSa.substr(2);
                    }
                    else
                    {
                        strPhone = strSa;
                    }

                    smsp::UpstreamSms upsms;
                    upsms._upsmsTimeStr = strT;
                    upsms._upsmsTimeInt = t;
                    upsms._phone = strPhone;
                    upsms._appendid = strDa;

					cout<<"dc:"<<strDc.data()<<endl;
                    if (0 == strDc.compare("0"))
                    {
                        int iLen = strSm.length();
                        if (iLen >= 256)
                        {
                            iLen = 256;
                        }

                        char* pT = (char*)strSm.data();
                        char cTemp[127] = {0};
                        for (int i = 0; i < iLen/2; i++)
                        {
                            char temp[3] = {0};
                    		memcpy(temp,pT,2);
                    		pT = pT + 2;

                    		int iT = hex_to_decimal_Ex(temp,2);
                    		cTemp[i] = iT;
                        }

                        cout<<"mo content:"<<cTemp<<endl;
                        upsms._content.assign(cTemp);
                    }
                    else if (0 == strDc.compare("8"))
                    {
                        UTF16 utf16[256] = {0};
						int iHex = strSm.length() / 4;
						if (iHex >= 256)
						{
							iHex = 256;
						}
						
						for (int i = 0; i < iHex; i++)
						{
							string strValue = strSm.substr(i*4,4);
							utf16[i] = hex_to_decimal_Ex(strValue.data(),4);
						}
						UTF8 utf8[256] = {0};

						UTF16ToUTF8(utf16, utf16 + strSm.length() / 4, utf8, utf8 + 256);
						cout<<"content:"<<utf8<<endl;
						upsms._content.assign((char*)utf8);
                    }
                    else if (0 == strDc.compare("15"))
                    {
                        upsms._content = AppendPer(strSm);
                        strSm = Decode(upsms._content);
                        utils::UTFString utfHelper = utils::UTFString();
                        utfHelper.g2u(strSm, upsms._content);
                        ///LogNotice("***MO***[%s].", upsms._content.data());
                    }
                    else
                    {
                        //LogError("datacode is error [%s].", strDc.data());
                        return false;
                    }
                    upsmsRes.push_back(upsms);
                }

                return true;
            }

            return true;
        }
        catch(...)
        {
            //LogError("[YTWLChannellib] parseReport is failed.");
            return false;
        }

    }


    unsigned char ToHex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }

    std::string YTWLChannellib::Encode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
        return strTemp;
    }

    int YTWLChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        ///LogNotice("***utf-8***[%s].", sms->_content.data());

        std::string content;
        utils::UTFString utfHelper = utils::UTFString();
        utfHelper.u2g(sms->m_strContent, content);
        ///LogNotice("***gbk***[%s].", content.data());

        content = Encode(content);
        ///LogNotice("***gbk-1***[%s].", content.data());

        std::string::size_type pos;
        pos = url->find("%contentEx%");
        if (pos != std::string::npos)
        {
            url->replace(pos, strlen("%contentEx%"), content);
        }

        return 0;
    }


} /* namespace smsp */
