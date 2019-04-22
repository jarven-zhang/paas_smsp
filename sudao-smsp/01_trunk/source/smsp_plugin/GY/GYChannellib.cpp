#include "GYChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "xml.h"
#include "HttpParams.h"

namespace smsp
{

    GYChannellib::GYChannellib()
    {
    }
    GYChannellib::~GYChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new GYChannellib;
        }
    }

    void GYChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool GYChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        // respone = {"code":0,"msg":"OK","result":{"taskId":"5061258999149855087"}}

        //respone = "{\"code\":0,\"msg\":\"OK\",\"result\":{{\"taskId\":\"5061258999149855087\"}}";

        //respone= "{\"code\":0,\"msg\":\"LACK PARAMETERS\",\"result\":{}}";


        try
        {
            /* BEGIN: Modified by LionGong, 2015/7/15 */
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;

            if (Json::json_format_string(respone, js) < 0)
            {
                //LogError("###GY send-response json message error###");
                return false;
            }
            if(!reader.parse(js,root))
            {
                //LogError("[GYParser::parseSend] parse is failed.")
                return false;
            }
            /* END:   Modified by LionGong, 2015/7/15 */
            ///std::string desc = root["desc"].asString();
            strReason = root["msg"].asString();     //strReason
            int result = root["code"].asInt();
            if(result!= 0)
            {
                //LogError("[GYParser::parseSend] return code error. return[%s]", respone.data());
                return false;
            }
            else
            {
                status = "0";           //status
            }

            Json::Value obj;


            if(!root["result"].isNull())
            {
                obj = root["result"];
            }
            else
            {
                //LogWarn("GY channel, format error. response[%s] ", respone.data());
                return false;
            }

            std::string taskId;
            taskId = obj["taskId"].asString();
            //LogNotice("taskId[%s]",taskId.data());

            if(taskId.empty())
            {
                //LogWarn("GY channel error, msgid is null!");
                return false;
            }
            else
            {
                smsid = taskId; //smsid
            }

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    bool GYChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse)
    {
        //贯云状态报告一次只推一条上来， 上行也是一次只推一条上来
        //u=ucpaas&p=e10adc3949ba59abbe56e057f20f883e&m=13632697284&t=5026412177130754519&d=DELIVRD&s=1     //状态报告

        //u=ucpaas&p=e10adc3949ba59abbe56e057f20f883e&m=13632697284&c=123测试2da&s=0000         //上行
        strResponse = "0"; //should return "0" to GYchannel
        //LogNotice("fjx: respone[%s]", respone.data());
        try
        {
            web::HttpParams param;
            param.Parse(respone);

            std::string strMsgid = param.getValue("t");
            std::string strDesc = param.getValue("d");
            std::string strContent = param.getValue("c");   //FOR UPSTREAM
            std::string strPhone = param.getValue("m");
            std::string strAppendID = param.getValue("s");

            if(!strContent.empty())             //UPSTREAM
            {
                smsp::UpstreamSms upsms;
                upsms._content = smsp::Channellib::urlDecode(strContent);       //_content
                if(!strPhone.empty())
                {
                    upsms._phone = strPhone;        //_phone
                }
                else
                {
                    //LogWarn("GYChannellib::parseReport, phone is null. respone[%s]", respone.data());
                }
                upsms._upsmsTimeInt= time(NULL);        //_upsmsTimeInt

                struct tm tblock = {0};
                time_t timer = time(NULL);
                localtime_r(&timer,&tblock);
                upsms._upsmsTimeStr = asctime(&tblock);  //

                //upsms._upsmsTimeStr= strToTime(upsms._upsmsTimeInt);      //_upsmsTimeStr

                if(!strAppendID.empty())
                {
                    upsms._appendid = strAppendID;      //_appendid
                }
                else
                {
                    //LogWarn("GYChannellib::parseReport, strAppendID is null. respone[%s]", respone.data());
                }

                //LogDebug("fjx: _content[%s], _phone[%s], _upsmsTimeInt[%d],  _upsmsTimeStr[%s], _appendid[%s]",
                //        upsms._content.data(), upsms._phone.data(), upsms._upsmsTimeInt, upsms._upsmsTimeStr.data(), upsms._appendid.data());
                upsmsRes.push_back(upsms);
            }
            else                            //REPORT
            {
                smsp::StateReport report;
                if(!strMsgid.empty())
                {
                    report._smsId = strMsgid;       //smsid
                }

                if(!strDesc.empty())
                {
                    report._desc = strDesc;         //desc

                    if(std::string::npos != strDesc.find("DELIVRD"))	//reportResp confirm suc
                    {
                        report._status =  SMS_STATUS_CONFIRM_SUCCESS;        //status
                    }
                    else	//reportResp confirm failed
                    {
                        report._status =  SMS_STATUS_CONFIRM_FAIL;
                    }
                }
                report._reportTime = time(NULL);
                //LogDebug("fjx: _smsId[%s], _desc[%s], _status[%d],  _reportTime[%d]",
                //         report._smsId.data(), report._desc.data(), report._status, report._reportTime);
                reportRes.push_back(report);
            }

            return true;
        }
        catch(...)
        {
            //LogWarn("[GYChannellib::parseReport] is failed.");
            return false;
        }
    }


    int GYChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        return 0;
    }




} /* namespace smsp */
