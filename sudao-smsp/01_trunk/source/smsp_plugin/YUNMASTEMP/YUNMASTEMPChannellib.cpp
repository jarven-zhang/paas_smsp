#include "YUNMASTEMPChannellib.h"
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
            return new YUNMASTEMPChannellib;
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

	
    YUNMASTEMPChannellib::YUNMASTEMPChannellib()
    {
    }
    YUNMASTEMPChannellib::~YUNMASTEMPChannellib()
    {

    }

    bool YUNMASTEMPChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
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

    bool YUNMASTEMPChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
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


    int YUNMASTEMPChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		/*
		mas_user_id=e45be2eb-48fb-46a1-a0e1-3b3f744d8301&template_id=5335&params=验证码789456&mobiles=18926016041&content=&sign=ZQPCyhcV&serial=123&mac=8BD72658AD88B9E495B7AB2D4BBDE353

		mas_user_id=%mas_user_id%&template_id=%tempid%&params=%param%&mobiles=%phone%&content=&sign=ZQPCyhcV&serial=%displaynum%&mac=%mac%

		mas_user_id,template_id,params,mobiles,content,sign,serial,access_token 拼接做md5 HEX
		mas_user_idmobilescontentsignserialaccess_token
		*/

		////content1:par1;content2:par2
		vector<string> vecInfo;
		split_Ex_Vec(sms->m_strTemplateParam, ";", vecInfo);
		string strParam = "";
		for (int i = 0; i < vecInfo.size(); i++)
		{
			if (i != (vecInfo.size() -1))
			{
				strParam.append(vecInfo.at(i));
				strParam.append(" ");
			}
			else
			{
				strParam.append(vecInfo.at(i));
			}
		}


		std::string::size_type pos1 = data->find("&sign=");
		std::string::size_type pos2 = data->find("&serial");
		string strSign = data->substr(pos1+6,pos2-pos1-6);

		


		string strMd5Data = "";
		strMd5Data.append(sms->m_strMasUserId);
		strMd5Data.append(sms->m_strChannelTemplateId);
		strMd5Data.append(strParam);
		strMd5Data.append(sms->m_strPhone);
		strMd5Data.append(strSign);
		strMd5Data.append(sms->m_strShowPhone);
		strMd5Data.append(sms->m_strMasAccessToken);

		/////cout<<"strMd5Data:"<<strMd5Data.data()<<endl;

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

		pos = data->find("%param%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%param%"),strParam);
		}

		pos = data->find("%mas_user_id%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%mas_user_id%"),sms->m_strMasUserId);
		}

		pos = data->find("%tempid%");
		if (pos != std::string::npos)
		{
			data->replace(pos, strlen("%tempid%"),sms->m_strChannelTemplateId);
		}
		
        return 0;
    }

    int YUNMASTEMPChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
    {
        smsChIntvl._itlValLev1 = 20;   //val for lev2, num of msg
        //smsChIntvl->_itlValLev2 = 20; //val for lev2

        smsChIntvl._itlTimeLev1 = 20;      //time for lev1, time of interval
        smsChIntvl._itlTimeLev2 = 2;       //time for lev 2
        //smsChIntvl->_itlTimeLev3 = 1;     //time for lev 3
		return 0;
    }


} /* namespace smsp */

