#include "GDChannellib.h"
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
            return new GDChannellib;
        }
    }
    
    GDChannellib::GDChannellib()
    {
    }
    GDChannellib::~GDChannellib()
    {

    }

    void GDChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }


	
    bool GDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //printf("%s\n", respone.data());
        try
        {
            respone.erase(respone.find_last_not_of("\r\t\n ") + 1);

            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogError("[GDParser::parseSend] Parse is failed.");
                return false;
            }
            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                //LogError("[GDParser::parseSend] element is NULL.")
                return false;
            }

            TiXmlElement *statusElement = element->FirstChildElement();
            if(NULL==statusElement)
            {
                //LogError("[GDParser::parseSend] statusElement is NULL.")
                return false;
            }

            TiXmlElement *messageElement = statusElement->NextSiblingElement();
            if(NULL==messageElement)
            {
                //add by lng,  submit error, save  err code. "11" no balance.
                TiXmlNode *statusNode = statusElement->FirstChild();
                if(NULL==statusNode)
                {
                    //LogError("[GDParser::parseSend] statusNode is NULL.")
                    return false;
                }
                status = statusNode->Value();

                if(status == "03")
                {
                    status = "0"; //success.
                }

                strReason = status;

                return false;
            }

            TiXmlElement *mobileElement = messageElement->FirstChildElement();
            if(NULL==mobileElement)
            {
                //LogError("[GDParser::parseSend] mobileElement is NULL.")
                return false;
            }
            TiXmlElement *msgidElement = mobileElement->NextSiblingElement();
            if(NULL==msgidElement)
            {
                //LogError("[GDParser::parseSend] msgidElement is NULL.")
                return false;
            }
            TiXmlNode *statusNode = statusElement->FirstChild();
            if(NULL==statusNode)
            {
                //LogError("[GDParser::parseSend] statusNode is NULL.")
                return false;
            }
            TiXmlNode *msgidNode = msgidElement->FirstChild();
            if(NULL==msgidNode)
            {
                //LogError("[GDParser::parseSend] msgidNode is NULL.")
                return false;
            }

            status = statusNode->Value();
            smsid = msgidNode->Value();

            if(status == "03")
            {
                status = "0";
            }
            strReason = status;

            return true;

            /*
            std::vector<char*> list;
            char szResult[256] = { 0 };
            respone.erase(respone.find_last_not_of("\r\t\n ") + 1);

            sprintf(szResult, "%s", respone.data());
            split(szResult, ",", list);
            status = list.at(0);
            if(list.size()<2)
                return false;
            smsid = list.at(1);

            return true;
            */
        }
        catch(...)
        {
            //LogWarn("[GDChannellib::parseSend] is failed.");
            return false;
        }
    }

    bool GDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        //printf("%s\n", respone.data());
        //args=2%2C18576685840%2C0%2CD3898315051311351700%2C20150513113520%3B@
        //args=2%2C18134908606%2C0%2CE2193915051300004000%2C20150513000044%3B3@
        //args=2%2C13338697064%2C0%2CD0862015051300003000%2C20150513000035%3B
        try
        {
            strResponse.assign("ok");
            smsp::StateReport report;
            smsp::UpstreamSms upsms;
            int nStatus;

            std::vector<char*> reportslist;
            std::vector<char*> infolist;
            //char szResult[20480] = { 0 };
            char* szResult = new char[respone.length()+1];
            memset(szResult, 0x0, respone.length()+1);


            int nPos;
            if (std::string::npos != (nPos = respone.find("=")))
                respone = respone.substr(nPos + 1, respone.length());

            sprintf(szResult, "%s", respone.data());
            split(szResult, ";", reportslist);
            size_t reportsize = reportslist.size();

            if(reportsize==0)
            {
                delete []szResult;
                return false;
            }

            std::string type;
            for(int i=0; i<reportsize; i++)
            {
                sprintf(szResult, "%s", reportslist.at(i));
                infolist.clear();
                split(szResult, ",", infolist);
                if(infolist.size()<5)
                    continue;
                //add by lng.
                type = infolist.at(0);
                if(type == "2")     //report.
                {
                    std::string state = infolist.at(2);
                    /* BEGIN: Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                    if(state=="0")	//reportResp confirm suc
                    {
                    	nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    }
                    else	//reportResp confirm failed
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }
                    /* END:   Modified by fuxunhao, 2015/12/3   PN:Batch Send Sms */
                    report._smsId = infolist.at(3);
                    report._status = nStatus;
                    report._reportTime = strToTime(infolist.at(4));
                    report._desc = state;
					report._phone = infolist.at(1);
                    reportRes.push_back(report);
                }
                else if(type == "0")    //upstream.
                {
                    upsms._phone = infolist.at(1);
                    upsms._appendid= infolist.at(2);
                    //std::string content_= web::UrlCode::UrlDecode(infolist.at(3));  //GD content is gbk
					std::string content_= smsp::Channellib::urlDecode(infolist.at(3));
                    utils::UTFString utfHelper = utils::UTFString();
                    utfHelper.g2u(content_, upsms._content);   //GBK TO UTF8
                    //LogNotice("fjx:GD up stream content[%s]", upsms._content.data());
                    upsms._upsmsTimeStr= infolist.at(4);
                    upsms._upsmsTimeInt= strToTime(infolist.at(4));
                    upsmsRes.push_back(upsms);
                }
            }

            delete []szResult;
            return true;
        }
        catch(...)
        {
            //LogWarn("[GDChannellib::parseReport] is failed.");
            return false;
        }
    }
	
    int GDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }


} /* namespace smsp */
