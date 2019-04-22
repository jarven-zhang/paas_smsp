#include "ZSSDHTTPChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    ZSSDHTTPChannellib::ZSSDHTTPChannellib()
    {
    }
    ZSSDHTTPChannellib::~ZSSDHTTPChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new ZSSDHTTPChannellib;
        }
    }

    void ZSSDHTTPChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool ZSSDHTTPChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        /*
        第一个非空白行为错误码
　　	第二个非空白行为错误信息提示。
		*/

		int result_code = 0;

		vector<std::string> vecResult;
		split_Ex_Vec(respone, "\r\n",vecResult);

		if (vecResult.size()  ==  0)
		{
			return false;
		}

		result_code = atoi(vecResult.at(0).c_str());
		if (vecResult.size()  ==  2)
		{
			strReason = vecResult.at(0) + "|" + vecResult.at(1);
		}
		else
		{
			strReason = vecResult.at(0);
		}

		if (result_code > 0)
		{
			status = "0";
			smsid = vecResult.at(0);
		}
		else
		{
			status = "4";
			smsid = "";
		}

        return true;
    }

    bool ZSSDHTTPChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /* 状态报告:
        xml=<?xml version="1.0" encoding="UTF-8" ?> 
		<Response>
		  <Report>
		    <MsgID>短信批次号</MsgID> 
		    <Mobile>手机号码</Mobile> 
		    <Status>状态</Status> 
		  </Report>
		  <Report>
		    <MsgID>短信批次号</MsgID> 
		    <Mobile>手机号码</Mobile> 
		    <Status>状态（DELIVRD表示成功，其他值为失败）</Status> 
		  </Report>
		</Response>
        */

		/* 上行:
		xml=<?xml version="1.0" encoding="UTF-8" ?> 
		<Response>
		  <MO>
		    <Mobile>手机号码</Mobile> 
		    <Message><![CDATA[ 回复内容（utf-8编码）]]></Message>
		    <ReceiveTime>回复时间（格式：2012-01-12 15:23:12）</ReceiveTime> 
		    <ExtendCode>扩展号</ExtendCode> 
		    <MsgID>短信批次号 （有可能为空字符）</MsgID> 
		  </MO>
		  <MO>
		    <Mobile>手机号码</Mobile> 
		    <Message><![CDATA[ 回复内容（utf-8编码）]]></Message>
		    <ReceiveTime>回复时间（格式：2012-01-12 15:23:12）</ReceiveTime> 
		    <ExtendCode>扩展号</ExtendCode> 
		    <MsgID>短信批次号 （有可能为空字符）</MsgID> 
		  </MO>
		</Response>
		*/

		std::string::size_type pos = respone.find_first_of("<");
		if (pos == std::string::npos)
		{
			return false;
		}
        respone = respone.substr(pos);
        respone.erase(respone.find_last_of(">") + 1);

		//cout << "response: " << respone << endl;
		
        smsp::StateReport report;
		smsp::UpstreamSms upstreamSms;
        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(), 0, TIXML_DEFAULT_ENCODING))
        {
        	//cout << "parse faile" << endl;
            return false;
        }

        TiXmlElement *element = myDocument.RootElement();
        if(NULL == element)
        {
        	//cout << "element null" << endl;
            return false;
        }

        TiXmlElement *statusboxElement = element->FirstChildElement();      //result node

        //get all report msg
        //TiXmlElement *stringElement = resultElement->NextSiblingElement();    //1th report node
        while(statusboxElement != NULL)
        {
            std::string type;
            type = statusboxElement->Value();
			//cout << "type: " << type << endl;
            if(type == "Report")
            {
                TiXmlElement *taskidElement = statusboxElement->FirstChildElement();
				//cout << "1" << endl;
                if(NULL == taskidElement)
                    return false;
                TiXmlNode *taskidNode = taskidElement->FirstChild();
				//cout << "2" << endl;
                if(NULL == taskidNode)
                    return false;

				TiXmlElement *mobileElement = taskidElement->NextSiblingElement();
				//cout << "3" << endl;
                if(NULL == mobileElement)
                    return false;
                TiXmlNode *mobileNode = mobileElement->FirstChild();
				//cout << "4" << endl;
                if(NULL == mobileNode)
                    return false;

                TiXmlElement *statusElement = mobileElement->NextSiblingElement();
				//cout << "5" << endl;
                if(NULL == statusElement)
                    return false;
                TiXmlNode *statusNode = statusElement->FirstChild();
				//cout << "6" << endl;
                if(NULL == statusNode)
                    return false;

                statusboxElement = statusboxElement->NextSiblingElement();

                std::string mobile = std::string(mobileNode->Value());
                std::string taskid = taskidNode->Value();
                std::string status = statusNode->Value();

                Int32 nStatus = 4;

                if(!strcmp(status.data(), "DELIVRD"))
                {
                    nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else
                {
                    nStatus = SMS_STATUS_CONFIRM_FAIL;
                }

				//cout << "mobile: " << mobile << "taskid: " << taskid << "status: " << status << endl;
				
                if(!taskid.empty())
                {
                    report._smsId = taskid;
                    report._reportTime = time(NULL);
                    report._status = nStatus;
                    report._desc = status;
					report._phone = mobile;
                    reportRes.push_back(report);
                }
                else
                    return false;
            }
			else if(type  ==  "MO")
            {
				TiXmlElement *mobileElement = statusboxElement->FirstChildElement();
				//cout << "7" << endl;
                if(NULL == mobileElement)
                    return false;
                TiXmlNode *mobileNode = mobileElement->FirstChild();
				//cout << "8" << endl;
                if(NULL == mobileNode)
                    return false;

                TiXmlElement *messageElement = mobileElement->NextSiblingElement();
				//cout << "9" << endl;
                if(NULL == messageElement)
                    return false;
                TiXmlNode *messageNode = messageElement->FirstChild();
				//cout << "10" << endl;
                if(NULL == messageNode)
                    return false;

				TiXmlElement *receiveTimeElement = messageElement->NextSiblingElement();
				//cout << "11" << endl;
                if(NULL == receiveTimeElement)
                    return false;
                TiXmlNode *receiveTimeNode = receiveTimeElement->FirstChild();
				//cout << "12" << endl;
                if(NULL == receiveTimeNode)
                    return false;

				TiXmlElement *extendCodeElement = receiveTimeElement->NextSiblingElement();
				//cout << "13" << endl;
                if(NULL == extendCodeElement)
                    return false;
                TiXmlNode *extendCodeNode = extendCodeElement->FirstChild();
				//cout << "14" << endl;
                if(NULL == extendCodeNode)
                    return false;

				TiXmlElement *taskidElement = extendCodeElement->NextSiblingElement();
				//cout << "15" << endl;
                if(NULL == taskidElement)
                    return false;
                TiXmlNode *taskidNode = taskidElement->FirstChild();
				//cout << "16" << endl;

                statusboxElement = statusboxElement->NextSiblingElement();

                std::string mobile = std::string(mobileNode->Value());
				std::string message = messageNode->Value();
				std::string receiveTime = receiveTimeNode->Value();
				std::string extendCode = extendCodeNode->Value();
                std::string taskid = "";

				if (NULL != taskidNode)
				{
					taskid = taskidNode->Value();
				}

                upstreamSms._phone = mobile;
				upstreamSms._content = message;
				upstreamSms._upsmsTimeStr = receiveTime;

				int extendCodeLen = extendCode.length();
				if (extendCodeLen <= 4)
				{
					return false;
				}
				upstreamSms._appendid.assign(extendCode, 4, extendCodeLen - 4);

				//cout << "mobile: " << mobile << "taskid: " << taskid << "message: " << message << "receiveTime: " << receiveTime << "extendCode: " << extendCode << endl;
				
				if (!taskid.empty())
				{
					upstreamSms._sequenceId = atoi(taskid.c_str());
				}
				
                upsmsRes.push_back(upstreamSms);
			}
            else
                statusboxElement = statusboxElement->NextSiblingElement();
        }
        return true;
    }


    int ZSSDHTTPChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        //do nothing
        return 0;
    }

} /* namespace smsp */
