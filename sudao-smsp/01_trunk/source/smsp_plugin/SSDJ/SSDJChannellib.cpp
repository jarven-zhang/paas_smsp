#include "SSDJChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"

namespace smsp
{

    SSDJChannellib::SSDJChannellib()
    {
    }
    SSDJChannellib::~SSDJChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new SSDJChannellib;
        }
    }

    void SSDJChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool SSDJChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
            //LogNotice("[SSDJParser::parseSend] respone:%s.", respone.data());

            TiXmlDocument myDocument;
            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogError("[SSDJParser::parseSend] Parse is failed.");
                return false;
            }

            TiXmlElement* pRootElement = myDocument.RootElement();
            if(NULL==pRootElement)
            {
                //LogError("[SSDJParser::parseSend] pRootElement is NULL.");
                return false;
            }
            TiXmlElement* pStatusElement = pRootElement->FirstChildElement();
            if(NULL==pStatusElement)
            {
                //LogError("[SSDJParser::parseSend] pStatusElement is NULL.");
                return false;
            }
            TiXmlElement* pMessageElement = pStatusElement->NextSiblingElement();
            if(NULL==pMessageElement)
            {
                //LogError("[SSDJParser::parseSend] pMessageElement is NULL.");
                return false;
            }
            TiXmlNode* pMessageNode = pMessageElement->FirstChild();
            if(NULL==pMessageNode)
            {
                //LogError("[SSDJParser::parseSend] pMessageNode is NULL.");
                return false;
            }
            TiXmlElement* pRemainpointElement = pMessageElement->NextSiblingElement();
            if(NULL == pRemainpointElement)
            {
                //LogError("[SSDJParser::parseSend] pRemainpointElement is NULL.");
                return false;
            }
            TiXmlElement* pTaskIdElement = pRemainpointElement->NextSiblingElement();
            if(NULL==pTaskIdElement)
            {

                //LogError("[SSDJParser::parseSend] pTaskIdElement is NULL.");
                return false;
            }
            TiXmlNode* pTaskIdNode = pTaskIdElement->FirstChild();
            if(NULL==pTaskIdNode)
            {
                //LogError("[SSDJParser::parseSend] pTaskIdNode is NULL.");
                return false;
            }

            status = pMessageNode->Value();
            if(status == "ok" || status == "10")
            {
                status = "0";
            }
            else
            {
                strReason.assign(status);
                status = "-1";

            }
            std::string taskid = pTaskIdNode->Value();

            if( taskid != "0")
            {
                smsid = taskid;
            }
            return true;
        }
        catch(...)
        {
            //LogWarn("[SSDJChannellib::parseSend] is failed.");
            return false;
        }
    }

    bool SSDJChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
            smsp::StateReport report;
            TiXmlDocument myDocument;

            respone.erase(respone.find_last_of(">") + 1);
            //LogNotice("[SSDJParser::parseReport]respone:[%s]", respone.data());

            if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
            {
                //LogError("[SSDJParser::parseReport] Parse is failed.");
                return false;
            }

            TiXmlElement* pRootElement = myDocument.RootElement();
            if(NULL == pRootElement)
            {
                //LogError("[SSDJParser::parseReport] pRootElement is NULL.");
                return false;
            }
           
            TiXmlElement* pStringElement = pRootElement->FirstChildElement();
            while(pStringElement != NULL)
            {
                std::string type = "";
                type = pStringElement->Value();

                if(type == "statusbox")  ///状态报告解析
                {
                    TiXmlElement* pMobileElement = pStringElement->FirstChildElement();
                    if(NULL == pMobileElement)
                    {
                        //LogError("[SSDJParser::parseReport] pMobileElement is NULL.");
                        return false;
                    }

                    TiXmlNode* pMobileNode = pMobileElement->FirstChild();
                    if(NULL == pMobileNode)
                    {
                        //LogError("[SSDJParser::parseReport] pMobileNode is NULL.");
                        return false;
                    }
                    TiXmlElement* pTaskIdElement = pMobileElement->NextSiblingElement();
                    if(NULL==pTaskIdElement)
                    {
                        //LogError("[SSDJParser::parseReport] pTaskIdElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pTaskIdNode = pTaskIdElement->FirstChild();
                    if(NULL==pTaskIdNode)
                    {
                        //LogError("[SSDJParser::parseReport] pTaskIdNode is NULL.");
                        return false;
                    }
                    TiXmlElement* pStatusElement = pTaskIdElement->NextSiblingElement();
                    if(NULL==pStatusElement)
                    {
                        //LogError("[SSDJParser::parseReport] pStatusElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pStatusNode = pStatusElement->FirstChild();
                    if(NULL==pStatusNode)
                    {
                        //LogError("[SSDJParser::parseReport] pStatusNode is NULL.");
                        return false;
                    }
                    TiXmlElement* pReceivetimeElement = pStatusElement->NextSiblingElement();
                    if(NULL==pReceivetimeElement)
                    {
                        //LogError("[SSDJParser::parseReport] pReceivetimeElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pReceivetimeNode = pReceivetimeElement->FirstChild();
                    if(NULL==pReceivetimeNode)
                    {
                        //LogError("[SSDJParser::parseReport] pReceivetimeNode is NULL.");
                        return false;
                    }

                    TiXmlElement* pErrorcodeElement = pReceivetimeElement->NextSiblingElement();
                    if(NULL==pErrorcodeElement)
                    {
                        //LogError("[SSDJParser::parseReport] pErrorcodeElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pErrorcodeNode = pErrorcodeElement->FirstChild();
                    if(NULL==pErrorcodeNode)
                    {
                        //LogError("[SSDJParser::parseReport] pErrorcodeNode is NULL.");
                        return false;
                    }

                    pStringElement = pStringElement->NextSiblingElement();

                    std::string strSmsid = "";
                    strSmsid = std::string(pTaskIdNode->Value());

                    std::string strToMobile = "";
                    strToMobile = pMobileNode->Value();

                    std::string strStatus = "";
                    strStatus = pStatusNode->Value();

                    char tmp[64] = {0};
                    sprintf(tmp, "%s", pReceivetimeNode->Value());

                    std::string strErrorcode = "";
                    strErrorcode = pErrorcodeNode->Value();

                    Int32 nStatus = 4;
                    if(strStatus == "10")
                        nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    else
                    {
                        nStatus = SMS_STATUS_CONFIRM_FAIL;
                    }

                    if(!strSmsid.empty())
                    {
                        report._smsId = strSmsid;
                        report._reportTime = strToTime(tmp);
                        report._status = nStatus;
                        report._desc = strErrorcode;
                        reportRes.push_back(report);
                    }
                    else
                    {
                        //LogError("[SSDJParser::parseReport] strSmsid is empty.");
                        return false;
                    }
                }
                else if (type == "callbox")  ///上行解析
                {
                    smsp::UpstreamSms upsms;
                    TiXmlElement* pCbMobileElement = pStringElement->FirstChildElement();
                    if(NULL == pCbMobileElement)
                    {
                        //LogError("[SSDJParser::parseReport] pCbMobileElement is NULL.");
                        return false;
                    }

                    TiXmlNode* pCbMobileNode = pCbMobileElement->FirstChild();
                    if(NULL == pCbMobileNode)
                    {
                        //LogError("[SSDJParser::parseReport] pCbMobileNode is NULL.");
                        return false;
                    }

                    TiXmlElement* pCbTaskIdElement = pCbMobileElement->NextSiblingElement();
                    if(NULL==pCbTaskIdElement)
                    {
                        //LogError("[SSDJParser::parseReport] pCbTaskIdElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pCbTaskIdNode = pCbTaskIdElement->FirstChild();
                    if(NULL==pCbTaskIdNode)
                    {
                        //LogError("[SSDJParser::parseReport] pCbTaskIdNode is NULL.");
                        return false;
                    }
                    TiXmlElement* pCbContentElement = pCbTaskIdElement->NextSiblingElement();
                    if(NULL==pCbContentElement)
                    {
                        //LogError("[SSDJParser::parseReport] pStatusElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pCbContentNode = pCbContentElement->FirstChild();
                    if(NULL==pCbContentNode)
                    {
                        //LogError("[SSDJParser::parseReport] pCbContentNode is NULL.");
                        return false;
                    }
                    TiXmlElement* pCbReceivetimeElement = pCbContentElement->NextSiblingElement();
                    if(NULL==pCbReceivetimeElement)
                    {
                        //LogError("[SSDJParser::parseReport] pCbReceivetimeElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pCbReceivetimeNode = pCbReceivetimeElement->FirstChild();
                    if(NULL==pCbReceivetimeNode)
                    {
                        //LogError("[SSDJParser::parseReport] pCbReceivetimeNode is NULL.");
                        return false;
                    }

                    TiXmlElement* pExtNoElement = pCbReceivetimeElement->NextSiblingElement();
                    if (NULL == pExtNoElement)
                    {
                        //LogError("[SSDJParser::parseReport] pExtNoElement is NULL.");
                        return false;
                    }
                    TiXmlNode* pExtNoNode = pExtNoElement->FirstChild();
                    if (NULL == pExtNoNode)
                    {
                        //LogError("[SSDJParser::parseReport] pExtNoNode is NULL.");
                    }


                    pStringElement = pStringElement->NextSiblingElement();

                    std::string strCbTaskid = "";
                    strCbTaskid = std::string(pCbTaskIdNode->Value());

                    std::string strCbToMobile = "";
                    strCbToMobile = pCbMobileNode->Value();

                    std::string strCbContent = "";
                    strCbContent = pCbContentNode->Value();

                    std::string strExtNo  = "";
                    if (NULL != pExtNoNode)
                    {
                        strExtNo = pExtNoNode->Value();
                    }

                    char tmp[64] = {0};
                    sprintf(tmp, "%s", pCbReceivetimeNode->Value());

                    /* BEGIN: Modified by fuxunhao, 2015/9/1   PN:smsp_SSDJ */
                    upsms._content = strCbContent;
                    upsms._upsmsTimeStr = pCbReceivetimeNode->Value();
                    upsms._phone = strCbToMobile;
                    upsms._upsmsTimeInt= strToTime(tmp);
                    upsms._appendid = strExtNo;////.substr(strExtNo.length() - 3); /// extno get last 3 number
                    /* END:   Modified by fuxunhao, 2015/9/1   PN:smsp_SSDJ */
                    upsmsRes.push_back(upsms);

                    //LogNotice("====strCbContent:%s, strCbToMobile:%s,  tmp:%s, strCbTaskid:%s, extno:%s.",
                    //          strCbContent.data(), strCbToMobile.data(), tmp, strCbTaskid.data(), upsms._appendid.data());




                }
                else
                {
                    //LogError("[SSDJParser::parseReport] type is invalid.");
                    return false;
                }

            }

            return true;
        }
        catch(...)
        {
            //LogWarn("[SSDJChannellib::parseReport] is failed.");
            return false;
        }
    }


    int SSDJChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }


} /* namespace smsp */
