#include "JXHTTPChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    JXHTTPChannellib::JXHTTPChannellib()
    {
    }
    JXHTTPChannellib::~JXHTTPChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new JXHTTPChannellib;
        }
    }

    void JXHTTPChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool JXHTTPChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        /*
        <?xml version="1.0" encoding="utf-8" ?><returnsms>
         <returnstatus>Success</returnstatus>
         <message>ok</message>
         <remainpoint>97</remainpoint>
         <taskID>31967797</taskID>
         <successCounts>1</successCounts></returnsms>
         */

        TiXmlDocument myDocument;

        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))       //校验XML格式合法性
        {
            //LogNotice("JXHTTPChannellib respone not illegal,return");
            return false;
        }

        TiXmlElement *element = myDocument.RootElement();       //获取根节点
        if(NULL==element)
        {
            //LogNotice("JXHTTPChannellib get root element error,return");
            return false;
        }

        //TiXmlElement *returnsmsElement = element->FirstChildElement();        //获取第一个子节点,returnsms节点

        if(element!=NULL)
        {
            std::string type;
            type = element->Value();
            if(type == "returnsms")
            {
                TiXmlElement *returnstatusElement = element->FirstChildElement();       //获取returnsms 下的第一个子节点<returnstatus>Success</returnstatus>
                if(NULL==returnstatusElement)
                {
                    //LogNotice("JXHTTPChannellib get returnstatusElement error,return");
                    return false;
                }
                TiXmlNode *returnstatusNode = returnstatusElement->FirstChild();
                if(NULL==returnstatusNode)
                {
                    //LogNotice("JXHTTPChannellib get returnstatusNode error,return");
                    return false;
                }

                TiXmlElement *messageElement = returnstatusElement->NextSiblingElement();   //获取returnsms 后面的一个子节点 <message>ok</message>
                if(NULL==messageElement)
                {
                    //LogNotice("JXHTTPChannellib get messageElement error,return");
                    return false;
                }
                TiXmlNode *messageNode = messageElement->FirstChild();
                if(NULL==messageNode)
                {
                    //LogNotice("JXHTTPChannellib get messageNode error,return");
                    return false;
                }

                TiXmlElement *remainpointElement = messageElement->NextSiblingElement();    //  <remainpoint>97</remainpoint>
                if(NULL==remainpointElement)
                {
                    //LogNotice("JXHTTPChannellib get remainpointElement error,return");
                    return false;
                }
                TiXmlNode *remainpointNode = remainpointElement->FirstChild();
                if(NULL==remainpointNode)
                {
                    //LogNotice("JXHTTPChannellib get remainpointNode error,return");
                    return false;
                }

                TiXmlElement *taskIDElement = remainpointElement->NextSiblingElement();     //<taskID>31967797</taskID>
                if(NULL==taskIDElement)
                {
                    //LogNotice("JXHTTPChannellib get taskIDElement error,return");
                    return false;
                }
                TiXmlNode *taskIDNode = taskIDElement->FirstChild();
                if(NULL==taskIDNode)
                {
                    //LogNotice("JXHTTPChannellib get taskIDNode error,return");
                    return false;
                }

                TiXmlElement *successCountsElement = taskIDElement->NextSiblingElement();   //<successCounts>1</successCounts></returnsms>
                if(NULL==successCountsElement)
                {
                    //LogNotice("JXHTTPChannellib get successCountsElement error,return");
                    return false;
                }
                TiXmlNode *successCountsNode = successCountsElement->FirstChild();
                if(NULL==successCountsNode)
                {
                    //LogNotice("JXHTTPChannellib get successCountsNode error,return");
                    return false;
                }


                std::string returnstatus = std::string(returnstatusNode->Value());  //[status]Success  or Faild

                std::string message = messageNode->Value();         //[desc]ok/包含非法字符：/其他错误 ......

                std::string remainpoint = remainpointNode->Value(); //no use

                std::string taskID = taskIDNode->Value();       //[msgID]

                std::string successCounts = successCountsNode->Value(); //no use

                if(returnstatus.find("Success") != std::string::npos)
                {
                    status = "0";
                }
                else
                {
                    status = "4";
                }

                strReason = message;

                if(!taskID.empty())
                {
                    smsid = taskID;
                }
                else
                {
                    //LogWarn("JXHTTP channel, taskID is null, SendResponse[%s]", respone.data());
                    return false;
                }

            }
            else
            {
                //LogWarn("JXHTTP channel，SendResponse format err. sendResponse[%s]", respone.data());
            }
        }

        return true;
    }

    bool JXHTTPChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        /*
        <?xml version="1.0" encoding="utf-8" ?>
        <returnsms>
            <statusbox>
                <mobile>15023239810</mobile>-------------对应的手机号码
                <taskid>1212</taskid>-------------同一批任务ID
                <status>10</status>---------状态报告----10：发送成功，20：发送失败
                <receivetime>2011-12-02 22:12:11</receivetime>-------------接收时间
                <errorcode>DELIVRD</errorcode>-上级网关返回值，不同网关返回值不同，仅作为参考
            </statusbox>
            <statusbox>
                <mobile>15023239811</mobile>
                <taskid>1212</taskid>
                <status>20</status>
                <receivetime>2011-12-02 22:12:11</receivetime>
                <errorcode>2</errorcode>
            </statusbox>
        </returnsms>
        */

        smsp::StateReport report;
        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(),0,TIXML_DEFAULT_ENCODING))
        {
            //LogNotice("JXHTTPChannel, SendRespone format err. SendRespone[%s]", respone.data());
            return false;
        }

        TiXmlElement *element = myDocument.RootElement();
        if(NULL==element)
        {
            return false;
        }

        TiXmlElement *statusboxElement = element->FirstChildElement();      //result node

        //get all report msg
        //TiXmlElement *stringElement = resultElement->NextSiblingElement();    //1th report node
        while(statusboxElement!=NULL)
        {
            std::string type;
            type = statusboxElement->Value();
            if(type == "statusbox")
            {
                TiXmlElement *mobileElement = statusboxElement->FirstChildElement();
                if(NULL==mobileElement)
                    return false;
                TiXmlNode *mobileNode = mobileElement->FirstChild();
                if(NULL==mobileNode)
                    return false;

                TiXmlElement *taskidElement = mobileElement->NextSiblingElement();
                if(NULL==taskidElement)
                    return false;
                TiXmlNode *taskidNode = taskidElement->FirstChild();
                if(NULL==taskidNode)
                    return false;

                TiXmlElement *statusElement = taskidElement->NextSiblingElement();
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

                statusboxElement = statusboxElement->NextSiblingElement();

                std::string mobile = std::string(mobileNode->Value());

                std::string taskid = taskidNode->Value();

                std::string status = statusNode->Value();

                std::string receivetime = receivetimeNode->Value();

                std::string errorcode = errorcodeNode->Value();


                Int32 nStatus = 4;

                if(!strcmp(status.data(), "10"))
                {
                    nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else
                {
                    nStatus = SMS_STATUS_CONFIRM_FAIL;
                }


                if(!taskid.empty())
                {
                    report._smsId=taskid;
                    report._reportTime=time(NULL);
                    report._status=nStatus;
                    report._desc = errorcode;
                    reportRes.push_back(report);
                }
                else
                    return false;
            }
            else
                statusboxElement = statusboxElement->NextSiblingElement();
        }
        return true;
    }


    int JXHTTPChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {
        //do nothing
        return 0;
    }

} /* namespace smsp */
