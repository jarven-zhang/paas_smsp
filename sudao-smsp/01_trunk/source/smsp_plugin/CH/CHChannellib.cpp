#include "CHChannellib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"

namespace smsp
{
CHChannellib::CHChannellib()
{
}

CHChannellib::~CHChannellib() 
{
}

extern "C"
{
    void * create()
    {
        return new CHChannellib;
    }
}

bool CHChannellib::parseSend( std::string respone,std::string& smsid,std::string& status, std::string& strReason )
{	
	/*	
	*	command=MT_RESPONSE&spid=M10015&mtstat=ACCEPTD&status=0&msgid=3358123
	*/
	try
    {	
    	if (respone.empty())
		{
			return false;
		}
		std::map<std::string,std::string> mapSet;
	    split_Ex_Map(respone, "&", mapSet);
			
		if(mapSet.size() <= 0){
			
			return false;
		} 
		if(mapSet[CH_FIELD_COMMAND] != COMMAND_MT_RSP){
			
			return false;
		}
		status = atoi(mapSet[CH_FIELD_STATUS].c_str()) == 0 ? "0" : mapSet[CH_FIELD_STATUS];
		smsid = mapSet[CH_FIELD_SMSID];
		strReason = status;
		printf("smsid: %s, status: %s, strReason: %s\n", smsid.c_str(), status.c_str(), strReason.c_str());
    }
    catch(...)
    {
        return false;
    }
	
	return true;
}


bool CHChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{
	/*	
		状态报告: command=RT_RESPONSE&spid=SP0004&msgid=123456&sa=13512345678&rterrcode=000
		
		上行: command=MO_RESPONSE&spid=SP0003&moinfo=159****,C4E3BAC3B5C4
	*/	
		
	try{		
		std::map<std::string, std::string> reportStringSet;
		split_Ex_Map(respone, "&", reportStringSet);
		
		if(reportStringSet.size() <= 0){
				
			return false;
		}

		/* report */
		if(reportStringSet[CH_FIELD_COMMAND] == COMMAND_RT_RSP){
			std::vector<char*> MobilesSet;
			smsp::StateReport report;
			char *mobiles = new char[reportStringSet[CH_FIELD_MOBILES].size() + 1];
			
			memset(mobiles, 0, reportStringSet[CH_FIELD_MOBILES].size() + 1);
			memcpy(mobiles, reportStringSet[CH_FIELD_MOBILES].c_str(), reportStringSet[CH_FIELD_MOBILES].size());
			split(mobiles, ",", MobilesSet);
			
			report._smsId = reportStringSet[CH_FIELD_SMSID];
			report._reportTime = time(NULL);
			report._status = atoi(reportStringSet[CH_FIELD_REPORT_STATUS].c_str()) == 0
									? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL;
			report._desc  = reportStringSet[CH_FIELD_REPORT_STATUS];
			for(std::vector<char*>::iterator it = MobilesSet.begin(); it != MobilesSet.end(); ++it){
				report._phone = *it;
								
				reportRes.push_back(report);
			}	
			
			delete []mobiles;
		}
		/* MO */
		else if(reportStringSet[CH_FIELD_COMMAND] == COMMAND_MO_RSP)
		{
			smsp::UpstreamSms up;
			std::vector<char*> moinfo;
			char *moinfos = new char[reportStringSet[CH_FIELD_MO_INFO].size() + 1];
			
			memset(moinfos, 0, reportStringSet[CH_FIELD_MO_INFO].size() + 1);
			memcpy(moinfos, reportStringSet[CH_FIELD_MO_INFO].c_str(), reportStringSet[CH_FIELD_MO_INFO].size());
			split(moinfos, ",", moinfo);
			
			if(2 != moinfo.size()){
				printf("invalid mo info: %s\n", reportStringSet[CH_FIELD_MO_INFO].c_str());
				delete []moinfos;
				
				return false;
			}	
			up._appendid = "";
			up._content  = moinfo[1];
			up._phone	 = moinfo[0];
			up._upsmsTimeInt = time(NULL);
			
			char szreachtime[64] = { 0 };
			struct tm pTime = {0};
			localtime_r((time_t*) &up._upsmsTimeInt,&pTime);
			strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
			up._upsmsTimeStr =	std::string(szreachtime);
			upsmsRes.push_back(up);
			
			delete []moinfos;
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

int CHChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){
	
	if(NULL == sms){return -1;}
	replace_all_distinct(sms->m_strContent, "%", "");
		
	return 0;
}

int CHChannellib::replace_all_distinct(string& str, const string& old_value, const string& new_value){
					
	for(string::size_type   pos(0); pos != string::npos; pos+=new_value.length()){
		if((pos = str.find(old_value, pos)) != string::npos){
			str.replace(pos, old_value.length(), new_value);
		}
		else{
			
			break;
		}
	}
		
	return 0;
}	 

} /* namespace smsp */
