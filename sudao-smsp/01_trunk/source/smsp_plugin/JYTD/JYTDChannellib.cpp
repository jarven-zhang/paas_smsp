#include "JYTDChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"

namespace smsp 
{

JYTDChannellib::JYTDChannellib() 
{
}
JYTDChannellib::~JYTDChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new JYTDChannellib;
    }
}

bool JYTDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{	
	try
	{
        ////如果成功返回success;批号ID   否则返回failure;错误提示
        ////success;337017332
        std::string::size_type pos = respone.find("success;");
        if (pos == std::string::npos)
        {
            ////LogError("[JYTDChannellib::parseSend] sumbit respone data[] format is error.",respone.data());
        }
        else
        {
            pos = respone.find(";");
            smsid = respone.substr(pos+1);

            pos = smsid.find("\r\n");
            smsid = smsid.substr(0,pos);
            
            status = "0";
            return true;
        }

        pos = respone.find("failure;");
        if (pos == std::string::npos)
        {
            return false;   
        }
        else
        {
            strReason = respone.substr(strlen("failure;"));
            return false;
        }
	}
	catch(...)
	{
        ////LogError("[JYTDChannellib::parseSend] except faield.");
		return false;
	}
}

bool JYTDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse) 
{	
	try
	{ 
        ////LogNotice("befor***respone:[%s]", respone.data());
        std::string::size_type pos = respone.find_first_of("<");
        respone = respone.substr(pos);
        respone.erase(respone.find_last_of(">") + 1);
        ////LogNotice("after***respone:[%s]", respone.data());

        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
        	////LogNotice("[JYTDChannellib::parseReport]myDocument Parse return false.");
        	return false;
        }

        TiXmlElement *elementRoot = myDocument.RootElement();
        if (NULL == elementRoot)
        {
            ////LogError("[JYTDChannellib::parseReport] elementRoot is NULL.");
            return false;
        }

        std::string type = elementRoot->Value();

        if (type == "smsreport")
        {
            TiXmlElement* elementPsreport = elementRoot->FirstChildElement();
            while(NULL != elementPsreport)
            {
                TiXmlElement* elementMobile = elementPsreport->FirstChildElement();
                if (NULL == elementMobile)
                {
                    ////LogError("[JYTDChannellib::parseReport] elementMobile is NULL.");
                    return false;
                }

                TiXmlElement* elementUserName = elementMobile->NextSiblingElement();
                if (NULL == elementUserName)
                {
                    ////LogError("[JYTDChannellib::parseReport] elementUserName is NULL.");
                    return false;
                }

                TiXmlElement* elementSmslogid = elementUserName->NextSiblingElement();
                if (NULL == elementSmslogid)
                {
                    ////LogError("[JYTDChannellib::parseReport] elementSmslogid is NULL.");
                    return false;
                }
                TiXmlNode* nodeSmslogid = elementSmslogid->FirstChild();
                if (NULL == nodeSmslogid)
                {
                    ////LogError("[JYTDChannellib::parseReport] nodeSmslogid is NULL.");
                    return false;
                }

                TiXmlElement* elementStatus = elementSmslogid->NextSiblingElement();
                if (NULL == elementStatus)
                {
                    ////LogError("[JYTDChannellib::parseReport] elementStatus is NULL.");
                    return false;
                }
                TiXmlNode* nodeStatus = elementStatus->FirstChild();
                if (NULL == nodeStatus)
                {
                    ////LogError("[JYTDChannellib::parseReport] nodeStatus is NULL.");
                    return false;
                }

                TiXmlElement* elementSendTime = elementStatus->NextSiblingElement();
                if (NULL == elementSendTime)
                {
                    ////LogError("[JYTDChannellib::parseReport] elementSendTime is NULL.");
                    return false;
                }
                TiXmlNode* nodeSendTime = elementSendTime->FirstChild();
                if (NULL == nodeSendTime)
                {
                    ////LogError("[JYTDChannellib::parseReport] nodeSendTime is NULL.");
                    return false;
                }

                elementPsreport = elementPsreport->NextSiblingElement();


                std::string strStatus = nodeStatus->Value();
                smsp::StateReport report;
                
                
                
                report._smsId = nodeSmslogid->Value();
				report._reportTime = time(NULL);
                if ("1" == strStatus)
                {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else
                {
                    report._status = SMS_STATUS_CONFIRM_FAIL;
                }
				
				report._desc = strStatus;
				reportRes.push_back(report);

                ////LogWarn("smsId[%s],status[%s].",report._smsId.data(),strStatus.data());
                
            }
        }
        /*
        else if (type == "smsrerecv")
        {
            TiXmlElement* elementPsrerecv = elementRoot->FirstChildElement();
            while(NULL != elementPsrerecv)
            {
                TiXmlElement* elementMobile = elementPsrerecv->FirstChildElement();
                if (NULL == elementMobile)
                {
                    LogError("[JYTDChannellib::parseReport] elementMobile is NULL.");
                    return false;
                }
                TiXmlNode* nodeMobile = elementMobile->FirstChild();
                if (NULL == nodeMobile)
                {
                    LogError("[JYTDChannellib::parseReport] nodeMobile is NULL.");
                    return false;
                }

                TiXmlElement* elementReceiver = elementMobile->NextSiblingElement();
                if (NULL == elementReceiver)
                {
                    LogError("[JYTDChannellib::parseReport] elementReceiver is NULL.");
                    return false;
                }
                TiXmlNode* nodeReceiver = elementReceiver->FirstChild();
                if (NULL == nodeReceiver)
                {
                    LogError("[JYTDChannellib::parseReport] nodeReceiver is NULL.");
                    return false;
                }

                TiXmlElement* elementContent = elementReceiver->NextSiblingElement();
                if (NULL == elementContent)
                {
                    LogError("[JYTDChannellib::parseReport] elementContent is NULL.");
                    return false;
                }
                TiXmlNode* nodeContent = elementContent->FirstChild();
                if (NULL == nodeContent)
                {
                    LogError("[JYTDChannellib::parseReport] nodeContent is NULL.");
                    return false;
                }

                TiXmlElement* elementSendTime = elementContent->NextSiblingElement();
                if (NULL == elementSendTime)
                {
                    LogError("[JYTDChannellib::parseReport] elementSendTime is NULL.");
                    return false;
                }
                TiXmlNode* nodeSendTime = elementSendTime->FirstChild();
                if (NULL == nodeSendTime)
                {
                    LogError("[JYTDChannellib::parseReport] nodeSendTime is NULL.");
                    return false;
                }

                elementPsrerecv = elementPsrerecv->NextSiblingElement();

                std::string strMobile = nodeMobile->Value();
                std::string strReceiver = nodeReceiver->Value();
                std::string strContent = nodeContent->Value();
                strContent = base::Base64::Decode(strContent);
                std::string strTime = nodeSendTime->Value();
                
                LogWarn("moblie[%s],receiver[%s],conntent[%s],time[%s].",
                    strMobile.data(),strReceiver.data(),strContent.data(),strTime.data());
            }
        }
        */
        else
        {
            ////LogError("[JYTDChannellib::parseReport] type[%s] is error.",type.data());
            return false;
        }
        
        
		return true;
	}
	catch(...)
	{
        ////LogError("[JYTDChannellib::parseReport] except faield.");
		return false;
	}
}


int JYTDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
{

	return 0;
}

int JYTDChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
{
	smsChIntvl._itlValLev1 = 0;

	smsChIntvl._itlTimeLev1 = 22;
	smsChIntvl._itlTimeLev2 = 22;
	return 0;
}
} /* namespace smsp */
