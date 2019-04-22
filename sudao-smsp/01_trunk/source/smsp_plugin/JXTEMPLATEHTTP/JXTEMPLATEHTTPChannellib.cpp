#include "JXTEMPLATEHTTPChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    JXTEMPLATEHTTPChannellib::JXTEMPLATEHTTPChannellib()
    {
    }
    JXTEMPLATEHTTPChannellib::~JXTEMPLATEHTTPChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new JXTEMPLATEHTTPChannellib;
        }
    }

    void JXTEMPLATEHTTPChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool JXTEMPLATEHTTPChannellib::parseSend(std::string response,std::string& smsid,std::string& status, std::string& strReason)
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

			std::string result_code = root["result_code"].asString();
			std::string result_desc = root["result_desc"].asString();
			smsid = root["taskId"].asString();

			strReason.assign(result_code).append("|").append(result_desc);

			status = (result_code == "0") ? "0" : "4";
			
			return true;
			
		}
		catch(...)
		{
			return false;
		}
	}

    bool JXTEMPLATEHTTPChannellib::parseReport(std::string response,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
		{
			if (response.empty())
			{
				generateResponse(strResponse, 1);
				return false;
			}

			Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(response, js) < 0)
            {
            	generateResponse(strResponse, 1);
                return false;
            }
            if(!reader.parse(js, root))
            {
            	generateResponse(strResponse, 1);
                return false;
            }

			smsp::StateReport report;

			std::string taskid = root["taskId"].asString();
			std::string phone = root["phone"].asString();
			std::string status = root["status"].asString();
			std::string msg = root["msg"].asString();

			Int32 nStatus;
            nStatus = (status == "0") ? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL;

			if(!taskid.empty())
            {
                report._smsId = taskid;
                report._reportTime = time(NULL);
                report._status = nStatus;
                report._desc = msg;
				report._phone = phone;
                reportRes.push_back(report);
				
				generateResponse(strResponse, 0);
				return true;
            }
            else
            {
            	generateResponse(strResponse, 1);
                return false;
            }			
		}
		catch(...)
		{
			generateResponse(strResponse, 1);
			return false;
		}
    }

    int JXTEMPLATEHTTPChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	//data: 
    	//oauth_token=%oauth_token%&serialId=%serialId%&templateId=%templateId%&phone=%phone%&params=%params%&sign=%sign%

		std::string strSerialId = "";
		std::string strSign = "";

		//serialId: 应用appkey+目标号码+时间戳，时间戳，格式yyyyMMddHHmmssSSS
		struct tm timenow = {0};
		time_t now = time(NULL);
		localtime_r(&now, &timenow);
		
		char szSec[32] = {0};
	    strftime(szSec, sizeof(szSec), "%Y%m%d%H%M%S", &timenow);

		struct timeval tv;    
   		gettimeofday(&tv, NULL);
		int usec = tv.tv_usec / 1000;    //只取前三位
		
		char szTimeStamp[64] = {0};
		snprintf(szTimeStamp, sizeof(szTimeStamp), "%s%d", szSec, usec);

		strSerialId.assign(sms->m_strAppKey).append(sms->m_strPhone).append(szTimeStamp);

		//sign计算方式:
		//data= serialId|appSecret|templateId|phone （多个参数拼接，| 竖线分割参数）
		//sign=MD5(data)
		std::string strData;
		strData.assign(strSerialId).append("|")\
			.append(sms->m_strAppSecret).append("|")\
			.append(sms->m_strChannelTemplateId).append("|")\
			.append(sms->m_strPhone);
		
		unsigned char md5[16] = { 0 };       
	    MD5((const unsigned char*) strData.c_str(), strData.length(), md5);
		std::string HEX_CHARS = "0123456789abcdef";
	    for (int i = 0; i < 16; i++)
	    {
	        strSign.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
	        strSign.append(1, HEX_CHARS.at(md5[i] & 0x0F));
	    }
		
		std::string::size_type pos = 0;
		pos = data->find("%oauth_token%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%oauth_token%"), sms->m_strAccessToken);
		}

		pos = data->find("%serialId%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%serialId%"), strSerialId);
		}

		pos = data->find("%templateId%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%templateId%"), sms->m_strChannelTemplateId);
		}

		pos = data->find("%params%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%params%"), sms->m_strTemplateParam);
		}

		pos = data->find("%sign%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%sign%"), strSign);
		}
		
        return 0;
    }

	int JXTEMPLATEHTTPChannellib::adaptEachChannelforOauth(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
	{		
		//需要从认证url中获取appSecret, appkey, 以供发送短信时使用
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

	bool JXTEMPLATEHTTPChannellib::parseOauthResponse(std::string response, models::AccessToken& accessToken, std::string& reason)
	{
		/*
		json格式:
		{"expires_in":6963840000000,"refresh_token":"e86aefcc34f7eRTY9390f1ab7453d","access_token":"909dea9183ebd15QWE5e23775ceb6fa5"}
		或:
		{"result_code":"1000000030","result_desc":"REFRESH_TOKEN_OVERDUE"}
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
				Int64 expires_in = atol(root["expires_in"].asString().c_str());
	            std::string access_token = root["access_token"].asString();
				std::string refresh_token = root["refresh_token"].asString();

				accessToken.m_iExpiresIn = expires_in;
				accessToken.m_strAccessToken = access_token;
				accessToken.m_strRefreshToken = refresh_token;
				return true;
			}
			else
			{
				std::string result_code = root["result_code"].asString();
				std::string result_desc = root["result_desc"].asString();

				reason.assign(result_code).append("|").append(result_desc);
				return false;
			}
			
		}
		catch(...)
		{
			return false;
		}
	}

	/*
	strResponse: 传回的响应
	flag:
		0: 成功
		1: 失败
	*/
	void JXTEMPLATEHTTPChannellib::generateResponse(std::string& strResponse, int flag)
	{
		/*
		返回json格式
		result_code	string 
			0  接收成功  Successfully received
			1  处理失败  Processing failed
		result_desc	string
			成功返回：Successfully received
			失败返回：Processing failed
		*/

		std::string result_code, result_desc;
		if (0 == flag)
		{
			result_code = "0";
			result_desc = "Successfully received";
		}
		else
		{
			result_code = "1";
			result_desc = "Processing failed";
		}
		
		Json::Value root;
		root["result_code"] = Json::Value(result_code);
		root["result_desc"] = Json::Value(result_desc);

		Json::FastWriter writer;
		strResponse = writer.write(root);
	}
} /* namespace smsp */
