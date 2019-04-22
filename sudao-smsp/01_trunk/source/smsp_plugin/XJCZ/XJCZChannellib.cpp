#include "XJCZChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"


namespace smsp
{

    XJCZChannellib::XJCZChannellib()
    {

    }
    XJCZChannellib::~XJCZChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new XJCZChannellib;
        }
    }

    void XJCZChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool XJCZChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        //respone:<?xml version="1.0" encoding="utf-8"?><Body><result>0</result></Body>
        try
        {
            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                return false;
            }


            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
                return false;

            TiXmlElement *statusElement = element->FirstChildElement();
            std::string result = statusElement->FirstChild()->Value();
            if(!strcmp(result.data(), "0"))
            {
                //smsid = result;
                status = "0";
                return true;
            }
            else
            {
                strReason = result;
                status =result;
                return false;
            }

        }
        catch(...)
        {
            //LogWarn("[XJCZChannellib::parseSend] is failed.");
            return false;
        }
    }

    bool XJCZChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /*  respone:
        <?xml version="1.0" encoding="utf-8"?>
        <Body>
            <result>1</result>
            <report>
                <msgid>6cc14c80000a6a6800000000</msgid>
                <srctermid>13632697284</srctermid>
                <desttermid>1065710106000010</desttermid>
                <msgcontent>REVMSVZSRA==</msgcontent>
                <srctype>1</srctype>
            </report>
            <report>
                <msgid>6cc6684000027ff700000000</msgid>
                <srctermid>13632697284</srctermid>
                <desttermid>1065710106000010</desttermid>
                <msgcontent>REVMSVZSRA==</msgcontent>
                <srctype>1</srctype>
            </report>
        <Body>
        */

        try
        {
            smsp::StateReport report;
            TiXmlDocument myDocument;
            //clean the data after the last '>'
            respone.erase(respone.find_last_of(">") + 1);
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                return false;
            }

            TiXmlElement *element = myDocument.RootElement();
            if(NULL==element)
            {
                return false;
            }

            TiXmlElement *resultElement = element->FirstChildElement();     //result node

            //get all report msg
            TiXmlElement *stringElement = resultElement->NextSiblingElement();  //1th report node
            while(stringElement!=NULL)
            {
                std::string type;
                type = stringElement->Value();
                if(type == "report")
                {
                    TiXmlElement *msgidElement = stringElement->FirstChildElement();
                    if(NULL==msgidElement)
                    {
                        //LogError("msgidElement is NULL.");
                        return false;
                    }
                    TiXmlNode *msgidNode = msgidElement->FirstChild();
                    if(NULL==msgidNode)
                    {
                        //LogError("msgidNode is NULL.");
                        ///return false;
                    }

                    TiXmlElement *srctermidElement = msgidElement->NextSiblingElement();
                    if(NULL==srctermidElement)
                    {
                        //LogError("srctermidElement is NULL.");
                        return false;
                    }
                    TiXmlNode *srctermidNode = srctermidElement->FirstChild();
                    if(NULL==srctermidNode)
                    {
                        //LogError("srctermidNode is NULL.");
                        return false;
                    }

                    TiXmlElement *desttermidElement = srctermidElement->NextSiblingElement();
                    if(NULL==desttermidElement)
                    {
                        //LogError("desttermidElement is NULL.");
                        return false;
                    }
                    TiXmlNode *desttermidNode = desttermidElement->FirstChild();
                    if(NULL==desttermidNode)
                    {
                        //LogError("desttermidNode is NULL.");
                        return false;
                    }

                    TiXmlElement *msgcontentElement = desttermidElement->NextSiblingElement();
                    if(NULL==msgcontentElement)
                    {
                        //LogError("msgcontentElement is NULL.");
                        return false;
                    }
                    TiXmlNode *msgcontentNode = msgcontentElement->FirstChild();
                    if(NULL==msgcontentNode)
                    {
                        //LogError("msgcontentNode is NULL.");
                        return false;
                    }

                    TiXmlElement *srctypeElement = msgcontentElement->NextSiblingElement();
                    if(NULL==srctypeElement)
                    {
                        //LogError("srctypeElement is NULL.");
                        return false;
                    }
                    TiXmlNode *srctypeNode = srctypeElement->FirstChild();
                    if(NULL==srctypeNode)
                    {
                        //LogError("srctypeNode is NULL.");
                        return false;
                    }


                    /* BEGIN: Added by fuxunhao, 2015/11/3   PN:AutoConfig */
                    TiXmlElement *usermsgidElement = srctypeElement->NextSiblingElement();
                    if(NULL==usermsgidElement)
                    {
                        //LogError("usermsgidElement is NULL.");
                        return false;
                    }

                    TiXmlNode *usermsgidNode = usermsgidElement->FirstChild();
                    if(NULL==usermsgidNode)
                    {
                        //LogError("usermsgidNode is NULL.");
                        return false;
                    }
                    /* END:   Added by fuxunhao, 2015/11/3   PN:AutoConfig */

                    stringElement = stringElement->NextSiblingElement();

                    std::string srctermid;
                    srctermid = std::string(srctermidNode->Value());
                    std::string smsid;
                    smsid = std::string(usermsgidNode->Value());
                    //LogNotice("=======smsid:%s=======", smsid.data());
                    std::string desttermid;
                    desttermid = desttermidNode->Value();
                    char Respmsgcontent[64] = {0};
                    sprintf(Respmsgcontent, "%s", msgcontentNode->Value());

                    std::string srctype;
                    srctype = srctypeNode->Value();

                    Int32 nStatus = SMS_STATUS_CONFIRM_FAIL;
                    std::string responseRslt = smsp::Channellib::decodeBase64(Respmsgcontent);

                    if(!strcmp(responseRslt.data(), "DELIVRD"))
                    {
                        nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    }
                    else
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }


                    if(!smsid.empty())
                    {
                        report._smsId=smsid;
                        report._reportTime=time(NULL);
                        report._status=nStatus;
                        report._desc = responseRslt;
                        reportRes.push_back(report);
                    }
                    else
                    {
                        //LogError("this is error.")
                        return false;
                    }
                }
                else
                    stringElement = stringElement->NextSiblingElement();
            }

            /*
                    //get all response msg and refill msgid. the correct msgid is usrmsgid
                    stringElement = resultElement->NextSiblingElement();    //1th report node
                    while(stringElement!=NULL)
                    {
                        std::string type;
                        type = stringElement->Value();
                        if (type == "response")
                        {
                            TiXmlElement *usermsgidElement = stringElement->FirstChildElement();
                            if(NULL==usermsgidElement)
                                return false;
                            TiXmlNode *usermsgidNode = usermsgidElement->FirstChild();
                            if(NULL==usermsgidNode)
                                return false;

                            TiXmlElement *srctermidElement = usermsgidElement->NextSiblingElement();
                            if(NULL==srctermidElement)
                                return false;
                            TiXmlNode *srctermidNode = srctermidElement->FirstChild();
                            if(NULL==srctermidNode)
                                return false;

                            TiXmlElement *desttermidElement = srctermidElement->NextSiblingElement();
                            if(NULL==desttermidElement)
                                return false;
                            TiXmlNode *desttermidNode = desttermidElement->FirstChild();
                            if(NULL==desttermidNode)
                                return false;

                            TiXmlElement *msgidElement = desttermidElement->NextSiblingElement();
                            if(NULL==msgidElement)
                                return false;
                            TiXmlNode *msgidNode = msgidElement->FirstChild();
                            if(NULL==msgidNode)
                                return false;

                            TiXmlElement *srctypeElement = msgidElement->NextSiblingElement();
                            if(NULL==srctypeElement)
                                return false;
                            TiXmlNode *srctypeNode = srctypeElement->FirstChild();
                            if(NULL==srctypeNode)
                                return false;

                            stringElement = stringElement->NextSiblingElement();

                            //GET DATA
                            std::string usermsgid;
                            usermsgid = std::string(usermsgidNode->Value());

                            std::string msgid;
                            msgid = std::string(msgidNode->Value());

                            for (std::vector<smsp::StateReport>::iterator it = reportRes.begin();it != reportRes.end(); it++) {
                                if (!strcmp(it->_smsId.data(), msgid.data()))
                                {
                                    it->_smsId = usermsgid;     //refill _smsId. we should fill usermsgid
                                    break;
                                }

                            }

                        }
                        else
                            stringElement = stringElement->NextSiblingElement();

                    }   //get all response msg
            */
            /* BEGIN: Added by fuxunhao, 2015/8/28   PN:smsp_SSDJ */
            stringElement = resultElement->NextSiblingElement();    //deliver node
            while(stringElement!=NULL)
            {
                std::string type;
                type = stringElement->Value();
                if ("deliver" == type)
                {
                    TiXmlElement* srctermidElement = stringElement->FirstChildElement();
                    if (NULL == srctermidElement)
                    {
                        //LogError("[XJCZParser::parseReport] srctermidElement is NULL.");
                        return false;
                    }

                    TiXmlNode* srctermidNode = srctermidElement->FirstChild();
                    if(NULL == srctermidNode)
                    {
                        //LogError("[XJCZParser::parseReport] srctermidNode is NULL.");
                        return false;
                    }

                    TiXmlElement* desttermidElement = srctermidElement->NextSiblingElement();
                    if (NULL == desttermidElement)
                    {
                        //LogError("[XJCZParser::parseReport] desttermidElement is NULL.");
                        return false;
                    }

                    TiXmlNode* desttermidNode = desttermidElement->FirstChild();
                    if(NULL == desttermidNode)
                    {
                        //LogError("[XJCZParser::parseReport] desttermidNode is NULL.");
                        return false;
                    }

                    TiXmlElement* msgcontentElement = desttermidElement->NextSiblingElement();
                    if (NULL == msgcontentElement)
                    {
                        //LogError("[XJCZParser::parseReport] msgcontentElement is NULL.");
                        return false;
                    }

                    TiXmlNode* msgcontentNode = msgcontentElement->FirstChild();
                    if(NULL == msgcontentNode)
                    {
                        //LogError("[XJCZParser::parseReport] msgcontentNode is NULL.");
                        return false;
                    }


                    std::string srctermid = std::string(srctermidNode->Value());
                    std::string desttermid = std::string(desttermidNode->Value());
                    std::string msgcontent = std::string(msgcontentNode->Value());

                    smsp::UpstreamSms upsms;
                    upsms._appendid = desttermid;/////.substr(desttermid.length()-3);
                    upsms._phone = srctermid;
                    upsms._content = smsp::Channellib::decodeBase64(msgcontent);

                    /* BEGIN: Added by fuxunhao, 2015/9/7   PN:smsp_SSDJ */
                    time_t t = time(NULL);
                    struct tm pT =  {0};
					localtime_r(&t,&pT);
                    char strT[50] = {0};
                    strftime(strT, 49, "%Y-%m-%d %X", &pT);
                    upsms._upsmsTimeStr = strT;
                    upsms._upsmsTimeInt = strToTime(strT);
                    /* END:   Added by fuxunhao, 2015/9/7   PN:smsp_SSDJ */

                    upsmsRes.push_back(upsms);
                    //LogNotice("========desttermid[%s], srctermid[%s], msgcontent[%s].", desttermid.data(),
                    //          srctermid.data(), upsms._content.data());

                }

                stringElement = stringElement->NextSiblingElement();
            }
            /* END:   Added by fuxunhao, 2015/8/28   PN:smsp_SSDJ */

            return true;
        }
        catch(...)
        {
            //LogWarn("[XJCZChannellib::parseReport] is failed.");
            return false;
        }
    }


    int XJCZChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        //2015-09-09 add by fangjinxiong: (1)replace smsid (2)replace %content%  (3)replace http head

        //REPLACE sms->_smsid
        if(sms->m_strChannelSmsId.length()>12)
        {
            //string dst = sms->_smsid;
            sms->m_strChannelSmsId = sms->m_strChannelSmsId.substr(0, 12);    //XJCZ msgid only 12bit
        }
        else
        {
            //LogWarn("smsid is less than 12!");
        }

        //REPLACE %content%
        std::string::size_type pos = data->find("%content%");
        if (pos != std::string::npos)
        {
            data->replace(pos, strlen("%content%"), smsp::Channellib::encodeBase64(sms->m_strContent));   //xjcz¡ã?sign¡ä??¨²o¨®??
        }

        //REPLACE http head
        vhead->push_back("Content-Type:text/xml; charset=utf-8");
        vhead->push_back("Action:\"submitreq\"");

        return 0;
    }


} /* namespace smsp */
