#include "SHSJChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"
#include <map>
#include <stdlib.h>
#include "Channellib.h"

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new SHSJChannellib;
        }
    }
    
    SHSJChannellib::SHSJChannellib()
    {
    }
    SHSJChannellib::~SHSJChannellib()
    {

    }

    void SHSJChannellib::test()
    {
    }

    bool SHSJChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
    	/*********************************************************************************************
    		respone:
		  			>0	成功,系统生成的任务id或自定义的任务id
		  			如：941219590864903168
    	**********************************************************************************************/
    	string_replace(respone, "\r\n", "");
		string_replace(respone, "\n", "");
		
		smsid  = respone;
		
		if(respone.find("-") == string::npos && respone != "0")
		{
			status = "0";	
			return true;
		}
		else
		{
			status = "-1";
			return false;
		}
    }

    bool SHSJChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/***************************************************************************************************************
			状态报告:
					report=号码|状态码|短信ID|扩展码|接收时间;号码|状态码|短信ID|扩展码|接收时间
				  如：
					report=18638305275|DELIVRD|941216240207392912|13999|2017-12-14 16:00:18;18550205250|UNDELIV|941216283891075032|13999|2017-12-14 16:00:19

					状态码：
						DELIVRD 状态成功
						UNDELIV 状态失败
						EXPIRED 因为用户长时间关机或者不在服务区等导致的短消息超时没有递交到用户手机上
						REJECTD 消息因为某些原因被拒绝
						MBBLACK 黑号

			上行：
					deliver=内容|扩展号|编码格式|号码|用户名|时间

				  如：
					deliver=T|13999|utf8|18638305275|yzx006|2017-12-14 16:00:18
		****************************************************************************************************************/
        std::vector<std::string> vectorAllReport;
		std::vector<std::string> vectorOneReport;

		if(respone.find("report=") == 0)
		{
			respone = respone.substr(strlen("report="));
		}
		else if(respone.find("deliver=") == 0)
		{
			respone = respone.substr(strlen("deliver="));
		}
		else
		{
			return false;
		}
		
		split_Ex_Vec(respone, ";", vectorAllReport);

		for(std::vector<std::string>::iterator iter = vectorAllReport.begin(); iter != vectorAllReport.end(); iter++)
		{
			vectorOneReport.clear();
			split_Ex_Vec(*iter, "|", vectorOneReport);

			if(vectorOneReport.size() == 5)//状态报告
			{
				smsp::StateReport report;
				
				report._smsId = vectorOneReport[2];
				report._phone = vectorOneReport[0];
				
				tm tm_time;
				strptime(vectorOneReport[4].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
				report._reportTime = mktime(&tm_time); //将tm时间转换为秒时间

				report._desc = 	vectorOneReport[1];
				
				if(report._desc == "DELIVRD")
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}

				reportRes.push_back(report);
			}
			else if(vectorOneReport.size() == 6)//上行
			{
				smsp::UpstreamSms mo;
				
				mo._phone = vectorOneReport[3];
				mo._content = urlDecode(vectorOneReport[0]);
				mo._upsmsTimeStr = vectorOneReport[5];
				
				tm tm_time;
				strptime(vectorOneReport[2].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
				mo._upsmsTimeInt = mktime(&tm_time); //将tm时间转换为秒时间
				
				upsmsRes.push_back(mo);
			}
			else
			{
				;
			}
		}

		return true;
    }

    int SHSJChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	/*************************************************************************************************************
    	
    	data:
    		username=yzx081&password=yzx08163&mobile=%phone%&content=%content%
    		
		*************************************************************************************************************/		
    	std::map<string, string> mapData;
		split_Ex_Map(*data, "&", mapData);

		string_replace(*data, "&password=" + mapData["password"], "&password=" + CalMd5(mapData["username"] + CalMd5(mapData["password"])));

		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		
        return 0;
	}
} /* namespace smsp */
