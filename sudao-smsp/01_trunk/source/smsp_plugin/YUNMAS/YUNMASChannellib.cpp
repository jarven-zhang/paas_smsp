#include "YUNMASChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "xml.h"

namespace smsp
{
 	extern "C"
    {
        void * create()
        {
            return new YUNMASChannellib;
        }
    }


	unsigned char ToHex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }

    unsigned char FromHex(unsigned char x)
    {
        unsigned char y=0;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else
        {
        }
        return y;
    }

    std::string UrlEncode(const std::string& str)
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
                strTemp += ToHex((unsigned char)str[i] >> 4);
                strTemp += ToHex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }

    std::string UrlDecode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+') strTemp += ' ';
            else if (str[i] == '%')
            {
//            if(i + 2 < length){
//              LogCrit("UrlDecode Err");
//              return "";
//            }
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high*16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }

	
    YUNMASChannellib::YUNMASChannellib()
    {
    }
    YUNMASChannellib::~YUNMASChannellib()
    {

    }

    bool YUNMASChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
			cout<<"1respone:"<<respone.c_str()<<endl;

			std::string::size_type pos = respone.find_first_of("{");
    		respone = respone.substr(pos);
			respone.erase(respone.find_last_of("}") + 1);
			
			cout<<"2respone:"<<respone.c_str()<<endl;
			
		
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;

            if (Json::json_format_string(respone, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js,root))
            {
                return false;
            }

			string strMsgGroup = "";
			string strRetDesc = "";
			
			string strRetCode = root["RET-CODE"].asString();
			if ("SC:0000" == strRetCode)
			{
				strMsgGroup = root["MSG-GROUP"].asString();
				status = "0";
				smsid = strMsgGroup;
				strReason = "SC:0000";
				return true;
			}
			else
			{
				strRetDesc = root["RET-DESC"].asString();
				strReason = strRetDesc;
				return false;
			}
			
        }
        catch(...)
        {
            return false;
        }
    }

    bool YUNMASChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
			cout<<"3respone:"<<respone.c_str()<<endl;
            std::map<std::string,std::string> mapSet;
            split_Ex_Map(respone, "&", mapSet);
            
			string strType = mapSet["type"];

			if ("2" == strType)
			{
				string strReportStatus = mapSet["report_status"];
				string strMobile = mapSet["mobile"];
				string strErrorCode = mapSet["error_code"];
				string strMsgGroup = mapSet["msg_group"];
				smsp::StateReport report;
				report._desc = strErrorCode;
				report._phone = strMobile;
				report._smsId = strMsgGroup;
				if("DELIVRD" == strReportStatus)	//reportResp confirm failed
                {
					report._status = SMS_STATUS_CONFIRM_SUCCESS; 
                    
                }
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}
				report._reportTime = time(NULL);
				reportRes.push_back(report);
				
			}
			else if ("3" == strType)
			{
				string strMobile = mapSet["mobile"];
				string strSerial = mapSet["serial"];
				string strSmsContent = mapSet["sms_content"];
				strSmsContent = UrlDecode(strSmsContent);

				smsp::UpstreamSms upsms;
				if (strSerial.length() < 16)
				{
					upsms._appendid = strSerial;
				}
				else
				{
					upsms._appendid = strSerial.substr(16);
				}
				
                upsms._content = strSmsContent;
                upsms._phone = strMobile;
                upsms._upsmsTimeInt = time(NULL);
                upsmsRes.push_back(upsms);
			}
			else
			{
				return false;
			}
            return true;
        }
        catch(...)
        {
            return false;
        }
    }


    int YUNMASChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		/*
		mas_user_id=4e0e429e-89cd-4d3f-81de-907f026a8d4d&mobiles=13410705167&content=【云之讯】你的验证码是345123，请注意查收。&sign=9sV1uxGC&serial=123&mac=93E90E732C4827D30BF1EDA0B039D683

		mas_user_id=%mas_user_id%&mobiles=%phone%&content=%contentEx%&sign=9sV1uxGC&serial=%displaynum%&mac=%mac%		

		mac 是mas_user_id，mobiles，content ，sign，serial,access_token 拼接做md5 HEX
		4e0e429e-89cd-4d3f-81de-907f026a8d4d13410705167【云之讯】你的验证码是345123，请注意查收。9sV1uxGC123SD78PbhG2QHSnrIGMYmiEMGWUb1WFx
		mas_user_idmobilescontentsignserialaccess_token
		*/

		std::string::size_type pos1 = data->find("&sign=");
		std::string::size_type pos2 = data->find("&serial");
		string strSign = data->substr(pos1+6,pos2-pos1-6);
		cout<<"strSign:"<<strSign.data()<<endl;

		string strMd5Data = "";
		strMd5Data.append(sms->m_strMasUserId);
		strMd5Data.append(sms->m_strPhone);
		strMd5Data.append(sms->m_strContent);
		strMd5Data.append(strSign);
		strMd5Data.append(sms->m_strShowPhone);
		strMd5Data.append(sms->m_strMasAccessToken);

		cout<<"strMd5Data:"<<strMd5Data.data()<<endl;

		string strMac = "";

		unsigned char md5[16] = { 0 };       
	    MD5((const unsigned char*) strMd5Data.c_str(), strMd5Data.length(), md5);
		std::string HEX_CHARS = "0123456789ABCDEF";
	    for (int i = 0; i < 16; i++)
	    {
	        strMac.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
	        strMac.append(1, HEX_CHARS.at(md5[i] & 0x0F));
	    }

		std::string::size_type pos = 0;
		pos = data->find("%mac%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%mac%"),strMac);
		}

		string strContentEx = UrlEncode(sms->m_strContent);
		pos = data->find("%contentEx%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%contentEx%"),strContentEx);
		}

		pos = data->find("%mas_user_id%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%mas_user_id%"),sms->m_strMasUserId);
		}
		
        return 0;
    }

    int YUNMASChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
    {
        smsChIntvl._itlValLev1 = 20;   //val for lev2, num of msg
        //smsChIntvl->_itlValLev2 = 20; //val for lev2

        smsChIntvl._itlTimeLev1 = 20;      //time for lev1, time of interval
        smsChIntvl._itlTimeLev2 = 2;       //time for lev 2
        //smsChIntvl->_itlTimeLev3 = 1;     //time for lev 3
		return 0;
    }


} /* namespace smsp */


int main()
{
    printf("Successful!");
}

