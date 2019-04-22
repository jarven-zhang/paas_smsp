#include "CZChannellib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <algorithm>
#include "ssl/md5.h"
#include "HttpParams.h"
#include "UTFString.h"
#include "json/json.h"

#define AS_STRING(JSON)		((JSON).type() == Json::stringValue ? (JSON).asString() : "")

namespace smsp 
{

bool cmp(std::string s1, std::string s2)  
{  
	
    return strcmp(s1.c_str(), s2.c_str()) < 0;  
} 

std::string getMd5Str(string& str)
{
	string strMd5 = "";
	unsigned char md5[16] = { 0 };       
	MD5((const unsigned char*) str.c_str(), str.length(), md5);
	std::string HEX_CHARS = "0123456789ABCDEF";
	for (int i = 0; i < 16; i++)
	{
	    strMd5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
	    strMd5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
	}
	return strMd5;
}

std::string formatParams(std::map<std::string, std::string> param_pairs){
	printf("param_pairs size: %d\n", param_pairs.size());
	
	if(MAX_PARAMS_COUNT < param_pairs.size())
		return "";
	int i = 0;
	std::string params_name[MAX_PARAMS_COUNT];
	std::string format_str;
	
	for(std::map<std::string, std::string>::iterator it = param_pairs.begin(); it != param_pairs.end(); it++){
		params_name[i++] = it->first;
		
	}	
		
	sort(params_name, params_name + param_pairs.size(), cmp);
	for(int i = 0; i < param_pairs.size(); i++){
		//printf("%s\n", params_name[i].c_str());
		format_str += params_name[i];
		format_str += param_pairs[params_name[i]];
	}		
	
	return format_str;
}

CZChannellib::CZChannellib() 
{
}

CZChannellib::~CZChannellib() 
{
}

extern "C"
{	
    void * create()
    {
        return new CZChannellib;
    }
}

bool CZChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
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
		std::vector<char*> rspSet;
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
		
		/*  */
		status = (RSP_CODE_OK == AS_STRING(root["rstCode"]) ? "0" : AS_STRING(root["rstCode"]));
        smsid = AS_STRING(root["msgId"]);
        strReason = AS_STRING(root["rstCode"]);
		printf("_smsid: %s, _status: %s, _strReason: %s\n", smsid.c_str(), status.c_str(), strReason.c_str());
    }
    catch(...)
    {	
    	printf("internal err\n");
        return false;
    }	
		
	return true;
}

bool CZChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
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
		if(!root["statusCode"].isNull() && !root["msgId"].isNull()){ 
			printf("report\n");
			smsp::StateReport report;
			
			report._smsId = AS_STRING(root["msgId"]);
			report._reportTime = time(NULL);
			report._status = (AS_STRING(root["statusCode"]) == REPORT_STATUS_CODE_OK
									? SMS_STATUS_CONFIRM_SUCCESS : SMS_STATUS_CONFIRM_FAIL);
			report._desc  = AS_STRING(root["statusCode"]);
			report._phone = AS_STRING(root["phoneNum"]);

			reportRes.push_back(report);	
		}	
		/* MO */
		else if(!root["content"].isNull() && !root["msgId"].isNull()){
			printf("MO\n");
			smsp::UpstreamSms up;
						
			up._appendid = AS_STRING(root["userData"]);
			up._content  = AS_STRING(root["content"]);
			up._phone	 = AS_STRING(root["callerNum"]);
			up._upsmsTimeInt = time(NULL);
			
			char szreachtime[64] = {0};
			struct tm pTime = {0};
			localtime_r((time_t*) &up._upsmsTimeInt, &pTime);
			strftime(szreachtime, sizeof(szreachtime), "%Y-%m-%d %H:%M:%S", &pTime);
			up._upsmsTimeStr =	std::string(szreachtime);
			
			upsmsRes.push_back(up);
		}
		
		strResponse.assign("ok");
	}
	catch(...){
		
		return false;
	}
	
	return true;
}

int CZChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){
	if(NULL == data || NULL == sms){
		return -1;
	}	
	printf("post data(with meta data): %s\n", (*data).c_str());
	Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    std::string js;
	std::vector<std::string> mem;
	std::map<std::string, std::string> param_pairs;
	
	/* 
	*	replace the meta data 
	*/
	std::string::size_type pos = (*data).find("%phone%");
	if (pos != std::string::npos)
    {
		(*data).replace(pos, strlen("%phone%"), sms->m_strPhone);
	}
	pos = (*data).find("%content%");
	if (pos != std::string::npos) 
    {
		(*data).replace(pos, strlen("%content%"), sms->m_strContent);
	}
	pos = (*data).find("%msgid%");
	if(pos!=std::string::npos)
	{	
		(*data).replace(pos, strlen("%msgid%"), sms->m_strChannelSmsId);
	}
	pos = (*data).find("%displaynum%");
	if(pos!=std::string::npos)
	{
		(*data).replace(pos, strlen("%displaynum%"), sms->m_strShowPhone);
	}	
	pos = (*data).find("%timestamp%");
	if(pos!=std::string::npos)
	{
		time_t now = time(NULL);
		struct tm *timeinfo;
		timeinfo = localtime(&now);
		char datatime[16];
				
		memset(datatime, 0, sizeof(datatime));
		snprintf(datatime, sizeof(datatime) - 1,"%04d%02d%02d%02d%02d%02d",
													timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
													timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		
		(*data).replace(pos, strlen("%timestamp%"), datatime);
	}
	
	printf("post data: %s\n", (*data).c_str());
	
	/* 
	*	parse json msg  
	*/
	if (Json::json_format_string(*data, js) < 0){
        printf("json_format_string err, post data: %s\n", data->c_str());
		
        return false;
    }	
    if(!reader.parse(js, root))
    {	
        printf("json parse err, json: %s\n", data->c_str());
        return false;
    }
	
	mem = root.getMemberNames();
	if(MAX_PARAMS_COUNT < mem.size()){
		printf("invalid params size: %d\n", mem.size());
		return false;
	}
	
	for (std::vector<std::string>::iterator it = mem.begin(); it != mem.end(); it++){
		if(Json::stringValue == root[*it].type()){
			param_pairs.insert(make_pair(*it, root[*it].asString()));
			
		}
	}
	
	/* ASCII sorting and convert to a string */
	std::string formatStr = formatParams(param_pairs);
	printf("format_str: %s\n", formatStr.c_str());
	
	/* url encoding */
	formatStr = smsp::Channellib::urlEncode(formatStr);
	printf("encoded format_str: %s\n", formatStr.c_str());

	/* md5 */
	std::string sign = getMd5Str(formatStr);
	printf("sign: %s\n", sign.c_str());
		
	/* rebuild json msg */
	Json::FastWriter fast_writer;
	
	root.removeMember("secret");
	root["sign"] = sign;
	*data = fast_writer.write(root);
	printf("rebuild post data: %s", (*data).c_str());
	
	return 0;
}	

} /* namespace smsp */
