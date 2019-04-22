#include "WTChannellib.h"
#include <stdio.h>
#include <time.h>
#include <fstream>
#include "xml.h"
#include "HttpParams.h"
#include "UTFString.h"

namespace smsp 
{
WTChannellib::WTChannellib() 
{
}

WTChannellib::~WTChannellib() 
{

}

extern "C"
{
    void * create()
    {
        return new WTChannellib;
    }
}

bool WTChannellib::parseSend(std::string respone,std::string& smsid,std::string& status, std::string& strReason)
{
    try
    {

        ////resptime,respstatus
        ////msgid
        std::string::size_type pos = respone.find(",0");
        if (pos == std::string::npos)
        {
            ///LogError("============err1.");
            std::cout<<"============err1."<<std::endl;
            pos = respone.find(",");
            if (pos == std::string::npos)
            {
                return false;
            }
            strReason = respone.substr(pos+1);
            return false;
        }

        std::string strContent = respone.substr(pos);

        pos = strContent.find("\n");
        if (pos == std::string::npos)
        {
            ///LogError("===============err2.");
            std::cout<<"============err2."<<std::endl;
            return false;
        }
        else
        {
            strContent = strContent.substr(pos+1);
        }

        pos = strContent.find("\n");
        if (pos == std::string::npos)
        {
            smsid = strContent;
        }
        else
        {
            smsid = strContent.substr(0,pos);
        }
        
        status = "0";
        
        ////LogWarn("smsid:[%s]",smsid.data());
        
        return true;
    }
    catch(...)
    {
        ////LogError("***WTChannellib::parseSend is except.***");
        std::cout<<"***WTChannellib::parseSend is except.***"<<std::endl;
        return false;
    }
}

bool WTChannellib::parseReport(std::string respone,std::vector<smsp::StateReport>& reportRes,std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse) 
{
    try
    {
        ////receiver=admin&pswd=12345&msgid=12345&reportTime=1012241002&mobile=13900210021&status=DELIVRD
        ///LogWarn("=======parseReport=========respone[%s].",respone.data());
        std::map<std::string,std::string> reportSet;
        split_Ex_Map(respone, "&", reportSet);

        std::string strSmsid = reportSet["msgid"];
        std::string strStat = reportSet["status"];
        int iStat = -1;
        if (0 == strStat.compare("DELIVRD"))
        {
            iStat = SMS_STATUS_CONFIRM_SUCCESS;
        }
        else
        {
            iStat = SMS_STATUS_CONFIRM_FAIL;
        }
        smsp::StateReport report;
        report._desc = strStat;
        report._smsId = strSmsid;
        report._status = iStat;
        report._reportTime = time(NULL);
        reportRes.push_back(report);

        ////LogWarn("********************smsid[%s].",strSmsid.data());
        return true;
    }
    catch(...)
    {
       ////LogError("***WTChannellib::parseReport is except.***");
       std::cout<<"***WTChannellib::parseReport is except.***"<<std::endl;
        return false; 
    }
}


int WTChannellib::adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead){

	return 0;
}

} /* namespace smsp */
