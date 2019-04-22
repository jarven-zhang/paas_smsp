#include "SYNIVERSEChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    SYNIVERSEChannellib::SYNIVERSEChannellib()
    {

    }
    SYNIVERSEChannellib::~SYNIVERSEChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new SYNIVERSEChannellib;
        }
    }

    void SYNIVERSEChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool SYNIVERSEChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //printf("%s\n", respone.data());
        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
            return false;
        }

        TiXmlElement *element = myDocument.RootElement();
        if(NULL==element)
            return false;
        TiXmlElement *trackIDElement = element->FirstChildElement();
        if(NULL==trackIDElement)
            return false;

        TiXmlElement *statusElement = trackIDElement->NextSiblingElement();
        if(NULL==statusElement)
            return false;
        TiXmlNode *trackIDNode = trackIDElement->FirstChild();
        if(NULL==trackIDNode)
            return false;

        TiXmlNode *statusNode = statusElement->FirstChild();
        if(NULL==statusNode)
            return false;

        status = statusNode->Value();
        strReason = status;
        if(status == "Message Accepted!!")
        {
            status = "0";
        }
        std::string trackid = trackIDNode->Value();
        smsid = trackid;
        return true;
    }

    bool SYNIVERSEChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        smsp::StateReport report;
        TiXmlDocument myDocument;
        //LogNotice("syniverse report[%s]", respone.data());
        //clean the data after the last '>'
        respone.erase(respone.find_last_of(">") + 1);
        //LogNotice("respone:[%s]", respone.data());
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
            //LogNotice("Syniverse-Report myDocument Parse return false.");
            return false;
        }

        TiXmlElement *element = myDocument.RootElement();
        if(NULL==element)
        {
            //LogNotice("SYNIVERSEParser::parseReport() myDocument.RootElement is null.");
            return false;
        }
        TiXmlElement *destinationElement = element->FirstChildElement();
        if(NULL==destinationElement)
        {
            //LogNotice("destination is null.");
            return false;
        }
        TiXmlNode *destinationNode = destinationElement->FirstChild();
        if(NULL==destinationNode)
        {
            //LogNotice("destinationNode is null");
            return false;
        }


        TiXmlElement *statuscodeElement = destinationElement->NextSiblingElement();
        if(NULL==statuscodeElement)
        {
            //LogNotice("statuscodeElement is null.");
            return false;
        }
        TiXmlNode *statuscodeNode = statuscodeElement->FirstChild();
        if(NULL==statuscodeNode)
        {
            //LogNotice("statuscodeNode is null");
            return false;
        }


        TiXmlElement *statusinfoElement = statuscodeElement->NextSiblingElement();
        if(NULL==statusinfoElement)
        {
            //LogNotice("statusinfoElement is null.");
            return false;
        }
        TiXmlNode *statusinfoNode = statusinfoElement->FirstChild();
        if(NULL==statusinfoNode)
        {
            //LogNotice("statusinfoNode is null");
            return false;
        }

        TiXmlElement *trackingidElement = statusinfoElement->NextSiblingElement();
        if(NULL==trackingidElement)
        {
            //LogNotice("trackingidElement is null.");
            return false;
        }
        TiXmlNode *trackingidNode = trackingidElement->FirstChild();
        if(NULL==trackingidNode)
        {
            //LogNotice("trackingidNode is null");
            return false;
        }

        std::string destination;
        destination = std::string(destinationNode->Value());
        std::string statuscode;
        statuscode = statuscodeNode->Value();
        std::string statusinfo;
        statusinfo = statusinfoNode->Value();
        std::string trackingid;
        trackingid = trackingidNode->Value();

        Int32 nStatus = 4;
        if(statuscode=="0")
            nStatus = SMS_STATUS_CONFIRM_SUCCESS;
        else
            nStatus = SMS_STATUS_CONFIRM_FAIL;

        if(!destination.empty())
        {
            report._smsId = trackingid;
            report._reportTime = time(NULL);
            report._status = nStatus;
            int pos = statusinfo.find("stat:");     //statusinfo[sm: submit date:1509171516 done date:1509171516 stat:DELIVRD text:ext: 54F400]
            int pos2 = statusinfo.find("text:");
            if(std::string::npos != pos && std::string::npos != pos2 && (pos2 > pos))
            {
                statusinfo = statusinfo.substr(pos + 5, pos2 - 1 - pos -5); //statusinfo[DELIVRD]
            }
            report._desc = statusinfo;
            reportRes.push_back(report);
        }
        return true;
    }


    int SYNIVERSEChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        //2015-09-09 (1)content should transfer ucs2  (2)phone remove first 2 num

        //REPLACE %content%
        std::string content;
        utils::UTFString utfHelper = utils::UTFString();
        utfHelper.u2u16(sms->m_strContent, content);   //content:ucs2
        //utfHelper->u2u16((*it), sendContent);   //new-lng.
        //LogNotice("utf-8[%s], ucs-2[%s]", sms->_content.data(), content.data());
        std::string::size_type pos;
        pos = data->find("%content%");
        if (pos != std::string::npos)
        {
            data->replace(pos, strlen("%content%"), smsp::Channellib::encodeBase64(content));
        }

        //remove first 2 numbers
        //sms->_phone = sms->_phone.substr(2);
        pos = data->find("%phone%");
        if (pos != std::string::npos)
        {
            data->replace(pos, strlen("%phone%"), sms->m_strPhone.substr(2));
        }
        return 0;
    }


} /* namespace smsp */
