#include "TESTINChannellib.h"
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
            return new TESTINChannellib;
        }
    }
    
    TESTINChannellib::TESTINChannellib()
    {
    }
    TESTINChannellib::~TESTINChannellib()
    {

    }

    void TESTINChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool TESTINChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
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

            int iSuccess = root["code"].asInt();
			strReason = root["msg"].asString();
            if (1000 == iSuccess)
            {
                status = "0";
            }
            else
            {
                status = "4";
            }

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool TESTINChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {   
            if (respone.length() < 10)
            {
                return false;
            }
               
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

            Json::Value obj;
			std::string op;
			op = root["op"].asString();
           
			if(op == "Sms.status")
			{
				obj = root["list"];
				for (int i = 0; i < obj.size(); i++)
				{    
					Json::Value tmpValue = obj[i];

					smsp::StateReport report;
					report._smsId = tmpValue["taskId"].asString();
					report._status = tmpValue["status"].asInt();
					report._desc = tmpValue["descr"].asString();
					if (report._status == 1)
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
			}
			else if(op == "Sms.mo")
			{
				obj = root["list"];
				for (int i = 0; i < obj.size(); i++)
				{    
					Json::Value tmpValue = obj[i];

					smsp::UpstreamSms Upstream;
					Upstream._phone = tmpValue["phone"].asString();
					Upstream._appendid = tmpValue["extNum"].asString();
					Upstream._content = tmpValue["content"].asString();
					Upstream._upsmsTimeInt = time(NULL);
					upsmsRes.push_back(Upstream);
				}
			}			
            
            strResponse = "SUCCESS";
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    int TESTINChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		if(sms == NULL || url == NULL || data == NULL || vhead == NULL)
		{
			return -1;
		}
		//&apiKey=a9209af396a2e76b822a328530437cd7&templateId=1475&secretKey=7DF95BA2C4774EC3
		struct timeval end;
		gettimeofday(&end, NULL);
		long msec = end.tv_sec * 1000 + end.tv_usec/1000;
		std::string stime = "";
        //itoa(msec,&stime);
		char buf[32] = {0};
		sprintf(buf,"%ld",msec);    
		stime.assign(buf);
		
		std::map<std::string, std::string> SetMap;
		std::string strDime = "&";
		split_Ex_Map(*data, strDime, SetMap);

		/////ping zi fu chuan
		std::string op = "Sms.send";
		std::string apiKey = SetMap["apiKey"];
		std::string ts = stime;
		std::string templateId = SetMap["templateId"];
		std::string phone = sms ->m_strPhone;
		std::string content = sms ->m_strContent;
		std::string taskId = sms ->m_strChannelSmsId;
		std::string secretKey = SetMap["secretKey"];
        std::string strExtNum = sms->m_strShowPhone;
		
		std::string temp = "apiKey";
		temp.append("=");
		temp.append(apiKey);
		temp.append("content");
		temp.append("=");
		temp.append(content);
        ////add extNum
        temp.append("extNum=");
        temp.append(strExtNum);
        
		temp.append("op");
		temp.append("=");
		temp.append(op);
		temp.append("phone");
		temp.append("=");
		temp.append(phone);
		temp.append("taskId");
		temp.append("=");
		temp.append(taskId);
		temp.append("templateId");
		temp.append("=");
		temp.append(templateId);
		temp.append("ts");
		temp.append("=");
		temp.append(ts);
		temp.append(secretKey);
		
		
		unsigned char md5[16] = {0};
        MD5((const unsigned char*) temp.data(), temp.length(), md5);

        std::string strMD5 = "";
        std::string HEX_CHARS = "0123456789abcdef";

        for (int i = 0; i < 16; i++)
        {
            strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }
		std::string sig = strMD5;
		
		
		/////gouzao fa Json
		std::string strJson = "";
		strJson.append("{");
		strJson.append("\"op\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(op);
		strJson.append("\"");
		strJson.append(",");
		
		strJson.append("\"apiKey\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(apiKey);
		strJson.append("\"");
		strJson.append(",");

		strJson.append("\"ts\"");
		strJson.append(":");
		strJson.append(ts);
		strJson.append(",");
		
		strJson.append("\"templateId\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(templateId);
		strJson.append("\"");
		strJson.append(",");

		strJson.append("\"phone\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(phone);
		strJson.append("\"");
		strJson.append(",");
		
		strJson.append("\"content\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(content);
		strJson.append("\"");
		strJson.append(",");
		
		strJson.append("\"taskId\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(taskId);
		strJson.append("\"");
		strJson.append(",");

        ////add extNum
        strJson.append("\"extNum\"");
        strJson.append(":");
        strJson.append("\"");
        strJson.append(strExtNum);
        strJson.append("\"");
        strJson.append(",");
		
		strJson.append("\"sig\"");
		strJson.append(":");
		strJson.append("\"");
		strJson.append(sig);
		strJson.append("\"");
		strJson.append("}");
		
		*data = strJson;
		
        return 0;
	}
} /* namespace smsp */
