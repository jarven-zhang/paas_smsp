#include "HLJWChannellib.h"
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
            return new HLJWChannellib;
        }
    }
    
    HLJWChannellib::HLJWChannellib()
    {
    }
    HLJWChannellib::~HLJWChannellib()
    {

    }

    void HLJWChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool HLJWChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        if ("00" == respone)
        {
            status = "0";
        }
        else
        {	
            status = "4";
        }	
		strReason = respone;

        return true;
    }

    bool HLJWChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /*	
            状态报告: PlatForm=A111&FUnikey=BCAE3A000B62A4420000000013800138000&FOrgAddr=10690036123456&FDestAddr=13800138000&FSubmitTime=20161125113900&FFeeTerminal=13800138000&FServiceUPID=123456&FReportCode=DELIVRD&FAckStatus=0&FLinkID=20161125113728995961

            上行: phone=13800138000&spNumber=10690036123456&msgContent=TD&linkid=34014001B20FD20F&serviceup=SQA0001&uptime=2017-09-15+13%3A48%3A07
            */	

        try {		
            std::map<std::string, std::string> mapResp;
            split_Ex_Map(respone, "&", mapResp);

            if(mapResp.size() <= 0){
                return false;
            }

            std::map<std::string, std::string>::iterator iter = mapResp.find("FReportCode");
            // StatusReport
            if (iter != mapResp.end()) 
            {
                smsp::StateReport report;

                report._phone = mapResp["FDestAddr"];
                report._smsId = mapResp["FLinkID"];
                report._desc = mapResp["FReportCode"];

                tm tm_time;
                strptime(mapResp["FSubmitTime"].c_str(), "%Y%m%d%H%M%S", &tm_time);
                report._reportTime = mktime(&tm_time); //将tm时间转换为秒时间

                if (report._desc == "DELIVRD"         /* 电信/移动 */
                        || report._desc == "0"   /* 联通 */
                        )
                {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
                }
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}

                reportRes.push_back(report);
                return true;
            }

            iter = mapResp.find("msgContent");
            /* MO */
            if (iter != mapResp.end())
            {
				smsp::UpstreamSms mo;
				mo._phone = mapResp["phone"];
                mo._appendid = mapResp["spNumber"];
				
				utils::UTFString utfHelper;
				string strContent;
                strContent = mapResp["msgContent"];
//				utfHelper.g2u(mapResp["msgContent"], strContent);
				mo._content = strContent;
				
                std::string uptime = smsp::Channellib::urlDecode(mapResp["uptime"]);

				mo._upsmsTimeStr = uptime;
				
				tm tm_time;
				strptime(uptime.c_str(), "%Y-%m-%d %H:%M:%S", &tm_time);
				mo._upsmsTimeInt = mktime(&tm_time); //将tm时间转换为秒时间
				
				upsmsRes.push_back(mo);
                return true;
            }
        }
        catch(...){

            return false;
        }

        return false;
    }

    int HLJWChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        // 0 12345678901 2345678901 234567
        // 2 13144846898 1526366154 000103
        std::string strChannelSmsId = sms->m_strChannelSmsId;
        if (strChannelSmsId.size() >= 28)
        {
            sms->m_strChannelSmsId = strChannelSmsId.substr(0, 1) + strChannelSmsId.substr(5, 7) + strChannelSmsId.substr(14, 8) + strChannelSmsId.substr(24, 4);
        }

        if (sms != NULL)
        {
            sms->m_strContent = Channellib::urlEncode(Channellib::urlEncode(sms->m_strContent));
        }

        return 0;
	}
} /* namespace smsp */
