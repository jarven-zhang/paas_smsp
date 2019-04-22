#include "BJBBChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include <string>
#include <iostream>
#include <sstream>


namespace smsp
{
    extern "C"
    {
        void * create()
        {
            return new BJBBChannellib;
        }
    }
    
    BJBBChannellib::BJBBChannellib()
    {
    }
    BJBBChannellib::~BJBBChannellib()
    {

    }

    void BJBBChannellib::test()
    {
        //LogNotice("RTT:  in lib  test ok!");

    }

	
	///mylib->parseSend(strData, pSms->m_strChannelSmsId, state, strReason);
    bool BJBBChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	 /*
	 message=0;batchNumber=3323;total=100;valid=99;duplicate=0
	 */
	
		try
 		{
			if (respone.empty())
			{
				return false;
			}
			std::map<std::string,std::string> mapSet;
            split_Ex_Map(respone, ";", mapSet);
			if (mapSet["message"] == "0")
			{
				status = "0";
				smsid = mapSet["batchNumber"];
			}
			else
			{
				status = "4";
				strReason = mapSet["message"];
							
			}
			return true	;	
 		}
        catch(...)
        {
          // cout<<"BJBBChannellib.cpp:parseSend() faild! "<<std::endl;        
           return false;
        }
    }

    bool BJBBChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {	
		int i = 0;

		try
		{				
			TiXmlDocument myDocument;		
		
            if(*(myDocument.Parse(respone.c_str(), 0, TIXML_ENCODING_UNKNOWN)) != 0)
            { 
            	//std::cout<<"BJBBChannellib.cpp:130 parseReport() faild! "<<std::endl;    						
                return false;
            }

			TiXmlElement *element = myDocument.RootElement();
            if( NULL==element )
            {
            	//std::cout<<"BJBBChannellib.cpp:142 parseReport() faild! "<<std::endl; 
                return false;
            }
			std::string reOrMo = std::string(element->Value());			
	
			if (reOrMo == "reports")
			{
				TiXmlElement *reportElement = element->FirstChildElement();
				if(reportElement == NULL)
				{
	            	//std::cout<<"BJBBChannellib.cpp:127 parseReport() faild! "<<std::endl; 			
					return false;
				}
				std::string is = std::string(reportElement->Value());
				// cout<<"is="<<is<<endl;
				while (reportElement != NULL)
				{
					TiXmlElement *channelElement = reportElement->FirstChildElement();
					std::string is1 = std::string(channelElement->Value());
					if(NULL==channelElement)
					{
						//cout<<"channel is null"<<endl;
						return false;
		            }
					TiXmlNode *channel = channelElement->FirstChild();

					TiXmlElement *msgIdElement = channelElement->NextSiblingElement();
					if(NULL==msgIdElement)
					{
						//cout<<"msgIdElement is null"<<endl;
						return false;
					}
					TiXmlNode *msgId = msgIdElement->FirstChild();
				
					TiXmlElement *statusElement = msgIdElement->NextSiblingElement();
					if(NULL==statusElement)
					{
						//cout<<"statusElement is null"<<endl;
						return false;
					}
					TiXmlNode *status = statusElement->FirstChild();
				
					TiXmlElement *phoneNumElement = statusElement->NextSiblingElement();
					if(NULL==phoneNumElement)
					{
						//cout<<"phoneNumElement is null"<<endl;
						return false;
					}
					TiXmlNode *phoneNum = phoneNumElement->FirstChild();
					
					TiXmlElement *receiveTimeElement = phoneNumElement->NextSiblingElement();
					TiXmlElement *memoElement = receiveTimeElement->NextSiblingElement();
					if(NULL==memoElement)
					{						
						//cout<<"memoElement is null"<<endl;
						return false;
					}
					TiXmlNode *memo = memoElement->FirstChild();

					smsp::StateReport report;
					
					if((NULL != msgId) && (NULL != status) && (NULL != memo) && (NULL != phoneNum))
					{

						report._reportTime = time(NULL);
						report._desc = memo->Value();
						
						report._smsId = msgId->Value();
						report._phone = phoneNum->Value();
						string SendStatus = status->Value();
					
						if(SendStatus == "1")	//reportResp confirm suc
						{
							report._status = SMS_STATUS_CONFIRM_SUCCESS;
						}
						else	//reportResp confirm failed
						{
							report._status = SMS_STATUS_CONFIRM_FAIL;
						}
						//cout<<"des="<<report._desc<<",msmid="<<report._smsId<<",phone="<<report._phone<<endl;
						reportRes.push_back(report);	
						
					}
					else
					{
						//std::cout << "report data empty error" << std::endl;
						return false;
					}

					reportElement = reportElement->NextSiblingElement();
					
				}
				
			}
			else
			{
				TiXmlElement *moElement = element->FirstChildElement();
				if(NULL==moElement)
				{	
					//std::cout << "moElement is null" << std::endl;
					return false;
				}
			
				while(moElement != NULL)
				{	
					TiXmlElement *phoneNumElement = moElement->FirstChildElement();
					if(NULL==phoneNumElement)
					{
		                return false;
		            }
					TiXmlNode* phoneNum = phoneNumElement->FirstChild();

					TiXmlElement *contentElement = phoneNumElement->NextSiblingElement();
					if(NULL==contentElement)
					{
		                return false;
		            }
					TiXmlNode* content = contentElement->FirstChild();

					TiXmlElement *receiveTimeElement = contentElement->NextSiblingElement();
					if(NULL==receiveTimeElement)
					{
		                return false;
		            }
					TiXmlNode* receiveTime = receiveTimeElement->FirstChild();

					smsp::UpstreamSms upstream;
					
					if(NULL !=phoneNum && NULL!= content && NULL != receiveTime)
					{

						upstream._phone = phoneNum->Value();
						upstream._upsmsTimeStr = receiveTime->Value();
						upstream._content = content->Value();
						upsmsRes.push_back(upstream);
						//cout<<"phone="<<upstream._phone<<"_upsmsTimeStr"<<upstream._upsmsTimeStr<<"_content="<<upstream._content<<endl;
		                
			        }
					else
					{
						//std::cout << "replies mo data error" << std::endl;
			            return false;
					}
						
					moElement = moElement->NextSiblingElement();
				}
		}

    }
	catch(...)
	{
		//std::cout << "BJBBChannellib::parseReport is failed." << std::endl;
		return false;
	}
	return true;

}
	
    int BJBBChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		std::string::size_type pos = url->find("%sendTime%");
		tm ptm = {0};
	    time_t now = time(NULL);
	    localtime_r((const time_t *) &now,&ptm);
	    char szTime[64] = { 0 };	        
	    strftime(szTime, sizeof(szTime), "%Y-%m-%d%H:%M:%S", &ptm);
		if (pos != std::string::npos) 
        {
			url->replace(pos, strlen("%sendTime%"), szTime);
		}
        return 0;
    }


} /* namespace smsp */
