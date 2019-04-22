#include "YFChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"
#include "HttpParams.h"


namespace smsp
{

    YFChannellib::YFChannellib()
    {
    }

    YFChannellib::~YFChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new YFChannellib;
        }
    }

    void YFChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool YFChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //num=2&success=1393710***4,1393710***5&faile=&err=发送成功&errid=0
        /*
            36
            num=1&success=13632697284,&faile=&err=·￠?3?|&errid=0
            0
        */

        //std::string rpt;


        try
        {
            std::string::size_type pos = respone.find_first_of('\n');
            if (pos != std::string::npos)
            {
                respone = respone.substr(pos + 1);
            }
            else
            {
                //LogNotice("YF report find no huiche");
                return false;
            }

            //LogNotice("fjx:sendreponse1[%s]", respone.data());
            respone.erase(respone.find_last_not_of("\n0\n ") + 1);

            //LogNotice("fjx:sendreponse[%s]", respone.data());


            web::HttpParams param;
            param.Parse(respone);

            std::string strNum = param.getValue("num");
            std::string strSuccPhone = param.getValue("success");
            std::string strFailePhone = param.getValue("faile");    //FOR UPSTREAM
            std::string strErr = param.getValue("err");
            std::string strErrid = param.getValue("errid");

            if(strNum != "1")   //我们每次只下发一条，所以这里必须是1，否则就是失败
            {
                status = "1";
            }
            else
            {
                status = "0";
            }

            utils::UTFString utfHelper = utils::UTFString();
            utfHelper.g2u(strErr, strReason);  //GBK TO UTF8

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool YFChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse)
    {
        /*
            &data=<?xml version="1.0" encoding="utf-8"?>
                <BODY>
                    <report>
                        <deliverId>ihg21pcz-6q2n</deliverId>
                        <sequeid>1448531138136326972841</sequeid>
                        <code>0</code>
                        <info>DELIVRD</info>
                        <msg></msg>
                        <mobile>13632697284</mobile>
                        <spnumber>10690129886186</spnumber>
                        <sdst>886186</sdst>
                        <gwdt>151126174551</gwdt>
                    </report>
                </BODY>75ca
        */
        //respone.erase(respone.find_last_not_of("\r\t\n ") + 1);


        try
        {
            strResponse = "0"; //should return "0" to YFchannel
            respone = smsp::Channellib::urlDecode(respone);

            std::string::size_type pos = respone.find_first_of("data=");
            if (pos != std::string::npos)
            {
                respone = respone.substr(pos + 5);
            }
            else
            {
                //LogNotice("yf report find no data=");
                return false;
            }
            respone.erase(respone.find_last_not_of("BODY> ") + 5);

            smsp::StateReport report;
            TiXmlDocument myDocument;

            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                return false;
            }

            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                return false;
            }
            TiXmlElement *stringElement = element->FirstChildElement();     //result node

            while(stringElement!=NULL)
            {
                std::string type;
                type = stringElement->Value();
                if(type == "report")
                {
                    TiXmlElement *deliverIdElement = stringElement->FirstChildElement();
                    if(NULL == deliverIdElement)
                    {
                        //LogError("deliverIdElement is NULL.");
                        return false;
                    }
                    TiXmlNode *deliverIdNode = deliverIdElement->FirstChild();
                    if(NULL==deliverIdNode)
                    {
                        //LogError("deliverIdNode is NULL.");
                        ///return false;
                    }

                    TiXmlElement *sequeidElement = deliverIdElement->NextSiblingElement();
                    if(NULL==sequeidElement)
                    {
                        //LogError("sequeidElement is NULL.");
                        return false;
                    }
                    TiXmlNode *sequeidNode = sequeidElement->FirstChild();
                    if(NULL==sequeidNode)
                    {
                        //LogError("sequeidNode is NULL.");
                        return false;
                    }

                    TiXmlElement *codeElement = sequeidElement->NextSiblingElement();
                    if(NULL==codeElement)
                    {
                        //LogError("codeElement is NULL.");
                        return false;
                    }
                    TiXmlNode *codeNode = codeElement->FirstChild();
                    if(NULL==codeNode)
                    {
                        //LogError("codeNode is NULL.");
                        return false;
                    }

                    TiXmlElement *infoElement = codeElement->NextSiblingElement();
                    if(NULL==infoElement)
                    {
                        //LogError("infoElement is NULL.");
                        return false;
                    }
                    TiXmlNode *infoNode = infoElement->FirstChild();
                    if(NULL==infoNode)
                    {
                        //LogError("infoNode is NULL.");
                        return false;
                    }

                    TiXmlElement *msgElement = infoElement->NextSiblingElement();
                    if(NULL==msgElement)
                    {
                        //LogError("srctypeElement is NULL.");
                        return false;
                    }
                    TiXmlNode *msgNode = msgElement->FirstChild();
                    /*
                    if(NULL==msgNode)
                    {
                        LogError("msgNode is NULL.");
                        return false;
                    }
                    */

                    TiXmlElement *mobileElement = msgElement->NextSiblingElement();
                    if(NULL==mobileElement)
                    {
                        //LogError("mobileElement is NULL.");
                        return false;
                    }
                    TiXmlNode *mobileNode = mobileElement->FirstChild();
                    if(NULL==mobileNode)
                    {
                        //LogError("mobileNode is NULL.");
                        return false;
                    }


                    stringElement = stringElement->NextSiblingElement();

                    std::string sequeid;
                    sequeid = std::string(sequeidNode->Value());
                    std::string code;
                    code = std::string(codeNode->Value());
                    //LogNotice("=======smsid:%s=======", sequeid.data());
                    std::string info;
                    info = infoNode->Value();


                    int nStatus = -1;
                    if(code != "0")
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }
                    else
                    {
                        nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    }

                    if(!sequeid.empty())
                    {
                        report._smsId=sequeid;
                        report._reportTime=time(NULL);
                        report._status=nStatus;
                        report._desc = info;
                        reportRes.push_back(report);
                    }
                    else
                    {
                        //LogError("sequeid is error.")
                        return false;
                    }
                }
                else
                    stringElement = stringElement->NextSiblingElement();
            }


            return true;
        }
        catch(...)
        {
            //LogWarn("[GYChannellib::parseReport] is failed.");
            return false;
        }
    }


    int YFChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }

} /* namespace smsp */
