#include "HLChannellib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"

namespace smsp 
{
HLChannellib::HLChannellib() 
{
}

HLChannellib::~HLChannellib() 
{
}

extern "C"
{
    void * create()
    {
        return new HLChannellib;
    }
}

bool HLChannellib::parseSend(const std::string respone,std::string& smsid,std::string& status, std::string& strReason )
{	
	/*	
	*	command=MT_RESPONSE&spid=M10015&mtstat=ACCEPTD&status=0&msgid=3358123
	*/
	try
    {			
    	if (respone.empty()){
			return false;
		}			
		std::vector<char*> rspSet;
		char *res = new char[respone.size() + 1];
		
		memset(res, 0, respone.size() + 1);
		memcpy(res, respone.c_str(), respone.size());
		split(res, ";", rspSet);
		if(rspSet.size() != 2){
			delete []res;
			return false;
		}		
		if(0 == strcmp(rspSet[0], MT_RSP_CODE_OK)){
			smsid = rspSet[1];
			status = "0";			//
			strReason = rspSet[0];
		}
		else{
			status = rspSet[0];
			strReason = rspSet[1];
		}	
		delete []res;
		
		printf("smsid: %s, status: %s, strReason: %s\n", smsid.c_str(), status.c_str(), strReason.c_str());
    }
    catch(...)
    {
        return false;
    }
	
	return true;
}


bool HLChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{
	/*	
		状态报告: command=RT_RESPONSE&spid=SP0004&msgid=123456&sa=13512345678&rterrcode=000
		
		上行: command=MO_RESPONSE&spid=SP0003&moinfo=159****,C4E3BAC3B5C4
	*/	
		
	try{		
		strResponse.assign("ok");
		std::map<std::string, std::string> reportStringSet;
		split_Ex_Map(respone, "&", reportStringSet);
		
		if(reportStringSet.size() <= 0){
				
			return false;
		}

		/* report */
		if(reportStringSet.size() == 3){
			std::vector<char*> MobilesSet;
			smsp::StateReport report;
			char *mobiles = new char[reportStringSet[FIELD_MOBILES].size() + 1];
			
			memset(mobiles, 0, reportStringSet[FIELD_MOBILES].size() + 1);
			memcpy(mobiles, reportStringSet[FIELD_MOBILES].c_str(), reportStringSet[FIELD_MOBILES].size());
			split(mobiles, ";", MobilesSet);
			
			report._smsId = reportStringSet[FIELD_SMSID];
			report._reportTime = time(NULL);
			report._status = (reportStringSet[FIELD_STATUS] == RT_RSP_CODE_OK_1) 
									? SMS_STATUS_CONFIRM_SUCCESS : (reportStringSet[FIELD_STATUS] == RT_RSP_CODE_OK_2 
																	? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL);
			report._desc  = reportStringSet[FIELD_STATUS];
			for(std::vector<char*>::iterator it = MobilesSet.begin(); it != MobilesSet.end(); ++it){
				
				report._phone = *it;
				reportRes.push_back(report);
			}

			delete []mobiles;
		}
		/* MO */
		else if(reportStringSet.size() == 2){
			smsp::UpstreamSms up;
			
			up._appendid = "";
			up._content  = reportStringSet[FIELD_MSG];
			up._phone	 = reportStringSet[FIELD_MOBILES];
			up._upsmsTimeInt = time(NULL);
			
			char szreachtime[64] = { 0 };
			struct tm pTime = {0};
			localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
			strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
			up._upsmsTimeStr =	std::string(szreachtime);
			upsmsRes.push_back(up);
		}
		else{
			
			return false;
		}
	}
	catch(...){
		
		return false;
	}
	
	return true;
}
			
int HLChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){
	if(vhead == NULL){
		return -1;
	}	
		
	string contentType = "Content-Type: application/x-www-form-urlencoded";
	vhead->push_back(contentType);
		
	return 0;
}	

} /* namespace smsp */
