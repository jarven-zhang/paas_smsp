#include "YMRTChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"

namespace smsp 
{
YMRTChannellib::YMRTChannellib() 
{
}

YMRTChannellib::~YMRTChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new YMRTChannellib;
    }
}

bool YMRTChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{	
	try
	{
        std::string::size_type pos = respone.find_first_of("<");
        respone = respone.substr(pos);
        respone.erase(respone.find_last_of(">") + 1);
        ////LogNotice("[YMRTChannellib::parseSend] respine[%s]",respone.data());

        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
        	////LogNotice("[YMRTChannellib::parseSend]myDocument Parse return false.");
        	std::cout<<"[YMRTChannellib::parseSend]myDocument Parse return false."<<std::endl;
        	return false;
        }

        TiXmlElement *elementRoot = myDocument.RootElement();
        if (NULL == elementRoot)
        {
            ////LogError("[YMRTChannellib::parseSend] elementRoot is NULL.");
            std::cout<<"[YMRTChannellib::parseSend] elementRoot is NULL."<<std::endl;
            return false;
        }

        TiXmlElement* elementError = elementRoot->FirstChildElement();
        if (NULL == elementError)
        {
            ////LogError("[YMRTChannellib::parseSend] elementError is NULL.");
            std::cout<<"[YMRTChannellib::parseSend] elementError is NULL."<<std::endl;
            return false;
        }
        TiXmlNode* nodeError = elementError->FirstChild();
        if (NULL == nodeError)
        {
            /////LogError("[YMRTChannellib::parseSend] nodeError is NULL.");
            std::cout<<"[YMRTChannellib::parseSend] nodeError is NULL."<<std::endl;
            return false;
        }

        TiXmlElement* elementMessage = elementError->NextSiblingElement();
        if (NULL == elementMessage)
        {
            ////LogError("[YMRTChannellib::parseSend] elementMessage is NULL.");
            std::cout<<"[YMRTChannellib::parseSend] elementMessage is NULL."<<std::endl;
            return false;
        }
        TiXmlNode* nodeMessage = elementError->FirstChild();
        if (NULL == nodeMessage)
        {
            ////LogError("[YMRTChannellib::parseSend] nodeMessage is NULL.");
            std::cout<<"[YMRTChannellib::parseSend] nodeMessage is NULL."<<std::endl;
            return false;
        }
            
        std::string strStatus = nodeError->Value();
        strReason = nodeMessage->Value();
        
        if ("0" == strStatus)
        {
            status = "0";
        }
        else
        {
            status = "4";
        }
        
        return true;
	}
	catch(...)
	{
        /////LogError("[YMRTChannellib::parseSend] except faield.");
        std::cout<<"[YMRTChannellib::parseSend] except faield."<<std::endl;
		return false;
	}
}

bool YMRTChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse) 
{	
	try
	{ 
        std::string::size_type pos = respone.find_first_of("<");
        respone = respone.substr(pos);
        respone.erase(respone.find_last_of(">") + 1);
        ////LogNotice("[YMRTChannellib::parseReport] respine[%s]",respone.data());

        
        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
        	////LogNotice("[YMRTChannellib::parseReport]myDocument Parse return false.");
        	std::cout<<"[YMRTChannellib::parseReport]myDocument Parse return false."<<std::endl;
        	return false;
        }

        TiXmlElement *elementRoot = myDocument.RootElement();
        if (NULL == elementRoot)
        {
            ////LogError("[YMRTChannellib::parseReport] elementRoot is NULL.");
            std::cout<<"[YMRTChannellib::parseReport] elementRoot is NULL."<<std::endl;
            return false;
        }

        TiXmlElement* elementError = elementRoot->FirstChildElement();
        if (NULL == elementError)
        {
            ////LogError("[YMRTChannellib::parseReport] elementError is NULL.");
            std::cout<<"[YMRTChannellib::parseReport] elementError is NULL."<<std::endl;
            return false;
        }

        TiXmlElement* elementMessage = elementError->NextSiblingElement();


        
        while(NULL != elementMessage)
        {
            TiXmlElement* elementSrctermid = elementMessage->FirstChildElement();
            if (NULL == elementSrctermid)
            {
                ////LogError("[YMRTChannellib::parseReport] elementSrctermid is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementSrctermid is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementSubmitDate = elementSrctermid->NextSiblingElement();
            if (NULL == elementSubmitDate)
            {
                ////LogError("[YMRTChannellib::parseReport] elementSubmitDate is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementSubmitDate is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementReceiveDate = elementSubmitDate->NextSiblingElement();
            if (NULL == elementReceiveDate)
            {
                ////LogError("[YMRTChannellib::parseReport] elementReceiveDate is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementReceiveDate is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementAddSerial = elementReceiveDate->NextSiblingElement();
            if (NULL == elementAddSerial)
            {
                ////LogError("[YMRTChannellib::parseReport] elementAddSerial is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementAddSerial is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementAddSerialRev = elementAddSerial->NextSiblingElement();
            if (NULL == elementAddSerialRev)
            {
                ////LogError("[YMRTChannellib::parseReport] elementAddSerialRev is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementAddSerialRev is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementState = elementAddSerialRev->NextSiblingElement();
            if (NULL == elementState)
            {
                ////LogError("[YMRTChannellib::parseReport] elementState is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementState is NULL."<<std::endl;
                return false;
            }
            TiXmlNode* nodeState = elementState->FirstChild();
            if (NULL == nodeState)
            {
                ////LogError("[YMRTChannellib::parseReport] nodeState is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] nodeState is NULL."<<std::endl;
                return false;
            }

            TiXmlElement* elementSeqid = elementState->NextSiblingElement();
            if (NULL == elementSeqid)
            {
                ////LogError("[YMRTChannellib::parseReport] elementSeqid is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] elementSeqid is NULL."<<std::endl;
                return false;
            }
            TiXmlNode* nodeSeqid = elementSeqid->FirstChild();
            if (NULL == nodeSeqid)
            {
                ////LogError("[YMRTChannellib::parseReport] nodeSeqid is NULL.");
                std::cout<<"[YMRTChannellib::parseReport] nodeSeqid is NULL."<<std::endl;
                return false;
            }

            
            elementMessage = elementMessage->NextSiblingElement();


            std::string strStatus = nodeState->Value();
            smsp::StateReport report;
            
            
            
            report._smsId = nodeSeqid->Value();
			report._reportTime = time(NULL);
            
            if ("DELIVRD" == strStatus)
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
		return true;
	}
	catch(...)
	{
        /////LogError("[YMRTChannellib::parseReport] except faield.");
        std::cout<<"[YMRTChannellib::parseReport] except faield."<<std::endl;
		return false;
	}
}


int YMRTChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
{
    static int indexNum = 1;
    char msgId[25] = {0};
    sprintf(msgId,"%ld%d",(long)time(NULL),indexNum);
    sms->m_strChannelSmsId.assign(msgId);
    
    std::string::size_type pos = data->find("%rtsmsid%");
    if (pos == std::string::npos)
    {
        ////LogError("data[%s] not find rtsmsid.",data->data());
        std::cout<<"data not find rtsmsid."<<std::endl;
        return -1;
    }

    data->replace(pos,strlen("%rtsmsid%"),msgId);

    indexNum++;
	return 0;
}

int YMRTChannellib::getChannelInterval(SmsChlInterval& smsChIntvl)
{
	smsChIntvl._itlValLev1 = 0;

	smsChIntvl._itlTimeLev1 = 22;
	smsChIntvl._itlTimeLev2 = 22;
	return 0;
}
} /* namespace smsp */
