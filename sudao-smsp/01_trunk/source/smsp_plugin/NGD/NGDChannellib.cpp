#include "NGDChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    NGDChannellib::NGDChannellib()
    {
    }
    NGDChannellib::~NGDChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new NGDChannellib;
        }
    }

    void NGDChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool NGDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //printf("%s\n", respone.data());
        try
        {
            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogError("[GEMSParser::parseSend] Parse is failed.");
                return false;
            }

            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                //LogError("[GEMSParser::parseSend] element is NULL.");
                return false;
            }
            TiXmlElement *statusElement = element->FirstChildElement();
            if(NULL==statusElement)
            {
                //LogError("[GEMSParser::parseSend] statusElement is NULL.");
                return false;
            }
            TiXmlElement *messageElement = statusElement->NextSiblingElement();
            if(NULL==messageElement)
            {
                //LogError("[GEMSParser::parseSend] messageElement is NULL.");
                return false;
            }
            TiXmlNode *messageNode = messageElement->FirstChild();
            if(NULL==messageNode)
            {
                //LogError("[GEMSParser::parseSend] messageNode is NULL.");
                return false;
            }
            TiXmlElement *remainpointElement = messageElement->NextSiblingElement();
            if(NULL==remainpointElement)
            {
                //LogError("[GEMSParser::parseSend] remainpointElement is NULL.");
                return false;
            }
            TiXmlElement *taskIdElement = remainpointElement->NextSiblingElement();
            if(NULL==taskIdElement)
            {

                //LogError("[GEMSParser::parseSend] taskIdElement is NULL.");
                return false;
            }
            TiXmlNode *taskIdNode = taskIdElement->FirstChild();
            if(NULL==taskIdNode)
            {
                //LogError("[GEMSParser::parseSend] taskIdNode is NULL.");
                return false;
            }

            status = messageNode->Value();
            if(status == "ok" || status == "10")
            {
                status = "0";
            }
            else
            {
                strReason.assign(status);
            }
            std::string taskid = taskIdNode->Value();
            //add by lng, 1035, if taskId=0, use generated smsid, save sms, save status.
            //1034, no balance, taskId = 0.
            if( taskid != "0")
            {
                smsid = taskid;
            }
            return true;
        }
        catch(...)
        {
            //LogWarn("[NGDChannellib::parseSend] is failed.");
            return false;
        }
    }

    bool NGDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
            smsp::StateReport report;
            TiXmlDocument myDocument;
            //clean the data after the last '>'
            respone.erase(respone.find_last_of(">") + 1);
            //LogNotice("respone:[%s]", respone.data());
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogNotice("GEMS-Report myDocument Parse return false.");
                return false;
            }

            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                //LogNotice("GEMSParser::parseReport() myDocument.RootElement is null.");
                return false;
            }

            TiXmlElement *stringElement = element->FirstChildElement();
            //LogNotice("xml parseReport start!");
            while(stringElement!=NULL)
            {
                std::string type;
                type = stringElement->Value();
                if(type == "statusbox")
                {
                    TiXmlElement *mobileElement = stringElement->FirstChildElement();
                    if(NULL==mobileElement)
                        return false;
                    TiXmlNode *mobileNode = mobileElement->FirstChild();
                    if(NULL==mobileNode)
                        return false;
                    TiXmlElement *taskIdElement = mobileElement->NextSiblingElement();
                    if(NULL==taskIdElement)
                        return false;
                    TiXmlNode *taskIdNode = taskIdElement->FirstChild();
                    if(NULL==taskIdNode)
                        return false;
                    TiXmlElement *statusElement = taskIdElement->NextSiblingElement();
                    if(NULL==statusElement)
                        return false;
                    TiXmlNode *statusNode = statusElement->FirstChild();
                    if(NULL==statusNode)
                        return false;
                    TiXmlElement *receivetimeElement = statusElement->NextSiblingElement();
                    if(NULL==receivetimeElement)
                        return false;
                    TiXmlNode *receivetimeNode = receivetimeElement->FirstChild();
                    if(NULL==receivetimeNode)
                        return false;

                    TiXmlElement *errorcodeElement = receivetimeElement->NextSiblingElement();
                    if(NULL==errorcodeElement)
                        return false;
                    TiXmlNode *errorcodeNode = errorcodeElement->FirstChild();
                    if(NULL==errorcodeNode)
                        return false;

                    stringElement = stringElement->NextSiblingElement();

                    std::string smsid;
                    smsid = std::string(taskIdNode->Value());
                    std::string toMobile;
                    toMobile = mobileNode->Value();
                    std::string status;
                    status = statusNode->Value();
                    char tmp[64] = {0};
                    sprintf(tmp, "%s", receivetimeNode->Value());

                    std::string errorcode;
                    errorcode = errorcodeNode->Value();

                    Int32 nStatus = 4;
                    /* BEGIN: Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                    if(status=="10")
                        nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    else
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }
                    /* END:   Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                    if(!smsid.empty())
                    {
                        report._smsId=smsid;
                        report._reportTime=strToTime(tmp);
                        report._status=nStatus;
                        report._desc = errorcode;
                        reportRes.push_back(report);
                        //printf("reporttime[%d][%s]",report._reportTime,nowTime);
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }

            }

            return true;
        }
        catch(...)
        {
            //LogWarn("[NGDChannellib::parseReport] is failed.");
            return false;
        }
    }


    int NGDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        //LogNotice("fjx: enter so, adaptEachChannel");
        //vhead->push_back("Content-Type: test type");
        return 0;
    }


} /* namespace smsp */
