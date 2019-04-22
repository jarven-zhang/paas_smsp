#include "BSTSXChannellib.h"
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
            return new BSTSXChannellib;
        }
    }
    
    BSTSXChannellib::BSTSXChannellib()
    {
    }
    BSTSXChannellib::~BSTSXChannellib()
    {

    }

    void BSTSXChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool BSTSXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
			int pos1 = 0;
			pos1 = respone.find(',',0);
			if(pos1 == string::npos)   //respon failed
			{
				status = respone;
				strReason = respone;
			}
			else					//respon ok
			{
				status = respone.substr(0,pos1);
				strReason = status;
				smsid = respone.substr(pos1 +1);
			}
			return true;	
        }
        catch(...)
        {
            return false;
        }
    }

    bool BSTSXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {

		/*
		content=[{"dstphone":"13712345678","logtime":"20160920153451","msgid":"2831","state":"DELIVRD"},{"dstphone":"13912345678","logtime":"20160920153452","msgid":"2833","state":"DELIVRD"}]
		*/
        try
        {       
        	string tempStr1 = respone.substr(8);
			string tempStr = "{\"content\":";
			tempStr.append(tempStr1);
			tempStr.append("}");
            if (tempStr.length() < 10)
            {
            	std::cout<<"temp str error"<<std::endl;
                return false;
            }
               
            Json::Reader reader(Json::Features::strictMode());			
            Json::Value root;
            std::string js;
            if (Json::json_format_string(tempStr, js) < 0)
            {
                return false;
            }			
            if(!reader.parse(js,root))
            {
                return false;
            }		
			string strStatus;
			Json::Value obj;
			obj = root["content"];
			for(int i = 0 ;i < obj.size(); i ++)
			{			
				Json::Value tmpValue = obj[i];
				smsp::StateReport report;
				report._smsId = tmpValue["msgid"].asString();
				strStatus = tmpValue["state"].asString();
				if(strStatus.compare("DELIVRD") == 0)
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}
				report._desc = strStatus;
				report._reportTime = time(NULL);
				reportRes.push_back(report);			
			}
          	strResponse = "SUCCESS";
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    int BSTSXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        return 0;
	}
} /* namespace smsp */
