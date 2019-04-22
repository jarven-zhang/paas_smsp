#include "ZJYDChannellib.h"
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
#include <ssl/sha.h>

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new ZJYDChannellib;
        }
    }
    
    ZJYDChannellib::ZJYDChannellib()
    {
    }
    ZJYDChannellib::~ZJYDChannellib()
    {

    }


	
	void random25(char *nonce)
	{
		char char_array[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
		'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 
		'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 
		'W', 'X', 'Y', 'Z' };
		int i;
		int index = 0;
		char tmp[3] = {0};
		srand(time(NULL));
		for(i = 0; i < 25; i++)
		{
			index = rand()%sizeof(char_array);
			sprintf(tmp,"%c", char_array[index]);
			strcat(nonce,tmp);
		}
		
	}


	#define b64_conv_bin2ascii(a)   (b64_data_bin2ascii[(a)&0x3f])  
	#define b64_conv_ascii2bin(a)   (b64_data_ascii2bin[(a)&0x7f])
	
	static const unsigned char b64_data_bin2ascii[65] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	static const unsigned char b64_data_ascii2bin[128] =  
	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xF0, 0xFF, 0xFF,  
			0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF,  
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xF2, 0xFF, 0x3F, 0x34,  
			0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF,  
			0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  
			0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,  
			0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
			0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,  
			0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31,  
			0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, }; 
	
	static int encode_base64(unsigned char *t, const unsigned char *f, int dlen)	
	{  
		int i, ret = 0;  
		unsigned long l;  
	  
		for (i = dlen; i > 0; i -= 3)  
		{  
			if (i >= 3)  
			{  
				l = (((unsigned long) f[0]) << 16L) | (((unsigned long) f[1]) << 8L) | f[2];  
				*(t++) = b64_conv_bin2ascii(l>>18L);  
				*(t++) = b64_conv_bin2ascii(l>>12L);  
				*(t++) = b64_conv_bin2ascii(l>> 6L);  
				*(t++) = b64_conv_bin2ascii(l );  
			}  
			else  
			{  
				l = ((unsigned long) f[0]) << 16L;	
				if (i == 2)  
					l |= ((unsigned long) f[1] << 8L);	
	  
				*(t++) = b64_conv_bin2ascii(l>>18L);  
				*(t++) = b64_conv_bin2ascii(l>>12L);  
				*(t++) = (i == 1) ? '=' : b64_conv_bin2ascii(l>> 6L);  
				*(t++) = '=';  
			}  
			ret += 4;  
			f += 3;  
		}  
	  
		*t = '\0';	
		return (ret);  
	} 

	void generate_PasswordDigest(char *randomnumber, const char *pwd, char *datetime, char *PwdDigest)
	{	
		SHA256_CTX ctx;
		unsigned char digest[SHA256_DIGEST_LENGTH];
		char inputStr[128] = {0};

		snprintf(inputStr, sizeof(inputStr), "%s%s%s", randomnumber, datetime, pwd);
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, inputStr, strlen(inputStr));
		SHA256_Final(digest, &ctx);
		encode_base64((unsigned char *)PwdDigest, (unsigned char *)digest, SHA256_DIGEST_LENGTH);
		
	}
	
	
	void ZJYD_set_http_headers(std::vector<std::string>* vhead, string& userName, string& passwd)
	{
	
		char wsseValue[1024] = {0};
		char nonce[25] = {0};
		char pwdDigest[128] = {0};
	
		time_t timep;
		struct tm *p = NULL;
		char datetime[64] = {0};
		time(&timep);
		p = gmtime(&timep);
		strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H:%M:%SZ", p);
		random25(nonce);	
		generate_PasswordDigest(nonce, passwd.c_str(), datetime, pwdDigest);
		snprintf(wsseValue, sizeof(wsseValue), "X-WSSE:UsernameToken Username=\"%s\",PasswordDigest=\"%s\",Nonce=\"%s\",Created=\"%s\"", 
		userName.c_str(), pwdDigest, nonce, datetime);
		string wsse(wsseValue);
		//vhead->push_back("Host:aep.sdp.com");
		vhead->push_back("Authorization:WSSE realm=\"SDP\",profile=\"UsernameToken\",type=\"AppKey\"");
		vhead->push_back(wsse);
		vhead->push_back("Content-Type:application/json;charset=UTF-8");
	
	
	}

	void splitExVector(std::string& strData, std::string strDime, std::vector<std::string>& vecSet)
	{
		int strDimeLen = strDime.size();
		int lastPosition = 0, index = -1;
		while (-1 != (index = strData.find(strDime, lastPosition)))
		{
			vecSet.push_back(strData.substr(lastPosition, index - lastPosition));
			lastPosition = index + strDimeLen;
		}
		string lastString = strData.substr(lastPosition);
		if (!lastString.empty())
		{
			vecSet.push_back(lastString);
		}
	}

    void ZJYDChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool ZJYDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	    /*{
		"code": "000000",
		"description": "SUCCESS",
		"result": {
			"status": "inprocess",                 succeed：成功发送。failed：发送失败。inprocess：处理
			"smsMsgId": "000002041105201708110516512006"
		}

		"code": "604",
		"description": " AppKey无效或AppKey校验失败",
		"result": {
			"status": "failed"
		}
}
		}*/
        try
        {
			Json::Reader reader(Json::Features::strictMode());
            std::string js;
			Json::Value root;
			Json::Value obj;
            if (Json::json_format_string(respone, js) < 0)
            {
				return false;
            }
            if(!reader.parse(js,root))
            {
				return false;
            }
			
			obj = root["result"];
			status = obj["status"].asString();
			if (root["description"].asString() == "SUCCESS")
			{
				smsid = obj["smsMsgId"].asString();
				status = "0";
			}
			else
			{
				status = root["code"].asString();
				strReason.append(status);
				strReason.append("|");
				strReason.append(root["description"].asString());
			}
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool ZJYDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
	/*		"SmsMsgId": "000002041102201708110714319000",
					"Status": "succeed",   failed
					"sendMode": "01"   01：通过闪信下发        02：通过USSD下发       03：USSD转闪信下发 
*/
		try
        {   
            if (respone.length() < 10)
            {
                return false;
            }
               
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
			smsp::StateReport report;
			report._smsId = root["SmsMsgId"].asString();
			report._desc = root["Status"].asString();
			if (root["Status"].asString() == "succeed")
			{
				report._status = SMS_STATUS_CONFIRM_SUCCESS;
			}
			else
			{
				report._status = SMS_STATUS_CONFIRM_FAIL;
			}
			report._reportTime = time(NULL);
			reportRes.push_back(report);
			strResponse = "SUCCESS";
            return true;
		}			                
        catch(...)
        {
            return false;
        }
    }

  void split_Ex_Map1(std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet)
    {
        std::vector<std::string> pair;
        int strDimeLen = strDime.size();
        int lastPosition = 0, index = -1;
        while (-1 != (index = strData.find(strDime, lastPosition)))
        {
            pair.push_back(strData.substr(lastPosition, index - lastPosition));
            lastPosition = index + strDimeLen;
        }
        string lastString = strData.substr(lastPosition);
        if (!lastString.empty())
        {
            pair.push_back(lastString);
        }

        std::string::size_type pos = 0;
        for (std::vector<std::string>::iterator ite = pair.begin(); ite != pair.end(); ite++)
        {
            pos = (*ite).find(':');
            if (pos != std::string::npos)
            {
                mapSet[(*ite).substr(0, pos)] = (*ite).substr(pos + 1);
            }
            else
            {
                //cout<<"error";
            }
        }
        return;
    }


    int ZJYDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		if(sms == NULL || url == NULL || data == NULL || vhead == NULL)
		{
			return -1;
		}
		/*{ { "from": "106575261108014",  "to": "%phone%", "smsTemplateId": "%templateID%", "paramValue":{ %param% }, 
		"notifyUrl": "%notifyUrl%","toType": "%toType%"}
		{
	    "from": "106575261108014",
	    "to": "8613543255823",
	    "smsTemplateId": "6cf8a548-6b9c-430c-9c69-258736703c28",
	    "paramValue": {
	        "var1": "aa",
	        "var2": "a  a"
	    },
	    "notifyUrl": "http://202.105.136.106:8989/ZJYDreport.do",
	    "toType": "1"
		}
		*/
		
		std::map<std::string,std::string> postDate;
		
        split_Ex_Map1(*data, ",", postDate);
		
		string userName = postDate["{userName"];
		string passwd = postDate["passwd"];

		ZJYD_set_http_headers(vhead, userName, passwd);
		
		int pos = data->find("from");
		if (pos != std::string::npos)
        {
			data->replace(1, pos - 2, "");
		}

		pos = data->find("%templateID%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%templateID%"), sms->m_strChannelTemplateId);
		}
		pos = data->find("%phone%");
		if (pos != std::string::npos)
        {
        	string temp("86");
			temp.append(sms->m_strPhone);
			data->replace(pos, strlen("%phone%"), temp);
		}
		pos = data->find("%toType%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%toType%"), "1");
		}

		int count = 1;
		std::vector<std::string> vecParam;
		splitExVector(sms->m_strTemplateParam, ";", vecParam);
		string tmpParam;
		std::vector<std::string>::iterator itr1= vecParam.begin();
		while(itr1 != vecParam.end())
		{
			tmpParam.append("\"var");
			tmpParam.append(1, count++ + 48);
			tmpParam.append("\"");
			tmpParam.append(": ");
			tmpParam.append("\"");
			tmpParam.append(itr1->data());
			tmpParam.append("\"");
			tmpParam.append(",");
			itr1 ++;
		}
		if(true != tmpParam.empty())
		{
			tmpParam = tmpParam.substr(0,tmpParam.length() - 1);
		}
		pos = data->find("%param%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%param%"), tmpParam);
		}		
		
        return 0;
	}
} /* namespace smsp */
