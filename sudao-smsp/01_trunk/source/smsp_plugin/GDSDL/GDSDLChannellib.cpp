#include "GDSDLChannellib.h"
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
            return new GDSDLChannellib;
        }
    }
    
    GDSDLChannellib::GDSDLChannellib()
    {
    }
    GDSDLChannellib::~GDSDLChannellib()
    {

    }

    void GDSDLChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }
		///status == "0" --->ok
    bool GDSDLChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
    	///result=1000&description=发送短信成功&smsId=170829155825336553
        try
        {
        	std::cout<<"respone is :"<<respone.data()<<std::endl;
        	int pos1 = respone.find("result=",0);
			int pos2 = respone.find('&',pos1);
			string result;
			if(pos1 != string::npos)
			{
				result = respone.substr(7,pos2 - (pos1 + 7));
				if(0 == result.compare("1000"))
				{
					status = "0";
				}
			}
			pos1 = respone.find("description=",pos2);
			pos1 = respone.find('&',pos1);
			if(pos2 != string::npos && pos1 != string::npos)
			{
				strReason = respone.substr(pos1 + 12,pos2 - (pos1 + 7));
			}
			pos1 = respone.find("smsId=",pos2);
			pos2 = respone.length();
			if(pos2 != string::npos && pos1 != string::npos)
			{
				smsid = respone.substr(pos1 + 6,pos2 - (pos1 + 6));
			}
			std::cout<<"status:"<<status.data()<<"strReason:"<<strReason.data()<<"smsid:"<<smsid.data()<<std::endl;
			return true;	
        }
        catch(...)
        {
            return false;
        }
    }

    bool GDSDLChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {		
		//result=1000&reports=170829161526340540,18682367823,0,DELIVRD,20170829161647;170829161526340540,13543255823,1,UNDELIV,20170829161647;
		//result=1000&confirm_time=2017-08-29 16:33:15&id=123157&replys=106906857763476666,13543255823,你好,2017-08-29 16:33:06,;
        try
        {   
        	std::cout<<"respone is :"<<respone.data()<<std::endl;
          	strResponse = "SUCCESS";
			int pos1 = respone.find("result=",0);
			int pos2 = respone.find('&',0);
			if(pos1 == string::npos || pos2 == string::npos)
			{
				return false;
			}
			string result = respone.substr(7,(pos2 - pos1 - 7));
			if(0 != result.compare("1000"))
			{
				return false;
			}
			if( string::npos != respone.find("reports=",0))
			{
				cout<<"parse report!"<<std::endl;
				int pos = respone.find(';',0);
				pos1 = respone.find("reports=",0);
				pos1 = pos1 + 8;
				for(;pos != string::npos;pos = respone.find(';',pos + 1) )
				{	
					pos2 = respone.find(',',pos1);
					smsp::StateReport report;
					report._smsId = respone.substr(pos1,pos2 - pos1);
					pos1 = pos1 + report._smsId.length() + 1;
					pos2 = respone.find(',',pos1);
					report._phone = respone.substr(pos1,pos2 - pos1);
					pos1 = pos1 + report._phone.length() + 1;
					pos2 = respone.find(',',pos1);
					string tmpStatus;
					tmpStatus = respone.substr(pos1,pos2 - pos1);
					if(0 == tmpStatus.compare("0"))
					{
						report._status = SMS_STATUS_CONFIRM_SUCCESS;
					}
					else
					{
						report._status = SMS_STATUS_CONFIRM_FAIL;
					}
					pos1 = pos1 + tmpStatus.length() + 1;
					pos2 = respone.find(',',pos1);
					report._desc = respone.substr(pos1,pos2 - pos1);
					report._reportTime = time(NULL);
					reportRes.push_back(report);
					pos1 = pos + 1;
					cout<<"_smsId:"<<report._smsId.data()<<" phone:"<<report._phone.data()<<" state:"<<tmpStatus.data()<<" desc:"<<report._desc.data()<<std::endl;
				}	
			}
			else if(string::npos != respone.find("confirm_time=",0))
			{
				//result=1000&confirm_time=2017-08-29 16:33:15&id=123157&replys=106906857763476666,13543255823,你好,2017-08-29 16:33:06,;
				cout<<"parse mo!"<<std::endl;
				int pos = respone.find(';',0);
				pos1 = respone.find("replys=",0);
				pos1 = pos1 + 7;
				for(;pos != string::npos;pos = respone.find(';' ,pos + 1))
				{
					pos2 = respone.find(',',pos1);
					smsp::UpstreamSms mo;
					string spNum;
					spNum = respone.substr(pos1,pos2 - pos1);
					mo._appendid = spNum.substr(14,spNum.length() - 14);
					pos1 = pos2 + 1;
					pos2 = respone.find(',',pos1);
					mo._phone = respone.substr(pos1,pos2 - pos1);
					pos1 = pos2 + 1;
					pos2 = respone.find(',',pos1);
					mo._content = respone.substr(pos1,pos2 - pos1);
					pos1 = pos2 + 1;
					pos2 = respone.find(',',pos1);
					mo._upsmsTimeStr = respone.substr(pos1,pos2 - pos1);
					mo._upsmsTimeInt = time(NULL);
					upsmsRes.push_back(mo);
					pos1 = pos + 1;
					cout<<"extern:"<<mo._appendid.data()<<"phone:"<<mo._phone.data()<<"content:"<<mo._content.data()<<std::endl;
				}

			}
			else
			{
				return false;
			}
			
            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    int GDSDLChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
    	if(vhead == NULL)
    	{
			return -1;
		}
    	//mylib->adaptEachChannel(pSms, &urltmp, &postData, &mv);	
    	//std::vector<std::string> mv;
    	string contentType = "Content-Type: application/x-www-form-urlencoded";
		vhead->push_back(contentType);
        return 0;
	}
} /* namespace smsp */
