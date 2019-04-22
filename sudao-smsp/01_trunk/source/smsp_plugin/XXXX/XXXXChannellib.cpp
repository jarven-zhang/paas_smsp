#include "XXXXChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"
#include <stdlib.h>


namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new XXXXChannellib;
        }
    }
    
    XXXXChannellib::XXXXChannellib()
    {
    }
    XXXXChannellib::~XXXXChannellib()
    {

    }

    void XXXXChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

	
	///mylib->parseSend(strData, pSms->m_strChannelSmsId, state, strReason);
    bool XXXXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	/*
	 <?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
	 <SmsResponse>
		 <batchId>1077</batchId>
		 <errorCode>PartailSuccess</errorCode>
		 <requestId>1234567890</requestId>
		 <status>10</status>
	 </SmsResponse>

	 */
		try
 		{
 			///cout<<"come in parseSend!"<<std::endl;
			respone = smsp::Channellib::urlDecode(respone);
			
			TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {  
            	std::cout<<"XXXXChannellib.cpp:59 parseSend() faild! "<<std::endl;
                return false;
            }			
			TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            { 
            	std::cout<<"XXXXChannellib.cpp:66 parseSend() faild! "<<std::endl;            
                return false;
				
            }
			
			TiXmlElement *batchIdElement = element->FirstChildElement();
			if(NULL==batchIdElement)
            {  
            	std::cout<<"XXXXChannellib.cpp:74 parseSend() faild! "<<std::endl;            
                return false;
            }		
			TiXmlNode *batchIdNode = batchIdElement->FirstChild();
			if(NULL==batchIdNode)
            { 
            	std::cout<<"XXXXChannellib.cpp:81 parseSend() faild! "<<std::endl;            
                return false;
            }
			std::string batchId ;
			batchId = std::string(batchIdNode->Value());
			
			TiXmlElement *errorCodeElement = batchIdElement->NextSiblingElement();
			if(NULL==errorCodeElement)
            {
            	std::cout<<"XXXXChannellib.cpp:97 parseSend() faild! "<<std::endl;            
                return false;
			}
			TiXmlNode *errorCodeNode = errorCodeElement->FirstChild();
			if(NULL==errorCodeNode)
            { 
            	std::cout<<"XXXXChannellib.cpp:109 parseSend() faild! "<<std::endl;            
                return false;
            }			
			std::string errorCode;
			errorCode = std::string(errorCodeNode->Value());
			
			TiXmlElement *requestIdElement = errorCodeElement->NextSiblingElement();
			if(NULL==requestIdElement)
            {
            	std::cout<<"XXXXChannellib.cpp:114 parseSend() faild! "<<std::endl;            
                return false;
			}
			TiXmlNode *requestIdNode = requestIdElement->FirstChild();
			if(NULL==requestIdNode)
            { 
            	std::cout<<"XXXXChannellib.cpp:120 parseSend() faild! "<<std::endl;            
                return false;
            }			
			std::string requestId;
			requestId = std::string(requestIdNode->Value());

			TiXmlElement *statusElement = requestIdElement->NextSiblingElement();
			if(NULL==statusElement)
            {
            	std::cout<<"XXXXChannellib.cpp:114 parseSend() faild! "<<std::endl;            
                return false;
			}
			TiXmlNode *statusNode = statusElement->FirstChild();
			if(NULL==statusNode)
            { 
            	std::cout<<"XXXXChannellib.cpp:120 parseSend() faild! "<<std::endl;            
                return false;
            }			
			std::string strStatus;
			strStatus = std::string(statusNode->Value());
			
			smsid = batchId;
			if(strStatus.compare("10") == 0)
				status = "0";
			else
				status = strStatus;
			strReason = errorCode;
			/////cout<<"smsid ="<<smsid.data()<<"  status ="<<status.data()<<"   strReason ="<<strReason.data()<<std::endl;
			return true	;	
 		}
        catch(...)
        {
           std::cout<<"XYCChannellib.cpp:127 parseSend() faild! "<<std::endl;        
           return false;
        }
    }

    bool XXXXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/*
		http://url?username=test &taskid=1234&status=DELIVRD,1&phones=13911111111,18911111111
		说明
		username ：    用户帐号
		taskid		：流水号(与发送时网关返回的批号对应)
		status	  : 状态报告 (数目与电话数一样，用英文半角逗号分隔)，成功状态为DELIVRD
		phones	  ：	发送号码(数目与状态报告数一样，用英文半角逗号分隔)

		http://url?username=test&msg=text&phone=13911111111&port=110&uptime=2008-01-01 12:00:00
		说明
		username ：用户帐号
		msg ：发送的短信内容（utf-8编码）
		phone: 发送号码
		port:扩展号
		uptime： 上行时间( 格式1900-01-01 00:00:00)
		*/
		try
		{
			/////cout<<"come in parseReport funcyion!"<<std::endl;
            strResponse.assign("0");
            smsp::StateReport report;
            smsp::UpstreamSms upsms;		
			respone = smsp::Channellib::urlDecode(respone);

			std::string strTemp = respone;
			std::string::size_type pos1 = strTemp.find('&',0);
			if(pos1 == std::string::npos)
			{
				cout<<"XXXXChannellib.cpp:167 parseSend() faild! "<<std::endl;
				return false;
			}
			std::string::size_type pos2 = strTemp.find('&',pos1 + 1);
			if(pos2 == std::string::npos)
			{
				cout<<"XXXXChannellib.cpp:173 parseSend() faild! "<<std::endl;
				return false;
			}			
			std::string::size_type pos3 = strTemp.find('&',pos2 + 1);
			if(pos3 == std::string::npos)
			{
				cout<<"XXXXChannellib.cpp:179 parseSend() faild! "<<std::endl;
				return false;
			}			
			std::string::size_type pos4 = strTemp.find('&',pos3 + 1);
			std::string::size_type pos5 = strTemp.find('&',pos4 + 1);

			//is report			
			if(pos4 == std::string::npos)
			{
				////cout<<"come in report"<<std::endl;
				std::string taskid;
				std::string status;
				std::string phone;
				std::string::size_type taskidLen;
				std::string::size_type statusLen;
				std::string::size_type phoneLen;
				taskidLen = pos2 - pos1 - 1;
				statusLen = pos3 - pos2 - 1;
				phoneLen = strTemp.length()  - pos3 - 1;
				
				taskid.assign(strTemp,pos1 + 1,taskidLen);
				////cout<<"strTaskid is : "<<taskid.data()<<std::endl;
				std::string::size_type pos = taskid.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:206 parseSend() faild! "<<std::endl;
					return false;
				}
				taskid.assign(taskid,pos + 1,taskid.length() - pos - 1);
				////cout<<"taskid is :"<<taskid.data()<<std::endl;
				
				status.assign(strTemp,pos2 + 1,statusLen);
				////cout<<"strStatus is : "<<status.data()<<std::endl;
				pos = status.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:218 parseSend() faild! "<<std::endl;
					return false;
				}
				status.assign(status,pos + 1,status.length() - pos - 1);
				////cout<<"status is :"<<status.data()<<std::endl;
				
				phone.assign(strTemp,pos3 + 1,phoneLen);
				////cout<<"strPhone is : "<<phone.data()<<std::endl;
				pos = phone.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:229 parseSend() faild! "<<std::endl;
					return false;
				}
				phone.assign(phone,pos + 1,phone.length() - pos - 1);
				////cout<<"phone is :"<<phone.data()<<std::endl;
				
				report._smsId = taskid;
				int iStatus ;
                if(!status.compare("DELIVRD"))	//reportResp confirm suc
                {
                   	iStatus = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else	//reportResp confirm failed
                {
                    iStatus = SMS_STATUS_CONFIRM_FAIL;
                }								
                report._status = iStatus;			
                report._desc = status;
				report._phone = phone;
                report._reportTime = time(NULL);				
                reportRes.push_back(report);
				return true;
			}	
			//is mo
			else
			{
				////cout<<"here is mo"<<std::endl;
				std::string msg;
				std::string phone;
				std::string port;
				std::string uptime;
				std::string::size_type msgLen;
				std::string::size_type phoneLen;
				std::string::size_type portLen;
				std::string::size_type uptimeLen;
				msgLen = pos2 - pos1 -1;
				phoneLen = pos3 - pos2 -1;
				portLen = pos4 - pos3 -1;
				if(pos5 == std::string::npos)
					uptimeLen =strTemp.length() - pos4 -1;
				else
				{
					uptimeLen = pos5 - pos4 - 1;
				}
				msg.assign(strTemp,pos1 + 1,msgLen);
				////cout<<"strMsg is : "<<msg.data()<<std::endl;
				std::string::size_type pos = msg.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:271 parseSend() faild! "<<std::endl;
					return false;
				}
				msg.assign(msg,pos + 1,msg.length() - pos - 1);
				////cout<<"msg is :"<<msg.data()<<std::endl;				
				
				phone.assign(strTemp,pos2 + 1,phoneLen);
				////cout<<"strPhone is : "<<phone.data()<<std::endl;
				pos = phone.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:282 parseSend() faild! "<<std::endl;
					return false;
				}
				phone.assign(phone,pos + 1,phone.length() - pos - 1);
				////cout<<"Phone is :"<<phone.data()<<std::endl;		

				port.assign(strTemp,pos3 + 1,portLen);
				////cout<<"strPort is : "<<port.data()<<std::endl;
				pos = port.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:293 parseSend() faild! "<<std::endl;
					return false;
				}
				port.assign(port,pos + 1,port.length() - pos - 1);
				////cout<<"Port is :"<<port.data()<<std::endl;	

				uptime.assign(strTemp,pos4 + 1,uptimeLen);
				////cout<<"strUptime is : "<<uptime.data()<<std::endl;
				pos = uptime.find('=',0);
				if(pos == std::string::npos)
				{
					cout<<"XXXXChannellib.cpp:304 parseSend() faild! "<<std::endl;
					return false;
				}
				uptime.assign(uptime,pos + 1,uptime.length() - pos - 1);
				////cout<<"Uptime is :"<<uptime.data()<<std::endl;	

				upsms._phone = phone;
				upsms._appendid= port;
				upsms._content = msg;
				upsms._upsmsTimeStr= uptime;
				char * reportTime = const_cast<char *>(uptime.data());
				upsms._upsmsTimeInt= strToTime(reportTime);
				upsmsRes.push_back(upsms);
				return true;			
			}
			
		}
        catch(...)
        {
        	cout<<"XXXXChannellib.cpp:293 parseSend() faild! "<<std::endl;
            //LogWarn("[GDChannellib::parseReport] is failed.");
            return false;
        }
    }


    int XXXXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		
		std::string::size_type pos = data->find("%contentEx%");
		if (pos != std::string::npos) 
        {
			data->replace(pos, strlen("%contentEx%"), sms->m_strContent);
		}
		
		vhead->push_back("Content-Type:application/xml");
        return 0;
    }


} /* namespace smsp */
