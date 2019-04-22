#include "STXChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"


namespace smsp 
{
STXChannellib::STXChannellib() 
{
}

STXChannellib::~STXChannellib() 
{
}

extern "C"
{
    void * create()
    {
        return new STXChannellib;
    }
}

bool STXChannellib::parseSend( std::string respone,std::string& smsid,std::string& status, std::string& strReason )
{
	/*
		{"result":0,"msgid":"1590930140638054700","ts":"20170930140638"}
	*/
    try
    {
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		std::string js;	
    	std::string strContent = respone;
        std::string::size_type pos = respone.find("\r\n");
        if ( pos == std::string::npos )
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

	    if ( Json::json_format_string( strContent, js ) < 0 )
	    {
	        printf("==err== json message error, req[%s]\n", strContent.data());
	        return false;
	    }

	    if( true != reader.parse( js, root ))
	    {
	        printf("==err== json parse is failed, req[%s]", js.data());	
	        return false;
	    }

		Int32  result = root["result"].asUInt();		
		string ts     = root["ts"].isNull() ? "" : root["ts"].asCString();

		if( !root["msgid"].isNull()){
			smsid  = root["msgid"].asCString();
		}
		
		status = "4";
		strReason = strContent;

		if( result == 0  ){
			status = "0";
		}
		else
		{
			char resultStr[64] = {0};
			snprintf(resultStr, sizeof(resultStr), "%d", result );
			strReason = string(resultStr);
		}
        return true;
		
    }
    catch(...)
    {
        return false;
    }

	return true;
	
}


bool STXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{
	/*
		状态报告
		receiver=&pswd=&msgid=1590930141410440500&reportTime=1709301414&mobile=15019409157&status=JA%3A0009
		上行         
		receiver=admin&pswd=12345&moTime=1208212205&destcode=1065751600001&mobile=13800210021&msg=hello
		批量状态上报                                              msgid,       reportTime,   mobile,        status  || msgid,        reportTime,   mobile,        status
		http://pushUrl?receiver=admin&pswd=12345&report=1234567890,1012241002,13900210021,DELIVRD||1234567891,1012241002,13800210021,UNDELIV
	*/
    try
    {		
        std::map<std::string, std::string> reportStringSet;
        split_Ex_Map( respone, "&", reportStringSet );
	
		std::string moTime = reportStringSet["moTime"];

		if( moTime.empty() ){
		    /*状态报告*/
			string reports = reportStringSet["report"];
			if(!reports.empty())
			{
				/* 解析状态报告批量*/
				std::vector<std::string> reportVecs; 
				split_Ex_Vec( reports, "||", reportVecs );
				for( UInt32 i=0; i<reportVecs.size(); i++ )
				{	
					smsp::StateReport report;
					std::vector< std::string > reportDetail;
					split_Ex_Vec( reportVecs.at(i), ",", reportDetail );					
					report._reportTime = time(NULL);
					
					if( reportDetail.size() >= 1 ){
						report._smsId = reportDetail.at(0);
					}

					if( reportDetail.size() >= 2 ){
						report._phone = reportDetail.at(2);
					}

					if( reportDetail.size() >= 3 ){
						report._desc = reportDetail.at(3);
					}

					report._status = report._desc == "DELIVRD" ? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL ;		
					reportRes.push_back( report );
					
				}

			}
			else
			{
				smsp::StateReport report;
				report._smsId = reportStringSet["msgid"];
				report._phone = reportStringSet["mobile"];
				report._reportTime = time(NULL);
				report._desc  = reportStringSet["status"];
				report._status = report._desc == "DELIVRD" ? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL ;				
				reportRes.push_back( report );
			}

		}
		else
		{
			smsp::UpstreamSms up;
			up._appendid = reportStringSet["destcode"];
			up._content  = reportStringSet["msg"];
			up._phone    = reportStringSet["mobile"];
			up._upsmsTimeInt = time(NULL);
			
			char szreachtime[64] = { 0 };
			struct tm pTime = {0};
			localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
			strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
			up._upsmsTimeStr =  std::string(szreachtime);			
			upsmsRes.push_back(up);
		}
	}
	catch(...){
	
		return false;
	}

	return true;

}


int STXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){

	return 0;
}

} /* namespace smsp */
