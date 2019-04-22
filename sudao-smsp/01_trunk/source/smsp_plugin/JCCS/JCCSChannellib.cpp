#include "JCCSChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "xml.h"
#include "HttpParams.h"

namespace smsp
{

    JCCSChannellib::JCCSChannellib()
    {
    }
    JCCSChannellib::~JCCSChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new JCCSChannellib;
        }
    }

    void JCCSChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool JCCSChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        // respone = {"code":0,"msg":"OK","result":{"taskId":"5061258999149855087"}}

        //respone = "{\"code\":0,\"msg\":\"OK\",\"result\":{{\"taskId\":\"5061258999149855087\"}}";

        //respone= "{\"code\":0,\"msg\":\"LACK PARAMETERS\",\"result\":{}}";


        try
        {
        	std::string::size_type pos=respone.find(",");
            if(respone[0]=='0')
            {
			smsid=respone.substr(pos+1);
					
            		//respone.copy(smsid,16,2);
			status="0";
			return true;
			
            }
	     else
	     	{
			smsid=respone.substr(pos+1);
			
	     		//respone.copy(smsid,16,2);
			status="-1";
			return false;
	     	}
        }
        catch(...)
        {
            return false;
        }
    }

    bool JCCSChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse)
    {
        //rpturl?data=05201617404137,13900000001,DELIVRD|05220128211118,13900000002,DELIVRD|05220141501133,13900000003,MK:0010
        strResponse = "0"; //should return "0" to GYchannel
        //LogNotice("fjx: respone[%s]", respone.data());
        try
        {	
          	std::string::size_type pos=respone.find("=");
		std::string url=respone.substr(0,pos);//将=前面的字符串截出
		std::string temp=respone.substr(pos+1);//将=后面的部分截出
		if(url.compare("data")==0)//如果是状态报告
		{	
			temp=temp+"|";
			while(temp.empty()==false)//如果temp不空
			{	
				std::string::size_type xpos=temp.find("|");
				std::string rp=temp.substr(0,xpos);//将|前面部分截出
				temp.erase(0,rp.length()+1);//将截出的状态报告删除
					
				std::string::size_type comma_pos=rp.find(",");
				smsp::StateReport report;
				report._smsId=rp.substr(0,comma_pos);
				rp.erase(0,comma_pos+1);
				
				comma_pos=rp.find(",");
				report._phone=rp.substr(0,comma_pos);
				rp.erase(0,comma_pos+1);

				report._desc=rp;
				if(rp.compare("DELIVRD")==0)
				{
					report._status=SMS_STATUS_CONFIRM_SUCCESS;
				}
				else
				{
					report._status=SMS_STATUS_CONFIRM_FAIL;
				}
				reportRes.push_back(report);

			} 
			return true;
		}
		else if(url.compare("mb")==0)
		{	
			std::string::size_type and_pos=temp.find("&");
			smsp::UpstreamSms upsms;
			upsms._phone=temp.substr(0,and_pos);
			temp.erase(0,and_pos+4);
			
			and_pos=temp.find("&");
			upsms._appendid=temp.substr(0,and_pos);
			temp.erase(0,and_pos+4);

			upsms._content=temp;

			upsms._upsmsTimeInt= time(NULL);        

                	struct tm tblock = {0};
                	time_t timer = time(NULL);
                	localtime_r(&timer,&tblock);
                	upsms._upsmsTimeStr = asctime(&tblock);  

			upsmsRes.push_back(upsms);
			
			return true;
			
		}
		else
		{
			return false;
		}
        }
        catch(...)
        {
            //LogWarn("[GYChannellib::parseReport] is failed.");
            return false;
        }
    }


    int JCCSChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }




} /* namespace smsp */
