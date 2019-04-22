#include "XHRChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "UTFString.h"
#include "xml.h"
#include "UTFString.h"
#include <map>
#include <vector>

namespace smsp
{

extern "C"
{
    void * create()
    {
        return new XHRChannellib;
    }
}

unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y=0;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else
    {
    }
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
//            if(i + 2 < length){
//              LogCrit("UrlDecode Err");
//              return "";
//            }
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

/*
 *copy from YTWL by zhh, change to HEX string
*/
std::string XHRChannellib::encode(const std::string& str)
{
    std::string strTemp = "";

    for (size_t i = 0; i < str.length(); i++)
    {
        strTemp += ToHex((unsigned char)str[i] >> 4);
        strTemp += ToHex((unsigned char)str[i] % 16);
    }

    return strTemp;
}

XHRChannellib::XHRChannellib()
{

}

XHRChannellib::~XHRChannellib()
{

}

bool XHRChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{
    try
    {
        long result = atol(respone.data());

        if(result > 0)
        {
            status = "0";

            smsid  = respone;

            return true;
        }

        status = "-1";

        strReason = respone;
    }
    catch(...)
    {
        return false;
    }

    return false;

}

bool XHRChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes,std::string& strResponse)
{
    /*
     *Report:
    <DR>
        <MSG mobile="" status="" msgid="" submit_date="" done_date="" />
        <MSG mobile="" status="" msgid="" submit_date="" done_date="" />
        <MSG mobile="" status="" msgid="" submit_date="" done_date="" />
        <MSG mobile="" status="" msgid="" submit_date="" done_date="" />
    </DR>

        mobile 接收者手机号, string
        status 短信状态，string
        msgid 短信唯一ID, 用户在短信提交时返回给用户的一个数字串msgid, string
        submit_date 短信提交时间, string
        done_date 完成时间, string
----------------------------------------------------------------------------------------------
    <MO>
       <MSG mobile="" cc="" spnumber="" message="" id="" time="" codec="utf-8" />
       <MSG mobile="" cc="" spnumber="" message="" id="" time="" codec="utf-8" />
       <MSG mobile="" cc="" spnumber="" message="" id="" time="" codec="utf-8" />
       <MSG mobile="" cc="" spnumber="" message="" id="" time="" codec="utf-8" />
    </MO>

        mobile 发送者号码
        cc 国家代码
        spnumber 特服号
        message  短信内容, 使用 codec 编码的 UrlEncode
        id 上行短信的id,唯一辨识
        codec 默认 utf-8
        time:时间，如：2016-06-16 14:18:45
    */
    try
    {
        smsp::StateReport report;
        smsp::UpstreamSms upsms;
        std::vector<string> reportMsgId;

        respone = smsp::Channellib::urlDecode(respone);

        TiXmlDocument myDocument;
        if(myDocument.Parse(respone.data(), 0, TIXML_DEFAULT_ENCODING))
        {
            std::cout << "XHRChannellib.cpp:" << __LINE__ << " parseReport() faild! " << std::endl;
            return false;
        }

        TiXmlElement *rootElement = myDocument.RootElement();
        if(NULL== rootElement)
        {
            std::cout << "XHRChannellib.cpp:" << __LINE__ << " no root !" << std::endl;
            return false;
        }

        TiXmlElement *msgElement = rootElement->FirstChildElement();
        if(NULL == msgElement)
        {
            std::cout << "XHRChannellib.cpp:" << __LINE__ << " no element!" << std::endl;
            return false;
        }

        if(0 == strcmp(rootElement->Value(), "DR"))
        {
            while(NULL != msgElement)
            {
                const char * cSmsId = msgElement->Attribute("msgid");
                std::string strSmsId = "";
                if(cSmsId != NULL)
                {
                    strSmsId = cSmsId;
                }

                const char * cMobile = msgElement->Attribute("mobile");
                std::string strMobile = "";
                if(cMobile !=NULL)
                {
                    string tmpSrc = cMobile;
                    if(tmpSrc.substr(0, 2) != "00")
                    {
                        strMobile += "00";
                    }

                    strMobile += cMobile;
                }

                const char * cStatus = msgElement->Attribute("status");
                std::string strStatus = "";
                if(cStatus !=NULL)
                {
                    strStatus = cStatus;
                }

                Int32 nStatus = 0;
                if(!strStatus.compare("DELIVRD"))
                {
                    nStatus = SMS_STATUS_CONFIRM_SUCCESS;
                    strResponse.assign("ok");
                }
                else
                {
                    nStatus = SMS_STATUS_CONFIRM_FAIL;
                    strResponse.assign("faile");
                }

                const char * cReportTime = msgElement->Attribute("done_date");
                std::string strReportTime = "20";
                if(NULL != cReportTime)
                {
                   strReportTime += cReportTime;
                }

                tm tm_time;
                strptime(strReportTime.data(), "%Y-%m-%d %H:%M", &tm_time);
                report._reportTime = mktime(&tm_time);

                report._smsId = strSmsId;
                report._status = nStatus;

                report._desc = cStatus;
                report._phone = strMobile;
                reportRes.push_back(report);

                msgElement = msgElement->NextSiblingElement();
            }
        }
        else if(0 == strcmp(rootElement->Value(), "MO"))
        {
            while(NULL != msgElement)
            {
                const char * cSmsId = msgElement->Attribute("id");
                std::string strSmsId = "";
                if(cSmsId != NULL)
                {
                    strSmsId = cSmsId;
                }

                std::string country = "";
                const char * cc_value = msgElement->Attribute("cc");
                if(cc_value !=NULL)
                {
                    country = cc_value;
                }

                std::string strMobile = "";
                const char * sendMobile = msgElement->Attribute("mobile");
                if(sendMobile !=NULL)
                {
                    strMobile = country + sendMobile;
                }

                std::string superNumber = "";
                const char * sMobile = msgElement->Attribute("spnumber");
                if(sendMobile !=NULL)
                {
                    superNumber = sMobile;
                }

                std::string strContent = "";
                const char * contentBuff = msgElement->Attribute("message");
                if(contentBuff != NULL)
                {
                    strContent = contentBuff;
                }

                const char * upTime = msgElement->Attribute("time");
                std::string strUpTime = "";
                if(upTime != NULL)
                {
                    strUpTime = upTime;
                }

                upsms._phone    = strMobile;
                upsms._appendid = superNumber;
                upsms._content  = strContent;
                upsms._upsmsTimeStr= strUpTime;

                upsms._upsmsTimeInt= strtoul(upsms._upsmsTimeStr.data(), NULL, 0);
                upsmsRes.push_back(upsms);

                msgElement = msgElement->NextSiblingElement();
            }
        }
    }
    catch(...)
    {
        //LogWarn("[XHRChannellib::parseReport] is failed.");
        return false;
    }
}

int XHRChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string*data, std::vector<std::string>* vhead)
{
    if(NULL == data || NULL == sms)
    {
        return -1;
    }

    vhead->push_back("Content-Type: application/x-www-form-urlencoded; charset=UTF-8");

    std::string::size_type pos = (*data).find("%dest%");
    if(pos != std::string::npos)
    {
        (*data).replace(pos, strlen("%dest%"), sms->m_strPhone);
    }

    std::string decodeContent;
    decodeContent = UrlDecode(sms->m_strContent);

    std::string sendContent;
    utils::UTFString utfHelper = utils::UTFString();

    pos = (*data).find("%codec%");
    if(pos!=std::string::npos)
    {
        if("UTF-8" == sms->m_Channel.coding)
        {
            (*data).replace(pos, strlen("%codec%"), "8");
            utfHelper.u2ucs2(decodeContent, sendContent);
        }
        else
        {
            (*data).replace(pos, strlen("%codec%"), "0");
            utfHelper.u2Ascii(decodeContent, sendContent);
        }
        sendContent = encode(sendContent);
    }

//    pos = (*data).find("%sender%");
//    if(pos != std::string::npos)
//    {
//        (*data).replace(pos, strlen("%sender%"), sms->m_strShowPhone.data());
//    }

    pos = (*data).find("%msg%");
    if(pos != std::string::npos)
    {
        (*data).replace(pos, strlen("%msg%"), sendContent);
    }

    return 0;
}

} /* namespace smsp */
