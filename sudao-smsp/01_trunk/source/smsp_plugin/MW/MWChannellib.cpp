#include "MWChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    MWChannellib::MWChannellib()
    {

    }
    MWChannellib::~MWChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new MWChannellib;
        }
    }

    void MWChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool MWChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //printf("%s\n", respone.data());
        //respone:<?xml version="1.0" encoding="utf-8"?><string xmlns="http://tempuri.org/">7759185939770092728</string>

        try
        {
            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogError("[MWParser::parseSend] Parse is failed.");
                return false;
            }


            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                //LogError("[MWParser::parseSend] element is NULL.");
                return false;
            }
            std::string result = element->FirstChild()->Value();
            if(result.length()>=10 && result.length()<=25)
            {
                smsid = result;
                status = "0";
                return true;
            }
            else
            {
                ///status =result;
                strReason = result;
                return false;
            }
        }
        catch(...)
        {
            //LogWarn("[MWChannellib::parseSend] is failed.");
            return false;
        }

    }

    bool MWChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        //respone=<?xml version="1.0" encoding="utf-8"?><ArrayOfString xmlns="http://tempuri.org/"><string>2015-06-23,14:00:09,7761955334682844252,106903290130601,0,DELIVRD</string></ArrayOfString>
        try
        {
            smsp::StateReport report;
            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                return false;
            }

            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
                return false;
            
            TiXmlElement *stringElement = element->FirstChildElement();
            while(stringElement!=NULL)
            {
                TiXmlNode *stringNode = stringElement->FirstChild();
                if(stringNode==NULL)
                    return false;
                std::string result = stringNode->Value();
                stringElement = stringElement->NextSiblingElement();
                std::vector<char*> list;
                char szResult[256] = { 0 };
                sprintf(szResult, "%s", result.data());
                split(szResult, ",", list);

                if(list.size()<6)
                    return false;

                std::string smsid = list.at(2);
                std::string errorDesc = list.at(5);
                std::string status = list.at(4);

                char reportTime[64] = {0};
                sprintf(reportTime, "%s %s",list.at(0),list.at(1));

                Int32 nStatus = atoi(status.data());
                if(nStatus == 1)
                    nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                else if(nStatus == 2)
                    nStatus = SMS_STATUS_CONFIRM_FAIL;

                report._smsId = smsid;
                report._desc = errorDesc;
                report._reportTime = MWChannellib::strToTime(reportTime);
                report._status = nStatus;

                //utfHelper->u2g(errorDesc,report._desc);
                printf("%s--%s\n", smsid.data(), errorDesc.data());
                reportRes.push_back(report);
            }
            return true;
        }
        catch(...)
        {
            //LogWarn("[MWChannellib::parseReport] is failed.");
            return false;
        }

    }


    int MWChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }



} /* namespace smsp */
