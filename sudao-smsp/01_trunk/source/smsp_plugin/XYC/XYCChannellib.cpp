#include "XYCChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"

namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new XYCChannellib;
        }
    }
    
    XYCChannellib::XYCChannellib()
    {
    }
    XYCChannellib::~XYCChannellib()
    {

    }

    void XYCChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

	
	///mylib->parseSend(strData, pSms->m_strChannelSmsId, state, strReason);
    bool XYCChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	 /*
	 <?xml version="1.0" encoding="UTF-8"?>
	 <Response>
	 <Result>0</Result>
	 <SmsIdList num="4">
		 <Item index="1" smsId="10201201200"  result="0"/>
		 <Item index="2" smsId="10201201201"  result="0"/>
	 <Item index="3" smsId="10201201202"  result="9"/>
	 <Item index="4" smsId="10201201203"  result="0"/>
	 </SmsIdList>
	 </Response>
	 */
	 //printf("come in parseSend");
	 ////std::cout<<"come in parseSend"<<std::endl;
		try
 		{
			respone = smsp::Channellib::urlDecode(respone);
			
			TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {  
            	std::cout<<"XYCChannellib.cpp:59 parseSend() faild! "<<std::endl;
                return false;
            }
			
			TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            { 
            	std::cout<<"XYCChannellib.cpp:66 parseSend() faild! "<<std::endl;            
                return false;
				
            }
			
			TiXmlElement *resultElement = element->FirstChildElement();
			if(NULL==resultElement)
            {  
            	std::cout<<"XYCChannellib.cpp:74 parseSend() faild! "<<std::endl;            
                return false;
            }
			
			TiXmlNode *resultNode = resultElement->FirstChild();
			if(NULL==resultNode)
            { 
            	std::cout<<"XYCChannellib.cpp:81 parseSend() faild! "<<std::endl;            
                return false;
            }

			std::string strResult ;
			strResult = std::string(resultNode->Value());
			int iResult = atoi(strResult.data());
			if(iResult !=0)
			{
            	std::cout<<"XYCChannellib.cpp:90 parseSend() faild! "<<std::endl;			
				return false;
			}
			
			TiXmlElement *sidNumElement = resultElement->NextSiblingElement();
			if(NULL==sidNumElement)
            {
            	std::cout<<"XYCChannellib.cpp:97 parseSend() faild! "<<std::endl;            
                return false;
			}

			const char* cNum = sidNumElement->Attribute("num");
			std::string strNum;
			strNum = std::string(cNum);	
			if(strNum.compare("1") != 0)
			{
            	std::cout<<"XYCChannellib.cpp:106 parseSend() faild! "<<std::endl;			
				return false;
			}
			
			TiXmlElement *OneElement = sidNumElement ->FirstChildElement();
			if(OneElement == NULL)
			{
            	std::cout<<"XYCChannellib.cpp:113 parseSend() faild! "<<std::endl;			
				return false;
			}
			const char * cSmsid = OneElement->Attribute("smsId");
			std::string strSmsid = cSmsid;
			smsid = strSmsid;
			const char * cStatus = OneElement->Attribute("result");
			std::string strStatus = cStatus;
			status = strStatus;
			strReason= status;
			return true	;	
 		}
        catch(...)
        {
           std::cout<<"XYCChannellib.cpp:127 parseSend() faild! "<<std::endl;        
           return false;
        }
    }

    bool XYCChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/*<?xml version="1.0" encoding="UTF-8"?>
		<Response>
		<Result>0</Result>
		<Report num="3">
		<Item smsId="1223232323" to_mobile="13500000000" status="0" reportTime="2013-07-20 12:00:03" desc="发送成功"/>
		<Item smsId="222323233" to_mobile="13600000000" status="2" reportTime="2013-07-20 12:00:04" desc="审核不通过"/>
		<Item smsId="22232366" to_mobile="13700000000" status="3" reportTime="2013-07-20 12:00:04" desc="发送失败"/>
		</Report>
		</Response>*/

		try
		{
            strResponse.assign("ok");
            smsp::StateReport report;
            smsp::UpstreamSms upsms;		
			respone = smsp::Channellib::urlDecode(respone);
			
			TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            { 
            	std::cout<<"XYCChannellib.cpp:154 parseReport() faild! "<<std::endl;            
                return false;
            }
			
			TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
            	std::cout<<"XYCChannellib.cpp:161 parseReport() faild! "<<std::endl; 
                return false;
            }

			TiXmlElement *resultElement = element->FirstChildElement();
			if(resultElement == NULL)
			{
            	std::cout<<"XYCChannellib.cpp:168 parseReport() faild! "<<std::endl; 			
				return false;
			}
			
			TiXmlNode *resultNode = resultElement->FirstChild();
			if(NULL==resultNode)
            {
            	std::cout<<"XYCChannellib.cpp:175 parseReport() faild! "<<std::endl;             
                return false;
            }	
			std::string strResult ;
			strResult = std::string(resultNode->Value());
			int iResult = atoi(strResult.data());
			if(iResult !=0)
			{
            	std::cout<<"XYCChannellib.cpp:183 parseReport() faild! "<<std::endl; 			
				return false;
			}	

			TiXmlElement *reportElement = resultElement->NextSiblingElement();
			if(NULL==reportElement)
            {
            	std::cout<<"XYCChannellib.cpp:190 parseReport() faild! "<<std::endl;             
                return false;
			}

			const char * reportOrMo = reportElement->Value();
			std::string strReportOrMo = reportOrMo;
			int isReport = -1;
			if(strReportOrMo.empty())
			{
            	std::cout<<"XYCChannellib.cpp:199 parseReport() faild! "<<std::endl;			
				return false;
			}		
			if(strReportOrMo.compare("Report") == 0)
			{
				isReport = 0;
			}
			if(strReportOrMo.compare("Mo") == 0)
			{
				isReport = 1;
			}
			if(isReport == 0)
			{
				const char* cNum = reportElement->Attribute("num");
				int Pos1 = 0;
				std::string strNum = cNum;
				int iNum = atoi(strNum.data());
				int nStatus = 0;
				////const char * cSmsId;
				////const char * cMobile;
				///const char * cStatus;
				///const char * cReportTime;
				///const char * cDesc;
				TiXmlElement * OneElement = reportElement->FirstChildElement();
				for(int i = 0;i < iNum ;i++)
				{				
					if(OneElement == NULL)
					{
            			std::cout<<"XYCChannellib.cpp:227 parseReport() faild! "<<std::endl; 					
						return false;
					}
					const char * cSmsId = OneElement->Attribute("smsId");
				 	std::string strSmsId = "";
			 		if(cSmsId != NULL)
			 		{			 		
						strSmsId = cSmsId;
					}
					const char * cMobile = OneElement->Attribute("to_mobile");
					std::string strMobile = "";
					if(cMobile !=NULL)
					{				
						strMobile = cMobile;
					}

					const char * cStatus = OneElement->Attribute("status");					
					std::string strStatus = "";
					if(cStatus !=NULL)
					{					
						strStatus = cStatus;
					}

					const char * cReportTime = OneElement->Attribute("reportTime");					
					std::string strReportTime = "";
					if(cReportTime != NULL)
					{
						strReportTime = cReportTime;
					}
					const char * cDesc = OneElement->Attribute("desc");					
					std::string strDesc = "";
					if(cDesc != NULL)
					{
						strDesc = cDesc;
					}
                    if(!strStatus.compare("0"))	//reportResp confirm suc
                    {
                    	nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    }
                    else	//reportResp confirm failed
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }
 
                    report._smsId = strSmsId;
                    report._status = nStatus;
					char * reportTime = const_cast<char *>(strReportTime.data());					
					UInt64 timeInt = strToTime(reportTime);				
                    report._reportTime = timeInt;
                    report._desc = strDesc;
					report._phone = strMobile;
                    reportRes.push_back(report);
					OneElement = OneElement->NextSiblingElement();
				}
				
				
			}
			else if(isReport == 1)
			{		
				const char* cNum = reportElement->Attribute("num");
				int Pos1 = 0;
				std::string strNum = cNum;
				int iNum = atoi(strNum.data());			
				const char * cSmsId;
				const char * cContent;
				const char * cFromMobile;
				const char * toPort;
				const char * cReportTime;
				TiXmlElement * OneElement = reportElement->FirstChildElement();
				for(int i = 0;i < iNum ;i++)
				{
					if(OneElement == NULL)
					{
            			std::cout<<"XYCChannellib.cpp:300 parseReport() faild! "<<std::endl;					
						return false;
					}
					cSmsId = OneElement->Attribute("id");
					std::string strSmsId = cSmsId;
					cContent = OneElement->Attribute("content");
					std::string strContent = cContent;
					cFromMobile = OneElement->Attribute("from_mobile");
					std::string strFromMobile = cFromMobile;
					toPort = OneElement->Attribute("to_port");
					std::string strPort = toPort;	
					cReportTime = OneElement->Attribute("rec_time");
					std::string strReportTime = cReportTime;	

					 upsms._phone = strFromMobile;
					 upsms._appendid= strPort;
					 upsms._content = strContent;
					 upsms._upsmsTimeStr= strReportTime;
					 char * reportTime = const_cast<char *>(strReportTime.data());
					 upsms._upsmsTimeInt= strToTime(reportTime);
					 upsmsRes.push_back(upsms);

					OneElement = OneElement->NextSiblingElement();
				}							
			}			
		}
        catch(...)
        {
            //LogWarn("[GDChannellib::parseReport] is failed.");
            return false;
        }
    }


    int XYCChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		std::string::size_type pos = data->find("%contentEx%");
		if (pos != std::string::npos) 
        {
			data->replace(pos, strlen("%contentEx%"), sms->m_strContent);
		}		
        return 0;
    }


} /* namespace smsp */
