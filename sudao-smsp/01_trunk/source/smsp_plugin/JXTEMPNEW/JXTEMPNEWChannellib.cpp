#include "JXTEMPNEWChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    JXTEMPNEWChannellib::JXTEMPNEWChannellib()
    {
    }
    JXTEMPNEWChannellib::~JXTEMPNEWChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new JXTEMPNEWChannellib;
        }
    }

    bool JXTEMPNEWChannellib::parseSend(std::string response,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
		{
			if (response.empty())
			{
				return false;
			}

			Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(response, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js, root))
            {
                return false;
            }

			std::string result_code = root["resultCode"].asString();
			std::string result_desc = root["error_description"].asString();
			if (result_code == "0")
			{
				strReason.assign(result_code);
			}
			else
			{
				strReason.assign(result_code).append("|").append(result_desc);
			}

			status = (result_code == "0") ? "0" : "4";
			
			return true;
			
		}
		catch(...)
		{
			return false;
		}
	}

    bool JXTEMPNEWChannellib::parseReport(std::string response,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
    	return false;
    }

    int JXTEMPNEWChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	//data: 
    	//oauth_token=%oauth_token%&serialId=%serialId%&templateId=%templateId%&phone=%phone%&params=%params%&sign=%sign%
    	/*
		oauth_token=%oauth_token%&tempId=%templateId%&destPhone=%phone%&tempArgs=%params%&messageType=0
		*/
		
		std::string::size_type pos = 0;
		pos = data->find("%oauth_token%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%oauth_token%"), sms->m_strAccessToken);
		}

		pos = data->find("%templateId%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%templateId%"), sms->m_strChannelTemplateId);
		}


		////content1:par1;content2:par2
		vector<string> vecInfo;
		split_Ex_Vec(sms->m_strTemplateParam, ";", vecInfo);
		string strParam = "";
		for (int i = 0; i < vecInfo.size(); i++)
		{
			string strTemp = "content";
			char cTemp[25] = {0};
			snprintf(cTemp,25,"%d",i+1);
			strTemp.append(cTemp);
			strTemp.append(":");
			strTemp.append(vecInfo.at(i));

			strParam.append(strTemp);
			if (i != (vecInfo.size() -1))
			{
				strParam.append(";");
			}	
		}

		pos = data->find("%params%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%params%"),strParam);
		}
		
        return 0;
    }

	int JXTEMPNEWChannellib::adaptEachChannelforOauth(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
	{		
		/*
		//需要从认证url中获取appSecret, appkey, 以供发送短信时使用
		获取认证token url地址:
		http://www.mportal.cn/open/oauth/authorize

		获取认证token post数据:
		grant_type=refresh_token&client_id=110faba2-ef8d-47f0-9127-31e02f176738&client_secret=xCla6zpZxZf9Eanwllcj/Q==&refresh_token=refresh_token
		*/
		
		vector<std::string> vecResult;
		split_Ex_Vec(*data, "&", vecResult);

		std::string::size_type pos = 0;
		std::string appSecretKey = "client_secret=";
		for (vector<std::string>::iterator itor = vecResult.begin(); itor != vecResult.end(); ++itor)
		{
			pos = (*itor).find(appSecretKey);
			if (pos != std::string::npos)
			{
				sms->m_strAppSecret = (*itor).substr(pos + appSecretKey.length());
				break;
			}
		}

		std::string appKey = "client_id=";
		for (vector<std::string>::iterator itor = vecResult.begin(); itor != vecResult.end(); ++itor)
		{
			pos = (*itor).find(appKey);
			if (pos != std::string::npos)
			{
				sms->m_strAppKey = (*itor).substr(pos + appKey.length());
				break;
			}
		}

		if (sms->m_strAppSecret.empty())
		{
			return 1;
		}
		
		vhead->push_back("Authorization: Basic czZCaGRSa3F0MzpnWDFmQmF0M2JW");
		vhead->push_back("Content-Type: application/x-www-form-urlencoded");
		return 0;
	}

	bool JXTEMPNEWChannellib::parseOauthResponse(std::string response, models::AccessToken& accessToken, std::string& reason)
	{
		/*
		{"expires_in":69638400,"refresh_token":"37613ertgde83f46157246ada6c1289","access_token":"ebcddb18a2a8fgyuo75166b7f384e1"}
		{"error":"1000000030","error_description":"REFRESH_TOKEN_OVERDUE"}
		*/
		
		try
		{	
			if (response.empty())
			{
				return false;
			}

			Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(response, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js, root))
            {
                return false;
            }
			if (root.isMember("access_token"))
			{

				Int64 expires_in = root["expires_in"].asDouble();
	            std::string access_token = root["access_token"].asString();
				std::string refresh_token = root["refresh_token"].asString();

				accessToken.m_iExpiresIn = expires_in;
				accessToken.m_strAccessToken = access_token;
				accessToken.m_strRefreshToken = refresh_token;
				return true;
			}
			else
			{
				std::string result_code = root["error"].asString();
				std::string result_desc = root["error_description"].asString();

				reason.assign(result_code).append("|").append(result_desc);
				return false;
			}
			
		}
		catch(...)
		{	
			cout<<"==test== catch"<<endl;
			return false;
		}
	}
	
	void JXTEMPNEWChannellib::generateResponse(std::string& strResponse, int flag)
	{
		return;
	}
} /* namespace smsp */
