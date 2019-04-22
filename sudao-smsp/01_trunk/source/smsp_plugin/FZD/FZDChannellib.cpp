#include "FZDChannellib.h"
#include <stdio.h>
#include <time.h>
#include "json/json.h"
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "HttpParams.h"

namespace smsp
{
	extern "C"
    {
        void * create()
        {
            return new FZDChannellib;
        }
    }

    FZDChannellib::FZDChannellib()
    {

    }
    FZDChannellib::~FZDChannellib()
    {

    }

    void FZDChannellib::test()
    {
        //LogNotice("fjx:  in lib  test ok!");

    }

    bool FZDChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
    {
        try
        {
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;

            if (Json::json_format_string(respone, js) < 0)
            {
                //LogNotice("[FZDParser] json_format_string is failed.");
                return false;
            }

            if(!reader.parse(js,root))
            {
                //LogNotice("[FZDParser] parse is failed.");
                return false;
            }

            int iRet = root["ret"].asInt();
            strReason = root["msg"].asString();
            smsid = root["data"].asString();

            char sTmp[250] = {0};
            sprintf(sTmp, "%d", iRet);

            status.assign(sTmp);

            return true;
        }
        catch(...)
        {
            //LogError("[FZDParser]:parseSend is failed.");
            return false;
        }
    }

    bool FZDChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
    {
        try
        {
            smsp::StateReport report;
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            std::string js;
            if (Json::json_format_string(respone, js) < 0)
            {
                //LogError("FZDParser report json message error");
                return false;
            }

            if(!reader.parse(js,root))
            {
                //LogError("[FZDParser::parseReport] parse is failed.")
                return false;
            }

            utils::UTFString utfHelper = utils::UTFString();
            int iRet = root["ret"].asInt();

            Json::Value obj;
            if(!root["data"].isNull())
                obj = root["data"];

            for(int i = 0; i < obj.size(); i++)
            {
                Json::Value tmpValue = obj[i];

                std::string reportTime = tmpValue["settime"].asString();
                int status = tmpValue["status"].asInt();
                std::string desc = tmpValue["desc"].asString();
                std::string smsid = tmpValue["id"].asString();

                if (10 == status)	//reportResp confirm suc
                {
                    status = SMS_STATUS_CONFIRM_SUCCESS;
                }
                else	//reportResp confirm failed
                {
                    status = SMS_STATUS_CONFIRM_FAIL;
                }

                char tmp[64] = {0};
                sprintf(tmp, "%s", reportTime.data());
                report._smsId=smsid;
                utfHelper.u2g(desc,report._desc);
                report._desc=desc;
                report._reportTime=strToTime(tmp);
                report._status=status;

                reportRes.push_back(report);
            }

            return true;
        }
        catch(...)
        {
            //LogNotice("FZDParser::parseReport is failed.");
            return false;
        }

    }


    int FZDChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
    {

        //REPLACE %MD5%
        web::HttpParams param;
        param.Parse(*data);
        int pos;
        pos = data->find_first_of('&');
        if (pos != std::string::npos)
        {
            //std::string tmp;
            //tmp = data->substr(pos + 1);
            data->assign(data->substr(pos + 1));
        }

        ///_smsId = UrlCode::UrlDecode(param._map["smsId"]);
        std::string strSecretKey = smsp::Channellib::urlDecode(param._map["SecretKey"]);
        std::string strUserId = smsp::Channellib::urlDecode(param._map["UserID"]);
        std::string strContent = smsp::Channellib::urlDecode(sms->m_strContent);

        std::string strSign = "";
        strSign.append("Content=");
        strSign.append(strContent);
        strSign.append("&");
        strSign.append("Sendto=");
        strSign.append(sms->m_strPhone);
        strSign.append("&");
        strSign.append("Serialno=");
        strSign.append(sms->m_strChannelSmsId);
        strSign.append("&");
        strSign.append("TplId=0&");
        strSign.append("UserID=");
        strSign.append(strUserId);
        strSign.append("^");
        strSign.append(strSecretKey);

        unsigned char md5[16] = {0};
        MD5((const unsigned char*) strSign.data(), strSign.length(), md5);

        std::string strMD5 = "";
        std::string HEX_CHARS = "0123456789ABCDEF";

        for (int i = 0; i < 16; i++)
        {
            strMD5.append(1, HEX_CHARS.at(md5[i] >> 4 & 0x0F));
            strMD5.append(1, HEX_CHARS.at(md5[i] & 0x0F));
        }

        pos = data->find("%MD5%");
        if (pos != std::string::npos)
        {
            data->replace(pos, strlen("%MD5%"), strMD5);
        }

        return 0;
    }


} /* namespace smsp */
