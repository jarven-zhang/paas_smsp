#include "RHChannellib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <algorithm>
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"
#include "xml.h"
//#include "base64.h"
#define AS_STRING(JSON)		((JSON).type() == Json::stringValue ? (JSON).asString() 					\
																  : ((JSON).type() == Json::intValue ? int2str((JSON).asInt()) : ""))

namespace smsp 
{
static string int2str(int idata){
	char chtmp[16] = {0};
	snprintf(chtmp, sizeof(chtmp), "%d", idata);
	
	return string(chtmp);
}

RHChannellib::RHChannellib()
{
}

RHChannellib::~RHChannellib()
{
}

extern "C"
{	
    void * create()
    {
        return new RHChannellib;
    }
}

bool RHChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{		
	/*
	*	{"disPlayNbr":"","msgId":"2c9076be60c4026a0160c4026a590000","rstCode":"200","rstMsg":"ok"}
	*/
	printf("on %s, respone: %s\n", __FUNCTION__, respone.c_str());
	try
    {			
    	if (respone.empty()){
			return false;
		}			
		
		/*
		*	parse XML message info
		*/
		TiXmlDocument myDocument;
		TiXmlElement *childElement = NULL;
			
	    if(myDocument.Parse(respone.c_str(), 0, TIXML_DEFAULT_ENCODING)){
	    	printf("Parse: %s faild!", respone.c_str());
			
			return false;
	    }	
		
		TiXmlElement *rootElement = myDocument.RootElement();
	    if(NULL == rootElement){		
	        printf("RootElement is NULL.");
						
	        return false;
	    }	
			
		childElement = rootElement;
		do{
			childElement = childElement->FirstChildElement();
			if(NULL == childElement){
				printf("ChildElement NULL.");
				
				return false;
			}
		}while(NULL == childElement->GetText());
		
		TiXmlElement *element = childElement;
		char tag_value[64];
		do{	
			memset(tag_value, 0, sizeof(tag_value));
			if(strlen(element->Value()) < sizeof(tag_value))
				strcpy(tag_value, element->Value());
																			
			if(0 == strcasecmp(tag_value, "result")){
				status = element->GetText();
				strReason = status;
			}		
			else{
				printf("invalid field: %s", tag_value);
			}		
			element = element->NextSiblingElement();
		}while(NULL != element);
				
		printf("_smsid: %s, _status: %s, _strReason: %s\n", smsid.c_str(), status.c_str(), strReason.c_str());
    }
    catch(...)
    {		
    	printf("internal err\n");
        return false;
    }	
	
	return true;
}

bool RHChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{	
	/*	
		状态报告: msgId, phoneNum, statusCode, statusMsg, statusTime, userData
		
		上行: msgId, extendCode, callerNum, content, statusTime, sign, userData
	*/
	printf("respone: %s\n", respone.c_str());
	try{
		if (respone.empty()){
			return false;
		}	
		Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        std::string js;

		if (Json::json_format_string(respone, js) < 0){
            printf("json_format_string err, json: %s\n", respone.c_str());
			
            return false;
        }	
        if(!reader.parse(js, root))
        {		
            printf("json parse err, json: %s\n", respone.c_str());
            return false;
        }			
			
		/* report */
		if(!root["report"].isNull()){
			printf("report\n");
			smsp::StateReport report;
							
			if(root["report"].type() == Json::arrayValue){
				for(int i = 0; i < (int)root["report"].size(); i++){
					Json::Value json_report;
					
					json_report = root["report"][i];					
					if(json_report.type() == Json::arrayValue){
						report._smsId = AS_STRING(json_report[(Json::Value::UInt)5]);
						report._reportTime = time(NULL);
						report._status = smsp::Channellib::decodeBase64(
												AS_STRING(json_report[(Json::Value::UInt)3])) == "DELIVRD"
														? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL;
						report._desc  = smsp::Channellib::decodeBase64(AS_STRING(json_report[(Json::Value::UInt)3]));
						report._phone = AS_STRING(json_report[(Json::Value::UInt)1]);
						
						reportRes.push_back(report);
					}	
					else{
						printf("not Json::arrayValue: %s!!!\n", respone.c_str());
					}
				}
			}
		}	
		/* MO */
		else if(!root["mo"].isNull()){
			printf("MO\n");
			smsp::UpstreamSms up;
			
			if(root["mo"].type() == Json::arrayValue){
				for(int i = 0; i < (int)root["mo"].size(); i++){
					Json::Value json_mo;
						
					json_mo = root["mo"][i];
					if(json_mo.type() == Json::arrayValue){
						up._appendid = AS_STRING(json_mo[(Json::Value::UInt)2]);
						up._content  = smsp::Channellib::decodeBase64(AS_STRING(json_mo[(Json::Value::UInt)3]));
						up._phone	 = AS_STRING(json_mo[(Json::Value::UInt)1]);
						up._upsmsTimeInt = time(NULL);
						
						char szreachtime[64] = {0};
						struct tm pTime = {0};
						localtime_r((time_t*) &up._upsmsTimeInt, &pTime);
						strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
						up._upsmsTimeStr =	std::string(szreachtime);
							
						upsmsRes.push_back(up);
					}	
					else{
						printf("not Json::arrayValue: %s!!!\n", respone.c_str());
					}
				}	
			}
		}	
		else{
			printf("invalid data: %s\n", respone.c_str());
		}	
		
		strResponse.assign("{\"result\":\"0\"}");
	}
	catch(...){
		
		return false;
	}
	
	return true;
}

int RHChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){
	if(NULL == data || NULL == sms){
		return -1;
	}			
	printf("post data(with meta data): %s, m_strChannelSmsId: %s, m_content: %s\n", 
			(*data).c_str(), sms->m_strChannelSmsId.c_str(), sms->m_strContent.c_str());
	
	/* HTTP hdr action */
	vhead->push_back("Action: \"submitreq\"");
	
    // 01520561593186643146911520561593000001
    // 0      1593    4314691
	sms->m_strChannelSmsId = sms->m_strChannelSmsId.substr(0, 1) + sms->m_strChannelSmsId.substr(6, 4) + sms->m_strChannelSmsId.substr(15, 7);
	/* replace the meta data */
	std::string::size_type pos = (*data).find("%content%");
	if (pos != std::string::npos){
		string content = smsp::Channellib::urlDecode(sms->m_strContent);
		printf("m_content: %s\n", content.c_str());
		(*data).replace(pos, strlen("%content%"), smsp::Channellib::encodeBase64(content));
	}
	
	return 0;
}	

} /* namespace smsp */
