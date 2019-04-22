#include "YQXChannellib.h"
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
            return new YQXChannellib;
        }
    }
    
    YQXChannellib::YQXChannellib()
    {
    }
    YQXChannellib::~YQXChannellib()
    {

    }

    void YQXChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool YQXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
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
            status = root["code"].asString();
			strReason = root["msg"].asString();
			smsid = root["msgid"].asString();
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool YQXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {

		//message=18682367823%2C170714152652614%2CDELIVRD]
		//message=18682367823,170715161554084,DELIVRD;18682367823,170715161555085,DELIVRD¦Ñ]
		//message=8618682367823/%/$/1065502007085568/%/$/??????/%/$/2017-07-14 15:36:02.0/r/n/-Typa]
		//message=8618682367823%2F%25%2F%24%2F1065502007085568%2F%25%2F%24%2F%BB%D8%C9%CF%D0%D0%2F%25%2F%24%2F2017-07-14+15%3A36%3A02.0%2Fr%2Fn%2F-Typa]
		//message=18682367823%2C170715161554084%2CDELIVRD%3B18682367823%2C170715161555085%2CDELIVRD¦Ñ]
        try
        {  
			if(respone.find("/%/$/",0) == string::npos)
			{
				string tmpResp;
				int pos1 = respone.find_last_of(',');
				if(pos1 == string::npos)
				{
					return false;
				}
				int pos2 = respone.find("message=",0);
				if(pos2 == string::npos)
				{
					return false;
				}
				tmpResp = respone.substr(pos2+8,pos1 + 7 -(pos2+8) +1);
				tmpResp.append(";");
				int pos3 = tmpResp.find(';',0);
				int pos = 0;
				//18682367823,170717191112643,DELIVRD;18682367823,170717191112642,DELIVRD;
				while(pos3 != string::npos)
				{
					smsp::StateReport report;
					int pos4 = tmpResp.find(',',pos);
					int pos5 = tmpResp.find(',',pos4+1);
					report._phone = tmpResp.substr(pos,pos4 - pos);				
					report._smsId =	tmpResp.substr(pos4 + 1,pos5 - pos4 -1);
					report._desc = tmpResp.substr(pos5 + 1,pos3 - pos5 -1);
					if(report._desc.compare("DELIVRD") == 0)
					{
						report._status = SMS_STATUS_CONFIRM_SUCCESS;
					}
					else
					{
						report._status = SMS_STATUS_CONFIRM_FAIL;
					}
					report._reportTime = time(NULL);
					reportRes.push_back(report);
					pos = pos3 + 1;
					pos3 = tmpResp.find(';',pos3 + 1);
				}
				
			}
			else
			{
				int pos1 = respone.find("message=",0);
				if(string::npos == pos1)
				{
					return false;
				}
				int pos2 = respone.find_last_of("/r/n/");
				if(string::npos == pos2)
				{
					return false;
				}
				string tmpResp = respone.substr(pos1+ 8,pos2 - pos1 - 8 + 5);
				int pos3 = tmpResp.find("/r/n/",0);
				int pos = 0;
				string content;
				string appendid;
				while(pos3 != string::npos)
				{
				//[message=8618682367823/%/$/1065502007085568/%/$/??¡ê?¡ê?/%/$/2017-07-18 11:41:07.0/r/n/]
					int pos4 = tmpResp.find("/%/$/",pos);
					int pos5 = tmpResp.find("/%/$/",pos4 + 1);
					int pos6 = tmpResp.find("/%/$/",pos5 + 1);
					smsp::UpstreamSms Upstream;
					Upstream._phone = tmpResp.substr(pos,pos4 - pos);
					appendid = tmpResp.substr(pos4 + 5,pos5 - pos4 - 5);
					Upstream._appendid = appendid.substr(12);
					content = tmpResp.substr(pos5 + 5,pos6 - pos5 - 5);
					utils::UTFString utfHelper = utils::UTFString();
                    utfHelper.g2u(content, Upstream._content);   //GBK TO UTF8
					//Upstream._content = tmpResp.substr(pos5 + 5,pos6 - pos5 - 5);
					Upstream._upsmsTimeInt = time(NULL);
					upsmsRes.push_back(Upstream);
					pos = pos3 + 5;
					pos3 = tmpResp.find("/r/n/",pos3 + 1);
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

    int YQXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	std::string::size_type pos = data->find("%content%");
		if (pos != std::string::npos) 
        {
			data->replace(pos, strlen("%content%"), sms->m_strContent);
		}		
        return 0;
	}
} /* namespace smsp */
