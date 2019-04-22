#include "LJLZChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"
#include "HttpParams.h"
#include "UTFString.h"

namespace smsp 
{
LJLZChannellib::LJLZChannellib() 
{
}

LJLZChannellib::~LJLZChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new LJLZChannellib;
    }
}

bool LJLZChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{
	/*
	ok:mid:tele
	ok:mid:tele,tele（多个号码返回格式）
	error:错误描述
	fail:错误代码码:tele
	fail:错误代码码:tele1,tele2
	*/
    try
    {
    	std::string strContent = respone;
        std::string::size_type pos = respone.find("\r\n");
        if (pos == std::string::npos)
        {
            std::string::size_type pos = respone.find("\n");
	        if (pos != std::string::npos)
	        {
	            strContent = respone.substr(0, pos);
	        }
        }
		else
		{
			strContent = respone.substr(0, pos);
		}

        vector<std::string> vecResult;
		split_Ex_Vec(strContent, ":", vecResult);

		smsid = "";
		status = "4";
		strReason = strContent;

		int iResultLen = vecResult.size();
		if (iResultLen < 2)
		{
			strReason = vecResult.at(1);
			return false;
		}
		else if (iResultLen >= 3)     //响应为  ok:mid:tele   或者  fail:错误代码码:tele
		{
			if (vecResult.at(0) == "ok")
			{
				status = "0";
				smsid = vecResult.at(1);
				strReason = "ok";
			}
			else
			{
				strReason = vecResult.at(1);
			}
		}
		
        return true;
    }
    catch(...)
    {
        //std::cout<<"***LJLZChannellib::parseSend is except.***"<<std::endl;
        return false;
    }
}

bool LJLZChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{
	/*
	type字段区分 状态报告 和 上行
	状态报告: 
		单条: type=4&msg=DELIVRD&mobile=18589033040&mid=100055187&port=11&area=%CE%DE&city=%CE%DE
	    批量: type=4&msg=DELIVRD,DELIVRD,REJECT&mobile=13666666666,13777777777,13888888888&mid=4067381, 4067383, 4067385&port=port
	上行: mid=100055187&type=1&msg=%B2%E2%CA%D4%C9%CF%D0%D02&mobile=18589033040&port=106904651970002&area=&city= 
	*/
    try
    {
        std::map<std::string,std::string> reportSet;
        split_Ex_Map(respone, "&", reportSet);

		std::string type = reportSet["type"];
		std::string port = reportSet["port"];
		std::string strSmsid;
		std::string strStatus;
		std::string strMobile;

		if (type == "4")    //状态报告
		{
			vector<std::string> vecSmsid;
			vector<std::string> vecStatus;
			vector<std::string> vecMobile;
			
			split_Ex_Vec(reportSet["mid"], ",", vecSmsid);
			split_Ex_Vec(reportSet["msg"], ",", vecStatus);
			split_Ex_Vec(reportSet["mobile"], ",", vecMobile);

			int vecResultLen = vecSmsid.size();
			if ((vecStatus.size() != vecResultLen) || (vecMobile.size() != vecResultLen))
			{
				return false;
			}

			int iStat = -1;
			for (int i = 0; i < vecResultLen; i++)
			{
				strSmsid = vecSmsid.at(i);
				strStatus = vecStatus.at(i);
				strMobile = vecMobile.at(i);

				if (0 == strStatus.compare("DELIVRD"))
		        {
		            iStat = SMS_STATUS_CONFIRM_SUCCESS;
		        }
		        else
		        {
		            iStat = SMS_STATUS_CONFIRM_FAIL;
		        }
		        smsp::StateReport report;
		        report._desc = strStatus;
		        report._smsId = strSmsid;
		        report._status = iStat;
				report._phone  = strMobile; /* 解析手机号码*/
		        report._reportTime = time(NULL);
		        reportRes.push_back(report);
			}
		}
		else if (type == "1")    //上行
		{
			strSmsid = reportSet["mid"];
			strStatus = reportSet["msg"];
			strMobile = reportSet["mobile"];

			std::string strContent;
			utils::UTFString utfHelper;
			utfHelper.g2u(strStatus, strContent);

			smsp::UpstreamSms upstream;
			upstream._phone = strMobile;
			////upstream._appendid = port.substr(9);
			upstream._appendid = port;
			upstream._content = strContent;
			upstream._sequenceId = atoi(strSmsid.c_str());

			time_t iCurtime = time(NULL);
			char szreachtime[64] = { 0 };
			struct tm pTime = {0};
			localtime_r((time_t*) &iCurtime,&pTime);
			
			strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
			upstream._upsmsTimeStr = std::string(szreachtime);
			upsmsRes.push_back(upstream);
		}
		else
		{
			return false;
		}
		
        return true;
    }
    catch(...)
    {
        //std::cout << "***LJLZChannellib::parseReport is except: response: ***" << respone << std::endl;
        return false; 
    }
}


int LJLZChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){

	return 0;
}

} /* namespace smsp */
