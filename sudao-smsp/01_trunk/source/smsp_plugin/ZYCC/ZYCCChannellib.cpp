#include "ZYCCChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"

namespace smsp
{

    ZYCCChannellib::ZYCCChannellib()
    {

    }
    ZYCCChannellib::~ZYCCChannellib()
    {

    }

    extern "C"
    {
        void * create()
        {
            return new ZYCCChannellib;
        }
    }

    void ZYCCChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool ZYCCChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        ///LogNotice("[ZYCCParser] parseSend respone:%s.", respone.data());
        try
        {
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;

            if (Json::json_format_string(respone, js) < 0)
            {
                //LogNotice("[ZYCCParser] json_format_string is failed.");
                return false;
            }

            if(!reader.parse(js,root))
            {
                //LogNotice("[ZYCCParser] parse is failed.");
                return false;
            }

            std::string result = root["result"].asString();
            strReason = root["reason"].asString();

            if (0 == result.compare("SUCCESS"))
            {
                status = "0";
            }
            else
            {
                status = "-1";
            }

            return true;
        }
        catch(...)
        {
            //LogError("[ZYCCParser]:parseSend is failed.");
            return false;
        }
    }

    bool ZYCCChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        //LogNotice("[ZYCCParser] parseReport respone:%s.", respone.data());

        try
        {
            strResponse.assign("{\"result\":\"SUCCESS\",\"reason\":\"\"}");
            std::vector<string> vecTmpRespone;
            std::string strTmpRespone = respone;

            if (0 == strTmpRespone.compare(0,strlen("data={{"), "data={{")) /// 推送了多条
            {
                strTmpRespone = strTmpRespone.substr(strlen("data={"));

                while(true)
                {
                    std::string::size_type posBegin = strTmpRespone.find_first_of('{');
                    if (posBegin == std::string::npos)
                    {
                        break;
                    }

                    std::string::size_type posEnd = strTmpRespone.find_first_of('}');
                    if (posEnd == std::string::npos)
                    {
                        //LogError("[ZYCCParser] parseReport respone format is failed.");
                        return false;
                    }

                    std::string strTmp = strTmpRespone.substr(posBegin, posEnd - posBegin + 1);
                    vecTmpRespone.push_back(strTmp);
                    strTmpRespone = strTmpRespone.substr(posEnd+1);

                    ///LogNotice("============respone strTmp:%s.", strTmp.data());
                }
            }
            else  /// 推送了一条
            {
                std::string strTmp = strTmpRespone.substr(strlen("data="));
                vecTmpRespone.push_back(strTmp);
                ///LogNotice("============respone strTmp:%s.", strTmp.data());
            }

            std::vector<string>::iterator itr = vecTmpRespone.begin();
            for (; itr != vecTmpRespone.end(); ++itr)
            {
                smsp::StateReport report;
                Json::Reader reader(Json::Features::strictMode());
                Json::Value root;
                std::string js;
                if (Json::json_format_string(*itr, js) < 0)
                {
                    //LogError("[ZYCCParser] parseReport json_format_string(%s) is failed.", itr->data());
                    return false;
                }

                if(!reader.parse(js,root))
                {
                    //LogError("[ZYCCParser] parseReport parse is failed.");
                    return false;
                }

                utils::UTFString utfHelper = utils::UTFString();

                std::string strUid = root["uid"].asString();
                std::string strphone = root["mobile"].asString();
                std::string strDateTime = root["datetime"].asString();
                std::string strStatus = root["status"].asString();
                std::string strDesc = root["desc"].asString();
                char tmp[64] = {0};
                sprintf(tmp, "%s", strDateTime.data());

                report._smsId = strUid;
                utfHelper.u2g(strDesc,report._desc);
                report._reportTime=strToTime(tmp);

                if (0 != strStatus.compare("0"))
                {
                    report._status= SMS_STATUS_CONFIRM_FAIL;
                }
                else
                {
                    report._status = SMS_STATUS_CONFIRM_SUCCESS;
                }
                reportRes.push_back(report);
            }

            return true;
        }
        catch(...)
        {
            //LogError("[ZYCCParser] parseReport is failed.");
            return false;
        }

    }


    int ZYCCChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        //REPLACE %uid%
        int pos = data->find("%uid%");
        if(pos!=std::string::npos)
        {
            data->replace(pos, strlen("%uid%"), sms->m_strChannelSmsId);
        }

        //REPLACE %extend%
        pos = data->find("%extend%");
        if(pos!=std::string::npos)
        {
            data->replace(pos, strlen("%extend%"), sms->m_strChannelSmsId);        //need fix?  extend should not replace by _smsid. it should replace by _displaynum
        }

        //REPLACE url
        std::string strAppKey = "";
        pos = url->find_last_of('/');
        if(pos!=std::string::npos)
        {
            strAppKey = url->substr(pos+1);
        }

        std::string strSign = "";
        getMD5ForZY(strAppKey, sms->m_strPhone, sms->m_strChannelSmsId, sms->m_strContent, sms->m_lSendDate, strSign);

        char tmpUrl[2048] = {0};
        sprintf(tmpUrl, "%s?timestamp=%d&sign=%s", url->data(), sms->m_lSendDate, strSign.data());
        //string strPostUrl = "";
        url->assign(tmpUrl);

        //url = strPostUrl;

        return 0;
    }


} /* namespace smsp */
