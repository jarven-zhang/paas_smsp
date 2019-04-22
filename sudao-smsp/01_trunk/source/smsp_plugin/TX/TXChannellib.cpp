#include "TXChannellib.h"
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include <cstring>
#include <math.h>
#include <map>

namespace smsp
{
//typedef unsigned long   UTF32;  /* at least 32 bits */
//typedef unsigned short  UTF16;  /* at least 16 bits */
//typedef unsigned char   UTF8;   /* typically 8 bits */
//typedef unsigned int    INT;

const std::string PSWD = "120C4730";
const std::string REPORT_HEAD = "report=";

TXChannellib::TXChannellib()
{

}

TXChannellib::~TXChannellib()
{

}

extern "C"
{
void * create()
{
    return new TXChannellib;
}
}

bool TXChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{
    /*
         *    成功： result=0&faillist=失败列表&balance=当前余额&linkid=网关序号&description=错误描述
              失败： result=1&faillist=失败列表&description=错误描述
        */
    try
    {
        std::string::size_type pos = respone.find("result");
        if (pos == std::string::npos)
        {
            //LogError("[%s] this is error format.", respone.data());
            return false;
        }

        std::map<std::string,std::string> mapSet;
        split_Ex_Map(respone, "&", mapSet);

        std::map<std::string,std::string>::iterator itr = mapSet.find("result");
        if (itr == mapSet.end())
        {
            //LogError("[%s] this is error format.", respone.data());
            return false;
        }

        if (0 == itr->second.compare("0"))
        {
            status = "0";
            smsid = mapSet["linkid"];
        }
        else
        {
            status = "-1";
        }

        string src_content = mapSet["description"];
        string tmp_content = smsp::Channellib::urlDecode(src_content);

        utils::UTFString utfHelper;
        utfHelper.gb23122u(tmp_content, strReason);

        return true;
    }
    catch(...)
    {
        //LogError("[TXChannellib]:parseSend is failed.");
        return false;
    }
}

bool TXChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,
                               std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
{
    /*
         *  up    : sender=13480690519&receiver=&content=%E5%93%88%E5%93%88%E5%93%88%E5%93%88&revceivtime=2019-04-16 16:15:20
         *
         *  report: report=13900000002,DELIVRD,2010-01-06 00:04,22DA53D4D87948BAB37EE04B060D894D
         *          手机号+,   +发送状态+, +接收时间(精确到分)+,+网关序号
        */

    try
    {
        //上行短信才有"sender"
        std::string::size_type pos = respone.find("sender");
        if (pos != std::string::npos)//up
        {
            smsp::UpstreamSms upsms;

            std::map<std::string,std::string> upSet;
            split_Ex_Map(respone, "&", upSet);

            upsms._content      = upSet["content"];
            upsms._phone        = upSet["sender"];
            upsms._upsmsTimeStr = upSet["revceivtime"];
            //upsms._appendid     = superNumber;

            tm tm_time;
            strptime(upSet["revceivtime"].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
            upsms._upsmsTimeInt = mktime(&tm_time);

            upsmsRes.push_back(upsms);

            return true;
        }

        //report
        smsp::StateReport report;
        string report_respone = respone;
        pos = respone.find(REPORT_HEAD);
        pos += strlen(REPORT_HEAD.data());

        report_respone = respone.substr(pos);

        std::vector<std::string> AllsmsReport;
        std::vector<std::string> vectorOneReport;
        splitExVectorSkipEmptyString(report_respone, ";", AllsmsReport);

        for(int i = 0; i < AllsmsReport.size(); i++)
        {
            vectorOneReport.clear();
            splitExVectorSkipEmptyString(AllsmsReport[i], ",", vectorOneReport);

            Int32 nStatus = 0;
            if(!vectorOneReport[1].compare("DELIVRD")) //report success
            {
                nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                strResponse = "SUCCESS";
            }
            else //failed
            {
                nStatus = SMS_STATUS_CONFIRM_FAIL;
            }

            report._smsId = vectorOneReport[3];
            report._phone = vectorOneReport[0];
            report._status = nStatus;
            report._desc = vectorOneReport[1];

            tm tm_time;
            strptime(vectorOneReport[2].data(), "%Y-%m-%d %H:%M:%S", &tm_time);
            report._reportTime = mktime(&tm_time);

            reportRes.push_back(report);
        }
        return true;
    }
    catch(...)
    {
        //LogError("[TXChannellib] parseReport is failed.");
        return false;
    }

}

int TXChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)
{
    //username=%username%&password=%password%&mobiles=%mobiles%&content=%content%&timestamp=%timestamp%

    if(NULL == data || NULL == sms){
        return -1;
    }

    vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=gb2312");

    std::string::size_type pos = (*data).find("%mobiles%");
    if(pos != std::string::npos)
    {
        (*data).replace(pos, strlen("%mobiles%"), sms->m_strPhone);
    }

    string strOut = "";
    pos = (*data).find("%content%");
    if(pos != std::string::npos)
    {
        string tmp_content = smsp::Channellib::urlDecode(sms->m_strContent);

        string gb2321_str = "";
        utils::UTFString utfHelper;
        utfHelper.u2gb2312(tmp_content, gb2321_str);

        strOut = smsp::Channellib::urlEncode(gb2321_str);

        (*data).replace(pos, strlen("%content%"), strOut);
    }

    char szTime[64] = { 0 };
    pos = (*data).find("%timestamp%");
    if(pos != std::string::npos)
    {
        tm ptm = {0};
        time_t now = time(NULL);
        localtime_r((const time_t *) &now, &ptm);

        strftime(szTime, sizeof(szTime), "%Y-%m-%d %H:%M:%S", &ptm);

        (*data).replace(pos, strlen("%timestamp%"), szTime);
    }

    //密码用MD5加密
    pos = (*data).find("%password%");
    if(pos != std::string::npos)
    {
        string input = PSWD;
        input += szTime;

        strOut = CalMd5(input);

        (*data).replace(pos, strlen("%password%"), strOut);
    }

    return 0;
}

#if 0
int TXChannellib::signatureMoveToTail(string& destSrc)
{
    if(destSrc.empty())
    {
        return -1;
    }

    int pos = destSrc.find("]");
    if(pos == std::string::npos)
    {
        pos = destSrc.find("】");
        if(pos == std::string::npos)
        {
            return -1;
        }
        pos += 3;
    }
    else
    {
        pos++;
    }

    string src_sign    = destSrc.substr(0, pos);
    string src_content = destSrc.substr(pos);

    destSrc = src_content + src_sign;

    return 0;
}
#endif

} /* namespace smsp */
