#include "YPWChannellib.h"
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
            return new YPWChannellib;
        }
    }

    YPWChannellib::YPWChannellib()
    {
    }
    YPWChannellib::~YPWChannellib()
    {

    }

    bool YPWChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
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
            
            strReason = root["msg"].asString();
            double iSid = root["sid"].asDouble();
            
            int iCode = root["code"].asInt();
            char cSid[100] = {0};
            snprintf(cSid,100,"%.0f",iSid);
            smsid.assign(cSid);
            
            if (0 == iCode)
            {
                status = "0";
            }

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool YPWChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {   
            if (respone.length() < 10)
            {
                return false;
            }

            string strData = "";
            strData.append("{\"ypw\":\"work\",\"respone\":");
            strData.append(respone);
            strData.append(",\"result\":1}");
               
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(strData, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js,root))
            {
                return false;
            }

            Json::Value obj;
            if(!root["respone"].isNull())
            {
                obj = root["respone"];
            }
            else
            {
                return false;
            }

            for (int i = 0; i < obj.size(); i++)
            {   
                
                Json::Value tmpValue = obj[i];
                std::string::size_type pos = respone.find("base_extend");
                if (pos == std::string::npos) ////RT
                {
                    smsp::StateReport report;
                    double iSid = tmpValue["sid"].asDouble();
                    char cSid[100] = {0};
                    snprintf(cSid,100,"%.0f",iSid);
                    report._smsId.assign(cSid);
                    report._desc = tmpValue["error_msg"].asString();
                    string strState = tmpValue["report_status"].asString();
                    if ("SUCCESS" == strState)
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
                else ////MO
                {   
                    smsp::UpstreamSms upsms;
                    upsms._appendid = tmpValue["extend"].asString();
                    upsms._content = tmpValue["text"].asString();
                    upsms._phone = tmpValue["mobile"].asString();
                    upsms._upsmsTimeStr = tmpValue["reply_time"].asString();
                    upsmsRes.push_back(upsms);
                }
            }
            return true;
        }
        catch(...)
        {
            return false;
        }
    }


    int YPWChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        std::string::size_type pos = data->find("%extend%");
		if(pos!=std::string::npos)
		{
			data->replace(pos, strlen("%extend%"), sms->m_strShowPhone);
		}

        return 0;
    }

    int YPWChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
    {
        smsChIntvl._itlValLev1 = 20;   //val for lev2, num of msg
        //smsChIntvl->_itlValLev2 = 20; //val for lev2

        smsChIntvl._itlTimeLev1 = 20;      //time for lev1, time of interval
        smsChIntvl._itlTimeLev2 = 2;       //time for lev 2
        //smsChIntvl->_itlTimeLev3 = 1;     //time for lev 3
		return 0;
    }


} /* namespace smsp */
