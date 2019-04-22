#include "SDZYChannellib.h"
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
            return new SDZYChannellib;
        }
    }
    
    SDZYChannellib::SDZYChannellib()
    {
    }
    SDZYChannellib::~SDZYChannellib()
    {

    }

    void SDZYChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

	
	///mylib->parseSend(strData, pSms->m_strChannelSmsId, state, strReason);
    bool SDZYChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
	 /*
	 <?xml version="1.0"  encoding="utf-8" ?>
	 <returnsms>
		 <returnstatus>status</returnstatus> ---------- 返回状态值：成功返回Success 失败返回：Faild
		 <message>message</message> ---------- 相关的错误描述
		 <remainpoint> remainpoint</remainpoint> ---------- 返回余额
		 <taskID>taskID</taskID>  -----------  返回本次任务的序列ID
		 <successCounts>successCounts</successCounts> --成功短信数：当成功后返回提交成功短信数
	 </returnsms>
	 */
	 //printf("come in parseSend");
		try
 		{
			respone = smsp::Channellib::urlDecode(respone);
			
			TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {  
            	std::cout<<"SDZYChannellib.cpp:59 parseSend() faild! "<<std::endl;
                return false;
            }			
			TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            { 
            	std::cout<<"SDZYChannellib.cpp:66 parseSend() faild! "<<std::endl;            
                return false;
				
            }
			
			TiXmlElement *resultElement = element->FirstChildElement();
			if(NULL==resultElement)
            {  
            	std::cout<<"SDZYChannellib.cpp:74 parseSend() faild! "<<std::endl;            
                return false;
            }
			
			TiXmlNode *resultNode = resultElement->FirstChild();
			if(NULL==resultNode)
            { 
            	std::cout<<"SDZYChannellib.cpp:81 parseSend() faild! "<<std::endl;            
                return false;
            }
			std::string strResult ;
			strResult = std::string(resultNode->Value());
			
			if(strResult.compare("Success") == 0)
			{
				status.assign("0");	
			}
			else
			{
				status.assign(strResult);
			}
			
			TiXmlElement *DesElement = resultElement->NextSiblingElement();
			if(NULL==DesElement)
            {
            	std::cout<<"XYCChannellib.cpp:97 parseSend() faild! "<<std::endl;            
                return false;
			}
			TiXmlNode *DesNode = DesElement->FirstChild();
			if(NULL==DesNode)
			{
				std::cout<<"weilu_test: DesNode is NULL"<<std::endl;
				return false;
			}
			strReason.assign(DesNode->Value());

			TiXmlElement *remianPoint = DesElement->NextSiblingElement();
			if(remianPoint == NULL)
			{
				std::cout<<"weilu_test:remain point is NULL"<<std::endl;
				return false;
			}
			
			TiXmlElement *TaskID = remianPoint->NextSiblingElement();
			if(TaskID ==NULL)
			{
				std::cout<<"weilu_test:task id is NULL"<<std::endl;
				return false;
			}
			TiXmlNode *TaskIDNode = TaskID->FirstChild();
			if(TaskIDNode == NULL)
			{
				std::cout<<"weilu_test:task in node is NULL"<<std::endl;
				return false;
			}
			smsid.assign(TaskIDNode->Value());		
			return true	;	
 		}
        catch(...)
        {
           std::cout<<"XYCChannellib.cpp:127 parseSend() faild! "<<std::endl;        
           return false;
        }
    }

    bool SDZYChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
		/*<?xml version="1.0" encoding="UTF-8"?> 
		<returnsms>
			<statusbox>
				<mobile>15023239810</mobile>-------------对应的手机号码
				<taskid>1212</taskid>-------------同一批任务ID
				<status>10</status>---------状态报告----10：发送成功，20：发送失败
				<receivetime>2011-12-02 22:12:11</receivetime>-------------接收时间
				<errorcode>DELIVRD</errorcode>-上级网关返回值，不同网关返回值不同，仅作为参考
				<extno>01</extno>--子号，即自定义扩展号
			</statusbox>
			<callbox>
				<mobile>15023239810</mobile>-------------对应的手机号码
				<taskid>1212</taskid>-------------同一批任务ID
				<content>你好，我不需要</content>---------上行内容
				<receivetime>2011-12-02 22:12:11</receivetime>-------------接收时间
				<extno>01</extno>----子号，即自定义扩展号
			</callbox>			
		</returnsms>
		*/
		int pos = respone.find_last_of('>');
		string tempStr = respone.substr(0,pos+1);
		try
		{
            strResponse.assign("ok");	
			tempStr = smsp::Channellib::urlDecode(tempStr);		
			TiXmlDocument myDocument;
            if(myDocument.Parse(tempStr.data(),0,TIXML_DEFAULT_ENCODING))
            { 
            	std::cout<<"SDZYChannellib.cpp:154 parseReport() faild! "<<std::endl;            
                return false;
            }			
			TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
            	std::cout<<"SDZYChannellib.cpp:161 parseReport() faild! "<<std::endl; 
                return false;
            }

			TiXmlElement *stateElement = element->FirstChildElement();
			while(true)
			{
				if(stateElement == NULL)
				{
	            	std::cout<<"SDZYChannellib.cpp:168 parseReport() faild! "<<std::endl; 			
					return false;
				}
				string strState = stateElement->Value();
				if(strState.compare("statusbox") == 0)
				{
					smsp::StateReport report;
					TiXmlElement *mobileElement = stateElement->FirstChildElement();
					if(mobileElement == NULL)
					{
						cout<<"can not find mobile element"<<std::endl;
						return false;
					}
					TiXmlNode *mobileNode = mobileElement ->FirstChild();
					if(mobileNode == NULL)
					{
						std::cout<<"can not find mobile node"<<std::endl;
						return false;
					}
					report._phone = mobileNode ->Value();

					TiXmlElement *TaskIdElement = mobileElement->NextSiblingElement();
					if(TaskIdElement == NULL)
					{
						std::cout<<"can not find task id element"<<std::endl;
						return false;
					}
					TiXmlNode *TaskIdNode = TaskIdElement->FirstChild();
					if(TaskIdNode == NULL)
					{
						std::cout<<"can not find task id node"<<std::endl;
						return false;
					}
					report._smsId = TaskIdNode->Value();

					TiXmlElement *statusElement = TaskIdElement->NextSiblingElement();
					if(statusElement == NULL)
					{
						std::cout<<"can not find status element"<<std::endl;
						return false;
					}
					TiXmlNode *statusNode = statusElement->FirstChild();
					if(statusNode == NULL)
					{
						std::cout<<"can not find status node"<<std::endl;
						return false;
					}
					string strStatus = statusNode ->Value();
					if(strStatus.compare("10") == 0)
					{
						report._status = SMS_STATUS_CONFIRM_SUCCESS;
					}
					else
					{
						report._status = SMS_STATUS_CONFIRM_FAIL;
					}
					
					TiXmlElement *receivetimeElement = statusElement->NextSiblingElement();
					if(receivetimeElement == NULL)
					{
						std::cout<<"can not find receivetime element"<<std::endl;
						return false;
					}
					TiXmlNode *receivetimeNode = receivetimeElement->FirstChild();
					if(receivetimeNode == NULL)
					{
						std::cout<<"can not find receivetime node"<<std::endl;
						return false;
					}
					string reportTime = receivetimeNode->Value();
					report._reportTime= strToTime(const_cast<char *>(reportTime.data()));

					TiXmlElement *desElement = receivetimeElement->NextSiblingElement();
					if(desElement == NULL)
					{
						std::cout<<"can not find desElement"<<std::endl;
						return false;
					}
					TiXmlNode *DesNode = desElement->FirstChild();
					if(DesNode == NULL)
					{
						std::cout<<"can not find des node"<<std::endl;
						return false;
					}
					report._desc = 	DesNode->Value();
					reportRes.push_back(report);
				}
				else if(strState.compare("callbox") == 0)
				{
					smsp::UpstreamSms upstream;
					TiXmlElement *mobileElement = stateElement->FirstChildElement();
					if(mobileElement == NULL)
					{
						cout<<"can not find mobile element"<<std::endl;
						return false;
					}
					TiXmlNode *mobileNode = mobileElement ->FirstChild();
					if(mobileNode == NULL)
					{
						std::cout<<"can not find mobile node"<<std::endl;
						return false;
					}
					upstream._phone= mobileNode ->Value();

					TiXmlElement *TaskIdElement = mobileElement->NextSiblingElement();
					if(TaskIdElement == NULL)
					{
						std::cout<<"can not find TaskId element"<<std::endl;
						return false;
					}
					
					TiXmlElement *contentElement = TaskIdElement->NextSiblingElement();
					if(contentElement == NULL)
					{
						std::cout<<"can not find content element"<<std::endl;
						return false;
					}
					TiXmlNode *contentNode = contentElement->FirstChild();
					if(contentNode == NULL)
					{
						std::cout<<"can not find content node"<<std::endl;
						return false;
					}
					upstream._content = contentNode->Value();
					
					TiXmlElement *receivetimeElement = contentElement->NextSiblingElement();
					if(receivetimeElement == NULL)
					{
						std::cout<<"can not find receivetime element"<<std::endl;
						return false;
					}
					TiXmlNode *receivetimeNode = receivetimeElement->FirstChild();
					if(receivetimeNode == NULL)
					{
						std::cout<<"can not find receivetime node"<<std::endl;
						return false;
					}
					upstream._upsmsTimeStr = receivetimeNode->Value();
					
					upstream._upsmsTimeInt= strToTime(const_cast<char *>(upstream._upsmsTimeStr.data()));
					
					TiXmlElement *extnoElement = receivetimeElement->NextSiblingElement();
					if(extnoElement == NULL)
					{
						std::cout<<"can not find extnotime element"<<std::endl;
						return false;
					}
					TiXmlNode *extnoNode = extnoElement->FirstChild();
					if(extnoNode == NULL)
					{
						std::cout<<"can not find extno node"<<std::endl;
						return false;
					}
					string temAppendId = extnoNode->Value();
					upstream._appendid = temAppendId.substr(2);				
					upsmsRes.push_back(upstream);					
				}
				else
				{
					std::cout<<"error can not find report or mo"<<std::endl;
					return false;
				}
				
				stateElement = stateElement->NextSiblingElement();
				if(stateElement == 0)
				{
					return true;
				}
			}	
		}						
        catch(...)
        {
            //LogWarn("[GDChannellib::parseReport] is failed.");
            return false;
        }
    }


    int SDZYChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
		return 0;
    }


} /* namespace smsp */
