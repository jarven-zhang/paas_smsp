#include "MASChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
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
            return new MASChannellib;
        }
    }
    
    MASChannellib::MASChannellib()
    {
    }
    MASChannellib::~MASChannellib()
    {

    }

    void MASChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool MASChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
		/***************************************
		{"code":0,"desc":"成功","siSmsId":"593809a2f83e4c0fbe851e4e5fd5931e"}
		{"code":1,"desc":"参数不完全,operatorId"}
		****************************************/
	   
        try
        {
			//cout<<"-----------parseSend-----------respone="<<respone<<endl;

			int pos1 = respone.find(":");
			int pos2 = respone.find(",");
			//cout << pos1<<" "<<pos2<<endl;
			string code = respone.substr(pos1 + 1, pos2 - pos1 -1);

			string data = "{";
			data.append(respone.substr(pos2 + 1));
			
			Json::Reader reader(Json::Features::strictMode());
            std::string js;
			Json::Value root;
			Json::Value obj;
            if (Json::json_format_string(data, js) < 0)
            {
				return false;
            }
            if(!reader.parse(js,root))
            {
				return false;
            }
			
			std::string desc = root["desc"].asString();
			
			//cout<<"parseSend----desc:"<<desc<<"code="<<code<<endl;
								
			if (code == "0")
			{
				status = "0";
				smsid = root["siSmsId"].asString();
				//cout<<"send success"<<endl;
			}
			else
			{
				status = "4";
				strReason.append(code);
				//cout<<"strReason="<<strReason<<endl;				
			}
				
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool MASChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/****************************************************************
		report:
		type=0&status=
		[{"channelId":22,"creatorId":1357,"flag":"success","message":"成功","mobile":"13570424044","reportTime":"2017-09-25 12:52:45",
		"responseTime":"2017-09-25 12:24:04","smsId":"9cfb65a6e50e4d88b6f66d666c12ddc7","status":"DELIVRD"}
		,{"channelId":1, "creatorId":1,"flag":"failure","message":"失败",,"mobile":"13613333333","reportTime":"2012-08-02 10:51:55",
		"responseTime":"2012-08-02 10:51:55","smsId":"d31a77ef-faaa-49ff-b89a-8f7e1503fade","status":"1"}]

		mo:
		[{"applicationId":"_sys10","channelId":1, "creatorId":1,"extCode":"0410","message":"这是回复内容","mobile":"1369999999",
		"destMobile":"09997010040","msgId":"4b4b44e9-4217-4a6d-bdf5-81b196cf863d","receiveTime":"2012-08-03 12:33:23"}
		,{"applicationId":"_sys11","channelId":1, "creatorId":1,"extCode":"0508","message":"这是回复内容","mobile":"1385555555",
		"destMobile":"09997010040","msgId":"0989903e-57d4-45b9-9b3d-11cea907ff4a","receiveTime":"2012-08-03 14:22:08"}]

		/*****************************************************************/
		try
        {   
            if (respone.length() < 10)
            {
                return false;
            }

			string type = respone.substr(5, 1);
			if (type != "1" && type != "0")
			{
				//cout<<"err,type = "<<type<<endl;
				return false;
			}
			//cout<<"[parseReport],type="<<type<<endl;
			string data = "{\"type\":\"0\",\"status\":";
			data.append(respone.substr(14));
			data.append("}");

			Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
			Json::Value obj;
            std::string js;
            if (Json::json_format_string(data, js) < 0)
            {
                return false;
            }
            if(!reader.parse(js,root))
            {
                return false;
            }
			smsp::UpstreamSms Upstream;
			smsp::StateReport report;

			//cout<<"parseReport====================revert data:"<<data<<endl;
			obj = root["status"];
			//cout<<"root.size()="<<root.size()<<"obj.size()="<<obj.size()<<endl;
			if (type == "0")//report
			{
				for (int i = 0; i < obj.size(); i++)
				{
					report._smsId = obj[i]["smsId"].asString();
					report._desc = obj[i]["status"].asString();
					report._phone = obj[i]["mobile"].asString();
					if (obj[i]["flag"].asString() == "success")
					{
						report._status = SMS_STATUS_CONFIRM_SUCCESS;
					}
					else
					{
						report._status = SMS_STATUS_CONFIRM_FAIL;
					}
					report._reportTime = time(NULL);
					reportRes.push_back(report);
					//cout<<"_smsId="<<report._smsId<<"_desc="<<report._desc<<"mobile="<<report._phone<<endl;
				}
			}
			else if (type == "1")//mo
			{
				//cout<<"this is mo"<<endl;
				for (int i = 0; i < obj.size(); i++)
				{
					Upstream._phone = obj[i]["mobile"].asString();
					string strAppendid = obj[i]["destMobile"].asString();
					Upstream._appendid = strAppendid.substr(4);
					Upstream._content = obj[i]["message"].asString();
					Upstream._upsmsTimeInt = time(NULL);
					upsmsRes.push_back(Upstream);
					//cout<<"mobile="<<Upstream._phone<<"_appendid="<<Upstream._appendid<<"_content="<<Upstream._content<<endl;
				}
			}

			strResponse = "ok";
            return true;
		}			                
        catch(...)
        {
            return false;
        }
    }

    int MASChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
/*
operatorId=CZqwyd01&password=sd55k29&mobiles=%phone%&extendCode=%displaynum%&message=%content%&sendMethod=0&needDelivery=1&channelId=%channelId%&sendLevel=0

*/
		if(sms == NULL || url == NULL || data == NULL || vhead == NULL)
		{
			return -1;
		}
		
		int pos = data->find("%channelId%");
		if (pos != std::string::npos)
        {
			data->replace(pos, strlen("%channelId%"), "22");//now is 22.  later according to content select channel.
		}
		
		vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
		//cout<<"-------adaptEachChannel------------data="<<*data<<endl;
		
        return 0;
	}

	
} /* namespace smsp */
