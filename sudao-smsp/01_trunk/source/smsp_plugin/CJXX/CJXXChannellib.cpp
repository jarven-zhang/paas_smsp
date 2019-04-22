#include "CJXXChannellib.h"
#include <time.h>
#include <stdlib.h>
#include "UTFString.h"

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new CJXXChannellib;
        }
    }
    
    CJXXChannellib::CJXXChannellib()
    {
    }
    CJXXChannellib::~CJXXChannellib()
    {

    }

    void CJXXChannellib::test()
    {
    }

    bool CJXXChannellib::parseSend(std::string respone, std::string& smsid, std::string& status, std::string& strReason)
    {
    	/*********************************************************************************************
    		respone:
		    		大于0的数字	：发送成功（得到大于0的数字、作为取报告的id）
		    		其他			：也是数字，代表失败
    	**********************************************************************************************/
		long result = atol(respone.data());

		smsid  = respone;
		
		if(result > 0)
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

    bool CJXXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/*****************************************************************************************************************************************
			状态报告:
					118$$$$13277986916$$$$2017/12/6 16:03:00$$$$成功$$$$DELIVRD||||118$$$$18589060708$$$$2017/12/6 16:03:00$$$$成功$$$$DELIVRD

			上行：
					18589060708$$$$牛逼$$$$2017/12/6 16:04:00
		******************************************************************************************************************************************/
		std::vector<std::string> vectorAllReport;
		std::vector<std::string> vectorOneReport;
		splitExVectorSkipEmptyString(respone, "||||", vectorAllReport);

		for(std::vector<std::string>::iterator iter = vectorAllReport.begin(); iter != vectorAllReport.end(); iter++)
		{
			vectorOneReport.clear();
			splitExVectorSkipEmptyString(*iter, "$$$$", vectorOneReport);

			if(vectorOneReport.size() == 5)//状态报告
			{
				smsp::StateReport report;
				
				report._smsId = vectorOneReport[0];
				report._phone = vectorOneReport[1];
				
				tm tm_time;
				strptime(vectorOneReport[2].data(), "%Y/%m/%d %H:%M:%S", &tm_time);
				report._reportTime = mktime(&tm_time); //将tm时间转换为秒时间

				report._desc = 	vectorOneReport[4];
				
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
			else if(vectorOneReport.size() == 3)//上行
			{
				smsp::UpstreamSms mo;
				utils::UTFString utfHelper;
				string strContent;
				
				mo._phone = vectorOneReport[0];
				
				utfHelper.g2u(vectorOneReport[1], strContent);
				mo._content = strContent;
				
				mo._upsmsTimeStr = vectorOneReport[2];
				
				tm tm_time;
				strptime(vectorOneReport[2].data(), "%Y/%m/%d %H:%M:%S", &tm_time);
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

    int CJXXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	/**********************************************************************************
    	POST data:
					Account=tedingL
					&Password=123
					&Phones=13277986916,18589060708
					&Content=%a1%be%b2%e2%ca%d4%a1%bf%c4%e3%ba%c3%a3%ac%cd%cb%b6%a9%bb%d8T
					&Channel=1&SendTime=
		**********************************************************************************/
		
		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=gb2312");
		
        return 0;
	}

	void CJXXChannellib::splitExVectorSkipEmptyString(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
	{
		int strDimeLen = strDime.size();
	    int lastPosition = 0, index = -1;
	    while (-1 != (index = strData.find(strDime, lastPosition)))
	    {
	    	if(index > lastPosition)
	    	{
	        	vecSet.push_back(strData.substr(lastPosition, index - lastPosition));
	    	}
	        lastPosition = index + strDimeLen;
	    }
	    string lastString = strData.substr(lastPosition);
	    if (!lastString.empty())
	    {
	        vecSet.push_back(lastString);
	    }
	}
} /* namespace smsp */
