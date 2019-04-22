#include "DXTEMPLATEChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    DXTEMPLATEChannellib::DXTEMPLATEChannellib()
    {
    }
    DXTEMPLATEChannellib::~DXTEMPLATEChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new DXTEMPLATEChannellib;
        }
    }

    void DXTEMPLATEChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool DXTEMPLATEChannellib::parseSend(std::string response,std::string& smsid,std::string& status, std::string& strReason)
    {
    //{"header":{"desc":"success","total":1,"succ":1,"status":0,"optTime":279},"body":[{"dst":"13543255823","taskid":"0f8bbf854c684ab1b5cb3feef313bc84"}]}
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
            	cout<<"weilu_test:json format error"<<std::endl;
                return false;
            }
            if(!reader.parse(js, root))
            {
            	cout<<"weilu_test:json parse error"<<std::endl;
                return false;
            }
			Json::Value header;
			Json::Value body;
			header = root["header"];
			body = root["body"];
			int iStatus = -1;
			iStatus = header["status"].asInt();
			char tmpStatus[8] = {0};
			snprintf(tmpStatus,8,"%d",iStatus);
			status = tmpStatus;
			strReason = header["desc"].asString();
			int i = 0;
			Json::Value tmpValue = body[i];	
			smsid = tmpValue["taskid"].asString();	
			return true;		
		}
		catch(...)
		{
			cout<<"weilu_test:something is wrong";
			return false;
		}
	}

    bool DXTEMPLATEChannellib::parseReport(std::string response,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		    /*
		    {
		    "header": {
		        "appId": "7f3bfd6f109c251eba2a53c313beaf7d",
		        "appKey": "f0503ccfaf8fb6ece9511c20e52360e8",
		        "path": "/ussd/ussdService/deliverUssd",
		        "traceId": "c0a80c1aj5hvwjb5-22"
		    },
		    "body": {
		        "deliveryresult": [
		            {
		                "msg": "The USSD_delivery succeed",
		                "status": "0",
		                "target": "13543255823"
		            }
		        ],
		        "taskid": "1b6a129d34574568b6317fac1f046efc"
		    }
		}


				{
		    "header": {
		        "appId": "7f3bfd6f109c251eba2a53c313beaf7d",
		        "appKey": "f0503ccfaf8fb6ece9511c20e52360e8",
		        "path": "/ussd/ussdService/deliverUssd",
		        "traceId": "c0a80c1aj5hvwjb5-51"
		    },
		    "body": {
		        "description": "The USSD_delivery succeed",
		        "statuscode": "0",
		        "taskid": "d83ff34fea34aaf094b4ee14c8f3fad9"
		    }
}
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
            	cout<<"weilu_test:json format error"<<std::endl;
                return false;
            }
            if(!reader.parse(js, root))
            {
            	cout<<"weilu_test:json parse error"<<std::endl;
                return false;
            }
			smsp::StateReport report;
			Json::Value body = root["body"];
			Json::Value deliveryresult = body["deliveryresult"];
			if(true != deliveryresult.isNull())
			{
				report._smsId= body["taskid"].asString();
				int i = 0;
				Json::Value tmpValue = deliveryresult[i];
				report._phone= tmpValue["target"].asString();
				std::string status = tmpValue["status"].asString();
				if(status == "0")
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;				
				}
				report._desc= tmpValue["msg"].asString();

			}
			else
			{
				report._smsId = body["taskid"].asString();
				std::string status = body["statuscode"].asString();
				if(status == "0")
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;				
				}
				report._desc= body["description"].asString();
				
			}
			report._reportTime = time(NULL);
			reportRes.push_back(report);
			strResponse = "SUCCESS";
			return true;
		}
		catch(...)
		{
			return false;
		}
    }

	string getCurrentTime()
	{
		time_t iCurtime = time(NULL);
		char szreachtime[64] = { 0 };
		struct tm pTime = {0};
		localtime_r((time_t*) &iCurtime,&pTime);
		
		strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
		return szreachtime;
	}

	void splitExVector(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
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

    int DXTEMPLATEChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		    /*
		 {
		    "header": {
		        "appkey": "f0503ccfaf8fb6ece9511c20e52360e8",
		        "startTime": "%strTime%",
		        "appId": "7f3bfd6f109c251eba2a53c313beaf7d"
		    },
		    "body": {
				"dst": "%phone%",
				"template": "%strTempId%",
				"argv": [%strParam%]
			}
		}  
		"花橘子","花柚子"
		*/
		string strCurrentTime = getCurrentTime();
		int pos = data->find("%strTime%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%strTime%"), strCurrentTime);
		}

		pos = data->find("%strTempId%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%strTempId%"), sms->m_strChannelTemplateId);
		}

		std::vector<std::string> vecParam;
		splitExVector(sms->m_strTemplateParam, ";", vecParam);
		string tmpParam;
		std::vector<std::string>::iterator itr1= vecParam.begin();
		while(itr1 != vecParam.end())
		{
			tmpParam.append("\"");
			tmpParam.append(itr1->data());
			tmpParam.append("\"");
			tmpParam.append(",");
			itr1 ++;
		}
		if(true != tmpParam.empty())
		{
			tmpParam = tmpParam.substr(0,tmpParam.length() - 1);
		}
		pos = data->find("%strParam%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%strParam%"), tmpParam);
		}		
        return 0;
    }
} /* namespace smsp */
