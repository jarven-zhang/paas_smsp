#include "TXYChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include <map>
#include <stdlib.h>





namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new TXYChannellib;
        }
    }
    
    TXYChannellib::TXYChannellib()
    {
    }
    TXYChannellib::~TXYChannellib()
    {

    }

    void TXYChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

	unsigned char FromHex(unsigned char x)
    {
        unsigned char y=0;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else
        {
            //LogCrit("FromHex Err");
        }
        return y;
    }
	
	std::string urlDecode(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (str[i] == '+') strTemp += ' ';
			else if (str[i] == '%')
			{
//			  if(i + 2 < length){
//				LogCrit("UrlDecode Err");
//				return "";
//			  }
				unsigned char high = FromHex((unsigned char)str[++i]);
				unsigned char low = FromHex((unsigned char)str[++i]);
				strTemp += high*16 + low;
			}
			else strTemp += str[i];
		}
		return strTemp;
	}
#if 1
    bool TXYChannellib::parseSendMutiResponse(std::string respone, smsp::mapStateResponse &responses, std::string& strReason )
	{

	try
        {
			cout<<"resm="<<respone<<endl;
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
			
			std::string msg = root["msg"].asString();
			std::string cd = root["code"].asString();
			cout<<"parseSendm----msg:"<<msg<<"code="<<cd<<endl;
			obj = root["result"];
			cout<<"size="<<obj.size()<<endl;	
			for (int i = 0; i < obj.size(); i++)    
			{
				smsp::StateResponse res;
				res._desc = "";
				res._phone = obj[i]["mobile"].asString();
				res._smsId = obj[i]["sid"].asString();	
				string status = obj[i]["code"].asString(); 
				

				cout<<"res._smsId="<<res._smsId<<"status="<<status<<endl;
								
				if (status == "0")
				{
					res._desc = "0";
					res._status = 0;
					cout<<"send success"<<endl;
				}
				else
				{
					res._status = 4;
					res._desc.append(obj[i]["code"].asString());
					res._desc.append("|");
					res._desc.append(obj[i]["detail"].asString());
					strReason = res._desc;
					cout<<"strReason="<<strReason<<endl;				
				}
				responses[res._phone] = res;
      			
			}
            return true;
        }
        catch(...)
        {
            return false;
        }

	}
#endif	

    bool TXYChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
    /***************************************
{
		"code": "0", 
		"msg": "提交成功！", 
		"result": [
			{
				"sid": "59106212281753716884", 
				"mobile": "19000000000", 
				"code": "29", 
				"msg": "提交的下行手机号码不符合规范.",
				"detail":"参数 mobile 格式不正确， 19000000000不是合法手机号格式", 
				"fee": 0
			}, 
			{
				"sid": "59106212281753716885",
				"dsisplay_number": "10690330402", 
				"mobile": "18665966245", 
				"code": "0", 
				"msg": "提交成功.", 
				"fee": 1
			}
		]

}
****************************************/
	   
        try
        {
			cout<<"res="<<respone<<endl;
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
			
			std::string msg = root["msg"].asString();
			std::string cd = root["code"].asString();
			cout<<"parseSend----msg:"<<msg<<"code="<<cd<<endl;
			obj = root["result"];
			
			//for (int i = 0; i < obj.size(); i++)    
				int i = 0;

				cout<<"size="<<obj.size()<<endl;		
	
				smsid = obj[i]["sid"].asString();	
				status = obj[i]["code"].asString(); 

				cout<<"sid="<<smsid<<"status="<<status<<endl;
								
				if (status == "0")
				{
					//status = "0";
					cout<<"send success"<<endl;
				}
				else
				{
					status = "4";
					strReason.append(status);
					strReason.append("|");
					strReason.append(obj[i]["detail"].asString());
					cout<<"strReason="<<strReason<<endl;				
				}
      
			
            return true;
        }
        catch(...)
        {
            return false;
        }
    }


    bool TXYChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {

		/*************
report:		
[
    {
        "sid": "59106212281734600110", 
		"uid": "1104620500",
        "mobile": "15044214400", 
        "report_status": "FAIL", 
        "desc": "UNDELIVRD",  
        "recv_time": "2016-12-28 17:34:59"
    }, 
    {
        "sid": "59106112281734357705", 
        "uid": "1104620500",
        "mobile": "15093147871", 
        "report_status": "SUCCESS", 
        "desc": "DELIVRD",  
        "recv_time": "2016-12-28 17:34:59"
    }
]
mo:
{
    "account": "123456", 
    "sign": "腾讯科技", 
    "reply_time": "2016-12-28 17:34:59", 
	"content": "可以给我打电话了", 
	"dsisplay_number": "10690330402",
    "extend": "02", 
    "mobile": "18087074369"
}

/*****************/
		cout<<"parseReport====================:"<<respone<<endl;
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
			cout<<"root.size()="<<root.size()<<endl;
			if (root.size() == 7)//mo
			{
				//if (root["dsisplay_number"].asString().empty())
				//	return false;
				smsp::UpstreamSms Upstream;
				Upstream._phone = root["mobile"].asString();
				Upstream._appendid = root["dsisplay_number"].asString();
				Upstream._content = root["content"].asString();
				Upstream._upsmsTimeInt = time(NULL);
				upsmsRes.push_back(Upstream);
				cout<<"mobile="<<Upstream._phone<<"_appendid="<<Upstream._appendid<<"_content="<<Upstream._content<<endl;
			}
			else
			{
				smsp::StateReport report;
				report._smsId = root["sid"].asString();
				report._desc = root["desc"].asString();
				report._phone = root["mobile"].asString();
				if (root["report_status"].asString() == "SUCCESS")
				{
					report._status = SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status = SMS_STATUS_CONFIRM_FAIL;
				}
				report._reportTime = time(NULL);
				reportRes.push_back(report);
				cout<<"_smsId="<<report._smsId<<"_desc="<<report._desc<<endl;
			
			}
			

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


    int TXYChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
       //"clientid": "b00p23","password": "25d55ad283aa400af464c76d713c07ad","mobile": "%phone%","content": "%content%","extend": "%displaynum%","uid": "%msgid%"
		try
		{
			if(sms == NULL || url == NULL || data == NULL || vhead == NULL)
			{
				return -1;
			}
			
			std::map<std::string,std::string> postDate;
			
	        split_Ex_Map1(*data, ",", postDate);
			
			int pos = data->find("%displaynum%");
			if (pos != std::string::npos)
	        {
				data->replace(pos, strlen("%displaynum%"), sms->m_strShowPhone);
			}
		
			pos = data->find("%msgid%");
			if (pos != std::string::npos)
	        {
				data->replace(pos, strlen("%msgid%"), sms->m_strChannelSmsId);
			}
			pos = data->find("%content%");
			if (pos != std::string::npos)
	        {
				data->replace(pos, strlen("%content%"), urlDecode(sms->m_strContent));
			}

			cout<<"-------adaptEachChannel------------data="<<*data<<endl;
			
	        return 0;
     		}
		catch(...)
	   	{
     	
			return -1;
		}

	}
	
} /* namespace smsp */
