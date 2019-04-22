#include "DCGChannellib.h"
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
            return new DCGChannellib;
        }
    }
    
    DCGChannellib::DCGChannellib()
    {
    }
    DCGChannellib::~DCGChannellib()
    {

    }

    void DCGChannellib::test()
    {
    }

    bool DCGChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
    	/*********************************************************************************************
    		respone:
		    		{
					    "code": 0,
					    "msg": "发送成功！",
					    "data": {
					        "task_id": "625",
					        "count": 1,
					        "failed": []
					    }
					}

					{
					    "code": 1108,
					    "msg": "",
					    "data": {
					        "task_id": "0",
					        "failed": [
					            "136111122223",
					            "aaaaa"
					        ]
					    }
					}

					{
					    "code": 1001,
					    "msg": "invalid timestamp,please check your system time.",
					    "data": ""
					}
    	**********************************************************************************************/
        try
        {
			Json::Reader reader;  
    		Json::Value root;
			if(reader.parse(respone, root))
			{
				if(root["code"].asInt() == 0)
				{
					status = "0";
					strReason = root["msg"].asString();
					smsid = root["data"]["task_id"].asString();
					
					return true;
				}
				else
				{
					status = "-1";
					strReason = root["msg"].asString();
					smsid = "0";

					return false;
				}	
			}
			else
			{
				//std::cout<<"parseSend Json parse fail!"<<endl;
				return false;
			}
        }
        catch(...)
        {
        	//std::cout<<"parseSend Json parse exception!"<<endl;
            return false;
        }
    }

    bool DCGChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/***************************************************************************************************************
			状态报告:
					{
					    "code": 0,
					    "msg": "",
					    "data": [
					        {
					            "task_id": "625",
					            "phone_no": "13543255823",
					            "receive_code": "99",
					            "receive_time": null,
					            "sub_port": "1234",
					            "sms_id": "123456789012345"
					        }
					    ]
					}
			待定：
					{
					    "code": 1001,
					    "msg": "invalid timestamp,please check your system time.",
					    "data": ""
					}

			上行：
					{
					    "code": 0,
					    "msg": "",
					    "data": [
					        {
					            "sub_port": "501",
					            "phone_no": "13543255823",
					            "msg": "淘达人敌人上行",
					            "reply_time": "1504591389"
					        }
					    ]
					}

			空：
					{
					    "code": 0,
					    "msg": "",
					    "data": []
					}
		****************************************************************************************************************/
        try
        {
			Json::Reader reader;  
    		Json::Value root;
			int type = MAYBE_ERROR;
			//std::cout<<respone<<" len:"<<respone.length()<<std::endl;
			if(respone.find("task_id") != std::string::npos)
			{
				type = MAYBE_REPORT;
			}
			else if(respone.find("sub_port") != std::string::npos)
			{
				type = MAYBE_MO;
			}
			else if(respone.length() == 31)
			{
				type = MAYBE_NULL;
			}
			else
			{
				type = MAYBE_ERROR;
			}
			
			if(reader.parse(respone, root))
			{
				if(root["code"].asInt() == 0)
				{
					if(type == MAYBE_REPORT)
					{
						for(int i=0; i<root["data"].size(); i++)
						{
							smsp::StateReport report;
							
							report._smsId = root["data"][i]["task_id"].asString();
							if(root["data"][i]["receive_code"].asString() == "0")
							{
								report._status = SMS_STATUS_CONFIRM_SUCCESS;
								report._desc   = "DELIVRD";
							}
							else
							{
								report._status = SMS_STATUS_CONFIRM_FAIL;
							}
							report._desc   = root["data"][i]["receive_code"].asString();
							report._reportTime = time(NULL);

							reportRes.push_back(report);
						}
					}
					else if(type == MAYBE_MO)
					{
						for(int i=0; i<root["data"].size(); i++)
						{
							smsp::UpstreamSms mo;
							
							mo._phone 		 = root["data"][i]["phone_no"].asString();
							mo._content 	 = root["data"][i]["msg"].asString();
							mo._upsmsTimeStr = root["data"][i]["reply_time"].asString();
							mo._upsmsTimeInt = strtoul(mo._upsmsTimeStr.data(), NULL, 0);
							mo._appendid 	 = root["data"][i]["sub_port"].asString();
							
							upsmsRes.push_back(mo);
						}
					}
					else if(type == MAYBE_NULL)
					{
						//std::cout<<"MAYBE_NULL"<<std::endl;
						return true;
					}
					else
					{
						//std::cout<<"MAYBE_ERROR"<<std::endl;
						return false;
					}
					
					strResponse = "SUCCESS";
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				//std::cout<<"parseReport Json parse fail!"<<endl;
				return false;
			}
        }
        catch(...)
        {
        	//std::cout<<"parseReport Json parse exception!"<<endl;
            return false;
        }
    }

    int DCGChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	//std::cout<<"old data: "<<*data<<std::endl;
		
    	std::map<string, string> mapData;
		splitExMap(*data, "&", mapData);
		
		time_t t = time(0);
		char ctime[32] = {0};
		snprintf(ctime, sizeof(ctime), "%lu", (uint64_t)t);
		string strtimestamp = ctime;
		string key = mapData["secret"] + mapData["app_key"] + strtimestamp;

		string check;
		unsigned char md5[16] = {0};
		MD5((const unsigned char*)key.data(), key.length(), md5);

		std::string HEX_CHARS = "0123456789abcdef";
        for (int i = 0; i < 16; i++)
        {
            check.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            check.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }

		//cout<<"timestamp:"<<strtimestamp<<std::endl;
		//cout<<"key:"<<key<<std::endl;
		//cout<<"check:"<<check<<std::endl;
		
		string_replace(*data, "&secret=" + mapData["secret"], "");
		string_replace(*data, "%timestamp%", strtimestamp);
		string_replace(*data, "%check%", check);

		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		
		//std::cout<<"new data: "<<* data<<std::endl;
        return 0;
	}

	void DCGChannellib::splitExMap(std::string& strData, std::string strDime, std::map<std::string,std::string>& mapSet)
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

	        pos = (*ite).find('=');
	        if (pos != std::string::npos)
	        {
	            mapSet[(*ite).substr(0, pos)] = (*ite).substr(pos + 1);
	        }
	        else
	        {
	            //LogCrit("parse split_Ex param err");
	        }
	    }
	    return;
	}

	void DCGChannellib::string_replace(string&s1, const string&s2, const string&s3)
	{
		string::size_type pos=0;
		string::size_type a=s2.size();
		string::size_type b=s3.size();
		while((pos=s1.find(s2,pos))!=string::npos)
		{
			s1.replace(pos,a,s3);
			pos+=b;
		}
	}
} /* namespace smsp */
